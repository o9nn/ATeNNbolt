#include "bolt/editor/lsp_server_configs.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <set>

namespace bolt {
namespace lsp {

// Helper to find executable in PATH
std::string LspServerConfigs::findExecutable(const std::string& name) {
    // Check if it's an absolute path
    if (std::filesystem::exists(name)) {
        return name;
    }

    // Search in PATH
    const char* pathEnv = std::getenv("PATH");
    if (!pathEnv) return "";

    std::string path(pathEnv);
    std::istringstream pathStream(path);
    std::string dir;

    while (std::getline(pathStream, dir, ':')) {
        std::filesystem::path candidate = std::filesystem::path(dir) / name;
        if (std::filesystem::exists(candidate)) {
            return candidate.string();
        }
    }

    return "";
}

const std::map<std::string, std::string>& LspServerConfigs::getExtensionMap() {
    static const std::map<std::string, std::string> extensionMap = {
        {".cpp", "cpp"},
        {".cxx", "cpp"},
        {".cc", "cpp"},
        {".c", "c"},
        {".hpp", "cpp"},
        {".hxx", "cpp"},
        {".h", "c"},  // Could be C or C++, default to C
        {".py", "python"},
        {".pyi", "python"},
        {".pyw", "python"},
        {".rs", "rust"},
        {".ts", "typescript"},
        {".tsx", "typescriptreact"},
        {".js", "javascript"},
        {".jsx", "javascriptreact"},
        {".json", "json"},
        {".go", "go"},
        {".java", "java"},
        {".kt", "kotlin"},
        {".kts", "kotlin"},
        {".lua", "lua"},
        {".rb", "ruby"},
        {".php", "php"},
        {".cs", "csharp"},
        {".swift", "swift"},
        {".zig", "zig"},
    };
    return extensionMap;
}

LspClientConfig LspServerConfigs::clangd(const std::string& rootPath, 
                                         const std::vector<std::string>& extraArgs) {
    LspClientConfig config;
    config.serverCommand = findExecutable("clangd");
    if (config.serverCommand.empty()) {
        config.serverCommand = "clangd";  // Fall back to name, let system find it
    }
    
    // Default clangd arguments for better performance
    config.serverArgs = {
        "--background-index",
        "--clang-tidy",
        "--completion-style=detailed",
        "--header-insertion=iwyu",
        "--pch-storage=memory",
        "-j=4"
    };
    
    // Add extra arguments
    config.serverArgs.insert(config.serverArgs.end(), extraArgs.begin(), extraArgs.end());
    
    config.languageId = "cpp";
    config.fileExtensions = {".cpp", ".cxx", ".cc", ".c", ".hpp", ".hxx", ".h"};
    config.rootPath = rootPath;
    config.timeoutMs = 10000;  // 10 second timeout for clangd
    
    return config;
}

LspClientConfig LspServerConfigs::pyright(const std::string& rootPath,
                                          const std::vector<std::string>& extraArgs) {
    LspClientConfig config;
    config.serverCommand = findExecutable("pyright-langserver");
    if (config.serverCommand.empty()) {
        config.serverCommand = findExecutable("pyright");
    }
    if (config.serverCommand.empty()) {
        config.serverCommand = "pyright-langserver";
    }
    
    // Pyright requires --stdio flag
    config.serverArgs = {"--stdio"};
    config.serverArgs.insert(config.serverArgs.end(), extraArgs.begin(), extraArgs.end());
    
    config.languageId = "python";
    config.fileExtensions = {".py", ".pyi", ".pyw"};
    config.rootPath = rootPath;
    config.timeoutMs = 5000;
    
    return config;
}

LspClientConfig LspServerConfigs::rustAnalyzer(const std::string& rootPath,
                                               const std::vector<std::string>& extraArgs) {
    LspClientConfig config;
    config.serverCommand = findExecutable("rust-analyzer");
    if (config.serverCommand.empty()) {
        config.serverCommand = "rust-analyzer";
    }
    
    config.serverArgs = extraArgs;
    config.languageId = "rust";
    config.fileExtensions = {".rs"};
    config.rootPath = rootPath;
    config.timeoutMs = 10000;
    
    return config;
}

LspClientConfig LspServerConfigs::typescriptLanguageServer(const std::string& rootPath,
                                                           const std::vector<std::string>& extraArgs) {
    LspClientConfig config;
    config.serverCommand = findExecutable("typescript-language-server");
    if (config.serverCommand.empty()) {
        config.serverCommand = "typescript-language-server";
    }
    
    config.serverArgs = {"--stdio"};
    config.serverArgs.insert(config.serverArgs.end(), extraArgs.begin(), extraArgs.end());
    
    config.languageId = "typescript";
    config.fileExtensions = {".ts", ".tsx", ".js", ".jsx"};
    config.rootPath = rootPath;
    config.timeoutMs = 5000;
    
    return config;
}

std::vector<std::string> LspServerConfigs::getAvailableConfigs() {
    return {"cpp", "python", "rust", "typescript"};
}

LspClientConfig LspServerConfigs::getConfigForLanguage(const std::string& languageId,
                                                       const std::string& rootPath) {
    if (languageId == "cpp" || languageId == "c") {
        return clangd(rootPath);
    } else if (languageId == "python") {
        return pyright(rootPath);
    } else if (languageId == "rust") {
        return rustAnalyzer(rootPath);
    } else if (languageId == "typescript" || languageId == "javascript") {
        return typescriptLanguageServer(rootPath);
    }
    
    return LspClientConfig{};  // Empty config for unknown languages
}

bool LspServerConfigs::isServerAvailable(const std::string& languageId) {
    std::string executable;
    
    if (languageId == "cpp" || languageId == "c") {
        executable = "clangd";
    } else if (languageId == "python") {
        executable = "pyright-langserver";
        if (findExecutable(executable).empty()) {
            executable = "pyright";
        }
    } else if (languageId == "rust") {
        executable = "rust-analyzer";
    } else if (languageId == "typescript" || languageId == "javascript") {
        executable = "typescript-language-server";
    } else {
        return false;
    }
    
    return !findExecutable(executable).empty();
}

std::vector<std::string> LspServerConfigs::getFileExtensions(const std::string& languageId) {
    if (languageId == "cpp" || languageId == "c") {
        return {".cpp", ".cxx", ".cc", ".c", ".hpp", ".hxx", ".h"};
    } else if (languageId == "python") {
        return {".py", ".pyi", ".pyw"};
    } else if (languageId == "rust") {
        return {".rs"};
    } else if (languageId == "typescript") {
        return {".ts", ".tsx"};
    } else if (languageId == "javascript") {
        return {".js", ".jsx"};
    }
    
    return {};
}

std::string LspServerConfigs::getLanguageIdForFile(const std::string& filePath) {
    std::filesystem::path path(filePath);
    std::string ext = path.extension().string();
    
    // Convert to lowercase
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);
    
    const auto& extMap = getExtensionMap();
    auto it = extMap.find(ext);
    if (it != extMap.end()) {
        return it->second;
    }
    
    return "";
}

// LspAutoConfigurator implementation

int LspAutoConfigurator::autoConfigureAll(LspClientManager& manager, const std::string& rootPath) {
    int configured = 0;
    
    for (const auto& langId : LspServerConfigs::getAvailableConfigs()) {
        if (LspServerConfigs::isServerAvailable(langId)) {
            auto config = LspServerConfigs::getConfigForLanguage(langId, rootPath);
            if (!config.serverCommand.empty()) {
                manager.registerLanguageServer(langId, config);
                configured++;
            }
        }
    }
    
    return configured;
}

int LspAutoConfigurator::configureForProject(LspClientManager& manager, const std::string& projectPath) {
    std::set<std::string> detectedLanguages;
    
    // Scan project directory for source files
    try {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(projectPath)) {
            if (entry.is_regular_file()) {
                std::string langId = LspServerConfigs::getLanguageIdForFile(entry.path().string());
                if (!langId.empty()) {
                    detectedLanguages.insert(langId);
                }
            }
        }
    } catch (const std::exception&) {
        // Ignore filesystem errors
    }
    
    int configured = 0;
    for (const auto& langId : detectedLanguages) {
        if (LspServerConfigs::isServerAvailable(langId)) {
            auto config = LspServerConfigs::getConfigForLanguage(langId, projectPath);
            if (!config.serverCommand.empty()) {
                manager.registerLanguageServer(langId, config);
                configured++;
            }
        }
    }
    
    return configured;
}

std::string LspAutoConfigurator::getAvailabilityReport() {
    std::ostringstream report;
    report << "Language Server Availability Report\n";
    report << "====================================\n\n";
    
    struct ServerInfo {
        std::string name;
        std::string langId;
        std::string executable;
    };
    
    std::vector<ServerInfo> servers = {
        {"clangd (C/C++)", "cpp", "clangd"},
        {"pyright (Python)", "python", "pyright"},
        {"rust-analyzer (Rust)", "rust", "rust-analyzer"},
        {"typescript-language-server (TS/JS)", "typescript", "typescript-language-server"}
    };
    
    for (const auto& server : servers) {
        bool available = LspServerConfigs::isServerAvailable(server.langId);
        report << (available ? "✅" : "❌") << " " << server.name << "\n";
        if (available) {
            auto config = LspServerConfigs::getConfigForLanguage(server.langId);
            report << "   Path: " << config.serverCommand << "\n";
        } else {
            report << "   Not found. Install: " << server.executable << "\n";
        }
        report << "\n";
    }
    
    return report.str();
}

} // namespace lsp
} // namespace bolt
