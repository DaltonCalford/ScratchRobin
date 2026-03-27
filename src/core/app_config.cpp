/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0
 */

#include "core/app_config.h"

#include <QStandardPaths>
#include <QDir>
#include <QFile>
#include <QSettings>
#include <algorithm>

namespace scratchrobin::core {

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
    
    QSettings settings(QString::fromStdString(getConfigFilePath()), QSettings::IniFormat);
    
    // Clear existing data
    layouts_.clear();
    servers_.clear();
    
    // Load general settings
    current_layout_name_ = settings.value("General/current_layout", "default").toString().toStdString();
    
    // Load layouts
    int layoutCount = settings.beginReadArray("Layouts");
    for (int i = 0; i < layoutCount; ++i) {
        settings.setArrayIndex(i);
        ScreenLayout layout;
        layout.profile_name = settings.value("name").toString().toStdString();
        layout.auto_save = settings.value("auto_save", true).toBool();
        
        // Load windows for this layout
        int windowCount = settings.beginReadArray("windows");
        for (int j = 0; j < windowCount; ++j) {
            settings.setArrayIndex(j);
            WindowLayout win;
            win.window_id = settings.value("id").toString().toStdString();
            win.x = settings.value("x", 100).toInt();
            win.y = settings.value("y", 100).toInt();
            win.width = settings.value("width", 1024).toInt();
            win.height = settings.value("height", 768).toInt();
            win.maximized = settings.value("maximized", false).toBool();
            win.visible = settings.value("visible", true).toBool();
            win.display_index = settings.value("display", 0).toInt();
            layout.windows.push_back(win);
        }
        settings.endArray();
        
        layouts_.push_back(layout);
    }
    settings.endArray();
    
    // Load servers
    int serverCount = settings.beginReadArray("Servers");
    for (int i = 0; i < serverCount; ++i) {
        settings.setArrayIndex(i);
        ServerConfig server;
        server.server_id = settings.value("id").toString().toStdString();
        server.display_name = settings.value("display_name").toString().toStdString();
        server.hostname = settings.value("hostname").toString().toStdString();
        server.port = settings.value("port", 3050).toInt();
        server.description = settings.value("description").toString().toStdString();
        server.is_active = settings.value("is_active", true).toBool();
        server.group = settings.value("group").toString().toStdString();
        
        // Load engines for this server
        int engineCount = settings.beginReadArray("engines");
        for (int j = 0; j < engineCount; ++j) {
            settings.setArrayIndex(j);
            DatabaseEngine engine;
            engine.engine_type = stringToDbEngineType(
                settings.value("type").toString().toStdString());
            engine.version = settings.value("version").toString().toStdString();
            engine.host_path = settings.value("host_path").toString().toStdString();
            
            // Load databases for this engine
            int dbCount = settings.beginReadArray("databases");
            for (int k = 0; k < dbCount; ++k) {
                settings.setArrayIndex(k);
                DatabaseConnection db;
                db.database_name = settings.value("name").toString().toStdString();
                db.connection_string = settings.value("connection_string").toString().toStdString();
                db.username = settings.value("username").toString().toStdString();
                db.encrypted_password = settings.value("encrypted_password").toString().toStdString();
                db.login_method = stringToLoginMethod(
                    settings.value("login_method", "standard").toString().toStdString());
                db.auto_connect = settings.value("auto_connect", false).toBool();
                db.save_password = settings.value("save_password", false).toBool();
                engine.databases.push_back(db);
            }
            settings.endArray();
            
            server.engines.push_back(engine);
        }
        settings.endArray();
        
        servers_.push_back(server);
    }
    settings.endArray();
    
    // If no data was loaded, reset to defaults
    if (layouts_.empty() && servers_.empty()) {
        resetToDefaults();
    }
    
    modified_ = false;
    return true;
}

bool AppConfig::save() {
    QSettings settings(QString::fromStdString(getConfigFilePath()), QSettings::IniFormat);
    settings.clear();
    
    // Save general settings
    settings.setValue("General/current_layout", QString::fromStdString(current_layout_name_));
    
    // Save layouts
    settings.beginWriteArray("Layouts");
    for (size_t i = 0; i < layouts_.size(); ++i) {
        settings.setArrayIndex(static_cast<int>(i));
        const auto& layout = layouts_[i];
        settings.setValue("name", QString::fromStdString(layout.profile_name));
        settings.setValue("auto_save", layout.auto_save);
        
        // Save windows
        settings.beginWriteArray("windows");
        for (size_t j = 0; j < layout.windows.size(); ++j) {
            settings.setArrayIndex(static_cast<int>(j));
            const auto& win = layout.windows[j];
            settings.setValue("id", QString::fromStdString(win.window_id));
            settings.setValue("x", win.x);
            settings.setValue("y", win.y);
            settings.setValue("width", win.width);
            settings.setValue("height", win.height);
            settings.setValue("maximized", win.maximized);
            settings.setValue("visible", win.visible);
            settings.setValue("display", win.display_index);
        }
        settings.endArray();
    }
    settings.endArray();
    
    // Save servers
    settings.beginWriteArray("Servers");
    for (size_t i = 0; i < servers_.size(); ++i) {
        settings.setArrayIndex(static_cast<int>(i));
        const auto& server = servers_[i];
        settings.setValue("id", QString::fromStdString(server.server_id));
        settings.setValue("display_name", QString::fromStdString(server.display_name));
        settings.setValue("hostname", QString::fromStdString(server.hostname));
        settings.setValue("port", server.port);
        settings.setValue("description", QString::fromStdString(server.description));
        settings.setValue("is_active", server.is_active);
        settings.setValue("group", QString::fromStdString(server.group));
        
        // Save engines
        settings.beginWriteArray("engines");
        for (size_t j = 0; j < server.engines.size(); ++j) {
            settings.setArrayIndex(static_cast<int>(j));
            const auto& engine = server.engines[j];
            settings.setValue("type", QString::fromStdString(dbEngineTypeToString(engine.engine_type)));
            settings.setValue("version", QString::fromStdString(engine.version));
            settings.setValue("host_path", QString::fromStdString(engine.host_path));
            
            // Save databases
            settings.beginWriteArray("databases");
            for (size_t k = 0; k < engine.databases.size(); ++k) {
                settings.setArrayIndex(static_cast<int>(k));
                const auto& db = engine.databases[k];
                settings.setValue("name", QString::fromStdString(db.database_name));
                settings.setValue("connection_string", QString::fromStdString(db.connection_string));
                settings.setValue("username", QString::fromStdString(db.username));
                if (db.save_password) {
                    settings.setValue("encrypted_password", QString::fromStdString(db.encrypted_password));
                }
                settings.setValue("login_method", QString::fromStdString(loginMethodToString(db.login_method)));
                settings.setValue("auto_connect", db.auto_connect);
                settings.setValue("save_password", db.save_password);
            }
            settings.endArray();
        }
        settings.endArray();
    }
    settings.endArray();
    
    settings.sync();
    modified_ = false;
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
    for (const auto& server : servers_) {
        for (const auto& engine : server.engines) {
            for (const auto& db : engine.databases) {
                if (!db.encrypted_password.empty()) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool AppConfig::decryptPasswords(const std::string& key) {
    Q_UNUSED(key)
    // TODO: Implement actual decryption when encryption is added
    decrypted_ = true;
    return true;
}

bool AppConfig::shouldAutoSavePositions() const {
    const auto* layout = findLayout(current_layout_name_);
    if (layout) return layout->auto_save;
    return true;
}

bool AppConfig::validateAndFixPositions() {
    // Check if window positions are valid (on screen)
    // Return true if any positions were fixed
    bool fixed = false;
    for (auto& layout : layouts_) {
        for (auto& win : layout.windows) {
            // Ensure windows have reasonable positions
            if (win.x < -1000 || win.x > 10000) {
                win.x = 100;
                fixed = true;
            }
            if (win.y < -1000 || win.y > 10000) {
                win.y = 100;
                fixed = true;
            }
            if (win.width < 100 || win.width > 10000) {
                win.width = 1024;
                fixed = true;
            }
            if (win.height < 100 || win.height > 10000) {
                win.height = 768;
                fixed = true;
            }
        }
    }
    return fixed;
}

}  // namespace scratchrobin::core
