#ifndef SCRATCHROBIN_CONFIG_MANAGER_H
#define SCRATCHROBIN_CONFIG_MANAGER_H

#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>

namespace scratchrobin {

class ConfigManager {
public:
    static ConfigManager& getInstance();

    bool loadFromFile(const std::string& filename);
    bool saveToFile(const std::string& filename);

    std::string getString(const std::string& key, const std::string& defaultValue = "") const;
    int getInt(const std::string& key, int defaultValue = 0) const;
    double getDouble(const std::string& key, double defaultValue = 0.0) const;
    bool getBool(const std::string& key, bool defaultValue = false) const;

    void setString(const std::string& key, const std::string& value);
    void setInt(const std::string& key, int value);
    void setDouble(const std::string& key, double value);
    void setBool(const std::string& key, bool value);

    bool hasKey(const std::string& key) const;
    void removeKey(const std::string& key);

private:
    ConfigManager();
    ~ConfigManager();

    class Impl;
    std::unique_ptr<Impl> impl_;

    // Disable copy and assignment
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_CONFIG_MANAGER_H
