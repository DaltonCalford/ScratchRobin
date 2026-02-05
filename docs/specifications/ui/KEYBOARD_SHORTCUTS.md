# ScratchRobin Keyboard Shortcuts Specification

**Status**: ✅ **IMPLEMENTED**  
**Last Updated**: 2026-02-03  
**Implementation**: `src/ui/shortcuts_dialog.cpp`, `src/ui/shortcuts_cheat_sheet.cpp`

---

## Overview

ScratchRobin provides comprehensive keyboard shortcut support for efficient database administration workflows. Shortcuts are categorized by context and fully customizable through the Preferences dialog.

---

## Global Shortcuts

### File Operations
| Shortcut | Action | Notes |
|----------|--------|-------|
| `Ctrl+N` | New SQL Editor | Opens a new SQL editor window |
| `Ctrl+Shift+N` | New Diagram | Opens a new diagram window |
| `Ctrl+O` | Open File | Open SQL script or diagram file |
| `Ctrl+S` | Save | Save current editor content |
| `Ctrl+Shift+S` | Save As | Save with new filename |
| `Ctrl+W` | Close Window | Close current child window |
| `Ctrl+Shift+W` | Close All | Close all child windows |
| `Ctrl+Q` | Quit | Exit application |

### Navigation
| Shortcut | Action | Notes |
|----------|--------|-------|
| `Ctrl+Tab` | Next Window | Switch to next open window |
| `Ctrl+Shift+Tab` | Previous Window | Switch to previous window |
| `F5` | Refresh | Refresh current view/catalog |
| `F1` | Context Help | Show help for current context |
| `Ctrl+Shift+F` | Global Search | Search across all objects |

---

## SQL Editor Shortcuts

### Execution
| Shortcut | Action | Notes |
|----------|--------|-------|
| `F5` / `Ctrl+Enter` | Execute SQL | Run current query or selection |
| `Ctrl+.` | Cancel Query | Cancel running query |
| `Ctrl+Shift+Enter` | Execute All | Run all statements |
| `Ctrl+E` | Explain Query | Show execution plan |

### Editing
| Shortcut | Action | Notes |
|----------|--------|-------|
| `Ctrl+Z` | Undo | Undo last edit |
| `Ctrl+Y` / `Ctrl+Shift+Z` | Redo | Redo last undo |
| `Ctrl+/` | Toggle Comment | Comment/uncomment selection |
| `Ctrl+Shift+/` | Block Comment | Add block comment |
| `Tab` | Indent | Increase indent (or insert spaces) |
| `Shift+Tab` | Outdent | Decrease indent |
| `Ctrl+D` | Duplicate Line | Duplicate current line |
| `Ctrl+Shift+Up` | Move Line Up | Move current line up |
| `Ctrl+Shift+Down` | Move Line Down | Move current line down |
| `Ctrl+Shift+L` | Delete Line | Delete current line |
| `Ctrl+J` | Join Lines | Join current and next line |
| `Ctrl+Space` | Auto-complete | Show completion suggestions |
| `Ctrl+Shift+Space` | Parameter Info | Show function parameters |
| `Ctrl+K, Ctrl+D` | Format Document | Format entire SQL |
| `Ctrl+K, Ctrl+F` | Format Selection | Format selected SQL |

### Selection
| Shortcut | Action | Notes |
|----------|--------|-------|
| `Ctrl+A` | Select All | Select entire content |
| `Ctrl+L` | Select Line | Select current line |
| `Ctrl+Shift+L` | Select All Matches | Multi-cursor on all matches |
| `Alt+Click` | Multi-cursor | Add cursor at click position |
| `Ctrl+D` (with selection) | Add Next Match | Add next occurrence to selection |
| `Ctrl+Shift+D` | Skip Match | Skip current, add next |

### Find/Replace
| Shortcut | Action | Notes |
|----------|--------|-------|
| `Ctrl+F` | Find | Open find dialog |
| `Ctrl+H` | Replace | Open replace dialog |
| `Ctrl+Shift+F` | Find in Files | Search across multiple files |
| `F3` | Find Next | Next occurrence |
| `Shift+F3` | Find Previous | Previous occurrence |
| `Ctrl+M` | Toggle Bookmark | Set/clear bookmark |
| `F2` | Next Bookmark | Go to next bookmark |
| `Shift+F2` | Previous Bookmark | Go to previous bookmark |
| `Ctrl+Shift+M` | Clear All Bookmarks | Remove all bookmarks |

### Transaction Control
| Shortcut | Action | Notes |
|----------|--------|-------|
| `Ctrl+T` | Begin Transaction | Start new transaction |
| `Ctrl+Shift+C` | Commit | Commit current transaction |
| `Ctrl+Shift+R` | Rollback | Rollback current transaction |
| `Ctrl+Shift+S` | Set Savepoint | Create savepoint |

---

## Catalog Tree Shortcuts

| Shortcut | Action | Notes |
|----------|--------|-------|
| `Enter` | Open Object | Open selected object editor |
| `F2` | Rename | Rename object (if supported) |
| `Delete` | Drop | Drop selected object (with confirmation) |
| `Ctrl+C` | Copy Name | Copy object name to clipboard |
| `Ctrl+Shift+C` | Copy Qualified Name | Copy schema.name to clipboard |
| `Space` | Expand/Collapse | Toggle node expansion |
| `Ctrl+F` | Filter | Focus filter text box |
| `Ctrl+R` | Refresh Node | Refresh selected node |
| `Ctrl+Shift+R` | Refresh All | Refresh entire tree |

---

## Diagram Canvas Shortcuts

### Selection and Navigation
| Shortcut | Action | Notes |
|----------|--------|-------|
| `Ctrl+A` | Select All | Select all entities |
| `Ctrl+Click` | Toggle Selection | Add/remove from selection |
| `Shift+Click` | Range Select | Select range |
| `Ctrl+Drag` | Marquee Select | Box selection |
| `Escape` | Deselect All | Clear selection |
| `Delete` | Delete Selected | Remove selected entities |
| `Ctrl+D` | Duplicate | Duplicate selection |
| `Ctrl+G` | Group | Group selected entities |
| `Ctrl+Shift+G` | Ungroup | Ungroup selected group |

### Movement
| Shortcut | Action | Notes |
|----------|--------|-------|
| `Arrow Keys` | Nudge | Move selection 1 grid unit |
| `Shift+Arrow` | Nudge Large | Move selection 10 grid units |
| `Ctrl+Arrow` | Resize | Resize from edge |

### Zoom and View
| Shortcut | Action | Notes |
|----------|--------|-------|
| `Ctrl++` / `Ctrl+=` | Zoom In | Increase zoom |
| `Ctrl+-` | Zoom Out | Decrease zoom |
| `Ctrl+0` | Fit to Window | Zoom to fit all content |
| `Ctrl+1` | Actual Size | 100% zoom |
| `Ctrl+2` | Fit Width | Fit to window width |
| `Ctrl+Wheel` | Zoom | Mouse wheel zoom |

### Layout
| Shortcut | Action | Notes |
|----------|--------|-------|
| `Ctrl+L` | Auto Layout | Run layout algorithm |
| `Ctrl+P` | Pin Selected | Pin selection |
| `Ctrl+Shift+P` | Unpin Selected | Unpin selection |
| `Ctrl+Shift+A` | Align Left | Align selected to left |
| `Ctrl+Shift+C` | Align Center | Center horizontally |
| `Ctrl+Shift+R` | Align Right | Align to right |
| `Ctrl+Shift+T` | Align Top | Align to top |
| `Ctrl+Shift+M` | Align Middle | Center vertically |
| `Ctrl+Shift+B` | Align Bottom | Align to bottom |
| `Ctrl+Shift+H` | Distribute H | Distribute horizontally |
| `Ctrl+Shift+V` | Distribute V | Distribute vertically |

### Notation
| Shortcut | Action | Notes |
|----------|--------|-------|
| `F7` | Previous Notation | Cycle to previous notation |
| `F8` | Next Notation | Cycle to next notation |

---

## Results Grid Shortcuts

| Shortcut | Action | Notes |
|----------|--------|-------|
| `Ctrl+C` | Copy | Copy selected cells |
| `Ctrl+Shift+C` | Copy with Headers | Include column headers |
| `Ctrl+A` | Select All | Select all cells |
| `Ctrl+F` | Find in Results | Search within results |
| `Ctrl+S` | Save Results | Export results to file |
| `Ctrl+Home` | Go to First | First row |
| `Ctrl+End` | Go to Last | Last row |
| `Page Up` | Page Up | Previous page |
| `Page Down` | Page Down | Next page |

---

## Customization

### Preferences Integration

Users can customize shortcuts via Preferences dialog:

```
Preferences -> Keyboard Shortcuts
```

**Features:**
- Search shortcuts by name or key
- View conflicts (duplicate assignments)
- Reset to defaults (per-category or all)
- Import/export shortcut schemes

### Shortcut Scheme Format

```toml
[shortcuts.global]
new_editor = "Ctrl+N"
new_diagram = "Ctrl+Shift+N"

[shortcuts.editor]
execute_sql = "F5"
cancel_query = "Ctrl+."
format_sql = "Ctrl+K,Ctrl+D"

[shortcuts.diagram]
auto_layout = "Ctrl+L"
zoom_in = "Ctrl++"
```

---

## Implementation Details

### Files
- `src/ui/shortcuts_dialog.h` / `.cpp` - Shortcut configuration
- `src/ui/shortcuts_cheat_sheet.h` / `.cpp` - Printable reference

### Key Classes
```cpp
class ShortcutsDialog : public wxDialog {
    // Shortcut configuration UI
    // Conflict detection
    // Import/export
};

class ShortcutsCheatSheet : public wxFrame {
    // Printable shortcut reference
    // Search/filter
};

class ShortcutManager {
    // Load/save shortcuts
    // Default schemes
    // Conflict resolution
};
```

### Platform Adaptations
- macOS: Use `Cmd` instead of `Ctrl`
- Linux/Windows: Standard `Ctrl` bindings
- Context-sensitive help shows platform-specific keys

---

## Shortcut Cheat Sheet

A printable cheat sheet is available via:
- Menu: `Help -> Keyboard Shortcuts`
- Shortcut: `Ctrl+Shift+/`

Categories in cheat sheet:
- Global
- SQL Editor
- Catalog Tree
- Diagram
- Results Grid

---

## Future Enhancements

- Vim/Emacs mode options
- Gesture support (touchpads)
- Voice command integration
- Shortcut recording/macro system
