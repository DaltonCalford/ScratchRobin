# Toolbar System Specification

**Version**: 1.0  
**Status**: Draft  
**Last Updated**: 2026-02-06

---

## 1. Overview

This specification defines the comprehensive toolbar system for ScratchRobin, including naming conventions, default/custom toolbars, icon management, templates, position persistence, and the icon-to-functionality mapping system.

### 1.1 Goals

- **Consistency**: Uniform naming and behavior across all toolbars
- **Customizability**: Users can create, modify, and save custom toolbars
- **Context Sensitivity**: Toolbars appear/disappear based on active document type
- **Persistence**: Toolbar positions and configurations are saved/restored
- **Extensibility**: New functionality can be surfaced by adding icons

---

## 2. Toolbar Architecture

### 2.1 Toolbar Types

| Type | Description | Visibility |
|------|-------------|------------|
| **Default** | Standard toolbars included with application | Always available |
| **Custom** | User-created toolbars | User-defined |
| **Context** | Document-type specific | Context-dependent |
| **Temporary** | Dynamically created | Task-specific lifetime |

---

## 3. Naming Conventions

### 3.1 Toolbar Naming

```
<category>.<name>.<variant>

Examples:
- main.standard          (Main application toolbar)
- main.compact          (Compact version)
- sql.editor            (SQL editor toolbar)
- diagram.erd           (ERD diagram toolbar)
- custom.user1          (User-created toolbar)
```

### 3.2 Categories

| Category | Purpose |
|----------|---------|
| `main` | Application-level toolbars |
| `sql` | SQL editor related |
| `diagram` | Diagram/ERD related |
| `table` | Table designer related |
| `view` | View editor related |
| `proc` | Procedure/function editor |
| `custom` | User-created toolbars |

### 3.3 Icon Naming

```
<action>_<object>[_<variant>]

Examples:
- file_new              (New file/document)
- file_open             (Open file)
- file_save             (Save file)
- edit_cut              (Cut)
- edit_copy             (Copy)
- sql_execute           (Execute SQL)
- diagram_add_table     (Add table to diagram)
```

---

## 4. Default Toolbars

### 4.1 Main Toolbar (`main.standard`)

| Icon ID | Action | Shortcut |
|---------|--------|----------|
| file_new | New SQL Editor | Ctrl+N |
| file_open | Open File | Ctrl+O |
| file_save | Save | Ctrl+S |
| (separator) | | |
| edit_cut | Cut | Ctrl+X |
| edit_copy | Copy | Ctrl+C |
| edit_paste | Paste | Ctrl+V |
| (separator) | | |
| edit_undo | Undo | Ctrl+Z |
| edit_redo | Redo | Ctrl+Y |
| (separator) | | |
| view_search | Find | Ctrl+F |
| tools_preferences | Preferences | |

### 4.2 SQL Editor Toolbar (`sql.editor`)

| Icon ID | Action | Shortcut |
|---------|--------|----------|
| sql_execute | Execute | F5 |
| sql_execute_selection | Execute Selection | Ctrl+F5 |
| sql_explain | Explain Plan | |
| sql_format | Format SQL | Ctrl+Shift+F |
| (separator) | | |
| sql_history | Query History | |
| sql_bookmarks | Bookmarks | |

### 4.3 Diagram Toolbar (`diagram.erd`)

| Icon ID | Action | Shortcut |
|---------|--------|----------|
| diagram_add_table | Add Table | |
| diagram_add_relation | Add Relationship | |
| (separator) | | |
| diagram_zoom_in | Zoom In | Ctrl++ |
| diagram_zoom_out | Zoom Out | Ctrl+- |
| diagram_fit | Fit to Window | |
| (separator) | | |
| diagram_auto_layout | Auto Layout | |
| diagram_reverse_engineer | Reverse Engineer | |
| diagram_forward_engineer | Generate DDL | |

---

## 5. Custom Toolbar System

### 5.1 Creating Custom Toolbars

Users can create custom toolbars through:
1. **UI**: Right-click on toolbar area -> "Customize Toolbars"
2. **Drag-and-drop**: Drag icons from palette to toolbar
3. **Configuration**: Edit toolbar JSON directly

### 5.2 Custom Toolbar Format

```json
{
    "id": "custom.my_tools",
    "name": "My Tools",
    "description": "Frequently used operations",
    "category": "custom",
    "default_visible": true,
    "default_dock": "top",
    "icon_size": 24,
    "show_labels": false,
    "items": [
        {
            "type": "icon",
            "id": "sql_execute",
            "action": "sql.execute",
            "shortcut": "F5",
            "tooltip": "Execute SQL"
        },
        {
            "type": "separator"
        },
        {
            "type": "dropdown",
            "id": "recent_connections",
            "label": "Recent",
            "items": [
                {"label": "Production DB", "action": "conn.connect:prod"},
                {"label": "Staging DB", "action": "conn.connect:staging"}
            ]
        }
    ]
}
```

### 5.3 Toolbar Item Types

| Type | Description | Properties |
|------|-------------|------------|
| `icon` | Clickable icon button | id, action, icon, tooltip, shortcut |
| `separator` | Visual separator | - |
| `spacer` | Empty space | width |
| `dropdown` | Dropdown menu | id, label, items |
| `text` | Text input field | id, width, placeholder |
| `label` | Static text | text |
| `toggle` | Toggle button | id, action, checked |
| `combo` | Dropdown combo box | id, items, selected |

---

## 6. Icon-to-Functionality Mapping

### 6.1 Action System

The action system provides loose coupling between icons and functionality:

```
Icon Click -> Action ID -> Action Handler -> Functionality
```

### 6.2 Action Registration

```cpp
// Register an action
ActionRegistry::Register("sql.execute", {
    .name = "Execute SQL",
    .description = "Execute the current SQL query",
    .icon = "sql_execute",
    .shortcut = "F5",
    .handler = [](ActionContext& ctx) {
        auto* editor = ctx.GetActiveEditor<SqlEditor>();
        if (editor) {
            editor->Execute();
        }
    },
    .can_execute = [](ActionContext& ctx) {
        return ctx.HasActiveEditor<SqlEditor>();
    }
});
```

### 6.3 Action Context

```cpp
struct ActionContext {
    IDocumentWindow* active_document = nullptr;
    DocumentType document_type = DocumentType::Unknown;
    std::string selected_text;
    std::string selected_object;
    ConnectionProfile* active_connection = nullptr;
    bool is_connected = false;
    
    template<typename T>
    T* GetActiveEditor() const;
    
    bool HasSelection() const;
    bool HasActiveEditor() const;
};
```

### 6.4 Standard Actions Reference

#### File Actions

| Action ID | Description | Context |
|-----------|-------------|---------|
| `file.new_sql` | New SQL Editor | Global |
| `file.new_diagram` | New Diagram | Global |
| `file.open` | Open File | Global |
| `file.save` | Save | Document |
| `file.save_as` | Save As | Document |
| `file.save_all` | Save All | Global |
| `file.close` | Close | Document |

#### Edit Actions

| Action ID | Description | Context |
|-----------|-------------|---------|
| `edit.undo` | Undo | Document |
| `edit.redo` | Redo | Document |
| `edit.cut` | Cut | Selection |
| `edit.copy` | Copy | Selection |
| `edit.paste` | Paste | Clipboard |
| `edit.select_all` | Select All | Document |
| `edit.find` | Find | Document |
| `edit.replace` | Replace | Document |

#### SQL Actions

| Action ID | Description | Context |
|-----------|-------------|---------|
| `sql.execute` | Execute | SQL Editor |
| `sql.execute_selection` | Execute Selection | SQL Editor |
| `sql.explain` | Explain Plan | SQL Editor |
| `sql.format` | Format SQL | SQL Editor |
| `sql.comment` | Toggle Comment | SQL Editor |
| `sql.bookmark` | Toggle Bookmark | SQL Editor |
| `sql.show_history` | Show History | SQL Editor |

#### Diagram Actions

| Action ID | Description | Context |
|-----------|-------------|---------|
| `diagram.add_table` | Add Table | Diagram |
| `diagram.add_relation` | Add Relationship | Diagram |
| `diagram.delete` | Delete Selected | Diagram |
| `diagram.zoom_in` | Zoom In | Diagram |
| `diagram.zoom_out` | Zoom Out | Diagram |
| `diagram.zoom_fit` | Fit to Window | Diagram |
| `diagram.auto_layout` | Auto Layout | Diagram |
| `diagram.reverse_engineer` | Reverse Engineer | Diagram |
| `diagram.forward_engineer` | Generate DDL | Diagram |

---

## 7. Position Persistence

### 7.1 Toolbar State Format

```json
{
    "toolbar_states": {
        "main.standard": {
            "visible": true,
            "docked": true,
            "dock_direction": "top",
            "dock_row": 0,
            "dock_position": 0,
            "floating_rect": null,
            "icon_size": 24,
            "show_labels": false,
            "orientation": "horizontal"
        },
        "sql.editor": {
            "visible": true,
            "docked": true,
            "dock_direction": "top",
            "dock_row": 1,
            "dock_position": 0,
            "icon_size": 24,
            "show_labels": true
        },
        "custom.my_tools": {
            "visible": true,
            "docked": false,
            "floating_rect": {
                "x": 100,
                "y": 100,
                "width": 400,
                "height": 40
            },
            "icon_size": 32,
            "show_labels": false
        }
    }
}
```

### 7.2 State Persistence Rules

| Aspect | Persistence |
|--------|-------------|
| Position | Saved per layout preset |
| Visibility | Saved per layout preset |
| Icon Size | Global setting |
| Show Labels | Global setting |
| Custom Toolbars | Global, all layouts |
| Custom Items | Part of toolbar definition |

---

## 8. Toolbar Templates

### 8.1 Template System

Templates provide starting points for creating toolbars:

| Template | Description | Target |
|----------|-------------|--------|
| `minimal` | Most common actions | Beginners |
| `standard` | Full feature set | General use |
| `advanced` | All available actions | Power users |
| `compact` | Small icons, no labels | Limited space |

### 8.2 Template Definition

```json
{
    "template_id": "sql.minimal",
    "name": "Minimal SQL Toolbar",
    "base": "sql.editor",
    "items": [
        "sql.execute",
        "sql.execute_selection",
        "separator",
        "sql.format",
        "separator",
        "sql.bookmark"
    ]
}
```

---

## 9. Icon Management

### 9.1 Icon Sources

Icons are loaded from multiple sources in priority order:

1. **User Icons** (`~/.config/ScratchRobin/icons/`)
2. **Theme Icons** (current theme directory)
3. **Built-in Icons** (application resources)
4. **System Icons** (platform icon theme)

### 9.2 Icon Sizes

| Size | Use Case |
|------|----------|
| 16px | Compact toolbars, high DPI |
| 24px | Standard toolbars |
| 32px | Large toolbars, touch-friendly |
| 48px | Floating palettes |

### 9.3 Icon Themes

- `default` - Standard ScratchRobin icons
- `monochrome` - Single color icons
- `colorful` - Full color icons
- `outline` - Outline style icons

---

## 10. Implementation

### 10.1 Class Structure

```cpp
class IconBarHost {
    void RegisterToolbar(std::shared_ptr<Toolbar> toolbar);
    void UnregisterToolbar(const std::string& id);
    std::shared_ptr<Toolbar> GetToolbar(const std::string& id);
    void ShowToolbar(const std::string& id, bool show);
    void SaveState();
    void RestoreState();
};

class Toolbar {
    std::string id_;
    std::string name_;
    std::vector<ToolbarItem> items_;
    ToolbarState state_;
    
    void AddItem(const ToolbarItem& item);
    void RemoveItem(const std::string& id);
    void SetIconSize(int size);
    void SetShowLabels(bool show);
    void SaveToJson();
    void LoadFromJson(const JsonValue& json);
};

class ActionRegistry {
    static void Register(const std::string& id, const Action& action);
    static Action* Get(const std::string& id);
    static bool Execute(const std::string& id, ActionContext& ctx);
    static bool CanExecute(const std::string& id, ActionContext& ctx);
};
```

### 10.2 Configuration Files

```
~/.config/ScratchRobin/
├── toolbars/
│   ├── custom/                    # User-created toolbars
│   │   ├── my_tools.json
│   │   └── quick_actions.json
│   └── templates/                 # User templates
│       └── minimal_sql.json
├── toolbar_state.json             # Current toolbar positions
└── icons/
    ├── custom/                    # User icons
    └── theme/                     # Theme overrides
```

---

## 11. Related Specifications

- [DOCKABLE_WORKSPACE_ARCHITECTURE.md](DOCKABLE_WORKSPACE_ARCHITECTURE.md)
- [KEYBOARD_SHORTCUTS.md](KEYBOARD_SHORTCUTS.md)
- [PREFERENCES.md](PREFERENCES.md)
