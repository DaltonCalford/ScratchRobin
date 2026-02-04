/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_SESSION_STATE_H
#define SCRATCHROBIN_SESSION_STATE_H

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace scratchrobin {

// Window state for position, size, and visibility
struct WindowState {
    std::string window_type;  // "MainFrame", "SqlEditorFrame", "DiagramFrame", etc.
    std::string title;
    int x = 0;
    int y = 0;
    int width = 800;
    int height = 600;
    bool maximized = false;
    bool minimized = false;
    bool visible = true;
};

// Editor state for SQL content persistence
struct EditorState {
    std::string file_path;        // empty if unsaved
    std::string content;
    int cursor_position = 0;
    std::string connection_profile;
    int64_t last_modified = 0;    // timestamp for recovery ordering
};

// Complete session state snapshot
struct SessionState {
    std::vector<WindowState> windows;
    std::vector<EditorState> editors;
    std::vector<std::string> recent_connections;
    std::string last_active_profile;
    int64_t timestamp = 0;
    bool clean_exit = false;
    bool auto_reconnect = false;
    int version = 1;              // for future migration
};

// Recovery information for crash recovery dialog
struct RecoveryInfo {
    std::vector<EditorState> unsaved_editors;
    std::vector<std::string> recent_connections;
    std::string last_active_profile;
    int64_t crash_timestamp = 0;
};

// Interface for objects that can provide session state
class SessionStateProvider {
public:
    virtual ~SessionStateProvider() = default;
    virtual WindowState GetWindowState() const = 0;
    virtual EditorState GetEditorState() const = 0;
    virtual bool HasUnsavedContent() const = 0;
};

// Forward declarations
class ConnectionManager;

// Session state manager for persistence and crash recovery
class SessionStateManager {
public:
    SessionStateManager();
    ~SessionStateManager();

    // Initialize with config directory path
    void Initialize(const std::string& config_dir);

    // Window state management
    void SaveWindowState(const WindowState& state);
    void RemoveWindowState(const std::string& window_type, const std::string& title);
    std::vector<WindowState> GetWindowStates() const;

    // Editor state management
    void SaveEditorState(const EditorState& state);
    void RemoveEditorState(const std::string& file_path);
    std::vector<EditorState> GetEditorStates() const;

    // Connection state management
    void SetLastActiveProfile(const std::string& profile_name);
    void AddRecentConnection(const std::string& profile_name);
    std::vector<std::string> GetRecentConnections() const;
    std::string GetLastActiveProfile() const;

    // Auto-reconnect setting
    void SetAutoReconnect(bool enabled);
    bool GetAutoReconnect() const;

    // Full session operations
    bool SaveSession(bool clean_exit = false);
    bool LoadSession(SessionState* out_state);
    void ClearSession();

    // Crash recovery
    bool HasCrashRecoveryData() const;
    RecoveryInfo GetRecoveryInfo() const;
    bool PerformRecovery(const RecoveryInfo& info, 
                         std::function<void(const EditorState&)> editor_restorer,
                         std::function<void(const std::string&)> connection_restorer);
    void MarkCrashFlag(bool crashed);
    bool WasUncleanShutdown() const;

    // Periodic auto-save
    void StartAutoSave(int interval_seconds = 30);
    void StopAutoSave();
    void TriggerAutoSave();
    bool IsAutoSaveRunning() const;

    // Set the active connection manager for session tracking
    void SetConnectionManager(ConnectionManager* manager);

private:
    std::string GetSessionFilePath() const;
    std::string GetCrashFlagPath() const;
    std::string EscapeTomlString(const std::string& value) const;
    bool WriteSessionToFile(const SessionState& state, const std::string& path);
    bool ReadSessionFromFile(const std::string& path, SessionState* out_state) const;

    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_SESSION_STATE_H
