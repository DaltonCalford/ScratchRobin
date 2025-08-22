#ifndef SCRATCHROBIN_STRING_UTILS_H
#define SCRATCHROBIN_STRING_UTILS_H

#include <string>
#include <vector>
#include <algorithm>
#include <cctype>

namespace scratchrobin::utils {

std::string trim(const std::string& str);
std::string trimLeft(const std::string& str);
std::string trimRight(const std::string& str);

std::string toLower(const std::string& str);
std::string toUpper(const std::string& str);

std::vector<std::string> split(const std::string& str, char delimiter);
std::vector<std::string> split(const std::string& str, const std::string& delimiter);

std::string join(const std::vector<std::string>& parts, const std::string& separator);
std::string join(const std::vector<std::string>& parts, char separator);

bool startsWith(const std::string& str, const std::string& prefix);
bool endsWith(const std::string& str, const std::string& suffix);

std::string replace(const std::string& str, const std::string& from, const std::string& to);
std::string replaceAll(const std::string& str, const std::string& from, const std::string& to);

bool isEmpty(const std::string& str);
bool isBlank(const std::string& str);

std::string capitalize(const std::string& str);
std::string titleCase(const std::string& str);

std::string reverse(const std::string& str);

std::string leftPad(const std::string& str, size_t length, char padChar = ' ');
std::string rightPad(const std::string& str, size_t length, char padChar = ' ');

std::string truncate(const std::string& str, size_t length, const std::string& suffix = "...");

bool contains(const std::string& str, const std::string& substring);
bool containsIgnoreCase(const std::string& str, const std::string& substring);

size_t countOccurrences(const std::string& str, const std::string& substring);

std::string remove(const std::string& str, const std::string& toRemove);
std::string removeAll(const std::string& str, const std::string& toRemove);

std::string escape(const std::string& str);
std::string unescape(const std::string& str);

bool isNumeric(const std::string& str);
bool isAlpha(const std::string& str);
bool isAlphaNumeric(const std::string& str);

} // namespace scratchrobin::utils

#endif // SCRATCHROBIN_STRING_UTILS_H
