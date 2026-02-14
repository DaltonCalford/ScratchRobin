/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "session_state.h"

#include <algorithm>
#include <cctype>
#include <chrono>
#include <cstdio>
#include <fstream>
#include <sstream>

#ifdef __WXGTK__
#include <sys/stat.h>
#endif

namespace scratchrobin {

namespace {

// Helper to get current timestamp
int64_t GetCurrentTimestamp() {
    return std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

// String trimming utilities
std::string Trim(const std::string& value) {
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    auto start = std::find_if(value.begin(), value.end(), not_space);
    auto end = std::find_if(value.rbegin(), value.rend(), not_space).base();
    if (start >= end) {
        return {};
    }
    return std::string(start, end);
}

// Ensure directory exists
bool EnsureDirectoryExists(const std::string& path) {
#ifdef __WXGTK__
    struct stat st;
    if (stat(path.c_str(), &st) == 0) {
        return S_ISDIR(st.st_mode);
    }
    return mkdir(path.c_str(), 0755) == 0;
#else
    // Generic fallback - assume directory exists or will be created
    (void)path;
    return true;
#endif
}

} // anonymous namespace

// Private implementation class
class SessionStateManager::Impl {
public:
    std::string config_dir_;
    SessionState current_state_;
    ConnectionManager* connection_manager_ = nullptr;
    bool auto_reconnect_ = false;
    bool auto_save_running_ = false;
    int auto_save_interval_ = 30;
    
    // Thread-safety not needed for now (UI thread only)
    // but structure is ready for future async improvements
};

SessionStateManager::SessionStateManager() 
    : impl_(std::make_unique<Impl>()) {
}

SessionStateManager::~SessionStateManager() {
    StopAutoSave();
}

void SessionStateManager::Initialize(const std::string& config_dir) {
    impl_->config_dir_ = config_dir;
    
    // Ensure config directory exists
    EnsureDirectoryExists(config_dir);
    
    // Load existing session if available
    LoadSession(&impl_->current_state_);
}

void SessionStateManager::SaveWindowState(const WindowState& state) {
    // Remove existing entry for this window type + title combination
    auto& windows = impl_->current_state_.windows;
    windows.erase(
        std::remove_if(windows.begin(), windows.end(),
            [&state](const WindowState& w) {
                return w.window_type == state.window_type && w.title == state.title;
            }),
        windows.end());
    
    // Add new state
    windows.push_back(state);
    
    // Limit number of tracked windows (prevent unbounded growth)
    const size_t max_windows = 50;
    if (windows.size() > max_windows) {
        windows.erase(windows.begin(), windows.end() - max_windows);
    }
}

void SessionStateManager::RemoveWindowState(const std::string& window_type, 
                                            const std::string& title) {
    auto& windows = impl_->current_state_.windows;
    windows.erase(
        std::remove_if(windows.begin(), windows.end(),
            [&window_type, &title](const WindowState& w) {
                return w.window_type == window_type && w.title == title;
            }),
        windows.end());
}

std::vector<WindowState> SessionStateManager::GetWindowStates() const {
    return impl_->current_state_.windows;
}

void SessionStateManager::SaveEditorState(const EditorState& state) {
    // Remove existing entry for this file path (or content match for unsaved)
    auto& editors = impl_->current_state_.editors;
    editors.erase(
        std::remove_if(editors.begin(), editors.end(),
            [&state](const EditorState& e) {
                if (!state.file_path.empty()) {
                    return e.file_path == state.file_path;
                }
                // For unsaved buffers, match by content hash (simplified: content itself)
                return e.file_path.empty() && e.content == state.content;
            }),
        editors.end());
    
    // Add new state with timestamp
    EditorState new_state = state;
    new_state.last_modified = GetCurrentTimestamp();
    editors.push_back(new_state);
    
    // Limit number of tracked editors
    const size_t max_editors = 20;
    if (editors.size() > max_editors) {
        // Keep most recently modified
        std::sort(editors.begin(), editors.end(),
            [](const EditorState& a, const EditorState& b) {
                return a.last_modified > b.last_modified;
            });
        editors.resize(max_editors);
    }
}

void SessionStateManager::RemoveEditorState(const std::string& file_path) {
    auto& editors = impl_->current_state_.editors;
    editors.erase(
        std::remove_if(editors.begin(), editors.end(),
            [&file_path](const EditorState& e) {
                return e.file_path == file_path;
            }),
        editors.end());
}

std::vector<EditorState> SessionStateManager::GetEditorStates() const {
    return impl_->current_state_.editors;
}

void SessionStateManager::SetLastActiveProfile(const std::string& profile_name) {
    impl_->current_state_.last_active_profile = profile_name;
}

void SessionStateManager::AddRecentConnection(const std::string& profile_name) {
    if (profile_name.empty()) {
        return;
    }
    
    auto& recent = impl_->current_state_.recent_connections;
    
    // Remove if already exists (to move to front)
    recent.erase(
        std::remove(recent.begin(), recent.end(), profile_name),
        recent.end());
    
    // Add to front
    recent.insert(recent.begin(), profile_name);
    
    // Limit recent connections
    const size_t max_recent = 10;
    if (recent.size() > max_recent) {
        recent.resize(max_recent);
    }
}

std::vector<std::string> SessionStateManager::GetRecentConnections() const {
    return impl_->current_state_.recent_connections;
}

std::string SessionStateManager::GetLastActiveProfile() const {
    return impl_->current_state_.last_active_profile;
}

void SessionStateManager::SetAutoReconnect(bool enabled) {
    impl_->auto_reconnect_ = enabled;
}

bool SessionStateManager::GetAutoReconnect() const {
    return impl_->auto_reconnect_;
}

std::string SessionStateManager::GetSessionFilePath() const {
    return impl_->config_dir_ + "/session.toml";
}

std::string SessionStateManager::GetCrashFlagPath() const {
    return impl_->config_dir_ + "/.crash_flag";
}

std::string SessionStateManager::EscapeTomlString(const std::string& value) const {
    std::string result;
    result.reserve(value.size() + 2);
    result.push_back('"');
    for (char c : value) {
        if (c == '\\' || c == '"') {
            result.push_back('\\');
        } else if (c == '\n') {
            result.push_back('\\');
            result.push_back('n');
            continue;
        } else if (c == '\r') {
            result.push_back('\\');
            result.push_back('r');
            continue;
        } else if (c == '\t') {
            result.push_back('\\');
            result.push_back('t');
            continue;
        }
        result.push_back(c);
    }
    result.push_back('"');
    return result;
}

bool SessionStateManager::WriteSessionToFile(const SessionState& state, 
                                              const std::string& path) {
    std::ofstream file(path, std::ios::out | std::ios::trunc);
    if (!file.is_open()) {
        return false;
    }
    
    // Write header
    file << "# ScratchRobin Session State\n";
    file << "# Auto-generated file - do not edit manually\n\n";
    
    // Write metadata
    file << "[metadata]\n";
    file << "version = " << state.version << "\n";
    file << "timestamp = " << state.timestamp << "\n";
    file << "clean_exit = " << (state.clean_exit ? "true" : "false") << "\n";
    file << "auto_reconnect = " << (impl_->auto_reconnect_ ? "true" : "false") << "\n";
    
    if (!state.last_active_profile.empty()) {
        file << "last_active_profile = " << EscapeTomlString(state.last_active_profile) << "\n";
    }
    
    file << "\n";
    
    // Write recent connections
    if (!state.recent_connections.empty()) {
        file << "[[recent_connection]]\n";
        for (const auto& conn : state.recent_connections) {
            file << "profile = " << EscapeTomlString(conn) << "\n";
        }
        file << "\n";
    }
    
    // Write window states
    for (const auto& win : state.windows) {
        file << "[[window]]\n";
        file << "type = " << EscapeTomlString(win.window_type) << "\n";
        file << "title = " << EscapeTomlString(win.title) << "\n";
        file << "x = " << win.x << "\n";
        file << "y = " << win.y << "\n";
        file << "width = " << win.width << "\n";
        file << "height = " << win.height << "\n";
        file << "maximized = " << (win.maximized ? "true" : "false") << "\n";
        file << "minimized = " << (win.minimized ? "true" : "false") << "\n";
        file << "visible = " << (win.visible ? "true" : "false") << "\n";
        file << "\n";
    }
    
    // Write editor states
    for (const auto& ed : state.editors) {
        file << "[[editor]]\n";
        file << "file_path = " << EscapeTomlString(ed.file_path) << "\n";
        file << "cursor_position = " << ed.cursor_position << "\n";
        file << "connection_profile = " << EscapeTomlString(ed.connection_profile) << "\n";
        file << "last_modified = " << ed.last_modified << "\n";
        // Write content as a multiline literal string for safety
        file << "content = '''" << ed.content << "'''\n";
        file << "\n";
    }
    
    return file.good();
}

bool SessionStateManager::ReadSessionFromFile(const std::string& path, 
                                               SessionState* out_state) const {
    if (!out_state) {
        return false;
    }
    
    *out_state = SessionState{};
    
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    std::string section;
    WindowState current_window;
    EditorState current_editor;
    bool in_window = false;
    bool in_editor = false;
    bool in_content = false;
    std::string content_buffer;
    
    while (std::getline(file, line)) {
        std::string trimmed = Trim(line);
        
        // Skip comments and empty lines (except within content)
        if (!in_content && (trimmed.empty() || trimmed[0] == '#')) {
            continue;
        }
        
        // Check for section headers
        if (trimmed.size() >= 2 && trimmed.front() == '[' && trimmed.back() == ']') {
            // Save previous sections
            if (in_window) {
                out_state->windows.push_back(current_window);
                current_window = WindowState{};
                in_window = false;
            }
            if (in_editor) {
                if (in_content) {
                    current_editor.content = content_buffer;
                    in_content = false;
                }
                out_state->editors.push_back(current_editor);
                current_editor = EditorState{};
                content_buffer.clear();
                in_editor = false;
            }
            
            // Parse section name
            if (trimmed.size() >= 4 && trimmed.rfind("[[", 0) == 0) {
                // Array of tables
                std::string name = Trim(trimmed.substr(2, trimmed.size() - 4));
                if (name == "window") {
                    in_window = true;
                    current_window = WindowState{};
                } else if (name == "editor") {
                    in_editor = true;
                    current_editor = EditorState{};
                } else if (name == "recent_connection") {
                    // Handle inline
                }
            } else {
                // Regular section
                section = Trim(trimmed.substr(1, trimmed.size() - 2));
            }
            continue;
        }
        
        // Parse key-value pairs
        auto eq_pos = line.find('=');
        if (eq_pos == std::string::npos) {
            // Check for end of multiline content
            if (in_content && trimmed == "'''") {
                current_editor.content = content_buffer;
                content_buffer.clear();
                in_content = false;
            } else if (in_content) {
                if (!content_buffer.empty()) {
                    content_buffer.push_back('\n');
                }
                content_buffer.append(line);
            }
            continue;
        }
        
        std::string key = Trim(line.substr(0, eq_pos));
        std::string value = Trim(line.substr(eq_pos + 1));
        
        // Handle multiline content start
        if (in_editor && key == "content" && value == "'''") {
            in_content = true;
            content_buffer.clear();
            continue;
        }
        
        // Parse values
        auto parse_string = [](const std::string& v) -> std::string {
            if (v.size() >= 2 && v.front() == '"' && v.back() == '"') {
                std::string inner = v.substr(1, v.size() - 2);
                // Unescape
                std::string result;
                for (size_t i = 0; i < inner.size(); ++i) {
                    if (inner[i] == '\\' && i + 1 < inner.size()) {
                        char next = inner[i + 1];
                        if (next == 'n') {
                            result.push_back('\n');
                        } else if (next == 'r') {
                            result.push_back('\r');
                        } else if (next == 't') {
                            result.push_back('\t');
                        } else {
                            result.push_back(next);
                        }
                        ++i;
                    } else {
                        result.push_back(inner[i]);
                    }
                }
                return result;
            }
            return v;
        };
        
        auto parse_bool = [](const std::string& v) -> bool {
            std::string lower;
            for (char c : v) {
                lower.push_back(static_cast<char>(std::tolower(c)));
            }
            return lower == "true" || lower == "yes" || lower == "1";
        };
        
        auto parse_int = [](const std::string& v) -> int {
            try {
                return std::stoi(v);
            } catch (...) {
                return 0;
            }
        };
        
        auto parse_int64 = [](const std::string& v) -> int64_t {
            try {
                return std::stoll(v);
            } catch (...) {
                return 0;
            }
        };
        
        // Process based on section
        if (section == "metadata") {
            if (key == "version") {
                out_state->version = parse_int(value);
            } else if (key == "timestamp") {
                out_state->timestamp = parse_int64(value);
            } else if (key == "clean_exit") {
                out_state->clean_exit = parse_bool(value);
            } else if (key == "auto_reconnect") {
                out_state->auto_reconnect = parse_bool(value);
            } else if (key == "last_active_profile") {
                out_state->last_active_profile = parse_string(value);
            }
        } else if (in_window) {
            if (key == "type") {
                current_window.window_type = parse_string(value);
            } else if (key == "title") {
                current_window.title = parse_string(value);
            } else if (key == "x") {
                current_window.x = parse_int(value);
            } else if (key == "y") {
                current_window.y = parse_int(value);
            } else if (key == "width") {
                current_window.width = parse_int(value);
            } else if (key == "height") {
                current_window.height = parse_int(value);
            } else if (key == "maximized") {
                current_window.maximized = parse_bool(value);
            } else if (key == "minimized") {
                current_window.minimized = parse_bool(value);
            } else if (key == "visible") {
                current_window.visible = parse_bool(value);
            }
        } else if (in_editor && !in_content) {
            if (key == "file_path") {
                current_editor.file_path = parse_string(value);
            } else if (key == "cursor_position") {
                current_editor.cursor_position = parse_int(value);
            } else if (key == "connection_profile") {
                current_editor.connection_profile = parse_string(value);
            } else if (key == "last_modified") {
                current_editor.last_modified = parse_int64(value);
            } else if (key == "content") {
                // Single-line content (starts with " not ''')
                if (value.size() >= 2 && value.front() == '"') {
                    current_editor.content = parse_string(value);
                }
            }
        } else if (key == "profile") {
            // Recent connection entry
            out_state->recent_connections.push_back(parse_string(value));
        }
    }
    
    // Save final sections
    if (in_window) {
        out_state->windows.push_back(current_window);
    }
    if (in_editor) {
        if (in_content) {
            current_editor.content = content_buffer;
        }
        out_state->editors.push_back(current_editor);
    }
    
    return true;
}

bool SessionStateManager::SaveSession(bool clean_exit) {
    impl_->current_state_.timestamp = GetCurrentTimestamp();
    impl_->current_state_.clean_exit = clean_exit;
    
    std::string path = GetSessionFilePath();
    
    // Ensure directory exists
    size_t last_slash = path.find_last_of("/\\");
    if (last_slash != std::string::npos) {
        EnsureDirectoryExists(path.substr(0, last_slash));
    }
    
    return WriteSessionToFile(impl_->current_state_, path);
}

bool SessionStateManager::LoadSession(SessionState* out_state) {
    if (!out_state) {
        return false;
    }
    
    std::string path = GetSessionFilePath();
    bool result = ReadSessionFromFile(path, out_state);
    
    // Sync auto_reconnect setting from loaded state
    if (result) {
        impl_->auto_reconnect_ = out_state->auto_reconnect;
    }
    
    return result;
}

void SessionStateManager::ClearSession() {
    impl_->current_state_ = SessionState{};
    
    // Remove session file
    std::string path = GetSessionFilePath();
    std::remove(path.c_str());
}

bool SessionStateManager::HasCrashRecoveryData() const {
    return WasUncleanShutdown();
}

RecoveryInfo SessionStateManager::GetRecoveryInfo() const {
    RecoveryInfo info;
    
    // Load the last session state
    SessionState state;
    std::string path = GetSessionFilePath();
    
    if (ReadSessionFromFile(path, &state)) {
        // Filter for unsaved editors (those with empty file_path)
        for (const auto& ed : state.editors) {
            if (ed.file_path.empty() && !ed.content.empty()) {
                info.unsaved_editors.push_back(ed);
            }
        }
        
        info.recent_connections = state.recent_connections;
        info.last_active_profile = state.last_active_profile;
        info.crash_timestamp = state.timestamp;
    }
    
    return info;
}

bool SessionStateManager::PerformRecovery(const RecoveryInfo& info,
                                           std::function<void(const EditorState&)> editor_restorer,
                                           std::function<void(const std::string&)> connection_restorer) {
    if (!editor_restorer && !connection_restorer) {
        return false;
    }
    
    bool success = true;
    
    // Restore editors
    if (editor_restorer) {
        for (const auto& ed : info.unsaved_editors) {
            try {
                editor_restorer(ed);
            } catch (...) {
                success = false;
            }
        }
    }
    
    // Restore connection if auto-reconnect is enabled
    if (connection_restorer && impl_->auto_reconnect_ && !info.last_active_profile.empty()) {
        try {
            connection_restorer(info.last_active_profile);
        } catch (...) {
            success = false;
        }
    }
    
    // Clear crash flag on successful recovery
    if (success) {
        MarkCrashFlag(false);
    }
    
    return success;
}

void SessionStateManager::MarkCrashFlag(bool crashed) {
    std::string flag_path = GetCrashFlagPath();
    
    if (crashed) {
        std::ofstream file(flag_path, std::ios::out | std::ios::trunc);
        if (file.is_open()) {
            file << GetCurrentTimestamp() << "\n";
        }
    } else {
        std::remove(flag_path.c_str());
    }
}

bool SessionStateManager::WasUncleanShutdown() const {
    std::string flag_path = GetCrashFlagPath();
    std::ifstream file(flag_path);
    return file.is_open();
}

void SessionStateManager::StartAutoSave(int interval_seconds) {
    impl_->auto_save_interval_ = interval_seconds;
    impl_->auto_save_running_ = true;
    
    // Mark crash flag at start (will be cleared on clean exit)
    MarkCrashFlag(true);
    
    // Initial save
    SaveSession(false);
}

void SessionStateManager::StopAutoSave() {
    impl_->auto_save_running_ = false;
}

void SessionStateManager::TriggerAutoSave() {
    if (impl_->auto_save_running_) {
        SaveSession(false);
    }
}

bool SessionStateManager::IsAutoSaveRunning() const {
    return impl_->auto_save_running_;
}

void SessionStateManager::SetConnectionManager(ConnectionManager* manager) {
    impl_->connection_manager_ = manager;
}

} // namespace scratchrobin
