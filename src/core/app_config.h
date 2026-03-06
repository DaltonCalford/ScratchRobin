#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>

// Forward declare QRect to avoid Qt dependency in header
class QRect;

namespace scratchrobin::core {

enum class LoginMethod { kStandard, kWindowsAuth, kKerberos, kCertificate, kToken };
enum class DbEngineType { kFirebird, kPostgreSQL, kMySQL, kSQLite, kScratchBird, kGeneric };

std::string loginMethodToString(LoginMethod method);
LoginMethod stringToLoginMethod(const std::string& str);
std::string dbEngineTypeToString(DbEngineType type);
DbEngineType stringToDbEngineType(const std::string& str);

struct WindowLayout {
    std::string window_id;
    int x = 0, y = 0, width = 800, height = 600;
    bool maximized = false;
    bool visible = true;
    int display_index = 0;
};

struct ScreenLayout {
    std::string profile_name;
    bool auto_save = true;
    std::vector<WindowLayout> windows;
    
    std::optional<WindowLayout> findWindow(const std::string& window_id) const;
    void updateWindow(const WindowLayout& layout);
};

struct DatabaseConnection {
    std::string database_name;
    std::string connection_string;
    std::string username;
    std::string encrypted_password;
    LoginMethod login_method = LoginMethod::kStandard;
    bool auto_connect = false;
    bool save_password = false;
};

struct DatabaseEngine {
    DbEngineType engine_type = DbEngineType::kGeneric;
    std::string version;
    std::string host_path;
    std::vector<DatabaseConnection> databases;
};

struct ServerConfig {
    std::string server_id;
    std::string display_name;
    std::string hostname;
    int port = 0;
    std::string description;
    bool is_active = true;
    std::string group;
    std::vector<DatabaseEngine> engines;
};

// Forward declaration
class Encryption;

class AppConfig {
public:
    static AppConfig& get();
    ~AppConfig();  // Needed for unique_ptr with incomplete type
    
    std::string getConfigFilePath() const;
    bool configExists() const;
    bool load(const std::string& key = "");
    bool save();
    void resetToDefaults();
    void setModified(bool modified) { modified_ = modified; }
    
    ScreenLayout* getCurrentLayout();
    const ScreenLayout* getCurrentLayout() const;
    ScreenLayout* findLayout(const std::string& name);
    const ScreenLayout* findLayout(const std::string& name) const;
    void setCurrentLayout(const std::string& name) { current_layout_name_ = name; }
    
    ServerConfig* findServer(const std::string& server_id);
    const std::vector<ServerConfig>& getServers() const { return servers_; }
    
    bool hasEncryptedPasswords() const;
    bool decryptPasswords(const std::string& key);
    bool shouldAutoSavePositions() const;
    bool validateAndFixPositions();

private:
    AppConfig();
    
    std::vector<ScreenLayout> layouts_;
    std::vector<ServerConfig> servers_;
    std::string current_layout_name_ = "default";
    std::string config_file_path_;
    std::string decryption_key_;
    bool modified_ = false;
    bool decrypted_ = false;
};

}  // namespace scratchrobin::core
