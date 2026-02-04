# ScratchRobin Session State Persistence Specification

**Status**: ✅ **IMPLEMENTED**  
**Last Updated**: 2026-02-03  
**Implementation**: `src/core/session_state.cpp`

---

## Overview

Session State Persistence allows ScratchRobin to save and restore the application state across restarts. This includes open windows, editor content, connection history, and UI layout.

---

## Scope

### Persisted State
- Open windows and positions
- SQL Editor content (unsaved buffers)
- Connection history
- Recent files
- Tree expansion state
- Inspector panel selection
- Grid column widths

### Not Persisted
- Query results (cleared on restart)
- Active transactions (rolled back)
- Undo/redo stacks
- Temporary data

---

## Storage

### File Location
- Linux: `~/.local/state/scratchrobin/session.json`
- macOS: `~/Library/Application Support/ScratchRobin/session.json`
- Windows: `%LOCALAPPDATA%\ScratchRobin\session.json`

### Format: JSON

```json
{
  "version": "1.0",
  "saved_at": "2026-02-03T14:30:22.123Z",
  "crash_recovery": false,
  "windows": [
    {
      "type": "sql_editor",
      "id": "editor_001",
      "title": "Query 1",
      "geometry": {"x": 100, "y": 100, "width": 800, "height": 600},
      "maximized": false,
      "content": "SELECT * FROM users;",
      "cursor_position": 20,
      "has_unsaved_changes": true,
      "connection_profile": "Production DB"
    },
    {
      "type": "diagram",
      "id": "diagram_001",
      "title": "ERD - mydb",
      "geometry": {"x": 150, "y": 150, "width": 1000, "height": 700},
      "file_path": "/home/user/diagrams/mydb.sberd",
      "zoom_level": 1.0
    }
  ],
  "connections": {
    "last_active": "Production DB",
    "recent": [
      {"name": "Production DB", "last_connected": "2026-02-03T14:30:00Z"},
      {"name": "Development", "last_connected": "2026-02-02T10:15:00Z"}
    ]
  },
  "ui_state": {
    "main_window": {
      "geometry": {"x": 50, "y": 50, "width": 1200, "height": 800},
      "maximized": true
    },
    "catalog_tree": {
      "expanded_nodes": ["public", "public.users", "public.orders"],
      "selected_node": "public.users",
      "filter_text": ""
    },
    "inspector": {
      "active_tab": "DDL"
    }
  },
  "editor_history": [
    "SELECT * FROM users WHERE active = true",
    "UPDATE orders SET status = 'shipped'",
    "DELETE FROM temp_logs WHERE created < NOW() - INTERVAL '7 days'"
  ],
  "grid_settings": {
    "users": {
      "column_widths": {"id": 50, "name": 150, "email": 200}
    }
  }
}
```

---

## Auto-Save Behavior

### Triggers
- Every 60 seconds (configurable)
- On window close
- On connection change
- Before long-running operations

### Crash Recovery
- Detect unclean shutdown
- Offer to restore previous session
- Show list of unsaved buffers
- Option to discard or recover each

---

## UI Requirements

### Recovery Dialog (on startup after crash)

```
+--------------------------------------------------+
| Session Recovery                                 |
+--------------------------------------------------+
|                                                  |
| ScratchRobin did not shut down cleanly.          |
|                                                  |
| The following windows had unsaved changes:       |
|                                                  |
| [x] Query 1 - "SELECT * FROM..." (modified)     |
| [x] Query 2 - "UPDATE orders..." (modified)     |
| [ ] Diagram - "mydb.sberd" (saved)              |
|                                                  |
| What would you like to do?                      |
|                                                  |
| ( ) Restore all windows with changes            |
| (*) Restore only selected windows               |
| ( ) Start fresh (discard all)                   |
|                                                  |
| [ ] Always ask on startup                       |
|                                                  |
| [View Details...]      [Cancel]  [Recover]      |
+--------------------------------------------------+
```

### Preferences Integration

Settings in Preferences dialog:
- Enable session persistence (checkbox)
- Auto-save interval (spinner)
- Maximum history entries (spinner)
- Clear session on exit (checkbox)
- Confirm before restoring (checkbox)

---

## Implementation Details

### Files
- `src/core/session_state.h` / `.cpp` - Session persistence

### Key Classes
```cpp
class SessionState {
public:
    // Window state
    void SaveWindowState(const WindowState& state);
    std::vector<WindowState> LoadWindowStates();
    
    // Editor state
    void SaveEditorContent(const std::string& id, const std::string& content);
    std::optional<std::string> LoadEditorContent(const std::string& id);
    
    // Tree state
    void SetTreeNodeExpanded(const std::string& path, bool expanded);
    bool IsTreeNodeExpanded(const std::string& path);
    
    // Recent connections
    void AddRecentConnection(const ConnectionState& conn);
    std::vector<ConnectionState> GetRecentConnections();
    
    // Save/Load
    bool SaveToFile(const std::filesystem::path& path);
    bool LoadFromFile(const std::filesystem::path& path);
    
    // Crash detection
    bool WasPreviousSessionClean();
    void MarkSessionClean(bool clean);
};
```

### Data Structures
```cpp
struct EditorState {
    std::string id;
    std::string title;
    std::string content;
    int cursor_position;
    int selection_start;
    int selection_end;
    bool has_unsaved_changes;
    std::optional<std::string> file_path;
    std::optional<std::string> connection_profile;
};

struct WindowGeometry {
    int x, y, width, height;
    bool maximized;
};

struct SessionData {
    std::string version;
    std::chrono::system_clock::time_point saved_at;
    std::vector<EditorState> editors;
    std::vector<std::string> expanded_tree_nodes;
    std::vector<std::string> recent_connections;
    WindowGeometry main_window;
    std::map<std::string, std::map<std::string, int>> grid_column_widths;
};
```

---

## Security Considerations

- Session file is stored in user's home directory (proper permissions)
- SQL content may contain sensitive data
- Consider encryption for session file (future)
- No passwords stored in session

---

## Future Enhancements

- Cloud-synced sessions across devices
- Named session profiles
- Session timeline/versioning
- Selective restore (choose which windows)
- Session sharing (export/import)
