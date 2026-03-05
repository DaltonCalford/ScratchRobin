/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "core/app_config.h"

#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/dir.h>
#include <wx/display.h>
#include <wx/fileconf.h>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <memory>

// OpenSSL for encryption
#include <openssl/evp.h>
#include <openssl/aes.h>
#include <openssl/rand.h>
#include <openssl/sha.h>

namespace scratchrobin::core {

// ============================================================================
// Enums
// ============================================================================

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

// ============================================================================
// Simple AES Encryption Helper
// ============================================================================

class Encryption {
public:
    // Derive a 256-bit key from password using SHA-256
    std::vector<unsigned char> deriveKey(const std::string& password) const {
        std::vector<unsigned char> key(32);
        SHA256(reinterpret_cast<const unsigned char*>(password.data()), 
               password.size(), key.data());
        return key;
    }
    
    // Encrypt plaintext with AES-256-CBC
    std::string encrypt(const std::string& plaintext, const std::string& password) const {
        if (plaintext.empty()) return "";
        
        auto key = deriveKey(password);
        
        // Generate random IV (16 bytes for CBC)
        unsigned char iv[16];
        if (RAND_bytes(iv, sizeof(iv)) != 1) {
            return "";
        }
        
        // Prepare output buffer
        std::vector<unsigned char> ciphertext(plaintext.size() + EVP_MAX_BLOCK_LENGTH);
        int len;
        int ciphertext_len;
        
        // Create context
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) return "";
        
        // Initialize encryption
        if (EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr, 
                               key.data(), iv) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return "";
        }
        
        // Encrypt
        if (EVP_EncryptUpdate(ctx, ciphertext.data(), &len,
                              reinterpret_cast<const unsigned char*>(plaintext.data()),
                              plaintext.size()) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return "";
        }
        ciphertext_len = len;
        
        // Finalize
        if (EVP_EncryptFinal_ex(ctx, ciphertext.data() + len, &len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return "";
        }
        ciphertext_len += len;
        
        EVP_CIPHER_CTX_free(ctx);
        
        // Combine: IV (16) + Ciphertext
        std::vector<unsigned char> result;
        result.reserve(16 + ciphertext_len);
        result.insert(result.end(), iv, iv + 16);
        result.insert(result.end(), ciphertext.begin(), ciphertext.begin() + ciphertext_len);
        
        // Base64 encode
        return base64Encode(result);
    }
    
    // Decrypt ciphertext with AES-256-CBC
    std::string decrypt(const std::string& ciphertext_b64, const std::string& password) const {
        if (ciphertext_b64.empty()) return "";
        
        auto key = deriveKey(password);
        
        // Base64 decode
        auto data = base64Decode(ciphertext_b64);
        if (data.size() < 16) return "";  // IV minimum
        
        // Extract IV and ciphertext
        unsigned char iv[16];
        std::copy(data.begin(), data.begin() + 16, iv);
        
        const unsigned char* ciphertext = data.data() + 16;
        int ciphertext_len = static_cast<int>(data.size() - 16);
        
        // Prepare output buffer
        std::vector<unsigned char> plaintext(ciphertext_len + EVP_MAX_BLOCK_LENGTH);
        int len;
        int plaintext_len;
        
        // Create context
        EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
        if (!ctx) return "";
        
        // Initialize decryption
        if (EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), nullptr,
                               key.data(), iv) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return "";
        }
        
        // Decrypt
        if (EVP_DecryptUpdate(ctx, plaintext.data(), &len, ciphertext, ciphertext_len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return "";
        }
        plaintext_len = len;
        
        // Finalize
        if (EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len) != 1) {
            EVP_CIPHER_CTX_free(ctx);
            return "";  // Decryption failed (wrong key or corrupted data)
        }
        plaintext_len += len;
        
        EVP_CIPHER_CTX_free(ctx);
        
        return std::string(reinterpret_cast<char*>(plaintext.data()), plaintext_len);
    }

private:
    std::string base64Encode(const std::vector<unsigned char>& data) const {
        static const char base64_chars[] = 
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        
        std::string result;
        int i = 0;
        unsigned char char_array_3[3];
        unsigned char char_array_4[4];
        
        for (size_t in_len = data.size(), pos = 0; in_len--; ) {
            char_array_3[i++] = data[pos++];
            if (i == 3) {
                char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
                char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
                char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
                char_array_4[3] = char_array_3[2] & 0x3f;
                
                for (int j = 0; j < 4; j++)
                    result += base64_chars[char_array_4[j]];
                i = 0;
            }
        }
        
        if (i) {
            for (int j = i; j < 3; j++)
                char_array_3[j] = '\0';
            
            char_array_4[0] = (char_array_3[0] & 0xfc) >> 2;
            char_array_4[1] = ((char_array_3[0] & 0x03) << 4) + ((char_array_3[1] & 0xf0) >> 4);
            char_array_4[2] = ((char_array_3[1] & 0x0f) << 2) + ((char_array_3[2] & 0xc0) >> 6);
            
            for (int j = 0; j < (i + 1); j++)
                result += base64_chars[char_array_4[j]];
            
            while ((i++ < 3))
                result += '=';
        }
        
        return result;
    }
    
    std::vector<unsigned char> base64Decode(const std::string& encoded) const {
        static const std::string base64_chars = 
            "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
        
        std::vector<unsigned char> result;
        int in_len = encoded.size();
        int i = 0;
        int j = 0;
        int in_ = 0;
        unsigned char char_array_4[4], char_array_3[3];
        
        while (in_len-- && (encoded[in_] != '=') && 
               (isalnum(encoded[in_]) || encoded[in_] == '+' || encoded[in_] == '/')) {
            char_array_4[i++] = encoded[in_]; in_++;
            if (i == 4) {
                for (j = 0; j < 4; j++)
                    char_array_4[j] = base64_chars.find(char_array_4[j]);
                
                char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
                char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
                char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];
                
                for (j = 0; j < 3; j++)
                    result.push_back(char_array_3[j]);
                i = 0;
            }
        }
        
        if (i) {
            for (j = i; j < 4; j++)
                char_array_4[j] = 0;
            
            for (j = 0; j < 4; j++)
                char_array_4[j] = base64_chars.find(char_array_4[j]);
            
            char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
            char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
            
            for (j = 0; j < (i - 1); j++)
                result.push_back(char_array_3[j]);
        }
        
        return result;
    }
};

// ============================================================================
// ScreenLayout
// ============================================================================

std::optional<WindowLayout> ScreenLayout::findWindow(const std::string& window_id) const {
    for (const auto& win : windows) {
        if (win.window_id == window_id) {
            return win;
        }
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

// ============================================================================
// AppConfig Singleton
// ============================================================================

AppConfig& AppConfig::get() {
    static AppConfig instance;
    return instance;
}

void AppConfig::initEncryption() const {
    if (!encryption_) {
        encryption_ = std::make_unique<Encryption>();
    }
}

std::string AppConfig::getConfigFilePath() const {
    if (!config_file_path_.empty()) {
        return config_file_path_;
    }
    wxString config_dir = wxStandardPaths::Get().GetUserConfigDir();
    if (!wxDirExists(config_dir)) {
        wxMkdir(config_dir, wxS_DIR_DEFAULT);
    }
    return (config_dir + "/scratchrobin-config.cfg").ToStdString();
}

bool AppConfig::configExists() const {
    return wxFileExists(wxString::FromUTF8(getConfigFilePath().c_str()));
}

bool AppConfig::load(const std::string& key) {
    initEncryption();
    decryption_key_ = key;
    
    std::string filepath = getConfigFilePath();
    
    // Use wxFileConfig for simple key-value storage
    wxFileConfig config(wxT("ScratchRobin"), wxEmptyString,
                        wxString::FromUTF8(filepath.c_str()),
                        wxEmptyString, wxCONFIG_USE_LOCAL_FILE);
    
    if (!config.Exists(wxT("version"))) {
        // Config doesn't exist yet, create default
        resetToDefaults();
        return true;
    }
    
    // Load current layout
    wxString current_layout;
    if (config.Read(wxT("current_layout"), &current_layout)) {
        current_layout_name_ = current_layout.ToStdString();
    }
    
    // Load layouts
    layouts_.clear();
    wxString layout_name;
    long layout_index;
    if (config.GetFirstGroup(layout_name, layout_index)) {
        do {
            if (layout_name == wxT("servers")) continue;
            
            ScreenLayout layout;
            layout.profile_name = layout_name.ToStdString();
            
            config.SetPath(layout_name);
            layout.auto_save = config.ReadBool(wxT("auto_save"), true);
            
            // Load windows in this layout
            wxString window_key;
            long window_index;
            if (config.GetFirstEntry(window_key, window_index)) {
                do {
                    if (window_key.StartsWith(wxT("window_"))) {
                        WindowLayout win;
                        win.window_id = window_key.AfterFirst('_').ToStdString();
                        
                        wxString value;
                        if (config.Read(window_key, &value)) {
                            // Parse "x,y,width,height,maximized,visible,display"
                            std::stringstream ss(value.ToStdString());
                            char comma;
                            ss >> win.x >> comma >> win.y >> comma 
                               >> win.width >> comma >> win.height >> comma
                               >> win.maximized >> comma >> win.visible >> comma
                               >> win.display_index;
                            layout.windows.push_back(win);
                        }
                    }
                } while (config.GetNextEntry(window_key, window_index));
            }
            
            config.SetPath(wxT(".."));
            layouts_.push_back(layout);
        } while (config.GetNextGroup(layout_name, layout_index));
    }
    
    // Load servers
    servers_.clear();
    config.SetPath(wxT("/servers"));
    
    wxString server_id;
    long server_index;
    if (config.GetFirstGroup(server_id, server_index)) {
        do {
            ServerConfig server;
            server.server_id = server_id.ToStdString();
            
            config.SetPath(server_id);
            server.display_name = config.Read(wxT("display_name"), server_id).ToStdString();
            server.hostname = config.Read(wxT("hostname"), wxT("localhost")).ToStdString();
            server.port = config.ReadLong(wxT("port"), 0);
            server.description = config.Read(wxT("description"), wxT("")).ToStdString();
            server.is_active = config.ReadBool(wxT("is_active"), true);
            server.group = config.Read(wxT("group"), wxT("")).ToStdString();
            
            // Load engines
            config.SetPath(wxT("engines"));
            wxString engine_type;
            long engine_index;
            if (config.GetFirstGroup(engine_type, engine_index)) {
                do {
                    DatabaseEngine engine;
                    engine.engine_type = stringToDbEngineType(engine_type.ToStdString());
                    
                    config.SetPath(engine_type);
                    engine.version = config.Read(wxT("version"), wxT("")).ToStdString();
                    engine.host_path = config.Read(wxT("host_path"), wxT("")).ToStdString();
                    
                    // Load databases
                    config.SetPath(wxT("databases"));
                    wxString db_name;
                    long db_index;
                    if (config.GetFirstEntry(db_name, db_index)) {
                        do {
                            DatabaseConnection db;
                            db.database_name = db_name.ToStdString();
                            
                            wxString value;
                            if (config.Read(db_name, &value)) {
                                // Parse connection info - format:
                                // connection_string|username|encrypted_password|login_method|auto_connect|save_password
                                std::string val = value.ToStdString();
                                size_t pos = 0;
                                std::string token;
                                std::vector<std::string> parts;
                                while ((pos = val.find('|')) != std::string::npos) {
                                    parts.push_back(val.substr(0, pos));
                                    val.erase(0, pos + 1);
                                }
                                parts.push_back(val);
                                
                                if (parts.size() >= 1) db.connection_string = parts[0];
                                if (parts.size() >= 2) db.username = parts[1];
                                if (parts.size() >= 3) db.encrypted_password = parts[2];
                                if (parts.size() >= 4) db.login_method = stringToLoginMethod(parts[3]);
                                if (parts.size() >= 5) db.auto_connect = (parts[4] == "1");
                                if (parts.size() >= 6) db.save_password = (parts[5] == "1");
                            }
                            
                            engine.databases.push_back(db);
                        } while (config.GetNextEntry(db_name, db_index));
                    }
                    
                    config.SetPath(wxT("../.."));
                    server.engines.push_back(engine);
                } while (config.GetNextGroup(engine_type, engine_index));
            }
            
            config.SetPath(wxT("../.."));
            servers_.push_back(server);
        } while (config.GetNextGroup(server_id, server_index));
    }
    
    // Validate and fix positions if displays have changed
    validateAndFixPositions();
    
    // If key provided, decrypt passwords
    if (!key.empty()) {
        decrypted_ = decryptPasswords(key);
    }
    
    modified_ = false;
    return true;
}

bool AppConfig::save() {
    if (!modified_) return true;
    
    std::string filepath = getConfigFilePath();
    
    wxFileConfig config(wxT("ScratchRobin"), wxEmptyString,
                        wxString::FromUTF8(filepath.c_str()),
                        wxEmptyString, wxCONFIG_USE_LOCAL_FILE);
    
    config.DeleteAll();
    
    config.Write(wxT("version"), 1);
    config.Write(wxT("current_layout"), wxString::FromUTF8(current_layout_name_.c_str()));
    
    // Save layouts
    for (const auto& layout : layouts_) {
        wxString path = wxString::FromUTF8(layout.profile_name.c_str());
        config.SetPath(path);
        config.Write(wxT("auto_save"), layout.auto_save);
        
        for (const auto& win : layout.windows) {
            wxString key = wxString::Format(wxT("window_%s"), 
                                            wxString::FromUTF8(win.window_id.c_str()));
            wxString value = wxString::Format(wxT("%d,%d,%d,%d,%d,%d,%d"),
                                             win.x, win.y, win.width, win.height,
                                             win.maximized ? 1 : 0, win.visible ? 1 : 0,
                                             win.display_index);
            config.Write(key, value);
        }
        
        config.SetPath(wxT("/"));
    }
    
    // Save servers
    config.SetPath(wxT("/servers"));
    for (const auto& server : servers_) {
        wxString server_path = wxString::FromUTF8(server.server_id.c_str());
        config.SetPath(server_path);
        
        config.Write(wxT("display_name"), wxString::FromUTF8(server.display_name.c_str()));
        config.Write(wxT("hostname"), wxString::FromUTF8(server.hostname.c_str()));
        config.Write(wxT("port"), server.port);
        config.Write(wxT("description"), wxString::FromUTF8(server.description.c_str()));
        config.Write(wxT("is_active"), server.is_active);
        config.Write(wxT("group"), wxString::FromUTF8(server.group.c_str()));
        
        // Save engines
        config.SetPath(wxT("engines"));
        for (const auto& engine : server.engines) {
            wxString engine_path = wxString::FromUTF8(dbEngineTypeToString(engine.engine_type).c_str());
            config.SetPath(engine_path);
            
            config.Write(wxT("version"), wxString::FromUTF8(engine.version.c_str()));
            config.Write(wxT("host_path"), wxString::FromUTF8(engine.host_path.c_str()));
            
            // Save databases
            config.SetPath(wxT("databases"));
            for (const auto& db : engine.databases) {
                wxString db_key = wxString::FromUTF8(db.database_name.c_str());
                wxString value = wxString::Format(wxT("%s|%s|%s|%s|%d|%d"),
                    wxString::FromUTF8(db.connection_string.c_str()),
                    wxString::FromUTF8(db.username.c_str()),
                    wxString::FromUTF8(db.encrypted_password.c_str()),
                    wxString::FromUTF8(loginMethodToString(db.login_method).c_str()),
                    db.auto_connect ? 1 : 0,
                    db.save_password ? 1 : 0);
                config.Write(db_key, value);
            }
            config.SetPath(wxT("../.."));
        }
        config.SetPath(wxT("../.."));
    }
    
    config.Flush();
    modified_ = false;
    return true;
}

void AppConfig::resetToDefaults() {
    layouts_.clear();
    servers_.clear();
    
    // Create default layout
    ScreenLayout default_layout;
    default_layout.profile_name = "default";
    default_layout.auto_save = true;
    
    // Default main window
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
    
    // Sample server (for demonstration)
    ServerConfig sample_server;
    sample_server.server_id = "localhost";
    sample_server.display_name = "Local Server";
    sample_server.hostname = "localhost";
    sample_server.port = 3050;
    sample_server.description = "Local ScratchBird instance";
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
        if (layout.profile_name == name) {
            return &layout;
        }
    }
    return nullptr;
}

const ScreenLayout* AppConfig::findLayout(const std::string& name) const {
    for (const auto& layout : layouts_) {
        if (layout.profile_name == name) {
            return &layout;
        }
    }
    return nullptr;
}

void AppConfig::setCurrentLayout(const std::string& name) {
    current_layout_name_ = name;
    modified_ = true;
}

ServerConfig* AppConfig::findServer(const std::string& server_id) {
    for (auto& server : servers_) {
        if (server.server_id == server_id) {
            return &server;
        }
    }
    return nullptr;
}

bool AppConfig::hasEncryptedPasswords() const {
    for (const auto& server : servers_) {
        for (const auto& engine : server.engines) {
            for (const auto& db : engine.databases) {
                if (!db.encrypted_password.empty() && db.save_password) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool AppConfig::decryptPasswords(const std::string& key) {
    initEncryption();
    decryption_key_ = key;
    
    for (auto& server : servers_) {
        for (auto& engine : server.engines) {
            for (auto& db : engine.databases) {
                if (!db.encrypted_password.empty() && db.save_password) {
                    std::string decrypted = encryption_->decrypt(db.encrypted_password, key);
                    if (decrypted.empty() && !db.encrypted_password.empty()) {
                        // Decryption failed
                        return false;
                    }
                    // Store decrypted temporarily (will be re-encrypted on save)
                }
            }
        }
    }
    decrypted_ = true;
    return true;
}

std::string AppConfig::encryptPassword(const std::string& plaintext, const std::string& key) const {
    initEncryption();
    if (!encryption_) return "";
    return encryption_->encrypt(plaintext, key);
}

std::string AppConfig::decryptPassword(const std::string& ciphertext, const std::string& key) const {
    initEncryption();
    if (!encryption_) return "";
    return encryption_->decrypt(ciphertext, key);
}

// ============================================================================
// Multi-Monitor Handling
// ============================================================================

bool AppConfig::validateAndFixPositions() {
    bool fixed = false;
    
    for (auto& layout : layouts_) {
        for (auto& win : layout.windows) {
            wxRect rect = win.toRect();
            if (!isPositionOnScreen(rect)) {
                snapToVisibleArea(win);
                fixed = true;
            }
        }
    }
    
    if (fixed) {
        modified_ = true;
    }
    return fixed;
}

bool AppConfig::isPositionOnScreen(const wxRect& rect) const {
    // Check if at least part of the window is on any display
    int display_count = wxDisplay::GetCount();
    for (int i = 0; i < display_count; ++i) {
        wxDisplay display(static_cast<unsigned int>(i));
        wxRect client_area = display.GetClientArea();
        if (client_area.Intersects(rect)) {
            return true;
        }
    }
    return false;
}

void AppConfig::snapToVisibleArea(WindowLayout& layout) {
    wxRect primary = getPrimaryDisplayWorkArea();
    
    // Center on primary display
    layout.x = primary.x + (primary.width - layout.width) / 2;
    layout.y = primary.y + (primary.height - layout.height) / 2;
    
    // Ensure it's within bounds
    if (layout.x < primary.x) layout.x = primary.x;
    if (layout.y < primary.y) layout.y = primary.y;
    if (layout.x + layout.width > primary.x + primary.width) {
        layout.x = primary.x + primary.width - layout.width;
    }
    if (layout.y + layout.height > primary.y + primary.height) {
        layout.y = primary.y + primary.height - layout.height;
    }
    
    // Ensure minimum size
    if (layout.width < 400) layout.width = 400;
    if (layout.height < 300) layout.height = 300;
    
    layout.display_index = 0;  // Reset to primary
}

wxRect AppConfig::getPrimaryDisplayWorkArea() const {
    wxDisplay display(static_cast<unsigned int>(0));
    return display.GetClientArea();
}

std::vector<wxRect> AppConfig::getAllDisplayWorkAreas() const {
    std::vector<wxRect> areas;
    int display_count = wxDisplay::GetCount();
    for (int i = 0; i < display_count; ++i) {
        wxDisplay display(static_cast<unsigned int>(i));
        areas.push_back(display.GetClientArea());
    }
    return areas;
}

bool AppConfig::shouldAutoSavePositions() const {
    const auto* layout = findLayout(current_layout_name_);
    if (layout) {
        return layout->auto_save;
    }
    return true;
}

}  // namespace scratchrobin::core
