# Dockable Workspace Architecture Specification

**Version**: 1.0  
**Status**: Draft  
**Last Updated**: 2026-02-06  
**Author**: ScratchRobin Development Team

---

## 1. Overview

This specification defines the dockable, floating workspace architecture for ScratchRobin. The design follows modern IDE conventions where all forms, panels, and toolbars can be docked, floated, or hidden based on user preference and workflow needs.

### 1.1 Design Philosophy

- **Minimal Chrome**: The main form takes up minimal screen real estate when not actively hosting content
- **Contextual UI**: Menus and toolbars appear/disappear based on the active document
- **Flexible Layouts**: Users can save and switch between workspace configurations
- **Multi-Monitor Support**: Full support for multiple displays with independent floating windows
- **macOS-Style Menu Sharing**: Single menu bar that changes based on active window

### 1.2 Key Features

| Feature | Description |
|---------|-------------|
| Dockable Panels | Navigator, Inspector, and other panels can dock or float |
| Tabbed Documents | Multiple documents in a tabbed interface |
| Layout Presets | Save/restore workspace configurations |
| Auto-Sizing | Main form resizes based on content |
| Menu Merging | Form-specific menus merge into main menu bar |
| Draggable Toolbars | Toolbars can be detached and reoriented |

---

## 2. Architecture Components

### 2.1 System Architecture

```
┌─────────────────────────────────────────────────────────────────┐
│                         MainFrame                                │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │                    LayoutManager                             │ │
│  │  ┌─────────────────────────────────────────────────────────┐│ │
│  │  │                  wxAuiManager                           ││ │
│  │  │                                                         ││ │
│  │  │  ┌──────────┐  ┌──────────────┐  ┌──────────────────┐  ││ │
│  │  │  │Navigator │  │ Tabbed       │  │   Inspector      │  ││ │
│  │  │  │  (Left)  │  │ Document     │  │   (Right/Bottom) │  ││ │
│  │  │  │          │  │ Manager      │  │                  │  ││ │
│  │  │  │          │  │ (Center)     │  │                  │  ││ │
│  │  │  └──────────┘  └──────────────┘  └──────────────────┘  ││ │
│  │  │                                                         ││ │
│  │  │  ┌──────────────────────────────────────────────────┐  ││ │
│  │  │  │           IconBarHost (Top/Bottom)               │  ││ │
│  │  │  │  - Draggable toolbars                            │  ││ │
│  │  │  │  - Context-sensitive visibility                  │  ││ │
│  │  │  └──────────────────────────────────────────────────┘  ││ │
│  │  └─────────────────────────────────────────────────────────┘│ │
│  └─────────────────────────────────────────────────────────────┘ │
│                                                                  │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │                  AutoSizePolicy                              │ │
│  │     Manages main form sizing based on content               │ │
│  └─────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

### 2.2 Component Responsibilities

#### 2.2.1 LayoutManager
- Central coordinator for all dockable windows
- Manages wxAuiManager for docking operations
- Handles layout preset loading/saving
- Coordinates menu/toolbar merging
- Tracks active window state

#### 2.2.2 NavigatorPanel
- Tree-based database object browser
- Dockable left panel by default
- Filter capability for large schemas
- Context menus for object operations
- Drag source for creating diagrams

#### 2.2.3 DocumentManager
- Tabbed interface for all open documents
- Supports multiple document types (SQL Editor, Diagram, etc.)
- Tab reordering via drag-drop
- Middle-click to close
- Modified indicator (*)

#### 2.2.4 InspectorPanel
- Property/details view for selected object
- Dockable right or bottom panel
- Multiple tabs: Overview, DDL, Dependencies
- Updates based on tree selection

#### 2.2.5 IconBarHost
- Manages all toolbars
- Shows/hides toolbars based on active form
- Handles drag-to-float and dock-back
- Supports horizontal/vertical orientation
- Multi-row/column when floating

#### 2.2.6 AutoSizePolicy
- Calculates optimal main form size
- Modes: Compact, Adaptive, Fixed, Fullscreen
- Respects minimum working size (800x600)
- Handles monitor boundaries

#### 2.2.7 WindowStateManager
- Persists window positions and states
- JSON-based configuration files
- Multi-monitor aware
- Handles monitor disconnection gracefully

---

## 3. Docking System

### 3.1 Dockable Window Interface

All dockable components implement `IDockableWindow`:

```cpp
class IDockableWindow {
public:
    // Identity
    virtual std::string GetWindowId() const = 0;
    virtual std::string GetWindowTitle() const = 0;
    virtual std::string GetWindowType() const = 0;
    
    // Capabilities
    virtual bool CanDock() const { return true; }
    virtual bool CanFloat() const { return true; }
    virtual bool CanClose() const { return true; }
    
    // Menu/Toolbar integration
    virtual wxMenuBar* GetMenuBar() { return nullptr; }
    virtual wxToolBar* GetToolBar() { return nullptr; }
    
    // Lifecycle
    virtual void OnActivate() {}
    virtual void OnDeactivate() {}
    virtual bool OnCloseRequest() { return true; }
    
    // State
    virtual bool IsModified() const { return false; }
    virtual bool Save() { return true; }
};
```

### 3.2 Dock Directions

```
┌─────────────────────────────────────────┐
│               TOP                       │
│    ┌─────────────────────────┐          │
│    │                         │  RIGHT   │
│ L  │        CENTER           │          │
│ E  │    (Document Manager)   │          │
│ F  │                         │          │
│ T  │                         │          │
│    │                         │          │
│    └─────────────────────────┘          │
│              BOTTOM                     │
└─────────────────────────────────────────┘

- LEFT: Navigator panel (default)
- RIGHT: Inspector panel (default)
- CENTER: Document manager
- FLOATING: Detached window
```

### 3.3 Docking Operations

| Operation | Trigger | Behavior |
|-----------|---------|----------|
| Drag to Dock | Drag panel to edge | Snaps to dock zone with preview |
| Drag to Float | Drag from gripper | Creates floating frame |
| Double-click Gripper | Double-click title area | Toggles dock/float |
| Close Button | Click X | Hides panel (can restore from View menu) |
| Ctrl+Click Drag | Ctrl + drag title | Direct float without preview |

---

## 4. Layout Presets

### 4.1 Built-in Presets

| Preset | Description | Use Case |
|--------|-------------|----------|
| **Default** | Navigator left, documents center, inspector right | Standard development |
| **Single Monitor** | Compact layout with auto-hiding panels | Limited screen space |
| **Dual Monitor** | Navigator on primary, documents span both | Multi-monitor setup |
| **Wide Screen** | Multiple side panels, wide document area | Ultrawide displays |
| **Compact** | Minimal UI, maximum document space | Presentation/focus mode |

### 4.2 Preset Format

```json
{
    "name": "My Custom Layout",
    "description": "Optimized for SQL development",
    "is_default": false,
    "version": 1,
    "main_form": {
        "x": 100,
        "y": 100,
        "width": 1920,
        "height": 1080,
        "maximized": false
    },
    "monitor": {
        "index": 0,
        "is_primary": true
    },
    "windows": [
        {
            "window_id": "navigator",
            "window_type": "navigator",
            "is_visible": true,
            "is_docked": true,
            "dock_direction": 0,
            "dock_proportion": 25,
            "dock_row": 0,
            "dock_layer": 1
        },
        {
            "window_id": "inspector",
            "window_type": "inspector",
            "is_visible": true,
            "is_docked": true,
            "dock_direction": 1,
            "dock_proportion": 30
        }
    ]
}
```

### 4.3 Preset Storage

- **Location**: `~/.config/ScratchRobin/layouts/*.json` (Linux)
- **Location**: `%APPDATA%/ScratchRobin/layouts/*.json` (Windows)
- **Location**: `~/Library/Application Support/ScratchRobin/layouts/*.json` (macOS)
- **Auto-save**: Session state saved to `_last_session.json`

---

## 5. Auto-Sizing System

### 5.1 Size Modes

| Mode | Description | Size Calculation |
|------|-------------|------------------|
| **COMPACT** | Menu + icon bar only | Min: 400x100 |
| **ADAPTIVE** | Grow to fit, shrink when empty | Min: 800x600 |
| **FIXED** | User-defined size | User-specified |
| **FULLSCREEN** | Maximized | Full screen |
| **CUSTOM** | Saved user preference | From preset |

### 5.2 Behavior

**When no documents open:**
```
┌─────────────────────────────┐
│  Menu  │  Icon Bar          │  ← 60px height
├─────────────────────────────┤
│                             │
│   (Empty workspace)         │  ← Minimized content
│                             │
└─────────────────────────────┘
      ~400-600px width
```

**When documents open:**
```
┌─────────────────────────────────────────┐
│  Menu  │  Icon Bar                      │
├────────┬──────────────────────┬─────────┤
│        │                      │         │
│ Nav    │   Document Manager   │ Inspect │
│ 25%    │       50%            │  25%    │
│        │                      │         │
└────────┴──────────────────────┴─────────┘
        1024px+ width, 768px+ height
```

### 5.3 Constraints

- Minimum compact size: 400x100
- Minimum working size: 800x600
- Maximum: Full screen dimensions
- User resize cooldown: 5 seconds (prevents auto-resize after manual resize)

---

## 6. Menu and Toolbar System

### 6.1 Menu Merging

When a document becomes active:

1. Form's menu bar is merged into MainFrame's menu bar
2. Form's toolbar is shown in IconBarHost
3. Previous form's menus are unmerged

```
Main Menu (Idle):          Main Menu (SQL Editor Active):
├─ File                    ├─ File
├─ Edit                    ├─ Edit
├─ View                    ├─ View
├─ Window                  ├─ Query          ← Merged
├─ Help                    ├─ Window
                           ├─ Help
```

### 6.2 Toolbar Context Sensitivity

| Active Form | Visible Toolbars |
|-------------|------------------|
| None | Main only |
| SQL Editor | Main + SQL Editor |
| Diagram | Main + Diagram |
| Table Designer | Main + Table Designer |

### 6.3 Toolbar Customization

- Drag to undock: Creates floating toolbar frame
- Resize floating frame: Changes orientation (H/V) or adds rows/columns
- Dock back: Drag to any edge of main window
- Double-click: Toggles orientation when floating

---

## 7. Document Manager

### 7.1 Document Types

| Type | Description | File Extension |
|------|-------------|----------------|
| SQL Editor | SQL query editor | .sql |
| Diagram | ERD/Database diagram | .sberd |
| Table Designer | Visual table editor | N/A |
| View Editor | SQL view editor | .sql |
| Procedure Editor | Stored procedure editor | .sql |

### 7.2 Tab Behavior

- **Click**: Activate document
- **Middle-click**: Close document
- **Right-click**: Context menu (Close, Close All, Close Others)
- **Drag**: Reorder tabs
- **Ctrl+Tab**: Next document
- **Ctrl+Shift+Tab**: Previous document

### 7.3 Modified State

- Modified documents show `*` in tab title
- Closing modified document prompts to save
- "Close All" prompts for each modified document

---

## 8. Floating Forms

### 8.1 Floating Frame Behavior

- Independent position and size
- Can move to any monitor
- Double-click title bar: Dock back to main
- Ctrl+Click drag: Move without docking preview
- Close button: Returns content to document manager or closes

### 8.2 Multi-Monitor Support

- Detects monitor configuration on startup
- Restores windows to correct monitor
- Handles monitor disconnection (moves to primary)
- Supports different DPI per monitor

---

## 9. Navigator Panel

### 9.1 Tree Structure

```
Database
├── Tables
│   ├── customers
│   │   ├── id (PK)
│   │   ├── name
│   │   └── email
│   └── orders
├── Views
├── Procedures
├── Functions
├── Triggers
└── Sequences
```

### 9.2 Context Menus by Type

**Tables:**
- Create Table...
- Create Select Query
- Create Update Query
- Create Delete Query
- Create Index...
- View Data
- Properties

**Views:**
- Create View...
- Refresh View
- View Data
- Script View

**Procedures:**
- Create Procedure...
- Execute
- Edit

### 9.3 Filter

- Text box at top of navigator
- Filters tree in real-time
- Supports wildcards (* and ?)
- Esc clears filter

---

## 10. Implementation Details

### 10.1 Class Hierarchy

```
IDockableWindow (interface)
    ├── INavigatorWindow (interface)
    │       └── NavigatorPanel
    ├── IDocumentWindow (interface)
    │       └── DockableForm
    │               ├── SqlEditorFrame
    │               ├── DiagramFrame
    │               └── TableDesignerFrame
    └── InspectorPanel

LayoutManager
    ├── wxAuiManager
    ├── MenuMerger
    └── IconBarHost

AutoSizePolicy
WindowStateManager
LayoutPreset
```

### 10.2 File Organization

```
src/ui/
├── layout/
│   ├── dockable_window.h/cpp      # Interfaces
│   ├── layout_manager.h/cpp       # Central manager
│   ├── layout_preset.h/cpp        # Serialization
│   ├── navigator_panel.h/cpp      # Tree control
│   └── inspector_panel.h/cpp      # Properties panel
├── auto_size_policy.h/cpp         # Sizing logic
├── window_state_manager.h/cpp     # Persistence
├── document_manager.h/cpp         # Tabbed interface
├── dockable_form.h/cpp            # Base form class
├── icon_bar_host.h/cpp            # Toolbar manager
├── draggable_toolbar.h/cpp        # Draggable toolbars
└── floating_frame.h/cpp           # Floating containers
```

### 10.3 Event System

```cpp
// Layout change events
enum class LayoutChangeEvent::Type {
    WindowRegistered,    // New window added
    WindowUnregistered,  // Window removed
    WindowDocked,        // Window docked
    WindowFloated,       // Window floated
    WindowClosed,        // Window closed
    LayoutLoaded,        // Preset loaded
    LayoutSaved          // Preset saved
};

// Observer pattern
LayoutManager::AddObserver(callback);
LayoutManager::RemoveObserver(callback);
```

---

## 11. Configuration

### 11.1 Configuration Files

| File | Purpose |
|------|---------|
| `~/.config/ScratchRobin/layouts/_last_session.json` | Auto-saved session |
| `~/.config/ScratchRobin/layouts/*.json` | User-defined presets |
| `~/.config/ScratchRobin/window_state.json` | Main form state |

### 11.2 Menu Items

**View Menu:**
- Navigator (Ctrl+Alt+N)
- Inspector (Ctrl+Alt+I)
- Document Manager (Ctrl+Alt+D)
- Full Screen (F11)

**Window Menu:**
- Auto-Size Mode
  - Compact
  - Adaptive
  - Fixed
  - Fullscreen
- Layout Presets
  - Default
  - Single Monitor
  - Dual Monitor
  - Wide Screen
  - Compact
  - Save Current Layout...
  - Manage Layouts...
- Remember Current Size
- Reset to Default Layout
- Close All Documents

---

## 12. Accessibility

### 12.1 Keyboard Navigation

- **Ctrl+Tab**: Next document
- **Ctrl+Shift+Tab**: Previous document
- **Ctrl+W**: Close current document
- **Ctrl+Alt+N**: Toggle Navigator
- **Ctrl+Alt+I**: Toggle Inspector
- **F11**: Full screen

### 12.2 Screen Readers

- All dockable windows have proper labels
- Tab order is logical
- Focus indication is visible
- Window state changes are announced

---

## 13. Related Specifications

- [UI_WINDOW_MODEL.md](UI_WINDOW_MODEL.md) - General window model
- [KEYBOARD_SHORTCUTS.md](KEYBOARD_SHORTCUTS.md) - Complete keyboard reference
- [PREFERENCES.md](PREFERENCES.md) - User preferences system
- [ACCESSIBILITY_AND_INPUT.md](ACCESSIBILITY_AND_INPUT.md) - Accessibility features

---

## 14. Glossary

| Term | Definition |
|------|------------|
| **Dock** | Attach a panel to an edge of the main window |
| **Float** | Detach a panel into an independent window |
| **Layout Preset** | Saved configuration of window positions |
| **Gripper** | Handle for dragging dockable panels |
| **Pane** | A dockable or floating window region |
| **AUI** | Advanced User Interface (wxWidgets docking framework) |
| **Document** | An editable unit (SQL file, diagram, etc.) |
| **Icon Bar** | Toolbar containing icon buttons |

---

## Appendix A: Migration from Old UI

### A.1 Breaking Changes

1. **Window State Format**: Old format not compatible - will reset to default
2. **Toolbar Customization**: New system uses JSON instead of registry/config
3. **Menu Structure**: Some items moved to Window menu

### A.2 Upgrade Path

1. First launch: Migrates to new format
2. Old layouts: Preserved as "Legacy Layout" preset
3. Settings: Automatically imported
