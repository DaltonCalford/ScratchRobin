# Window Types Catalog

This document catalogs all standard window types in the robin-migrate application.

## Overview

The application uses a dynamic window system where different window types contribute different menus and toolbars. The window type determines:
- Which menus are shown in the menu bar
- Which toolbars are available
- Docking behavior and capabilities
- Persistence settings

## Window Categories

All windows fall into one of these categories:

| Category | Description | Examples |
|----------|-------------|----------|
| `kContainer` | Main container window | Main application window |
| `kNavigator` | Tree/explorer panels | Project Navigator, Object Browser |
| `kIconBar` | Toolbar containers | Standard toolbar, Custom toolbars |
| `kEditor` | Document editors | SQL Editor, Object Editors |
| `kInspector` | Properties panels | Object Properties, Database Properties |
| `kOutput` | Output/result panels | Results grid, Query Plan, Output console |
| `kDialog` | Modal/non-modal dialogs | Settings, Connection dialog |
| `kPopup` | Temporary popups | Message boxes, Progress dialogs |

## Standard Window Types

### 1. Navigator Windows

| Type ID | Name | Description |
|---------|------|-------------|
| `project_navigator` | Project Navigator | Tree view of servers, databases, and objects |
| `object_navigator` | Object Navigator | Alternative object browser with filtering |
| `search_navigator` | Search Navigator | Search results and object finder |

**Characteristics:**
- Dockable: Left, Right, Bottom, Tab
- Default: Left side
- Size: 300x600 (min: 200x300)

### 2. SQL Editor Windows

| Type ID | Name | Description |
|---------|------|-------------|
| `sql_editor` | SQL Editor | Single SQL statement editor with syntax highlighting |
| `sql_script_editor` | SQL Script | Multi-statement script editor with execution |
| `query_builder` | Query Builder | Visual query designer with drag-and-drop |

**Characteristics:**
- Dockable: Any direction + Float + Tab
- Default: Center (tabbed)
- Multiple instances allowed (max: 5-20)
- Menus: SQL Editor set (Query, Transaction)

### 3. Results/Output Windows

| Type ID | Name | Description |
|---------|------|-------------|
| `results_panel` | Results | Query results in data grid format |
| `query_plan_panel` | Query Plan | Query execution plan visualization |
| `output_panel` | Output | Messages, errors, execution output |
| `history_panel` | SQL History | History of executed SQL statements |

**Characteristics:**
- Dockable: Left, Right, Bottom, Tab
- Default: Bottom
- Output panel is singleton
- Results can have multiple instances

### 4. Database Object Editors (DDL)

These editors correspond to EBNF database object types:

| Type ID | Object Type | Description |
|---------|-------------|-------------|
| `table_editor` | TABLE | Visual table designer with columns, constraints, indexes |
| `view_editor` | VIEW | View definition editor with SQL preview |
| `procedure_editor` | PROCEDURE | Stored procedure editor |
| `function_editor` | FUNCTION | User-defined function editor |
| `trigger_editor` | TRIGGER | Trigger editor with event selection |
| `sequence_editor` | SEQUENCE | Sequence/Generator editor |
| `generator_editor` | GENERATOR | Firebird-specific generator editor |
| `domain_editor` | DOMAIN | Domain/user-defined type editor |
| `index_editor` | INDEX | Index designer with column selection |
| `constraint_editor` | CONSTRAINT | Table constraint editor (PK, FK, Check, Unique) |
| `exception_editor` | EXCEPTION | Database exception editor |
| `package_editor` | PACKAGE | Package editor (header and body) |
| `role_editor` | ROLE | Database role editor with privileges |
| `user_editor` | USER | Database user editor |
| `charset_editor` | CHARACTER SET | Character set editor |
| `collation_editor` | COLLATION | Collation sequence editor |
| `filter_editor` | FILTER | BLOB filter editor |
| `shadow_editor` | SHADOW | Database shadow configuration |
| `synonym_editor` | SYNONYM | Synonym/alias editor |

**Characteristics:**
- Dockable: Any direction + Float + Tab
- Default: Center (tabbed)
- Multiple instances allowed
- Menus: Object Editor set

### 5. Data Editors (DML)

| Type ID | Name | Description |
|---------|------|-------------|
| `table_data_editor` | Table Data | Editable data grid for table contents |
| `blob_editor` | BLOB Editor | Binary/text BLOB content editor |
| `array_editor` | Array Editor | Array column data editor |

**Characteristics:**
- Table data: Full docking support
- BLOB: Can float
- Array: Fixed size popup

### 6. Properties/Inspector Windows

| Type ID | Name | Description |
|---------|------|-------------|
| `object_properties` | Properties | Generic object properties and metadata |
| `database_properties` | Database Properties | Database connection and settings |
| `server_properties` | Server Properties | Server connection properties |
| `column_properties` | Column Properties | Column-specific properties |
| `index_properties` | Index Properties | Index-specific properties |

**Characteristics:**
- Dockable: Left, Right, Float, Tab
- Default: Right side
- Size: 350x600 (min: 250x300)

### 7. Tool Windows

| Type ID | Name | Description |
|---------|------|-------------|
| `icon_bar` | Toolbar | Configurable icon toolbar |
| `status_bar` | Status Bar | Application status information |
| `search_bar` | Search Bar | Search/find bar |

**Characteristics:**
- Icon bar: Dockable Top, Bottom, Left, Right
- Default: Top
- Cannot close or float

### 8. Dialog Windows

| Type ID | Name | Description |
|---------|------|-------------|
| `settings_dialog` | Settings | Application settings dialog |
| `preferences_dialog` | Preferences | User preferences dialog |
| `connection_dialog` | Connection | Database connection dialog |
| `about_dialog` | About | About dialog |
| `print_dialog` | Print | Print dialog |
| `export_dialog` | Export | Data export dialog |
| `import_dialog` | Import | Data import dialog |
| `backup_dialog` | Backup | Database backup dialog |
| `restore_dialog` | Restore | Database restore dialog |
| `monitor_dialog` | Monitor | Database monitor dialog |

**Characteristics:**
- Floating only (no docking)
- Modal or non-modal
- Tool window style (smaller title bar)
- Do not contribute to main menu

### 9. Popup Windows

| Type ID | Name | Description |
|---------|------|-------------|
| `message_popup` | Message | General message popup |
| `error_popup` | Error | Error message popup |
| `warning_popup` | Warning | Warning popup |
| `info_popup` | Information | Info popup |
| `confirm_popup` | Confirm | Confirmation dialog |
| `input_popup` | Input | Input prompt popup |
| `progress_popup` | Progress | Progress indicator |
| `wait_popup` | Wait | Please wait indicator |

**Characteristics:**
- Fixed size, centered
- Non-resizable
- Always on top
- Temporary (auto-close)
- Modal

### 10. Special Windows

| Type ID | Name | Description |
|---------|------|-------------|
| `splash_screen` | Splash | Application startup splash screen |
| `welcome_screen` | Welcome | Welcome/start page |
| `tip_of_day` | Tip of Day | Tip of the day dialog |

**Characteristics:**
- Splash: Fixed size, always on top, no close button
- Welcome: Can tab, center dock

## Window Type Behavior Matrix

| Type | Singleton | Multi | Dock | Float | Tab | Modal |
|------|-----------|-------|------|-------|-----|-------|
| Project Navigator | ✓ | - | L/R | - | ✓ | - |
| SQL Editor | - | ✓ (20) | Any | ✓ | ✓ | - |
| Results | - | ✓ (10) | L/R/B | - | ✓ | - |
| Object Editors | - | ✓ (20) | Any | ✓ | ✓ | - |
| Properties | - | ✓ | L/R | ✓ | ✓ | - |
| Dialogs | - | ✓ | - | ✓ | - | ✓/✗ |
| Popups | - | ✓ | - | ✓ | - | ✓ |

## Menu Contributions by Window Type

| Window Type | Contributed Menus | Merge Mode |
|-------------|-------------------|------------|
| Main Window | File, Edit, View, Window, Help | Base (no merge) |
| SQL Editor | Query, Transaction | Merge with Main |
| Script Editor | Query, Transaction, Script | Merge with Main |
| Object Editor | Database, Tools | Merge with Main |
| Navigator | Database | Merge with Main |
| Data Editor | Data | Merge with Main |
| Settings | - | Replace Main |

## Implementation

### Registration

Window types are registered in `WindowTypeCatalog::initialize()`:

```cpp
void WindowTypeCatalog::initialize() {
    registerNavigatorTypes();
    registerSQLEditorTypes();
    registerResultsTypes();
    registerObjectEditorTypes();
    registerDataEditorTypes();
    registerPropertyTypes();
    registerToolTypes();
    registerDialogTypes();
    registerPopupTypes();
    registerSpecialTypes();
}
```

### Object Type Mapping

Database object types are mapped to editor window types via `ObjectTypeMapper`:

```cpp
ObjectTypeMapper* mapper = ObjectTypeMapper::get();
std::string window_type = mapper->getEditorWindowType("TABLE");
// Returns: "table_editor"
```

### Creating Windows

```cpp
// Get window type definition
WindowTypeCatalog* catalog = WindowTypeCatalog::get();
WindowTypeDefinition* def = catalog->findWindowType("sql_editor");

// Create window via WindowManager
WindowCreateParams params;
params.type_id = WindowTypes::kSQLEditor;
params.title = "New Query";
DockableWindow* window = WindowManager::get()->createWindow(params);
```

## Related Documentation

- [Dynamic Menu System](DYNAMIC_MENUS.md) - How menus switch based on window type
- [Toolbar Editor](TOOLBAR_EDITOR.md) - Customizing toolbars per window type
- [Menu Editor](MENU_EDITOR.md) - Assigning menus to window types
- [Docking Framework](DOCKING_FRAMEWORK.md) - Window docking behavior
