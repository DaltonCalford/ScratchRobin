# ScratchRobin Preferences System Specification

**Status**: ✅ **IMPLEMENTED**  
**Last Updated**: 2026-02-03  
**Implementation**: `src/ui/preferences_dialog.cpp`

---

## Overview

The Preferences System provides a centralized configuration interface for all ScratchRobin settings. Settings are stored in TOML format and organized into logical categories.

---

## Configuration File

**Location:**
- Linux: `~/.config/scratchrobin/scratchrobin.toml`
- macOS: `~/Library/Application Support/ScratchRobin/scratchrobin.toml`
- Windows: `%APPDATA%\ScratchRobin\scratchrobin.toml`

**Format:** TOML (Tom's Obvious, Minimal Language)

---

## Preference Categories

### 1. Editor Preferences

**Configuration Keys:**
```toml
[editor]
font_family = "Consolas"
font_size = 11
tab_width = 4
use_spaces_for_tabs = true
auto_indent = true
show_line_numbers = true
highlight_current_line = true
word_wrap = false
show_whitespace = false
auto_save = true
auto_save_interval_seconds = 60

[editor.colors]
background = "#FFFFFF"
foreground = "#000000"
current_line = "#F5F5F5"
selection = "#0078D7"
comment = "#008000"
keyword = "#0000FF"
string = "#A31515"
number = "#098658"
```

**UI Controls:**
- Font family selector
- Font size spinner
- Tab width dropdown (2, 4, 8)
- Spaces vs tabs radio buttons
- Checkboxes for display options
- Color pickers for syntax highlighting

### 2. Results Grid Preferences

**Configuration Keys:**
```toml
[results]
max_rows = 10000
page_size = 1000
show_row_numbers = true
null_display = "NULL"
true_display = "true"
false_display = "false"
date_format = "YYYY-MM-DD"
datetime_format = "YYYY-MM-DD HH:MM:SS"
font_family = "Consolas"
font_size = 10
auto_resize_columns = true
alternate_row_colors = true

[results.colors]
header_background = "#F0F0F0"
alternate_row = "#F9F9F9"
null_value = "#808080"
```

**UI Controls:**
- Max rows spinner (1000-100000)
- Page size dropdown
- Display format fields
- Checkboxes for grid options

### 3. Connection Preferences

**Configuration Keys:**
```toml
[connection]
default_connect_timeout_seconds = 30
default_query_timeout_seconds = 0
auto_reconnect = true
max_reconnect_attempts = 3
ssl_verify_mode = "prefer"
default_backend = "ScratchBird"

[connection.defaults]
host = "localhost"
port = 3092
```

**UI Controls:**
- Timeout spinners
- SSL mode dropdown
- Auto-reconnect checkbox
- Default backend selector

### 4. Export Preferences

**Configuration Keys:**
```toml
[export]
default_format = "CSV"
csv_delimiter = ","
csv_quote_char = "\""
csv_escape_char = "\""
csv_include_header = true
csv_null_value = "NULL"
json_pretty_print = false
json_include_metadata = false
```

**UI Controls:**
- Default format dropdown
- CSV delimiter selection
- Header checkbox
- JSON pretty print toggle

### 5. Diagram Preferences

**Configuration Keys:**
```toml
[diagram]
default_notation = "crowsfoot"
default_paper_size = "A4"
default_orientation = "landscape"
grid_visible = true
grid_snap = true
grid_size = 20
show_page_breaks = false
auto_save_diagrams = true

[diagram.colors]
entity_fill = "#FFFFFF"
entity_border = "#000000"
header_fill = "#E0E0E0"
primary_key = "#0000FF"
foreign_key = "#008000"
relationship_line = "#000000"
```

**UI Controls:**
- Default notation dropdown
- Grid options
- Color pickers
- Paper size selector

### 6. Network Preferences

**Configuration Keys:**
```toml
[network]
proxy_enabled = false
proxy_host = ""
proxy_port = 8080
proxy_username = ""
proxy_password = ""  # Encrypted
use_system_proxy = false
```

**UI Controls:**
- Enable proxy checkbox
- Host/port fields
- Authentication fields
- System proxy checkbox

---

## UI Requirements

### Preferences Dialog

**Layout:**
```
+--------------------------------------------------+
| Preferences                                      |
+--------------------------------------------------+
| +--------------+  +---------------------------+  |
| | Categories   |  | Settings Panel            |  |
| |              |  |                           |  |
| | • Editor     |  | [Editor settings          |  |
| | • Results    |  |  specific to category]    |  |
| | • Connection |  |                           |  |
| | • Export     |  |                           |  |
| | • Diagram    |  |                           |  |
| | • Network    |  |                           |  |
| |              |  |                           |  |
| |              |  |                           |  |
| +--------------+  +---------------------------+  |
|                                                  |
| [Restore Defaults]   [Cancel]  [OK]              |
+--------------------------------------------------+
```

**Category Navigation:**
- Tree/list on left
- Settings panel on right
- Search/filter box at top
- Apply/OK/Cancel buttons

### Editor Settings Panel

**Sections:**
- **Font**: Family, size, line height
- **Tabs**: Width, spaces vs tabs
- **Display**: Line numbers, current line, whitespace
- **Behavior**: Auto-save, auto-indent
- **Colors**: Syntax highlighting colors

### Results Settings Panel

**Sections:**
- **Limits**: Max rows, page size
- **Display**: Row numbers, null display
- **Formatting**: Date/time formats
- **Appearance**: Colors, fonts

### Connection Settings Panel

**Sections:**
- **Timeouts**: Connect, query
- **SSL/TLS**: Default verify mode
- **Defaults**: Auto-reconnect settings

---

## Implementation Details

### Files
- `src/ui/preferences_dialog.h` / `.cpp` - Main preferences window
- `src/core/config.h` / `.cpp` - Configuration storage

### Key Classes
```cpp
class PreferencesDialog : public wxDialog {
    // Category tree/list
    // Settings panels
    // Apply/restore logic
};

class AppConfig {
    // Configuration structure
    // Load/save TOML
    // Default values
};
```

### Persistence
- Load on startup
- Save on OK/Apply
- Auto-save for specific settings (editor)
- Backup before save

### Migration
- Version detection in config file
- Automatic migration from old formats
- Deprecated setting warnings

---

## Default Values

All preferences have sensible defaults defined in code. Users can restore defaults at any time via "Restore Defaults" button (per-category or global).

---

## Future Enhancements

- Import/export preferences
- Per-project settings
- Cloud sync of settings
- Settings search
- Settings profiles (workspaces)
