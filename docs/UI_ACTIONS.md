# ScratchRobin UI Actions Reference

Complete list of all actions that can be mapped to buttons, toolbars, menus, or keyboard shortcuts.

## Table of Contents
- [File Actions](#file-actions)
- [Edit Actions](#edit-actions)
- [View Actions](#view-actions)
- [Query Actions](#query-actions)
- [Transaction Actions](#transaction-actions)
- [Database Actions](#database-actions)
- [Tools Actions](#tools-actions)
- [Window Actions](#window-actions)
- [Help Actions](#help-actions)
- [Data Management Actions](#data-management-actions)
- [Collaboration Actions](#collaboration-actions)
- [Extensions Actions](#extensions-actions)

---

## File Actions

| Action ID | Display Name | Description | Default Shortcut |
|-----------|--------------|-------------|------------------|
| `file.new_connection` | New Connection | Create a new database connection | |
| `file.open_sql` | Open SQL Script | Open a SQL file in editor | Ctrl+O |
| `file.save` | Save | Save current SQL script | Ctrl+S |
| `file.save_as` | Save As... | Save with a new name | Ctrl+Shift+S |
| `file.exit` | Exit | Close the application | |

---

## Edit Actions

| Action ID | Display Name | Description | Default Shortcut |
|-----------|--------------|-------------|------------------|
| `edit.undo` | Undo | Undo last action | Ctrl+Z |
| `edit.redo` | Redo | Redo last undone action | Ctrl+Y |
| `edit.cut` | Cut | Cut selected text | Ctrl+X |
| `edit.copy` | Copy | Copy selected text | Ctrl+C |
| `edit.paste` | Paste | Paste from clipboard | Ctrl+V |
| `edit.find` | Find | Open find dialog | Ctrl+F |
| `edit.find_replace` | Find and Replace | Open find/replace dialog | Ctrl+H |
| `edit.find_next` | Find Next | Find next occurrence | F3 |
| `edit.find_previous` | Find Previous | Find previous occurrence | Shift+F3 |
| `edit.preferences` | Preferences | Open preferences dialog | |

---

## View Actions

| Action ID | Display Name | Description | Default Shortcut |
|-----------|--------------|-------------|------------------|
| `view.navigator` | Project Navigator | Toggle navigator panel | |
| `view.results` | Results Panel | Toggle results panel | |
| `view.editor` | SQL Editor | Toggle editor panel | |
| `view.refresh` | Refresh | Refresh current view | F5 |
| `view.view_manager` | View Manager | Open view manager | |
| `view.trigger_manager` | Trigger Manager | Open trigger manager | |
| `view.git_integration` | Git Integration | Open Git panel | |
| `view.report_manager` | Report Manager | Open report manager | |
| `view.dashboard_manager` | Dashboard Manager | Open dashboard builder | |
| `view.table_designer` | Table Designer | Open table designer | |
| `view.sql_history` | SQL History | Show query history | |
| `view.query_favorites` | Query Favorites | Show favorite queries | |

---

## Query Actions

| Action ID | Display Name | Description | Default Shortcut |
|-----------|--------------|-------------|------------------|
| `query.execute` | Execute | Execute current SQL | F9 |
| `query.execute_selection` | Execute Selection | Execute selected SQL | Ctrl+F9 |
| `query.execute_script` | Execute Script | Execute as script | |
| `query.stop` | Stop Execution | Cancel running query | |
| `query.explain` | Explain Plan | Show query plan | |
| `query.explain_analyze` | Explain Analyze | Execute and show plan | |
| `query.format_sql` | Format SQL | Format SQL code | |
| `query.comment` | Comment Lines | Add line comments | Ctrl+/ |
| `query.uncomment` | Uncomment Lines | Remove line comments | |
| `query.toggle_comment` | Toggle Comment | Toggle comments | |
| `query.uppercase` | Uppercase | Convert to uppercase | |
| `query.lowercase` | Lowercase | Convert to lowercase | |

---

## Transaction Actions

| Action ID | Display Name | Description | Default Shortcut |
|-----------|--------------|-------------|------------------|
| `transaction.start` | Start Transaction | BEGIN TRANSACTION | |
| `transaction.commit` | Commit | COMMIT | |
| `transaction.rollback` | Rollback | ROLLBACK | |
| `transaction.savepoint` | Set Savepoint | Create savepoint | |
| `transaction.autocommit` | Auto Commit | Toggle auto-commit | |
| `transaction.read_only` | Read Only | Toggle read-only mode | |
| `transaction.isolation.read_uncommitted` | Read Uncommitted | Set isolation level | |
| `transaction.isolation.read_committed` | Read Committed | Set isolation level | |
| `transaction.isolation.repeatable_read` | Repeatable Read | Set isolation level | |
| `transaction.isolation.serializable` | Serializable | Set isolation level | |

---

## Database Actions

| Action ID | Display Name | Description | Default Shortcut |
|-----------|--------------|-------------|------------------|
| `db.connect` | Connect | Connect to database | |
| `db.disconnect` | Disconnect | Disconnect from database | |
| `db.backup` | Backup | Backup database | |
| `db.restore` | Restore | Restore database | |

---

## Tools Actions

### Schema & Design
| Action ID | Display Name | Description |
|-----------|--------------|-------------|
| `tools.generate_ddl` | Generate DDL | Generate DDL for objects |
| `tools.compare_schemas` | Compare Schemas | Schema comparison tool |
| `tools.compare_data` | Compare Data | Data comparison tool |
| `tools.schema_compare` | Schema Compare Panel | Open schema compare |
| `tools.schema_migration` | Schema Migration | Migration tool |
| `tools.data_modeler` | Data Modeler | ER diagram modeler |
| `tools.er_diagram` | ER Diagram | View ER diagram |
| `tools.er_diagram_designer` | ER Diagram Designer | Design ER diagrams |

### Query & Performance
| Action ID | Display Name | Description |
|-----------|--------------|-------------|
| `tools.query_builder` | Query Builder | Visual query builder |
| `tools.query_plan_visualizer` | Query Plan Visualizer | Visualize execution plans |
| `tools.sql_debugger` | SQL Debugger | Debug SQL procedures |
| `tools.procedure_debugger` | Procedure Debugger | Debug stored procedures |
| `tools.sql_profiler` | SQL Profiler | Profile query performance |
| `tools.code_formatter` | Code Formatter | Format SQL code |
| `tools.performance_dashboard` | Performance Dashboard | Performance metrics |
| `tools.slow_query_log` | Slow Query Log | View slow queries |
| `tools.health_check` | Health Check | Database health check |
| `tools.vacuum_analyze` | Vacuum/Analyze | Maintenance operations |

### Import/Export
| Action ID | Display Name | Description |
|-----------|--------------|-------------|
| `tools.import.sql` | Import SQL | Import SQL script |
| `tools.import.csv` | Import CSV | Import CSV data |
| `tools.import.json` | Import JSON | Import JSON data |
| `tools.import.excel` | Import Excel | Import Excel file |
| `tools.export.sql` | Export SQL | Export as SQL |
| `tools.export.csv` | Export CSV | Export as CSV |
| `tools.export.json` | Export JSON | Export as JSON |
| `tools.export.excel` | Export Excel | Export as Excel |
| `tools.export.xml` | Export XML | Export as XML |
| `tools.export.html` | Export HTML | Export as HTML |

### Monitoring
| Action ID | Display Name | Description |
|-----------|--------------|-------------|
| `tools.monitor.connections` | Monitor Connections | Active connections |
| `tools.monitor.transactions` | Monitor Transactions | Active transactions |
| `tools.monitor.statements` | Monitor Statements | Active statements |
| `tools.monitor.locks` | Monitor Locks | Lock monitoring |
| `tools.monitor.performance` | Monitor Performance | Performance metrics |

### Administration
| Action ID | Display Name | Description |
|-----------|--------------|-------------|
| `tools.user_management` | User Management | Manage users |
| `tools.role_management` | Role Management | Manage roles |
| `tools.permission_browser` | Permission Browser | Browse permissions |
| `tools.audit_log_viewer` | Audit Log Viewer | View audit logs |
| `tools.ssl_certificate_manager` | SSL Certificate Manager | Manage SSL certs |
| `tools.backup_manager` | Backup Manager | Backup management |
| `tools.scheduled_jobs` | Scheduled Jobs | Job scheduler |
| `tools.migration_wizard` | Migration Wizard | Database migration |
| `tools.scheduler` | Job Scheduler | Schedule jobs |
| `tools.alert_system` | Alert System | Configure alerts |

### Utilities
| Action ID | Display Name | Description |
|-----------|--------------|-------------|
| `tools.doc_generator` | Documentation Generator | Generate docs |
| `tools.toolbar_editor` | Toolbar Editor | Customize toolbars |
| `tools.menu_editor` | Menu Editor | Customize menus |
| `tools.theme_settings` | Theme Settings | Appearance settings |

---

## Window Actions

| Action ID | Display Name | Description | Default Shortcut |
|-----------|--------------|-------------|------------------|
| `window.new` | New Window | New SQL editor tab | |
| `window.close` | Close Window | Close current tab | Ctrl+W |
| `window.close_all` | Close All Windows | Close all tabs | |
| `window.next` | Next Window | Next tab | Ctrl+Tab |
| `window.previous` | Previous Window | Previous tab | Ctrl+Shift+Tab |
| `window.cascade` | Cascade Windows | Cascade layout | |
| `window.tile_horizontal` | Tile Horizontal | Horizontal layout | |
| `window.tile_vertical` | Tile Vertical | Vertical layout | |
| `window.save_layout` | Save Layout | Save current layout | |
| `window.load_layout` | Load Layout | Load saved layout | |
| `window.reset_layout` | Reset Layout | Reset to default | |
| `window.manage_layouts` | Manage Layouts | Layout manager | |

---

## Help Actions

| Action ID | Display Name | Description | Default Shortcut |
|-----------|--------------|-------------|------------------|
| `help.shortcuts` | Keyboard Shortcuts | Show shortcuts | |
| `help.tip_of_day` | Tip of the Day | Show random tip | |
| `help.about` | About | About dialog | |

---

## Data Management Actions

| Action ID | Display Name | Description |
|-----------|--------------|-------------|
| `data.generator` | Data Generator | Generate test data |
| `data.masking` | Data Masking | Mask sensitive data |
| `data.cleansing` | Data Cleansing | Clean data |
| `data.lineage` | Data Lineage | Track data lineage |

---

## Collaboration Actions

| Action ID | Display Name | Description |
|-----------|--------------|-------------|
| `collab.team_sharing` | Team Sharing | Share queries |
| `collab.query_comments` | Query Comments | Comment on queries |
| `collab.change_tracking` | Change Tracking | Track changes |

---

## Extensions Actions

| Action ID | Display Name | Description |
|-----------|--------------|-------------|
| `extensions.plugin_manager` | Plugin Manager | Manage plugins |
| `extensions.macro_recording` | Macro Recording | Record macros |
| `extensions.scripting_console` | Scripting Console | Python/Lua console |

---

## SQL Editor Context Menu Actions

These actions are available from the SQL editor's right-click context menu:

| Action ID | Display Name | Description |
|-----------|--------------|-------------|
| `editor.zoom_in` | Zoom In | Increase font size |
| `editor.zoom_out` | Zoom Out | Decrease font size |
| `editor.zoom_reset` | Reset Zoom | Reset font size |
| `editor.format` | Format SQL | Format SQL code |
| `editor.minify` | Minify SQL | Minify SQL code |
| `editor.toggle_comment` | Toggle Comment | Comment/uncomment |

---

## Data Modeler Actions

Actions available in the Data Modeler panel:

| Action ID | Display Name | Description |
|-----------|--------------|-------------|
| `modeler.new` | New Model | Create new model |
| `modeler.open` | Open Model | Open existing model |
| `modeler.save` | Save Model | Save current model |
| `modeler.add_table` | Add Table | Add table to model |
| `modeler.add_view` | Add View | Add view to model |
| `modeler.add_relationship` | Add Relationship | Add FK relationship |
| `modeler.reverse_engineer` | Reverse Engineer | Import from DB |
| `modeler.forward_engineer` | Forward Engineer | Generate DDL |
| `modeler.validate` | Validate Model | Check model |
| `modeler.auto_layout` | Auto Layout | Auto-arrange |

---

## Usage Examples

### Creating a Toolbar Button
```cpp
QAction* action = new QAction(QIcon(":/icons/execute.png"), tr("Execute"), this);
action->setShortcut(QKeySequence("F9"));
action->setToolTip(tr("Execute SQL (F9)"));
connect(action, &QAction::triggered, this, &MainWindow::onQueryExecute);
toolbar->addAction(action);
```

### Mapping by Action ID
```cpp
// In toolbar editor or configuration
QString actionId = "query.execute";  // Maps to onQueryExecute
QString actionId = "file.save";      // Maps to onFileSave
QString actionId = "transaction.commit"; // Maps to onTransactionCommit
```

### Menu Organization
```
File
  ├─ New Connection       [file.new_connection]
  ├─ Open SQL Script      [file.open_sql]
  ├─ Save                 [file.save]
  └─ Exit                 [file.exit]

Query
  ├─ Execute              [query.execute]        (F9)
  ├─ Execute Selection    [query.execute_selection] (Ctrl+F9)
  ├─ Stop                 [query.stop]
  ├─ Explain Plan         [query.explain]
  └─ Format SQL           [query.format_sql]

Transaction
  ├─ Start Transaction    [transaction.start]
  ├─ Commit               [transaction.commit]
  ├─ Rollback             [transaction.rollback]
  └─ Set Savepoint        [transaction.savepoint]
```

---

## Total Action Count: ~120 Actions

This covers all user-triggerable functionality in ScratchRobin that can be bound to:
- Toolbar buttons
- Menu items
- Keyboard shortcuts
- Macro recordings
- Custom keybindings
