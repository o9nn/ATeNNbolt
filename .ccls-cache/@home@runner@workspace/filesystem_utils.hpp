#ifndef FILESYSTEM_UTILS_HPP
#define FILESYSTEM_UTILS_HPP

#include <fstream>
#include <string>
#include <vector>
#include <stdexcept>

class FileSystemUtils {
public:
    static std::string readFile(const std::string& filename) {
        std::ifstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open file: " + filename);
        }

        std::string content((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
        return content;
    }

    static void writeFile(const std::string& filename, const std::string& content) {
        std::ofstream file(filename);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open file for writing: " + filename);
        }
        file << content;
    }

    static void appendToFile(const std::string& filename, const std::string& content) {
        std::ofstream file(filename, std::ios::app);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open file for appending: " + filename);
        }
        file << content;
    }
};

#endif