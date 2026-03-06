#include "core/app_config.h"

#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <algorithm>

namespace scratchrobin::core {

// Simple stub implementation for Qt6 transition

std::string loginMethodToString(LoginMethod method) {
    switch (method) {
        case LoginMethod::kStandard: return "standard";
        case LoginMethod::kWindowsAuth: return "windows";
        case LoginMethod::kKerberos: return "kerberos";
        case LoginMethod::kCertificate: return "certificate";
        case LoginMethod::kToken: return "token";
    }
    return "standard";
}

LoginMethod stringToLoginMethod(const std::string& str) {
    if (str == "windows") return LoginMethod::kWindowsAuth;
    if (str == "kerberos") return LoginMethod::kKerberos;
    if (str == "certificate") return LoginMethod::kCertificate;
    if (str == "token") return LoginMethod::kToken;
    return LoginMethod::kStandard;
}

std::string dbEngineTypeToString(DbEngineType type) {
    switch (type) {
        case DbEngineType::kFirebird: return "firebird";
        case DbEngineType::kPostgreSQL: return "postgresql";
        case DbEngineType::kMySQL: return "mysql";
        case DbEngineType::kSQLite: return "sqlite";
        case DbEngineType::kScratchBird: return "scratchbird";
        case DbEngineType::kGeneric: return "generic";
    }
    return "generic";
}

DbEngineType stringToDbEngineType(const std::string& str) {
    if (str == "firebird") return DbEngineType::kFirebird;
    if (str == "postgresql") return DbEngineType::kPostgreSQL;
    if (str == "mysql") return DbEngineType::kMySQL;
    if (str == "sqlite") return DbEngineType::kSQLite;
    if (str == "scratchbird") return DbEngineType::kScratchBird;
    return DbEngineType::kGeneric;
}

// ScreenLayout implementation
std::optional<WindowLayout> ScreenLayout::findWindow(const std::string& window_id) const {
    for (const auto& win : windows) {
        if (win.window_id == window_id) return win;
    }
    return std::nullopt;
}

void ScreenLayout::updateWindow(const WindowLayout& layout) {
    for (auto& win : windows) {
        if (win.window_id == layout.window_id) {
            win = layout;
            return;
        }
    }
    windows.push_back(layout);
}

// AppConfig implementation
AppConfig::AppConfig() = default;
AppConfig::~AppConfig() = default;

AppConfig& AppConfig::get() {
    static AppConfig instance;
    return instance;
}

std::string AppConfig::getConfigFilePath() const {
    QString configDir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    QDir().mkpath(configDir);
    return (configDir + "/scratchrobin-config.ini").toStdString();
}

bool AppConfig::configExists() const {
    return QFile::exists(QString::fromStdString(getConfigFilePath()));
}

bool AppConfig::load(const std::string& key) {
    Q_UNUSED(key)
    resetToDefaults();
    return true;
}

bool AppConfig::save() {
    return true;
}

void AppConfig::resetToDefaults() {
    layouts_.clear();
    servers_.clear();
    
    ScreenLayout default_layout;
    default_layout.profile_name = "default";
    default_layout.auto_save = true;
    
    WindowLayout main_win;
    main_win.window_id = "main";
    main_win.x = 100;
    main_win.y = 100;
    main_win.width = 1024;
    main_win.height = 768;
    main_win.maximized = false;
    main_win.display_index = 0;
    default_layout.windows.push_back(main_win);
    
    layouts_.push_back(default_layout);
    current_layout_name_ = "default";
    
    ServerConfig sample_server;
    sample_server.server_id = "localhost";
    sample_server.display_name = "Local Server";
    sample_server.hostname = "localhost";
    sample_server.port = 3050;
    sample_server.is_active = true;
    servers_.push_back(sample_server);
    
    modified_ = true;
}

ScreenLayout* AppConfig::getCurrentLayout() {
    return findLayout(current_layout_name_);
}

const ScreenLayout* AppConfig::getCurrentLayout() const {
    return findLayout(current_layout_name_);
}

ScreenLayout* AppConfig::findLayout(const std::string& name) {
    for (auto& layout : layouts_) {
        if (layout.profile_name == name) return &layout;
    }
    return nullptr;
}

const ScreenLayout* AppConfig::findLayout(const std::string& name) const {
    for (const auto& layout : layouts_) {
        if (layout.profile_name == name) return &layout;
    }
    return nullptr;
}

ServerConfig* AppConfig::findServer(const std::string& server_id) {
    for (auto& server : servers_) {
        if (server.server_id == server_id) return &server;
    }
    return nullptr;
}

bool AppConfig::hasEncryptedPasswords() const {
    return false;
}

bool AppConfig::decryptPasswords(const std::string& key) {
    Q_UNUSED(key)
    return true;
}

bool AppConfig::shouldAutoSavePositions() const {
    const auto* layout = findLayout(current_layout_name_);
    if (layout) return layout->auto_save;
    return true;
}

bool AppConfig::validateAndFixPositions() {
    return false;
}

}  // namespace scratchrobin::core
