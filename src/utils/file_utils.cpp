#include "file_utils.h"
#include <filesystem>
#include <fstream>
#include <sstream>

namespace fs = std::filesystem;

namespace scratchrobin::utils {

bool fileExists(const std::string& path) {
    return fs::exists(path) && fs::is_regular_file(path);
}

bool directoryExists(const std::string& path) {
    return fs::exists(path) && fs::is_directory(path);
}

bool createDirectory(const std::string& path) {
    try {
        return fs::create_directories(path);
    } catch (...) {
        return false;
    }
}

bool removeFile(const std::string& path) {
    try {
        return fs::remove(path);
    } catch (...) {
        return false;
    }
}

std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

bool writeFile(const std::string& path, const std::string& content) {
    std::ofstream file(path);
    if (!file.is_open()) {
        return false;
    }

    file << content;
    return file.good();
}

std::vector<std::string> listFiles(const std::string& directory) {
    std::vector<std::string> files;
    try {
        for (const auto& entry : fs::directory_iterator(directory)) {
            files.push_back(entry.path().string());
        }
    } catch (...) {
        // Directory doesn't exist or can't be read
    }
    return files;
}

std::string getFileExtension(const std::string& path) {
    size_t dotPos = path.find_last_of('.');
    if (dotPos != std::string::npos && dotPos != path.length() - 1) {
        return path.substr(dotPos + 1);
    }
    return "";
}

std::string getFileName(const std::string& path) {
    size_t slashPos = path.find_last_of("/\\");
    if (slashPos != std::string::npos) {
        return path.substr(slashPos + 1);
    }
    return path;
}

std::string getDirectory(const std::string& path) {
    size_t slashPos = path.find_last_of("/\\");
    if (slashPos != std::string::npos) {
        return path.substr(0, slashPos);
    }
    return ".";
}

} // namespace scratchrobin::utils
