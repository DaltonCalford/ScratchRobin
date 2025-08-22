#ifndef SCRATCHROBIN_FILE_UTILS_H
#define SCRATCHROBIN_FILE_UTILS_H

#include <string>
#include <vector>

namespace scratchrobin::utils {

bool fileExists(const std::string& path);
bool directoryExists(const std::string& path);
bool createDirectory(const std::string& path);
bool removeFile(const std::string& path);
std::string readFile(const std::string& path);
bool writeFile(const std::string& path, const std::string& content);
std::vector<std::string> listFiles(const std::string& directory);
std::string getFileExtension(const std::string& path);
std::string getFileName(const std::string& path);
std::string getDirectory(const std::string& path);

} // namespace scratchrobin::utils

#endif // SCRATCHROBIN_FILE_UTILS_H
