#include "config_manager.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <algorithm>

namespace scratchrobin {

class ConfigManager::Impl {
public:
    std::unordered_map<std::string, std::string> config_;
    mutable std::mutex mutex_;
};

ConfigManager& ConfigManager::getInstance() {
    static ConfigManager instance;
    return instance;
}

ConfigManager::ConfigManager()
    : impl_(std::make_unique<Impl>()) {
}

ConfigManager::~ConfigManager() = default;

bool ConfigManager::loadFromFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    std::ifstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    impl_->config_.clear();
    std::string line;

    while (std::getline(file, line)) {
        // Simple key=value parsing (ignoring comments starting with #)
        size_t commentPos = line.find('#');
        if (commentPos != std::string::npos) {
            line = line.substr(0, commentPos);
        }

        size_t equalsPos = line.find('=');
        if (equalsPos != std::string::npos) {
            std::string key = line.substr(0, equalsPos);
            std::string value = line.substr(equalsPos + 1);

            // Trim whitespace
            key.erase(key.begin(), std::find_if(key.begin(), key.end(),
                [](char c) { return !std::isspace(c); }));
            key.erase(std::find_if(key.rbegin(), key.rend(),
                [](char c) { return !std::isspace(c); }).base(), key.end());

            value.erase(value.begin(), std::find_if(value.begin(), value.end(),
                [](char c) { return !std::isspace(c); }));
            value.erase(std::find_if(value.rbegin(), value.rend(),
                [](char c) { return !std::isspace(c); }).base(), value.end());

            if (!key.empty()) {
                impl_->config_[key] = value;
            }
        }
    }

    return true;
}

bool ConfigManager::saveToFile(const std::string& filename) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    std::ofstream file(filename);
    if (!file.is_open()) {
        return false;
    }

    for (const auto& pair : impl_->config_) {
        file << pair.first << " = " << pair.second << std::endl;
    }

    return true;
}

std::string ConfigManager::getString(const std::string& key, const std::string& defaultValue) const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);

    auto it = impl_->config_.find(key);
    return it != impl_->config_.end() ? it->second : defaultValue;
}

int ConfigManager::getInt(const std::string& key, int defaultValue) const {
    std::string value = getString(key);
    if (value.empty()) {
        return defaultValue;
    }

    try {
        return std::stoi(value);
    } catch (const std::exception&) {
        return defaultValue;
    }
}

double ConfigManager::getDouble(const std::string& key, double defaultValue) const {
    std::string value = getString(key);
    if (value.empty()) {
        return defaultValue;
    }

    try {
        return std::stod(value);
    } catch (const std::exception&) {
        return defaultValue;
    }
}

bool ConfigManager::getBool(const std::string& key, bool defaultValue) const {
    std::string value = getString(key);
    if (value.empty()) {
        return defaultValue;
    }

    // Convert string to lowercase for comparison
    for (char& c : value) {
        c = std::tolower(c);
    }

    return value == "true" || value == "1" || value == "yes" || value == "on";
}

void ConfigManager::setString(const std::string& key, const std::string& value) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    impl_->config_[key] = value;
}

void ConfigManager::setInt(const std::string& key, int value) {
    setString(key, std::to_string(value));
}

void ConfigManager::setDouble(const std::string& key, double value) {
    setString(key, std::to_string(value));
}

void ConfigManager::setBool(const std::string& key, bool value) {
    setString(key, value ? "true" : "false");
}

bool ConfigManager::hasKey(const std::string& key) const {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    return impl_->config_.find(key) != impl_->config_.end();
}

void ConfigManager::removeKey(const std::string& key) {
    std::lock_guard<std::mutex> lock(impl_->mutex_);
    impl_->config_.erase(key);
}

} // namespace scratchrobin
