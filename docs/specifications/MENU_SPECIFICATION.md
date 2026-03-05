# Menu Specification

## Overview
This document specifies every menu item in robin-migrate, including hierarchy, shortcuts, enabled states, and associated actions.

## Menu Architecture

### Dynamic Menu System
Menus are built dynamically based on:
1. Window type (active window contributes menus)
2. Connection state (some items require connection)
3. Selection state (some items require object selection)
4. Transaction state (commit/rollback availability)

---

## File Menu

**Menu ID**: `file`  
**Display**: `&File`  
**Sort Order**: 10

| Item ID | Display | Shortcut | Action ID | Enabled When | Icon |
|---------|---------|----------|-----------|--------------|------|
| file.new_connection | &New Connection... | Ctrl+N | file.new_connection | always | document-new |
| file.open_sql | &Open SQL Script... | Ctrl+O | file.open_sql | always | document-open |
| sep1 | ───────── | | | | |
| file.save | &Save | Ctrl+S | file.save_sql | dirty editor | document-save |
| file.save_as | Save &As... | Ctrl+Shift+S | file.save_as | has editor | document-save-as |
| sep2 | ───────── | | | | |
| file.print | &Print... | Ctrl+P | file.print | has results | document-print |
| file.print_preview | Print Pre&view | | file.print_preview | has results | |
| sep3 | ───────── | | | | |
| file.exit | E&xit | Ctrl+Q | file.exit | always | window-close |

**Window Type Contributions**:
- All window types show File menu

---

## Edit Menu

**Menu ID**: `edit`  
**Display**: `&Edit`  
**Sort Order**: 20

| Item ID | Display | Shortcut | Action ID | Enabled When | Icon |
|---------|---------|----------|-----------|--------------|------|
| edit.undo | &Undo | Ctrl+Z | edit.undo | can undo | edit-undo |
| edit.redo | &Redo | Ctrl+Shift+Z | edit.redo | can redo | edit-redo |
| sep1 | ───────── | | | | |
| edit.cut | Cu&t | Ctrl+X | edit.cut | has selection | edit-cut |
| edit.copy | &Copy | Ctrl+C | edit.copy | has selection | edit-copy |
| edit.paste | &Paste | Ctrl+V | edit.paste | clipboard has text | edit-paste |
| edit.select_all | Select &All | Ctrl+A | edit.select_all | has editor | |
| sep2 | ───────── | | | | |
| edit.find | &Find... | Ctrl+F | edit.find | has editor | edit-find |
| edit.find_replace | Find and &Replace... | Ctrl+H | edit.find_replace | has editor | edit-find-replace |
| edit.find_next | Find &Next | F3 | edit.find_next | has search term | |
| edit.find_prev | Find &Previous | Shift+F3 | edit.find_prev | has search term | |
| sep3 | ───────── | | | | |
| edit.preferences | &Preferences... | Ctrl+Comma | edit.preferences | always | tools-preferences |

---

## View Menu

**Menu ID**: `view`  
**Display**: `&View`  
**Sort Order**: 30

| Item ID | Display | Shortcut | Action ID | Checkable | Default |
|---------|---------|----------|-----------|-----------|---------|
| view.project_navigator | &Project Navigator | | view.project_navigator | ✓ | checked |
| view.properties | &Properties Panel | | view.properties | ✓ | checked |
| view.output | &Output Panel | | view.output | ✓ | unchecked |
| view.query_plan | &Query Plan Panel | | view.query_plan | ✓ | unchecked |
| sep1 | ───────── | | | | |
| view.refresh | &Refresh | F5 | view.refresh | | |
| sep2 | ───────── | | | | |
| view.zoom_in | Zoom &In | Ctrl++ | view.zoom_in | | |
| view.zoom_out | Zoom &Out | Ctrl+- | view.zoom_out | | |
| view.zoom_reset | &Reset Zoom | Ctrl+0 | view.zoom_reset | | |
| sep3 | ───────── | | | | |
| view.fullscreen | &Fullscreen | F11 | view.fullscreen | ✓ | unchecked |

---

## Database Menu

**Menu ID**: `database`  
**Display**: `&Database`  
**Sort Order**: 40

| Item ID | Display | Shortcut | Action ID | Enabled When | Icon |
|---------|---------|----------|-----------|--------------|------|
| db.connect | &Connect | | db.connect | disconnected | db-connect |
| db.disconnect | &Disconnect | | db.disconnect | connected | db-disconnect |
| db.reconnect | &Reconnect | | db.reconnect | connected | |
| sep1 | ───────── | | | | |
| db.new_database | &New Database... | | db.new | always | db-new |
| db.register | &Register Database... | | db.register | always | |
| db.unregister | &Unregister Database | | db.unregister | db selected | |
| sep2 | ───────── | | | | |
| db.backup | &Backup... | | db.backup | connected | db-backup |
| db.restore | &Restore... | | db.restore | connected | db-restore |
| sep3 | ───────── | | | | |
| db.properties | Pr&operties... | | db.properties | db selected | |

**Submenu: New Object** (under Database menu when connected):
```
New &Table...        table.new       Ctrl+T
New &View...         view.new        
New &Procedure...    procedure.new   
New &Function...     function.new    
New &Trigger...      trigger.new     
New &Index...        index.new       
```

---

## Query Menu

**Menu ID**: `query`  
**Display**: `&Query`  
**Sort Order**: 50  
**Window Types**: sql_editor, script_editor

| Item ID | Display | Shortcut | Action ID | Enabled When | Icon |
|---------|---------|----------|-----------|--------------|------|
| query.execute | &Execute | F9 | query.execute | has editor, connected | query-execute |
| query.execute_selection | Execute &Selection | Ctrl+F9 | query.execute_selection | has selection, connected | query-execute-selection |
| query.execute_script | Execute &Script | | query.execute_script | has editor, connected | |
| query.stop | &Stop | | query.stop | executing | query-stop |
| sep1 | ───────── | | | | |
| query.explain | &Explain Plan | | query.explain | has editor, connected | query-explain |
| query.explain_analyze | Explain &Analyze | | query.explain_analyze | has editor, connected | |
| sep2 | ───────── | | | | |
| query.format | &Format SQL | | query.format | has editor | query-format |
| query.comment | &Comment Lines | | query.comment | has selection | |
| query.uncomment | &Uncomment Lines | | query.uncomment | has selection | |
| query.toggle_comment | &Toggle Comment | Ctrl+/ | query.toggle_comment | has editor | |
| sep3 | ───────── | | | | |
| query.uppercase | To &Uppercase | | query.uppercase | has selection | |
| query.lowercase | To &Lowercase | | query.lowercase | has selection | |

---

## Transaction Menu

**Menu ID**: `transaction`  
**Display**: `&Transaction`  
**Sort Order**: 60  
**Window Types**: sql_editor  
**Enabled**: Connected and transaction-capable

| Item ID | Display | Shortcut | Action ID | Enabled When | Icon |
|---------|---------|----------|-----------|--------------|------|
| tx.start | &Start Transaction | | tx.start | connected, no transaction | transaction-start |
| tx.commit | &Commit | | tx.commit | in transaction | transaction-commit |
| tx.rollback | &Rollback | | tx.rollback | in transaction | transaction-rollback |
| tx.savepoint | Set &Savepoint... | | tx.savepoint | in transaction | |
| sep1 | ───────── | | | | |
| tx.autocommit | &Auto Commit | | tx.autocommit | ✓ checked if on | transaction-autocommit |
| tx.isolation | &Isolation Level ▶ | | tx.isolation | connected | |
| sep2 | ───────── | | | | |
| tx.readonly | &Read Only | | tx.readonly | not in transaction | |

**Isolation Level Submenu**:
```
&Read Uncommitted
&Read Committed    [default]
&Repeatable Read
&Serializable
```

---

## Tools Menu

**Menu ID**: `tools`  
**Display**: `&Tools`  
**Sort Order**: 70

| Item ID | Display | Action ID | Enabled When | Icon |
|---------|---------|-----------|--------------|------|
| tools.generate_ddl | &Generate DDL... | tools.generate_ddl | has selection | tools-generate-ddl |
| tools.compare_schema | Compare &Schemas... | tools.compare_schema | connected | |
| tools.compare_data | Compare &Data... | tools.compare_data | connected | |
| sep1 | ───────── | | | |
| tools.import | &Import Data ▶ | | | |
| tools.export | &Export Data ▶ | | | |
| sep2 | ───────── | | | |
| tools.migrate | Database &Migration... | tools.migrate | connected | |
| tools.scheduler | Job &Scheduler... | tools.scheduler | connected | |
| sep3 | ───────── | | | |
| tools.monitor | &Monitor ▶ | | connected | tools-monitor |

**Import Submenu**:
```
Import &SQL...       import.sql
Import &CSV...       import.csv
Import &JSON...      import.json
Import E&xcel...     import.excel
```

**Export Submenu**:
```
Export &SQL...       export.sql
Export &CSV...       export.csv
Export &JSON...      export.json
Export &Excel...     export.excel
Export &XML...       export.xml
Export &HTML...      export.html
```

**Monitor Submenu**:
```
&Connections         monitor.connections
&Transactions        monitor.transactions
&Active Statements   monitor.statements
&Locks               monitor.locks
&Performance         monitor.performance
```

---

## Window Menu

**Menu ID**: `window`  
**Display**: `&Window`  
**Sort Order**: 80

| Item ID | Display | Shortcut | Action ID | Enabled When |
|---------|---------|----------|-----------|--------------|
| window.new | &New Window | | window.new | always |
| window.close | &Close Window | Ctrl+W | window.close | has window |
| window.close_all | Close &All Windows | | window.close_all | has windows |
| sep1 | ───────── | | | |
| window.next | &Next Window | Ctrl+Tab | window.next | multiple windows |
| window.prev | &Previous Window | Ctrl+Shift+Tab | window.prev | multiple windows |
| sep2 | ───────── | | | |
| window.cascade | &Cascade | | window.cascade | multiple windows |
| window.tile_h | Tile &Horizontal | | window.tile_h | multiple windows |
| window.tile_v | Tile &Vertical | | window.tile_v | multiple windows |

---

## Help Menu

**Menu ID**: `help`  
**Display**: `&Help`  
**Sort Order**: 90

| Item ID | Display | Shortcut | Action ID | Icon |
|---------|---------|----------|-----------|------|
| help.docs | &Documentation | F1 | help.docs | help-contents |
| help.context | &Context Help | Shift+F1 | help.context | help-context |
| help.shortcuts | &Keyboard Shortcuts | | help.shortcuts | |
| sep1 | ───────── | | | |
| help.tip | &Tip of the Day | | help.tip_of_day | |
| sep2 | ───────── | | | |
| help.about | &About... | | help.about | help-about |

---

## Context Menus

### Server Context Menu

**Applies To**: Server nodes in navigator

| Item | Action | Icon |
|------|--------|------|
| Connect | db.connect | db-connect |
| Disconnect | db.disconnect | db-disconnect |
| ───────── | | |
| New Database... | db.new | db-new |
| Register Database... | db.register | |
| ───────── | | |
| Properties... | db.properties | |

### Database Context Menu

| Item | Action | Icon |
|------|--------|------|
| Connect | db.connect | |
| Disconnect | db.disconnect | |
| ───────── | | |
| New Object ▶ | | |
|   Table... | table.new | table-new |
|   View... | view.new | view-new |
|   Procedure... | procedure.new | procedure-new |
|   Function... | function.new | function-new |
|   Trigger... | trigger.new | trigger-new |
| ───────── | | |
| Backup... | db.backup | db-backup |
| Restore... | db.restore | db-restore |
| ───────── | | |
| Properties... | db.properties | |

### Table Context Menu

| Item | Action | Icon |
|------|--------|------|
| Browse Data | table.browse_data | |
| ───────── | | |
| Edit Table... | table.alter | table-edit |
| Generate DDL | table.generate_ddl | |
| ───────── | | |
| Export Data... | table.export_data | document-export |
| Import Data... | table.import_data | document-import |
| ───────── | | |
| Properties... | table.properties | |

### SQL Editor Context Menu

| Item | Action | Enabled |
|------|--------|---------|
| Execute | query.execute | connected |
| Execute Selection | query.execute_selection | has selection, connected |
| ───────── | | |
| Cut | edit.cut | has selection |
| Copy | edit.copy | has selection |
| Paste | edit.paste | |
| ───────── | | |
| Format SQL | query.format | |
| Comment | query.comment | has selection |
| Uncomment | query.uncomment | has selection |
| ───────── | | |
| Find... | edit.find | |
| Replace... | edit.find_replace | |

---

## Menu State Management

### Connection State Impact

| State | Enabled Items | Disabled Items |
|-------|--------------|----------------|
| Disconnected | Connect, Register Database | Disconnect, Execute, Backup |
| Connected | Disconnect, Execute, Backup, New Object | Connect |
| Executing | Stop | Execute, Disconnect |

### Selection State Impact

| Selection | Additional Menu Items |
|-----------|---------------------|
| Table selected | Edit Table, Browse Data, Generate DDL |
| Procedure selected | Edit Procedure, Execute, Debug |
| Text in editor | Cut, Copy, Comment |

### Transaction State Impact

| State | Enabled Items |
|-------|--------------|
| No transaction | Start Transaction, Auto Commit |
| In transaction | Commit, Rollback, Savepoint |

---

## Implementation Notes

### Menu Building
```cpp
// Menu is built from MenuTemplate in ActionCatalog
MenuTemplate* template = catalog->findMenuTemplate("database");
for (auto* group : template->action_groups) {
    for (auto& action_id : group->action_ids) {
        ActionDefinition* action = catalog->findAction(action_id);
        // Create menu item with action properties
        // Set enabled based on action.requirements
    }
    // Add separator if group.show_separator_after
}
```

### Dynamic Updates
- Menus updated on:
  - Window activation (different window type)
  - Connection state change
  - Transaction state change
  - Selection change

### Accelerators
- Defined in ActionDefinition::accelerator
- Format: "Ctrl+X", "F5", "Ctrl+Shift+Z"
- Platform-specific conversion (Ctrl/Cmd on Mac)
