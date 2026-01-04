#ifndef LSP_SERVER_CONFIGS_HPP
#define LSP_SERVER_CONFIGS_HPP

#include "bolt/editor/lsp_client.hpp"
#include <string>
#include <vector>
#include <map>
#include <functional>

namespace bolt {
namespace lsp {

/**
 * @brief Pre-configured language server configurations for common languages.
 * 
 * This class provides factory methods for creating LspClientConfig objects
 * for well-known language servers like clangd, pyright, rust-analyzer, etc.
 */
class LspServerConfigs {
public:
    /**
     * @brief Get the configuration for clangd (C/C++ language server).
     * 
     * @param rootPath The root path of the project.
     * @param extraArgs Additional command-line arguments for clangd.
     * @return LspClientConfig configured for clangd.
     */
    static LspClientConfig clangd(const std::string& rootPath = ".", 
                                  const std::vector<std::string>& extraArgs = {});

    /**
     * @brief Get the configuration for pyright (Python language server).
     * 
     * @param rootPath The root path of the project.
     * @param extraArgs Additional command-line arguments for pyright.
     * @return LspClientConfig configured for pyright.
     */
    static LspClientConfig pyright(const std::string& rootPath = ".",
                                   const std::vector<std::string>& extraArgs = {});

    /**
     * @brief Get the configuration for rust-analyzer (Rust language server).
     * 
     * @param rootPath The root path of the project.
     * @param extraArgs Additional command-line arguments for rust-analyzer.
     * @return LspClientConfig configured for rust-analyzer.
     */
    static LspClientConfig rustAnalyzer(const std::string& rootPath = ".",
                                        const std::vector<std::string>& extraArgs = {});

    /**
     * @brief Get the configuration for typescript-language-server.
     * 
     * @param rootPath The root path of the project.
     * @param extraArgs Additional command-line arguments.
     * @return LspClientConfig configured for TypeScript/JavaScript.
     */
    static LspClientConfig typescriptLanguageServer(const std::string& rootPath = ".",
                                                    const std::vector<std::string>& extraArgs = {});

    /**
     * @brief Get a list of all available pre-configured language servers.
     * 
     * @return Vector of language IDs that have pre-configured settings.
     */
    static std::vector<std::string> getAvailableConfigs();

    /**
     * @brief Get configuration by language ID.
     * 
     * @param languageId The language ID (e.g., "cpp", "python", "rust").
     * @param rootPath The root path of the project.
     * @return LspClientConfig for the specified language, or empty config if not found.
     */
    static LspClientConfig getConfigForLanguage(const std::string& languageId,
                                                const std::string& rootPath = ".");

    /**
     * @brief Check if a language server is available on the system.
     * 
     * @param languageId The language ID to check.
     * @return true if the language server executable is found.
     */
    static bool isServerAvailable(const std::string& languageId);

    /**
     * @brief Get the file extensions supported by a language.
     * 
     * @param languageId The language ID.
     * @return Vector of file extensions (e.g., {".cpp", ".hpp", ".c", ".h"}).
     */
    static std::vector<std::string> getFileExtensions(const std::string& languageId);

    /**
     * @brief Get the language ID for a file based on its extension.
     * 
     * @param filePath The file path or name.
     * @return The language ID, or empty string if not recognized.
     */
    static std::string getLanguageIdForFile(const std::string& filePath);

private:
    // Helper to find executable in PATH
    static std::string findExecutable(const std::string& name);
    
    // Extension to language ID mapping
    static const std::map<std::string, std::string>& getExtensionMap();
};

/**
 * @brief Auto-configuration helper that detects and configures available language servers.
 */
class LspAutoConfigurator {
public:
    /**
     * @brief Automatically configure all available language servers.
     * 
     * @param manager The LspClientManager to configure.
     * @param rootPath The root path of the project.
     * @return Number of language servers configured.
     */
    static int autoConfigureAll(LspClientManager& manager, const std::string& rootPath = ".");

    /**
     * @brief Configure language servers for a specific project based on detected files.
     * 
     * @param manager The LspClientManager to configure.
     * @param projectPath The path to the project directory.
     * @return Number of language servers configured.
     */
    static int configureForProject(LspClientManager& manager, const std::string& projectPath);

    /**
     * @brief Get a report of available and missing language servers.
     * 
     * @return A formatted string describing the status of each language server.
     */
    static std::string getAvailabilityReport();
};

} // namespace lsp
} // namespace bolt

#endif // LSP_SERVER_CONFIGS_HPP
