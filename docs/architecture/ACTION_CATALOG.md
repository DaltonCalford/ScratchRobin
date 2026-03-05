# Action Catalog

This document catalogs all actions available in the robin-migrate application.

## Overview

The Action Catalog is a comprehensive registry of all user actions in the application. Actions are the building blocks for:
- Menu items
- Toolbar buttons
- Keyboard shortcuts
- Context menu items
- Scriptable commands

Each action has:
- **Unique ID**: Used for referencing (e.g., `file.new_connection`)
- **Display Name**: Human-readable name (e.g., "New Connection...")
- **Description**: Detailed explanation of what the action does
- **Category**: Functional grouping (File, Edit, Database, Table, etc.)
- **Icon**: Visual representation
- **Accelerator**: Keyboard shortcut
- **Requirements**: Conditions for enabling (needs connection, needs selection, etc.)

## Action Categories

### Application Level Categories

| Category | Description | Example Actions |
|----------|-------------|-----------------|
| `kFile` | File operations | New Connection, Open, Save, Exit |
| `kEdit` | Edit operations | Cut, Copy, Paste, Undo, Find |
| `kView` | View operations | Zoom, Fullscreen, Show/Hide panels |
| `kWindow` | Window management | New Window, Close, Cascade |
| `kHelp` | Help operations | Documentation, About |

### Database Level Categories

| Category | Description | Example Actions |
|----------|-------------|-----------------|
| `kDatabase` | Database operations | Connect, Backup, Restore |
| `kServer` | Server management | Register Server, Start/Stop |
| `kSchema` | Schema operations | Create Schema, Drop Schema |

### SQL and Query Categories

| Category | Description | Example Actions |
|----------|-------------|-----------------|
| `kQuery` | SQL execution | Execute, Explain, Stop |
| `kTransaction` | Transaction control | Commit, Rollback, Auto-commit |
| `kScript` | Script operations | Debug, Step Into, Breakpoint |

### DDL Object Categories

| Category | Description | Count |
|----------|-------------|-------|
| `kTable` | Table operations | 24 actions |
| `kDbView` | View operations | 14 actions |
| `kProcedure` | Procedure operations | 14 actions |
| `kFunction` | Function operations | 14 actions |
| `kTrigger` | Trigger operations | 11 actions |
| `kIndex` | Index operations | 10 actions |
| `kConstraint` | Constraint operations | 8 actions |
| `kDomain` | Domain operations | 10 actions |
| `kSequence` | Sequence operations | 11 actions |
| `kPackage` | Package operations | 13 actions |
| `kException` | Exception operations | 8 actions |
| `kRole` | Role operations | 13 actions |
| `kUser` | User operations | 14 actions |
| `kGroup` | Group operations | 8 actions |
| `kJob` | Job operations | 12 actions |
| `kSchedule` | Schedule operations | 8 actions |

### Data and Tools Categories

| Category | Description | Example Actions |
|----------|-------------|-----------------|
| `kData` | Data editing | Insert, Update, Delete, Filter |
| `kImportExport` | Import/Export | Import CSV, Export SQL, Export Excel |
| `kTools` | General tools | Generate DDL, Compare Schemas |
| `kMonitor` | Monitoring | Connections, Transactions, Locks |
| `kMaintenance` | Maintenance | Analyze, Vacuum, Reindex |
| `kNavigation` | Navigation | Back, Forward, Refresh |
| `kSearch` | Search operations | Find, Replace, Advanced Search |

## Action Reference by Category

### File Actions

| Action ID | Display Name | Accelerator | Description |
|-----------|--------------|-------------|-------------|
| `file.new_connection` | New Connection... | Ctrl+N | Create a new database connection |
| `file.open_sql` | Open SQL Script... | Ctrl+O | Open a SQL script file |
| `file.save_sql` | Save | Ctrl+S | Save current SQL script |
| `file.save_as` | Save As... | Ctrl+Shift+S | Save with a new name |
| `file.print` | Print... | Ctrl+P | Print current document |
| `file.print_preview` | Print Preview | | Preview print output |
| `file.exit` | Exit | Ctrl+Q | Exit the application |

### Edit Actions

| Action ID | Display Name | Accelerator | Description |
|-----------|--------------|-------------|-------------|
| `edit.undo` | Undo | Ctrl+Z | Undo last action |
| `edit.redo` | Redo | Ctrl+Shift+Z | Redo last undone action |
| `edit.cut` | Cut | Ctrl+X | Cut selection to clipboard |
| `edit.copy` | Copy | Ctrl+C | Copy selection to clipboard |
| `edit.paste` | Paste | Ctrl+V | Paste from clipboard |
| `edit.select_all` | Select All | Ctrl+A | Select all content |
| `edit.find` | Find... | Ctrl+F | Find text |
| `edit.find_replace` | Find and Replace... | Ctrl+H | Find and replace text |
| `edit.find_next` | Find Next | F3 | Find next occurrence |
| `edit.find_prev` | Find Previous | Shift+F3 | Find previous occurrence |
| `edit.goto_line` | Go to Line... | Ctrl+G | Go to specific line |
| `edit.preferences` | Preferences... | Ctrl+Comma | Edit application preferences |

### View Actions

| Action ID | Display Name | Accelerator | Description |
|-----------|--------------|-------------|-------------|
| `view.refresh` | Refresh | F5 | Refresh current view |
| `view.zoom_in` | Zoom In | Ctrl++ | Increase zoom level |
| `view.zoom_out` | Zoom Out | Ctrl+- | Decrease zoom level |
| `view.zoom_reset` | Reset Zoom | Ctrl+0 | Reset zoom to default |
| `view.fullscreen` | Fullscreen | F11 | Toggle fullscreen mode |
| `view.status_bar` | Status Bar | | Show/hide status bar |
| `view.search_bar` | Search Bar | | Show/hide search bar |
| `view.project_navigator` | Project Navigator | | Show/hide project navigator |
| `view.properties` | Properties | | Show/hide properties panel |
| `view.output` | Output | | Show/hide output panel |
| `view.query_plan` | Query Plan | | Show/hide query plan panel |

### Database Actions

| Action ID | Display Name | Requirements | Description |
|-----------|--------------|--------------|-------------|
| `db.connect` | Connect | | Connect to database |
| `db.disconnect` | Disconnect | | Disconnect from database |
| `db.reconnect` | Reconnect | | Reconnect to database |
| `db.new_database` | New Database... | | Create a new database |
| `db.register` | Register Database... | | Register existing database |
| `db.unregister` | Unregister Database | | Unregister from tree |
| `db.properties` | Properties... | | Database properties |
| `db.backup` | Backup... | Connection | Backup to file |
| `db.restore` | Restore... | Connection | Restore from backup |
| `db.verify` | Verify... | Connection | Verify integrity |
| `db.compact` | Compact... | Connection | Compact database |
| `db.shutdown` | Shutdown | Connection | Shutdown database |

### Query Actions

| Action ID | Display Name | Accelerator | Requirements |
|-----------|--------------|-------------|--------------|
| `query.execute` | Execute | F9 | Connection |
| `query.execute_selection` | Execute Selection | Ctrl+F9 | Connection |
| `query.execute_script` | Execute Script | | Connection |
| `query.explain` | Explain | | Connection |
| `query.explain_analyze` | Explain Analyze | | Connection |
| `query.stop` | Stop | | Connection |
| `query.format` | Format SQL | | |
| `query.comment` | Comment | | |
| `query.uncomment` | Uncomment | | |
| `query.toggle_comment` | Toggle Comment | Ctrl+/ | |
| `query.uppercase` | Uppercase | | |
| `query.lowercase` | Lowercase | | |

### Transaction Actions

| Action ID | Display Name | Requirements |
|-----------|--------------|--------------|
| `tx.start` | Start Transaction | Connection |
| `tx.commit` | Commit | Connection |
| `tx.rollback` | Rollback | Connection |
| `tx.savepoint` | Savepoint... | Connection |
| `tx.rollback_to` | Rollback to Savepoint... | Connection |
| `tx.autocommit` | Auto Commit | Connection |
| `tx.isolation` | Isolation Level... | Connection |
| `tx.readonly` | Read Only | Connection |

### Table Actions

| Action ID | Display Name | Description |
|-----------|--------------|-------------|
| `table.new` | New Table... | Create new table |
| `table.create` | Create Table... | Create from definition |
| `table.alter` | Alter Table... | Modify structure |
| `table.drop` | Drop Table... | Delete table |
| `table.recreate` | Recreate Table... | Drop and recreate |
| `table.truncate` | Truncate Table... | Remove all data |
| `table.rename` | Rename Table... | Rename table |
| `table.copy` | Copy Table... | Copy to another DB |
| `table.duplicate` | Duplicate Table... | Duplicate in same DB |
| `table.properties` | Properties... | Table properties |
| `table.constraints` | Constraints... | Manage constraints |
| `table.indexes` | Indexes... | Manage indexes |
| `table.triggers` | Triggers... | Manage triggers |
| `table.dependencies` | Dependencies... | Show dependencies |
| `table.permissions` | Permissions... | Manage permissions |
| `table.statistics` | Statistics... | Table statistics |
| `table.export_data` | Export Data... | Export data |
| `table.import_data` | Import Data... | Import data |
| `table.generate_ddl` | Generate DDL | Generate CREATE TABLE |
| `table.generate_select` | Generate SELECT | Generate SELECT statement |
| `table.generate_insert` | Generate INSERT | Generate INSERT statement |
| `table.generate_update` | Generate UPDATE | Generate UPDATE statement |
| `table.generate_delete` | Generate DELETE | Generate DELETE statement |
| `table.browse_data` | Browse Data | Open data editor |

### View Actions

| Action ID | Display Name | Description |
|-----------|--------------|-------------|
| `view.new` | New View... | Create new view |
| `view.create` | Create View... | Create from definition |
| `view.alter` | Alter View... | Modify definition |
| `view.drop` | Drop View... | Delete view |
| `view.refresh_def` | Refresh View | Refresh definition |
| `view.recreate` | Recreate View... | Drop and recreate |
| `view.rename` | Rename View... | Rename view |
| `view.copy` | Copy View... | Copy to another DB |
| `view.object_properties` | Properties... | View properties |
| `view.dependencies` | Dependencies... | Show dependencies |
| `view.permissions` | Permissions... | Manage permissions |
| `view.generate_ddl` | Generate DDL | Generate CREATE VIEW |
| `view.browse_data` | Browse Data | Open data editor |

### Procedure Actions

| Action ID | Display Name | Description |
|-----------|--------------|-------------|
| `procedure.new` | New Procedure... | Create new procedure |
| `procedure.create` | Create Procedure... | Create from definition |
| `procedure.alter` | Alter Procedure... | Modify procedure |
| `procedure.drop` | Drop Procedure... | Delete procedure |
| `procedure.recreate` | Recreate Procedure... | Drop and recreate |
| `procedure.rename` | Rename Procedure... | Rename procedure |
| `procedure.copy` | Copy Procedure... | Copy to another DB |
| `procedure.properties` | Properties... | Procedure properties |
| `procedure.execute` | Execute... | Execute procedure |
| `procedure.debug` | Debug... | Debug procedure |
| `procedure.dependencies` | Dependencies... | Show dependencies |
| `procedure.permissions` | Permissions... | Manage permissions |
| `procedure.generate_ddl` | Generate DDL | Generate CREATE PROCEDURE |
| `procedure.generate_call` | Generate CALL | Generate CALL statement |

### Function Actions

| Action ID | Display Name | Description |
|-----------|--------------|-------------|
| `function.new` | New Function... | Create new function |
| `function.create` | Create Function... | Create from definition |
| `function.alter` | Alter Function... | Modify function |
| `function.drop` | Drop Function... | Delete function |
| `function.recreate` | Recreate Function... | Drop and recreate |
| `function.rename` | Rename Function... | Rename function |
| `function.copy` | Copy Function... | Copy to another DB |
| `function.properties` | Properties... | Function properties |
| `function.execute` | Execute... | Execute function |
| `function.debug` | Debug... | Debug function |
| `function.dependencies` | Dependencies... | Show dependencies |
| `function.permissions` | Permissions... | Manage permissions |
| `function.generate_ddl` | Generate DDL | Generate CREATE FUNCTION |
| `function.generate_call` | Generate CALL | Generate function call |

### Trigger Actions

| Action ID | Display Name | Description |
|-----------|--------------|-------------|
| `trigger.new` | New Trigger... | Create new trigger |
| `trigger.create` | Create Trigger... | Create from definition |
| `trigger.alter` | Alter Trigger... | Modify trigger |
| `trigger.drop` | Drop Trigger... | Delete trigger |
| `trigger.enable` | Enable | Enable trigger |
| `trigger.disable` | Disable | Disable trigger |
| `trigger.recreate` | Recreate Trigger... | Drop and recreate |
| `trigger.rename` | Rename Trigger... | Rename trigger |
| `trigger.copy` | Copy Trigger... | Copy to another table |
| `trigger.properties` | Properties... | Trigger properties |
| `trigger.generate_ddl` | Generate DDL | Generate CREATE TRIGGER |

### Data Actions

| Action ID | Display Name | Accelerator |
|-----------|--------------|-------------|
| `data.insert` | Insert Row | Insert |
| `data.update` | Update Row | |
| `data.delete` | Delete Row | Delete |
| `data.refresh` | Refresh | F5 |
| `data.filter` | Filter... | |
| `data.clear_filter` | Clear Filter | |
| `data.sort_asc` | Sort Ascending | |
| `data.sort_desc` | Sort Descending | |
| `data.export` | Export... | |
| `data.import` | Import... | |
| `data.print` | Print... | |
| `data.commit` | Commit Changes | |
| `data.rollback` | Rollback Changes | |
| `data.first` | First | |
| `data.last` | Last | |
| `data.next` | Next | |
| `data.prev` | Previous | |
| `data.goto` | Go to Row... | |

### Import/Export Actions

| Action ID | Display Name |
|-----------|--------------|
| `import.sql` | Import SQL... |
| `import.csv` | Import CSV... |
| `import.xml` | Import XML... |
| `import.json` | Import JSON... |
| `import.excel` | Import Excel... |
| `export.sql` | Export SQL... |
| `export.csv` | Export CSV... |
| `export.xml` | Export XML... |
| `export.json` | Export JSON... |
| `export.excel` | Export Excel... |
| `export.html` | Export HTML... |
| `export.pdf` | Export PDF... |

### Tools Actions

| Action ID | Display Name |
|-----------|--------------|
| `tools.generate_ddl` | Generate DDL... |
| `tools.generate_data` | Generate Test Data... |
| `tools.compare_schema` | Compare Schemas... |
| `tools.compare_data` | Compare Data... |
| `tools.migrate` | Database Migration... |
| `tools.script` | Script Console... |
| `tools.scheduler` | Job Scheduler... |

### Monitor Actions

| Action ID | Display Name |
|-----------|--------------|
| `monitor.connections` | Connections |
| `monitor.transactions` | Transactions |
| `monitor.statements` | Active Statements |
| `monitor.locks` | Locks |
| `monitor.performance` | Performance |
| `monitor.logs` | Logs |
| `monitor.alerts` | Alerts |

## Action Groups

Actions are organized into groups for menu construction:

| Group ID | Actions |
|----------|---------|
| `file.basic` | New Connection, Open SQL, Save, Save As |
| `file.print` | Print, Print Preview |
| `file.exit` | Exit |
| `edit.undo` | Undo, Redo |
| `edit.clipboard` | Cut, Copy, Paste |
| `edit.search` | Find, Find Replace, Find Next, Find Prev |
| `db.connect` | Connect, Disconnect, Reconnect |
| `db.manage` | New DB, Register, Unregister, Properties |
| `db.backup` | Backup, Restore, Verify |
| `query.execute` | Execute, Execute Selection, Stop |
| `query.tools` | Explain, Explain Analyze, Format |
| `transaction.basic` | Start, Commit, Rollback |
| `window.basic` | New Window, Close, Close All |
| `window.arrange` | Cascade, Tile Horizontal, Tile Vertical |

## Menu Templates

Pre-configured menu structures:

| Menu ID | Groups |
|---------|--------|
| `file` | file.basic, file.print, file.exit |
| `edit` | edit.undo, edit.clipboard, edit.search, edit.prefs |
| `database` | db.connect, db.manage, db.backup |
| `query` | query.execute, query.tools |
| `transaction` | transaction.basic, transaction.options |
| `window` | window.basic, window.navigate, window.arrange |
| `help` | help.docs, help.about |

## Using the Action Catalog

### Finding an Action

```cpp
#include "ui/window_framework/action_catalog.h"

// Get the catalog
ActionCatalog* catalog = ActionCatalog::get();

// Find a specific action
const ActionDefinition* action = catalog->findAction(ActionIds::TABLE_NEW);

// Get all actions in a category
auto table_actions = catalog->getActionsByCategory(ActionCategory::kTable);

// Search for actions
auto results = catalog->searchActions("export");
```

### Checking Action Requirements

```cpp
const ActionDefinition* action = catalog->findAction(ActionIds::TABLE_ALTER);
if (action) {
    bool needs_connection = action->requirements.needs_connection;
    bool needs_selection = action->requirements.needs_selection;
}

// Check if can execute
bool can_execute = catalog->canExecute(ActionIds::QUERY_EXECUTE);
```

### Building Menus from Templates

```cpp
const MenuTemplate* menu_template = catalog->findMenuTemplate("database");
if (menu_template) {
    // Get action groups for this menu
    auto groups = catalog->getActionGroupsForMenu("database");
    
    // Build menu from groups
    for (const auto* group : groups) {
        for (const auto& action_id : group->action_ids) {
            const ActionDefinition* action = catalog->findAction(action_id);
            // Add to wxMenu...
        }
    }
}
```

## Action Statistics

| Category | Action Count |
|----------|--------------|
| File | 7 |
| Edit | 12 |
| View | 11 |
| Window | 8 |
| Help | 5 |
| Database | 12 |
| Server | 5 |
| Schema | 3 |
| Query | 12 |
| Transaction | 8 |
| Script | 8 |
| Table | 24 |
| View | 13 |
| Procedure | 14 |
| Function | 14 |
| Trigger | 11 |
| Index | 10 |
| Constraint | 8 |
| Domain | 10 |
| Sequence | 11 |
| Package | 13 |
| Exception | 8 |
| Role | 13 |
| User | 14 |
| Group | 8 |
| Job | 12 |
| Schedule | 8 |
| Data | 19 |
| Import/Export | 12 |
| Tools | 7 |
| Monitor | 7 |
| Maintenance | 5 |
| Navigation | 7 |
| Search | 8 |
| **Total** | **~380** |

## Related Documentation

- [Window Types Catalog](WINDOW_TYPES_CATALOG.md) - Window type definitions
- [Dynamic Menu System](DYNAMIC_MENUS.md) - How menus switch based on window type
- [Toolbar Editor](TOOLBAR_EDITOR.md) - Customizing toolbars with actions
- [Menu Editor](MENU_EDITOR.md) - Assigning actions to menus
