/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>
#include <wx/gdicmn.h>
#include <wx/string.h>

namespace scratchrobin::core {

// Forward declarations
class Encryption;

// Login method types
enum class LoginMethod {
    kStandard,
    kWindowsAuth,
    kKerberos,
    kCertificate,
    kToken
};

// Database engine types
enum class DbEngineType {
    kFirebird,
    kPostgreSQL,
    kMySQL,
    kSQLite,
    kScratchBird,
    kGeneric
};

std::string loginMethodToString(LoginMethod method);
LoginMethod stringToLoginMethod(const std::string& str);
std::string dbEngineTypeToString(DbEngineType type);
DbEngineType stringToDbEngineType(const std::string& str);

// Window position/state
struct WindowLayout {
    std::string window_id;           // e.g., "main", "sql_editor", "settings"
    int x{0};
    int y{0};
    int width{800};
    int height{600};
    bool maximized{false};
    bool visible{true};
    int display_index{0};            // Which display this was on
    
    wxRect toRect() const { return wxRect(x, y, width, height); }
    void fromRect(const wxRect& rect) {
        x = rect.x; y = rect.y; width = rect.width; height = rect.height;
    }
};

// Screen layout profile (user can have multiple)
struct ScreenLayout {
    std::string profile_name;        // e.g., "default", "work", "home"
    std::vector<WindowLayout> windows;
    bool auto_save{false};           // Whether to auto-save position changes
    
    std::optional<WindowLayout> findWindow(const std::string& window_id) const;
    void updateWindow(const WindowLayout& layout);
};

// Database connection info
struct DatabaseConnection {
    std::string database_name;
    std::string connection_string;
    std::string username;
    std::string encrypted_password;  // AES encrypted
    LoginMethod login_method{LoginMethod::kStandard};
    std::map<std::string, std::string> properties;  // Extra connection params
    bool auto_connect{false};
    bool save_password{true};        // Whether to store password (encrypted)
};

// Database engine on a server
struct DatabaseEngine {
    DbEngineType engine_type{DbEngineType::kGeneric};
    std::string version;
    std::string host_path;           // Path or identifier
    std::vector<DatabaseConnection> databases;
    std::map<std::string, std::string> engine_properties;
};

// Server configuration
struct ServerConfig {
    std::string server_id;           // Unique identifier
    std::string display_name;
    std::string hostname;
    int port{0};
    std::string description;
    std::vector<DatabaseEngine> engines;
    bool is_active{true};
    std::string group;               // For organizing in tree
    std::map<std::string, std::string> server_properties;
};

// Application configuration root
class AppConfig {
public:
    static AppConfig& get();
    
    // Load/save configuration
    bool load(const std::string& key = "");  // key for password decryption
    bool save();
    bool saveAs(const std::string& filepath);
    
    // File path
    std::string getConfigFilePath() const;
    bool configExists() const;
    
    // Screen layouts
    std::vector<ScreenLayout>& getLayouts() { return layouts_; }
    const std::vector<ScreenLayout>& getLayouts() const { return layouts_; }
    ScreenLayout* getCurrentLayout();
    const ScreenLayout* getCurrentLayout() const;
    ScreenLayout* findLayout(const std::string& name);
    const ScreenLayout* findLayout(const std::string& name) const;
    void setCurrentLayout(const std::string& name);
    void addLayout(const ScreenLayout& layout);
    void deleteLayout(const std::string& name);
    
    // Servers
    std::vector<ServerConfig>& getServers() { return servers_; }
    const std::vector<ServerConfig>& getServers() const { return servers_; }
    ServerConfig* findServer(const std::string& server_id);
    void addServer(const ServerConfig& server);
    void deleteServer(const std::string& server_id);
    
    // Password encryption
    bool hasEncryptedPasswords() const;
    bool isDecrypted() const { return decrypted_; }
    bool decryptPasswords(const std::string& key);
    std::string encryptPassword(const std::string& plaintext, const std::string& key) const;
    std::string decryptPassword(const std::string& ciphertext, const std::string& key) const;
    
    // Multi-monitor handling
    bool validateAndFixPositions();  // Returns true if positions were fixed
    void snapToVisibleArea(WindowLayout& layout);
    bool isPositionOnScreen(const wxRect& rect) const;
    wxRect getPrimaryDisplayWorkArea() const;
    std::vector<wxRect> getAllDisplayWorkAreas() const;
    
    // Modified flag
    bool isModified() const { return modified_; }
    void setModified(bool modified = true) { modified_ = modified; }
    
    // Save positions on exit
    bool shouldAutoSavePositions() const;
    void setAutoSavePositions(bool auto_save);
    
    // Reset to defaults
    void resetToDefaults();

private:
    AppConfig() = default;
    
    std::vector<ScreenLayout> layouts_;
    std::vector<ServerConfig> servers_;
    std::string current_layout_name_{"default"};
    
    std::string config_file_path_;
    bool modified_{false};
    bool decrypted_{false};
    std::string decryption_key_;
    
    // JSON serialization
    std::string toJson() const;
    bool fromJson(const std::string& json);
    
    // Encryption helper (mutable for lazy initialization in const methods)
    mutable std::unique_ptr<Encryption> encryption_;
    void initEncryption() const;
};

}  // namespace scratchrobin::core
