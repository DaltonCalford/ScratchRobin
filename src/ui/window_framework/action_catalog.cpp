/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "ui/window_framework/action_catalog.h"
#include <algorithm>

namespace scratchrobin::ui {

// ============================================================================
// Category String Conversions
// ============================================================================
std::string actionCategoryToString(ActionCategory cat) {
    switch (cat) {
        case ActionCategory::kFile: return "File";
        case ActionCategory::kEdit: return "Edit";
        case ActionCategory::kDbView: return "DbView";
        case ActionCategory::kWindow: return "Window";
        case ActionCategory::kHelp: return "Help";
        case ActionCategory::kDatabase: return "Database";
        case ActionCategory::kServer: return "Server";
        case ActionCategory::kSchema: return "Schema";
        case ActionCategory::kQuery: return "Query";
        case ActionCategory::kTransaction: return "Transaction";
        case ActionCategory::kScript: return "Script";
        case ActionCategory::kTable: return "Table";
        case ActionCategory::kView: return "View";
        case ActionCategory::kProcedure: return "Procedure";
        case ActionCategory::kFunction: return "Function";
        case ActionCategory::kTrigger: return "Trigger";
        case ActionCategory::kIndex: return "Index";
        case ActionCategory::kConstraint: return "Constraint";
        case ActionCategory::kDomain: return "Domain";
        case ActionCategory::kSequence: return "Sequence";
        case ActionCategory::kPackage: return "Package";
        case ActionCategory::kException: return "Exception";
        case ActionCategory::kRole: return "Role";
        case ActionCategory::kUser: return "User";
        case ActionCategory::kGroup: return "Group";
        case ActionCategory::kJob: return "Job";
        case ActionCategory::kSchedule: return "Schedule";
        case ActionCategory::kCollation: return "Collation";
        case ActionCategory::kCharset: return "Charset";
        case ActionCategory::kFilter: return "Filter";
        case ActionCategory::kShadow: return "Shadow";
        case ActionCategory::kSynonym: return "Synonym";
        case ActionCategory::kData: return "Data";
        case ActionCategory::kImportExport: return "Import/Export";
        case ActionCategory::kTools: return "Tools";
        case ActionCategory::kMonitor: return "Monitor";
        case ActionCategory::kMaintenance: return "Maintenance";
        case ActionCategory::kMigration: return "Migration";
        case ActionCategory::kNavigation: return "Navigation";
        case ActionCategory::kSearch: return "Search";
        case ActionCategory::kCustom: return "Custom";
        default: return "Unknown";
    }
}

ActionCategory actionCategoryFromString(const std::string& str) {
    if (str == "File") return ActionCategory::kFile;
    if (str == "Edit") return ActionCategory::kEdit;
    if (str == "View") return ActionCategory::kView;
    if (str == "Window") return ActionCategory::kWindow;
    if (str == "Help") return ActionCategory::kHelp;
    if (str == "Database") return ActionCategory::kDatabase;
    if (str == "Server") return ActionCategory::kServer;
    if (str == "Schema") return ActionCategory::kSchema;
    if (str == "Query") return ActionCategory::kQuery;
    if (str == "Transaction") return ActionCategory::kTransaction;
    if (str == "Script") return ActionCategory::kScript;
    if (str == "Table") return ActionCategory::kTable;
    if (str == "View") return ActionCategory::kView;
    if (str == "Procedure") return ActionCategory::kProcedure;
    if (str == "Function") return ActionCategory::kFunction;
    if (str == "Trigger") return ActionCategory::kTrigger;
    if (str == "Index") return ActionCategory::kIndex;
    if (str == "Constraint") return ActionCategory::kConstraint;
    if (str == "Domain") return ActionCategory::kDomain;
    if (str == "Sequence") return ActionCategory::kSequence;
    if (str == "Package") return ActionCategory::kPackage;
    if (str == "Exception") return ActionCategory::kException;
    if (str == "Role") return ActionCategory::kRole;
    if (str == "User") return ActionCategory::kUser;
    if (str == "Group") return ActionCategory::kGroup;
    if (str == "Job") return ActionCategory::kJob;
    if (str == "Schedule") return ActionCategory::kSchedule;
    if (str == "Data") return ActionCategory::kData;
    if (str == "Import/Export") return ActionCategory::kImportExport;
    if (str == "Tools") return ActionCategory::kTools;
    if (str == "Monitor") return ActionCategory::kMonitor;
    if (str == "Maintenance") return ActionCategory::kMaintenance;
    if (str == "Navigation") return ActionCategory::kNavigation;
    if (str == "Search") return ActionCategory::kSearch;
    return ActionCategory::kCustom;
}

// ============================================================================
// ActionCatalog Implementation
// ============================================================================
ActionCatalog* ActionCatalog::get() {
    static ActionCatalog instance;
    return &instance;
}

void ActionCatalog::initialize() {
    registerFileActions();
    registerEditActions();
    registerViewActions();
    registerWindowActions();
    registerHelpActions();
    
    registerDatabaseActions();
    registerServerActions();
    registerSchemaActions();
    
    registerQueryActions();
    registerTransactionActions();
    registerScriptActions();
    
    registerTableActions();
    registerViewActions();
    registerProcedureActions();
    registerFunctionActions();
    registerTriggerActions();
    registerIndexActions();
    registerConstraintActions();
    registerDomainActions();
    registerSequenceActions();
    registerPackageActions();
    registerExceptionActions();
    registerRoleActions();
    registerUserActions();
    registerGroupActions();
    registerJobActions();
    registerScheduleActions();
    registerOtherObjectActions();
    
    registerDataActions();
    registerImportExportActions();
    registerToolsActions();
    registerMonitorActions();
    registerMaintenanceActions();
    registerNavigationActions();
    registerSearchActions();
    
    registerActionGroups();
    registerMenuTemplates();
}

// ============================================================================
// Helper to register an action with common defaults
// ============================================================================
static ActionDefinition makeAction(const char* id, const char* name, 
                                    const char* desc, ActionCategory cat,
                                    const char* icon = nullptr,
                                    const char* accel = nullptr) {
    ActionDefinition a;
    a.action_id = id;
    a.display_name = name;
    a.description = desc;
    a.tooltip = desc;
    a.category = cat;
    a.scope = ActionScope::kGlobal;
    a.default_icon = icon ? icon : "";
    a.accelerator = accel ? accel : "";
    return a;
}

// ============================================================================
// File Actions
// ============================================================================
void ActionCatalog::registerFileActions() {
    int order = 0;
    auto add = [&](const ActionDefinition& a) {
        ActionDefinition def = a;
        def.sort_order = order++;
        registerAction(def);
    };
    
    add(makeAction(ActionIds::FILE_NEW_CONNECTION, "New Connection...",
                   "Create a new database connection", ActionCategory::kFile,
                   "document-new", "Ctrl+N"));
    
    add(makeAction(ActionIds::FILE_OPEN_SQL, "Open SQL Script...",
                   "Open a SQL script file", ActionCategory::kFile,
                   "document-open", "Ctrl+O"));
    
    add(makeAction(ActionIds::FILE_SAVE_SQL, "Save",
                   "Save current SQL script", ActionCategory::kFile,
                   "document-save", "Ctrl+S"));
    
    add(makeAction(ActionIds::FILE_SAVE_AS, "Save As...",
                   "Save SQL script with a new name", ActionCategory::kFile,
                   "document-save-as", "Ctrl+Shift+S"));
    
    add(makeAction(ActionIds::FILE_PRINT, "Print...",
                   "Print current document", ActionCategory::kFile,
                   "document-print", "Ctrl+P"));
    
    add(makeAction(ActionIds::FILE_PRINT_PREVIEW, "Print Preview",
                   "Preview print output", ActionCategory::kFile));
    
    add(makeAction(ActionIds::FILE_EXIT, "Exit",
                   "Exit the application", ActionCategory::kFile,
                   "window-close", "Ctrl+Q"));
}

// ============================================================================
// Edit Actions
// ============================================================================
void ActionCatalog::registerEditActions() {
    int order = 0;
    auto add = [&](const ActionDefinition& a) {
        ActionDefinition def = a;
        def.sort_order = order++;
        registerAction(def);
    };
    
    add(makeAction(ActionIds::EDIT_UNDO, "Undo",
                   "Undo last action", ActionCategory::kEdit,
                   "edit-undo", "Ctrl+Z"));
    
    add(makeAction(ActionIds::EDIT_REDO, "Redo",
                   "Redo last undone action", ActionCategory::kEdit,
                   "edit-redo", "Ctrl+Shift+Z"));
    
    add(makeAction(ActionIds::EDIT_CUT, "Cut",
                   "Cut selection to clipboard", ActionCategory::kEdit,
                   "edit-cut", "Ctrl+X"));
    
    add(makeAction(ActionIds::EDIT_COPY, "Copy",
                   "Copy selection to clipboard", ActionCategory::kEdit,
                   "edit-copy", "Ctrl+C"));
    
    add(makeAction(ActionIds::EDIT_PASTE, "Paste",
                   "Paste from clipboard", ActionCategory::kEdit,
                   "edit-paste", "Ctrl+V"));
    
    add(makeAction(ActionIds::EDIT_SELECT_ALL, "Select All",
                   "Select all content", ActionCategory::kEdit,
                   nullptr, "Ctrl+A"));
    
    add(makeAction(ActionIds::EDIT_FIND, "Find...",
                   "Find text", ActionCategory::kEdit,
                   "edit-find", "Ctrl+F"));
    
    add(makeAction(ActionIds::EDIT_FIND_REPLACE, "Find and Replace...",
                   "Find and replace text", ActionCategory::kEdit,
                   "edit-find-replace", "Ctrl+H"));
    
    add(makeAction(ActionIds::EDIT_FIND_NEXT, "Find Next",
                   "Find next occurrence", ActionCategory::kEdit,
                   nullptr, "F3"));
    
    add(makeAction(ActionIds::EDIT_FIND_PREV, "Find Previous",
                   "Find previous occurrence", ActionCategory::kEdit,
                   nullptr, "Shift+F3"));
    
    add(makeAction(ActionIds::EDIT_GOTO_LINE, "Go to Line...",
                   "Go to specific line number", ActionCategory::kEdit,
                   nullptr, "Ctrl+G"));
    
    add(makeAction(ActionIds::EDIT_PREFERENCES, "Preferences...",
                   "Edit application preferences", ActionCategory::kEdit,
                   "tools-preferences", "Ctrl+Comma"));
}

// ============================================================================
// View Actions
// ============================================================================
void ActionCatalog::registerViewActions() {
    int order = 0;
    auto add = [&](const ActionDefinition& a) {
        ActionDefinition def = a;
        def.sort_order = order++;
        registerAction(def);
    };
    
    add(makeAction(ActionIds::VIEW_REFRESH_DEF, "Refresh",
                   "Refresh current view", ActionCategory::kView,
                   "view-refresh", "F5"));
    
    add(makeAction(ActionIds::VIEW_ZOOM_IN, "Zoom In",
                   "Increase zoom level", ActionCategory::kView,
                   "view-zoom-in", "Ctrl++"));
    
    add(makeAction(ActionIds::VIEW_ZOOM_OUT, "Zoom Out",
                   "Decrease zoom level", ActionCategory::kView,
                   "view-zoom-out", "Ctrl+-"));
    
    add(makeAction(ActionIds::VIEW_ZOOM_RESET, "Reset Zoom",
                   "Reset zoom to default", ActionCategory::kView,
                   nullptr, "Ctrl+0"));
    
    add(makeAction(ActionIds::VIEW_FULLSCREEN, "Fullscreen",
                   "Toggle fullscreen mode", ActionCategory::kView,
                   "view-fullscreen", "F11"));
    
    add(makeAction(ActionIds::VIEW_STATUS_BAR, "Status Bar",
                   "Show/hide status bar", ActionCategory::kView));
    
    add(makeAction(ActionIds::VIEW_SEARCH_BAR, "Search Bar",
                   "Show/hide search bar", ActionCategory::kView));
    
    add(makeAction(ActionIds::VIEW_PROJECT_NAVIGATOR, "Project Navigator",
                   "Show/hide project navigator", ActionCategory::kView));
    
    add(makeAction(ActionIds::VIEW_OBJECT_PROPERTIES, "Properties",
                   "Show/hide properties panel", ActionCategory::kView));
    
    add(makeAction(ActionIds::VIEW_OUTPUT, "Output",
                   "Show/hide output panel", ActionCategory::kView));
    
    add(makeAction(ActionIds::VIEW_QUERY_PLAN, "Query Plan",
                   "Show/hide query plan panel", ActionCategory::kView));
}

// ============================================================================
// Window Actions
// ============================================================================
void ActionCatalog::registerWindowActions() {
    int order = 0;
    auto add = [&](const ActionDefinition& a) {
        ActionDefinition def = a;
        def.sort_order = order++;
        registerAction(def);
    };
    
    add(makeAction(ActionIds::WINDOW_NEW, "New Window",
                   "Open new application window", ActionCategory::kWindow,
                   "window-new"));
    
    add(makeAction(ActionIds::WINDOW_CLOSE, "Close",
                   "Close current window", ActionCategory::kWindow,
                   "window-close", "Ctrl+W"));
    
    add(makeAction(ActionIds::WINDOW_CLOSE_ALL, "Close All",
                   "Close all windows", ActionCategory::kWindow));
    
    add(makeAction(ActionIds::WINDOW_NEXT, "Next Window",
                   "Switch to next window", ActionCategory::kWindow,
                   nullptr, "Ctrl+Tab"));
    
    add(makeAction(ActionIds::WINDOW_PREV, "Previous Window",
                   "Switch to previous window", ActionCategory::kWindow,
                   nullptr, "Ctrl+Shift+Tab"));
    
    add(makeAction(ActionIds::WINDOW_CASCADE, "Cascade",
                   "Arrange windows cascaded", ActionCategory::kWindow));
    
    add(makeAction(ActionIds::WINDOW_TILE_H, "Tile Horizontal",
                   "Arrange windows horizontally", ActionCategory::kWindow));
    
    add(makeAction(ActionIds::WINDOW_TILE_V, "Tile Vertical",
                   "Arrange windows vertically", ActionCategory::kWindow));
}

// ============================================================================
// Help Actions
// ============================================================================
void ActionCatalog::registerHelpActions() {
    int order = 0;
    auto add = [&](const ActionDefinition& a) {
        ActionDefinition def = a;
        def.sort_order = order++;
        registerAction(def);
    };
    
    add(makeAction(ActionIds::HELP_DOCS, "Documentation",
                   "Open documentation", ActionCategory::kHelp,
                   "help-contents", "F1"));
    
    add(makeAction(ActionIds::_HELP_CONTEXT, "Context Help",
                   "Get help for current context", ActionCategory::kHelp,
                   "help-context", "Shift+F1"));
    
    add(makeAction(ActionIds::HELP_SHORTCUTS, "Keyboard Shortcuts",
                   "Show keyboard shortcuts", ActionCategory::kHelp));
    
    add(makeAction(ActionIds::HELP_TIP_OF_DAY, "Tip of the Day",
                   "Show tip of the day", ActionCategory::kHelp));
    
    add(makeAction(ActionIds::HELP_ABOUT, "About",
                   "About this application", ActionCategory::kHelp,
                   "help-about"));
}

// ============================================================================
// Database Actions
// ============================================================================
void ActionCatalog::registerDatabaseActions() {
    int order = 0;
    auto add = [&](const ActionDefinition& a) {
        ActionDefinition def = a;
        def.sort_order = order++;
        def.requirements.needs_connection = false;
        registerAction(def);
    };
    
    add(makeAction(ActionIds::DB_CONNECT, "Connect",
                   "Connect to database", ActionCategory::kDatabase,
                   "db-connect"));
    
    add(makeAction(ActionIds::DB_DISCONNECT, "Disconnect",
                   "Disconnect from database", ActionCategory::kDatabase,
                   "db-disconnect"));
    
    add(makeAction(ActionIds::DB_RECONNECT, "Reconnect",
                   "Reconnect to database", ActionCategory::kDatabase));
    
    add(makeAction(ActionIds::DB_NEW, "New Database...",
                   "Create a new database", ActionCategory::kDatabase,
                   "db-new"));
    
    add(makeAction(ActionIds::DB_REGISTER, "Register Database...",
                   "Register existing database", ActionCategory::kDatabase));
    
    add(makeAction(ActionIds::DB_UNREGISTER, "Unregister Database",
                   "Unregister database from tree", ActionCategory::kDatabase));
    
    add(makeAction(ActionIds::DB_PROPERTIES, "Properties...",
                   "Database properties", ActionCategory::kDatabase));
    
    add(makeAction(ActionIds::DB_BACKUP, "Backup...",
                   "Backup database to file", ActionCategory::kDatabase,
                   "db-backup"));
    
    add(makeAction(ActionIds::DB_RESTORE, "Restore...",
                   "Restore database from backup", ActionCategory::kDatabase,
                   "db-restore"));
    
    add(makeAction(ActionIds::DB_VERIFY, "Verify...",
                   "Verify database integrity", ActionCategory::kDatabase));
    
    add(makeAction(ActionIds::DB_COMPACT, "Compact...",
                   "Compact database", ActionCategory::kDatabase));
    
    add(makeAction(ActionIds::DB_SHUTDOWN, "Shutdown",
                   "Shutdown database", ActionCategory::kDatabase));
}

// ============================================================================
// Server Actions
// ============================================================================
void ActionCatalog::registerServerActions() {
    int order = 0;
    auto add = [&](const ActionDefinition& a) {
        ActionDefinition def = a;
        def.sort_order = order++;
        registerAction(def);
    };
    
    add(makeAction(ActionIds::SERVER_REGISTER, "Register Server...",
                   "Register a database server", ActionCategory::kServer));
    
    add(makeAction(ActionIds::SERVER_UNREGISTER, "Unregister Server",
                   "Unregister server from tree", ActionCategory::kServer));
    
    add(makeAction(ActionIds::SERVER_PROPERTIES, "Properties...",
                   "Server registration info", ActionCategory::kServer));
    
    add(makeAction(ActionIds::SERVER_START, "Start Server",
                   "Start the database server", ActionCategory::kServer));
    
    add(makeAction(ActionIds::SERVER_STOP, "Stop Server",
                   "Stop the database server", ActionCategory::kServer));
}

// ============================================================================
// Schema Actions
// ============================================================================
void ActionCatalog::registerSchemaActions() {
    int order = 0;
    auto add = [&](const ActionDefinition& a) {
        ActionDefinition def = a;
        def.sort_order = order++;
        registerAction(def);
    };
    
    add(makeAction(ActionIds::SCHEMA_CREATE, "Create Schema...",
                   "Create new schema", ActionCategory::kSchema));
    
    add(makeAction(ActionIds::SCHEMA_DROP, "Drop Schema...",
                   "Drop schema", ActionCategory::kSchema));
    
    add(makeAction(ActionIds::SCHEMA_PROPERTIES, "Properties...",
                   "Schema properties", ActionCategory::kSchema));
}

// ============================================================================
// Query Actions
// ============================================================================
void ActionCatalog::registerQueryActions() {
    int order = 0;
    auto add = [&](const ActionDefinition& a) {
        ActionDefinition def = a;
        def.sort_order = order++;
        def.requirements.needs_connection = true;
        registerAction(def);
    };
    
    add(makeAction(ActionIds::QUERY_EXECUTE, "Execute",
                   "Execute SQL query", ActionCategory::kQuery,
                   "query-execute", "F9"));
    
    add(makeAction(ActionIds::QUERY_EXECUTE_SELECTION, "Execute Selection",
                   "Execute selected SQL only", ActionCategory::kQuery,
                   "query-execute-selection", "Ctrl+F9"));
    
    add(makeAction(ActionIds::QUERY_EXECUTE_SCRIPT, "Execute Script",
                   "Execute entire SQL script", ActionCategory::kQuery));
    
    add(makeAction(ActionIds::QUERY_EXPLAIN, "Explain",
                   "Show query execution plan", ActionCategory::kQuery,
                   "query-explain"));
    
    add(makeAction(ActionIds::QUERY_EXPLAIN_ANALYZE, "Explain Analyze",
                   "Execute and show actual plan", ActionCategory::kQuery));
    
    add(makeAction(ActionIds::QUERY_STOP, "Stop",
                   "Stop executing query", ActionCategory::kQuery,
                   "query-stop"));
    
    add(makeAction(ActionIds::QUERY_FORMAT, "Format SQL",
                   "Format SQL query", ActionCategory::kQuery,
                   "query-format"));
    
    add(makeAction(ActionIds::QUERY_COMMENT, "Comment",
                   "Comment selected lines", ActionCategory::kQuery));
    
    add(makeAction(ActionIds::QUERY_UNCOMMENT, "Uncomment",
                   "Uncomment selected lines", ActionCategory::kQuery));
    
    add(makeAction(ActionIds::QUERY_TOGGLE_COMMENT, "Toggle Comment",
                   "Toggle comment on selected lines", ActionCategory::kQuery,
                   nullptr, "Ctrl+/"));
    
    add(makeAction(ActionIds::QUERY_UPPERCASE, "Uppercase",
                   "Convert to uppercase", ActionCategory::kQuery));
    
    add(makeAction(ActionIds::QUERY_LOWERCASE, "Lowercase",
                   "Convert to lowercase", ActionCategory::kQuery));
}

// ============================================================================
// Transaction Actions
// ============================================================================
void ActionCatalog::registerTransactionActions() {
    int order = 0;
    auto add = [&](const ActionDefinition& a) {
        ActionDefinition def = a;
        def.sort_order = order++;
        def.requirements.needs_connection = true;
        registerAction(def);
    };
    
    add(makeAction(ActionIds::TX_START, "Start Transaction",
                   "Begin a new transaction", ActionCategory::kTransaction,
                   "transaction-start"));
    
    add(makeAction(ActionIds::TX_COMMIT, "Commit",
                   "Commit current transaction", ActionCategory::kTransaction,
                   "transaction-commit"));
    
    add(makeAction(ActionIds::TX_ROLLBACK, "Rollback",
                   "Rollback current transaction", ActionCategory::kTransaction,
                   "transaction-rollback"));
    
    add(makeAction(ActionIds::TX_SAVEPOINT, "Savepoint...",
                   "Create savepoint", ActionCategory::kTransaction));
    
    add(makeAction(ActionIds::TX_ROLLBACK_TO, "Rollback to Savepoint...",
                   "Rollback to savepoint", ActionCategory::kTransaction));
    
    add(makeAction(ActionIds::TX_AUTOCOMMIT, "Auto Commit",
                   "Toggle auto-commit mode", ActionCategory::kTransaction,
                   "transaction-autocommit"));
    
    add(makeAction(ActionIds::TX_ISOLATION, "Isolation Level...",
                   "Set transaction isolation level", ActionCategory::kTransaction));
    
    add(makeAction(ActionIds::TX_READONLY, "Read Only",
                   "Set transaction read only", ActionCategory::kTransaction));
}

// ============================================================================
// Script Actions
// ============================================================================
void ActionCatalog::registerScriptActions() {
    int order = 0;
    auto add = [&](const ActionDefinition& a) {
        ActionDefinition def = a;
        def.sort_order = order++;
        def.requirements.needs_connection = true;
        registerAction(def);
    };
    
    add(makeAction(ActionIds::SCRIPT_RUN, "Run Script",
                   "Run SQL script", ActionCategory::kScript));
    
    add(makeAction(ActionIds::SCRIPT_DEBUG, "Debug",
                   "Start debugging", ActionCategory::kScript));
    
    add(makeAction(ActionIds::SCRIPT_STEP_INTO, "Step Into",
                   "Step into function/procedure", ActionCategory::kScript,
                   nullptr, "F11"));
    
    add(makeAction(ActionIds::SCRIPT_STEP_OVER, "Step Over",
                   "Step over function/procedure", ActionCategory::kScript,
                   nullptr, "F10"));
    
    add(makeAction(ActionIds::SCRIPT_STEP_OUT, "Step Out",
                   "Step out of function/procedure", ActionCategory::kScript,
                   nullptr, "Shift+F11"));
    
    add(makeAction(ActionIds::SCRIPT_CONTINUE, "Continue",
                   "Continue execution", ActionCategory::kScript,
                   nullptr, "F5"));
    
    add(makeAction(ActionIds::SCRIPT_BREAKPOINT, "Toggle Breakpoint",
                   "Toggle breakpoint at cursor", ActionCategory::kScript,
                   nullptr, "F9"));
    
    add(makeAction(ActionIds::SCRIPT_STOP_DEBUG, "Stop Debugging",
                   "Stop debugging", ActionCategory::kScript,
                   nullptr, "Shift+F5"));
}

// ============================================================================
// Table Actions
// ============================================================================
void ActionCatalog::registerTableActions() {
    int order = 0;
    auto add = [&](const ActionDefinition& a) {
        ActionDefinition def = a;
        def.sort_order = order++;
        def.requirements.needs_connection = true;
        registerAction(def);
    };
    
    add(makeAction(ActionIds::TABLE_NEW, "New Table...",
                   "Create new table", ActionCategory::kTable,
                   "table-new"));
    
    add(makeAction(ActionIds::TABLE_CREATE, "Create Table...",
                   "Create table from definition", ActionCategory::kTable));
    
    add(makeAction(ActionIds::TABLE_ALTER, "Alter Table...",
                   "Modify table structure", ActionCategory::kTable,
                   "table-edit"));
    
    add(makeAction(ActionIds::TABLE_DROP, "Drop Table...",
                   "Delete table", ActionCategory::kTable,
                   "table-delete"));
    
    add(makeAction(ActionIds::TABLE_RECREATE, "Recreate Table...",
                   "Drop and recreate table", ActionCategory::kTable));
    
    add(makeAction(ActionIds::TABLE_TRUNCATE, "Truncate Table...",
                   "Remove all data from table", ActionCategory::kTable));
    
    add(makeAction(ActionIds::TABLE_RENAME, "Rename Table...",
                   "Rename table", ActionCategory::kTable));
    
    add(makeAction(ActionIds::TABLE_COPY, "Copy Table...",
                   "Copy table to another database", ActionCategory::kTable));
    
    add(makeAction(ActionIds::TABLE_DUPLICATE, "Duplicate Table...",
                   "Duplicate table in same database", ActionCategory::kTable));
    
    add(makeAction(ActionIds::TABLE_PROPERTIES, "Properties...",
                   "Table properties", ActionCategory::kTable));
    
    add(makeAction(ActionIds::TABLE_CONSTRAINTS, "Constraints...",
                   "Manage table constraints", ActionCategory::kTable));
    
    add(makeAction(ActionIds::TABLE_INDEXES, "Indexes...",
                   "Manage table indexes", ActionCategory::kTable));
    
    add(makeAction(ActionIds::TABLE_TRIGGERS, "Triggers...",
                   "Manage table triggers", ActionCategory::kTable));
    
    add(makeAction(ActionIds::TABLE_DEPENDENCIES, "Dependencies...",
                   "Show table dependencies", ActionCategory::kTable));
    
    add(makeAction(ActionIds::TABLE_PERMISSIONS, "Permissions...",
                   "Manage table permissions", ActionCategory::kTable));
    
    add(makeAction(ActionIds::TABLE_STATISTICS, "Statistics...",
                   "Table statistics", ActionCategory::kTable));
    
    add(makeAction(ActionIds::TABLE_EXPORT_DATA, "Export Data...",
                   "Export table data", ActionCategory::kTable));
    
    add(makeAction(ActionIds::TABLE_IMPORT_DATA, "Import Data...",
                   "Import data into table", ActionCategory::kTable));
    
    add(makeAction(ActionIds::TABLE_GENERATE_DDL, "Generate DDL",
                   "Generate CREATE TABLE script", ActionCategory::kTable));
    
    add(makeAction(ActionIds::TABLE_GENERATE_SELECT, "Generate SELECT",
                   "Generate SELECT statement", ActionCategory::kTable));
    
    add(makeAction(ActionIds::TABLE_GENERATE_INSERT, "Generate INSERT",
                   "Generate INSERT statement", ActionCategory::kTable));
    
    add(makeAction(ActionIds::TABLE_GENERATE_UPDATE, "Generate UPDATE",
                   "Generate UPDATE statement", ActionCategory::kTable));
    
    add(makeAction(ActionIds::TABLE_GENERATE_DELETE, "Generate DELETE",
                   "Generate DELETE statement", ActionCategory::kTable));
    
    add(makeAction(ActionIds::TABLE_BROWSE_DATA, "Browse Data",
                   "Open table data editor", ActionCategory::kTable));
}

// ============================================================================
// View Actions
// ============================================================================
void ActionCatalog::registerDbViewActions() {
    int order = 0;
    auto add = [&](const ActionDefinition& a) {
        ActionDefinition def = a;
        def.sort_order = order++;
        def.requirements.needs_connection = true;
        registerAction(def);
    };
    
    add(makeAction(ActionIds::VIEW_NEW, "New View...",
                   "Create new view", ActionCategory::kDbView,
                   "view-new"));
    
    add(makeAction(ActionIds::VIEW_CREATE, "Create View...",
                   "Create view from definition", ActionCategory::kDbView));
    
    add(makeAction(ActionIds::VIEW_ALTER, "Alter View...",
                   "Modify view definition", ActionCategory::kDbView));
    
    add(makeAction(ActionIds::VIEW_DROP, "Drop View...",
                   "Delete view", ActionCategory::kDbView));
    
    add(makeAction(ActionIds::VIEW_REFRESH, "Refresh View",
                   "Refresh view definition", ActionCategory::kDbView));
    
    add(makeAction(ActionIds::VIEW_RECREATE, "Recreate View...",
                   "Drop and recreate view", ActionCategory::kDbView));
    
    add(makeAction(ActionIds::VIEW_RENAME, "Rename View...",
                   "Rename view", ActionCategory::kDbView));
    
    add(makeAction(ActionIds::VIEW_COPY, "Copy View...",
                   "Copy view to another database", ActionCategory::kDbView));
    
    add(makeAction(ActionIds::VIEW_PROPERTIES, "Properties...",
                   "View properties", ActionCategory::kDbView));
    
    add(makeAction(ActionIds::VIEW_DEPENDENCIES, "Dependencies...",
                   "Show view dependencies", ActionCategory::kDbView));
    
    add(makeAction(ActionIds::VIEW_PERMISSIONS, "Permissions...",
                   "Manage view permissions", ActionCategory::kDbView));
    
    add(makeAction(ActionIds::VIEW_GENERATE_DDL, "Generate DDL",
                   "Generate CREATE VIEW script", ActionCategory::kDbView));
    
    add(makeAction(ActionIds::VIEW_BROWSE_DATA, "Browse Data",
                   "Open view data editor", ActionCategory::kDbView));
}

// ============================================================================
// Procedure Actions
// ============================================================================
void ActionCatalog::registerProcedureActions() {
    int order = 0;
    auto add = [&](const ActionDefinition& a) {
        ActionDefinition def = a;
        def.sort_order = order++;
        def.requirements.needs_connection = true;
        registerAction(def);
    };
    
    add(makeAction(ActionIds::PROCEDURE_NEW, "New Procedure...",
                   "Create new procedure", ActionCategory::kProcedure,
                   "procedure-new"));
    
    add(makeAction(ActionIds::PROCEDURE_CREATE, "Create Procedure...",
                   "Create procedure from definition", ActionCategory::kProcedure));
    
    add(makeAction(ActionIds::PROCEDURE_ALTER, "Alter Procedure...",
                   "Modify procedure", ActionCategory::kProcedure));
    
    add(makeAction(ActionIds::PROCEDURE_DROP, "Drop Procedure...",
                   "Delete procedure", ActionCategory::kProcedure));
    
    add(makeAction(ActionIds::PROCEDURE_RECREATE, "Recreate Procedure...",
                   "Drop and recreate procedure", ActionCategory::kProcedure));
    
    add(makeAction(ActionIds::PROCEDURE_RENAME, "Rename Procedure...",
                   "Rename procedure", ActionCategory::kProcedure));
    
    add(makeAction(ActionIds::PROCEDURE_COPY, "Copy Procedure...",
                   "Copy procedure to another database", ActionCategory::kProcedure));
    
    add(makeAction(ActionIds::PROCEDURE_PROPERTIES, "Properties...",
                   "Procedure properties", ActionCategory::kProcedure));
    
    add(makeAction(ActionIds::PROCEDURE_EXECUTE, "Execute...",
                   "Execute procedure", ActionCategory::kProcedure));
    
    add(makeAction(ActionIds::PROCEDURE_DEBUG, "Debug...",
                   "Debug procedure", ActionCategory::kProcedure));
    
    add(makeAction(ActionIds::PROCEDURE_DEPENDENCIES, "Dependencies...",
                   "Show procedure dependencies", ActionCategory::kProcedure));
    
    add(makeAction(ActionIds::PROCEDURE_PERMISSIONS, "Permissions...",
                   "Manage procedure permissions", ActionCategory::kProcedure));
    
    add(makeAction(ActionIds::PROCEDURE_GENERATE_DDL, "Generate DDL",
                   "Generate CREATE PROCEDURE script", ActionCategory::kProcedure));
    
    add(makeAction(ActionIds::PROCEDURE_GENERATE_CALL, "Generate CALL",
                   "Generate CALL statement", ActionCategory::kProcedure));
}

// ============================================================================
// Function Actions
// ============================================================================
void ActionCatalog::registerFunctionActions() {
    int order = 0;
    auto add = [&](const ActionDefinition& a) {
        ActionDefinition def = a;
        def.sort_order = order++;
        def.requirements.needs_connection = true;
        registerAction(def);
    };
    
    add(makeAction(ActionIds::FUNCTION_NEW, "New Function...",
                   "Create new function", ActionCategory::kFunction,
                   "function-new"));
    
    add(makeAction(ActionIds::FUNCTION_CREATE, "Create Function...",
                   "Create function from definition", ActionCategory::kFunction));
    
    add(makeAction(ActionIds::FUNCTION_ALTER, "Alter Function...",
                   "Modify function", ActionCategory::kFunction));
    
    add(makeAction(ActionIds::FUNCTION_DROP, "Drop Function...",
                   "Delete function", ActionCategory::kFunction));
    
    add(makeAction(ActionIds::FUNCTION_RECREATE, "Recreate Function...",
                   "Drop and recreate function", ActionCategory::kFunction));
    
    add(makeAction(ActionIds::FUNCTION_RENAME, "Rename Function...",
                   "Rename function", ActionCategory::kFunction));
    
    add(makeAction(ActionIds::FUNCTION_COPY, "Copy Function...",
                   "Copy function to another database", ActionCategory::kFunction));
    
    add(makeAction(ActionIds::FUNCTION_PROPERTIES, "Properties...",
                   "Function properties", ActionCategory::kFunction));
    
    add(makeAction(ActionIds::FUNCTION_EXECUTE, "Execute...",
                   "Execute function", ActionCategory::kFunction));
    
    add(makeAction(ActionIds::FUNCTION_DEBUG, "Debug...",
                   "Debug function", ActionCategory::kFunction));
    
    add(makeAction(ActionIds::FUNCTION_DEPENDENCIES, "Dependencies...",
                   "Show function dependencies", ActionCategory::kFunction));
    
    add(makeAction(ActionIds::FUNCTION_PERMISSIONS, "Permissions...",
                   "Manage function permissions", ActionCategory::kFunction));
    
    add(makeAction(ActionIds::FUNCTION_GENERATE_DDL, "Generate DDL",
                   "Generate CREATE FUNCTION script", ActionCategory::kFunction));
    
    add(makeAction(ActionIds::FUNCTION_GENERATE_CALL, "Generate CALL",
                   "Generate function call statement", ActionCategory::kFunction));
}

// ============================================================================
// Trigger Actions
// ============================================================================
void ActionCatalog::registerTriggerActions() {
    int order = 0;
    auto add = [&](const ActionDefinition& a) {
        ActionDefinition def = a;
        def.sort_order = order++;
        def.requirements.needs_connection = true;
        registerAction(def);
    };
    
    add(makeAction(ActionIds::TRIGGER_NEW, "New Trigger...",
                   "Create new trigger", ActionCategory::kTrigger,
                   "trigger-new"));
    
    add(makeAction(ActionIds::TRIGGER_CREATE, "Create Trigger...",
                   "Create trigger from definition", ActionCategory::kTrigger));
    
    add(makeAction(ActionIds::TRIGGER_ALTER, "Alter Trigger...",
                   "Modify trigger", ActionCategory::kTrigger));
    
    add(makeAction(ActionIds::TRIGGER_DROP, "Drop Trigger...",
                   "Delete trigger", ActionCategory::kTrigger));
    
    add(makeAction(ActionIds::TRIGGER_ENABLE, "Enable",
                   "Enable trigger", ActionCategory::kTrigger));
    
    add(makeAction(ActionIds::TRIGGER_DISABLE, "Disable",
                   "Disable trigger", ActionCategory::kTrigger));
    
    add(makeAction(ActionIds::TRIGGER_RECREATE, "Recreate Trigger...",
                   "Drop and recreate trigger", ActionCategory::kTrigger));
    
    add(makeAction(ActionIds::TRIGGER_RENAME, "Rename Trigger...",
                   "Rename trigger", ActionCategory::kTrigger));
    
    add(makeAction(ActionIds::TRIGGER_COPY, "Copy Trigger...",
                   "Copy trigger to another table", ActionCategory::kTrigger));
    
    add(makeAction(ActionIds::TRIGGER_PROPERTIES, "Properties...",
                   "Trigger properties", ActionCategory::kTrigger));
    
    add(makeAction(ActionIds::TRIGGER_GENERATE_DDL, "Generate DDL",
                   "Generate CREATE TRIGGER script", ActionCategory::kTrigger));
}

// ============================================================================
// Index Actions
// ============================================================================
void ActionCatalog::registerIndexActions() {
    int order = 0;
    auto add = [&](const ActionDefinition& a) {
        ActionDefinition def = a;
        def.sort_order = order++;
        def.requirements.needs_connection = true;
        registerAction(def);
    };
    
    add(makeAction(ActionIds::INDEX_NEW, "New Index...",
                   "Create new index", ActionCategory::kIndex,
                   "index-new"));
    
    add(makeAction(ActionIds::INDEX_CREATE, "Create Index...",
                   "Create index from definition", ActionCategory::kIndex));
    
    add(makeAction(ActionIds::INDEX_ALTER, "Alter Index...",
                   "Modify index", ActionCategory::kIndex));
    
    add(makeAction(ActionIds::INDEX_DROP, "Drop Index...",
                   "Delete index", ActionCategory::kIndex));
    
    add(makeAction(ActionIds::INDEX_RECREATE, "Recreate Index...",
                   "Drop and recreate index", ActionCategory::kIndex));
    
    add(makeAction(ActionIds::INDEX_RENAME, "Rename Index...",
                   "Rename index", ActionCategory::kIndex));
    
    add(makeAction(ActionIds::INDEX_REBUILD, "Rebuild Index...",
                   "Rebuild index", ActionCategory::kIndex));
    
    add(makeAction(ActionIds::INDEX_STATISTICS, "Statistics...",
                   "Index statistics", ActionCategory::kIndex));
    
    add(makeAction(ActionIds::INDEX_PROPERTIES, "Properties...",
                   "Index properties", ActionCategory::kIndex));
    
    add(makeAction(ActionIds::INDEX_GENERATE_DDL, "Generate DDL",
                   "Generate CREATE INDEX script", ActionCategory::kIndex));
}

// ============================================================================
// Constraint Actions
// ============================================================================
void ActionCatalog::registerConstraintActions() {
    int order = 0;
    auto add = [&](const ActionDefinition& a) {
        ActionDefinition def = a;
        def.sort_order = order++;
        def.requirements.needs_connection = true;
        registerAction(def);
    };
    
    add(makeAction(ActionIds::CONSTRAINT_NEW, "New Constraint...",
                   "Create new constraint", ActionCategory::kConstraint));
    
    add(makeAction(ActionIds::CONSTRAINT_CREATE, "Create Constraint...",
                   "Create constraint from definition", ActionCategory::kConstraint));
    
    add(makeAction(ActionIds::CONSTRAINT_ALTER, "Alter Constraint...",
                   "Modify constraint", ActionCategory::kConstraint));
    
    add(makeAction(ActionIds::CONSTRAINT_DROP, "Drop Constraint...",
                   "Delete constraint", ActionCategory::kConstraint));
    
    add(makeAction(ActionIds::CONSTRAINT_ENABLE, "Enable",
                   "Enable constraint", ActionCategory::kConstraint));
    
    add(makeAction(ActionIds::CONSTRAINT_DISABLE, "Disable",
                   "Disable constraint", ActionCategory::kConstraint));
    
    add(makeAction(ActionIds::CONSTRAINT_PROPERTIES, "Properties...",
                   "Constraint properties", ActionCategory::kConstraint));
    
    add(makeAction(ActionIds::CONSTRAINT_GENERATE_DDL, "Generate DDL",
                   "Generate ALTER TABLE ADD CONSTRAINT script", ActionCategory::kConstraint));
}

// ============================================================================
// Domain Actions
// ============================================================================
void ActionCatalog::registerDomainActions() {
    int order = 0;
    auto add = [&](const ActionDefinition& a) {
        ActionDefinition def = a;
        def.sort_order = order++;
        def.requirements.needs_connection = true;
        registerAction(def);
    };
    
    add(makeAction(ActionIds::DOMAIN_NEW, "New Domain...",
                   "Create new domain", ActionCategory::kDomain));
    
    add(makeAction(ActionIds::DOMAIN_CREATE, "Create Domain...",
                   "Create domain from definition", ActionCategory::kDomain));
    
    add(makeAction(ActionIds::DOMAIN_ALTER, "Alter Domain...",
                   "Modify domain", ActionCategory::kDomain));
    
    add(makeAction(ActionIds::DOMAIN_DROP, "Drop Domain...",
                   "Delete domain", ActionCategory::kDomain));
    
    add(makeAction(ActionIds::DOMAIN_RECREATE, "Recreate Domain...",
                   "Drop and recreate domain", ActionCategory::kDomain));
    
    add(makeAction(ActionIds::DOMAIN_RENAME, "Rename Domain...",
                   "Rename domain", ActionCategory::kDomain));
    
    add(makeAction(ActionIds::DOMAIN_COPY, "Copy Domain...",
                   "Copy domain to another database", ActionCategory::kDomain));
    
    add(makeAction(ActionIds::DOMAIN_PROPERTIES, "Properties...",
                   "Domain properties", ActionCategory::kDomain));
    
    add(makeAction(ActionIds::DOMAIN_DEPENDENCIES, "Dependencies...",
                   "Show domain dependencies", ActionCategory::kDomain));
    
    add(makeAction(ActionIds::DOMAIN_GENERATE_DDL, "Generate DDL",
                   "Generate CREATE DOMAIN script", ActionCategory::kDomain));
}

// ============================================================================
// Sequence Actions
// ============================================================================
void ActionCatalog::registerSequenceActions() {
    int order = 0;
    auto add = [&](const ActionDefinition& a) {
        ActionDefinition def = a;
        def.sort_order = order++;
        def.requirements.needs_connection = true;
        registerAction(def);
    };
    
    add(makeAction(ActionIds::SEQUENCE_NEW, "New Sequence...",
                   "Create new sequence", ActionCategory::kSequence));
    
    add(makeAction(ActionIds::SEQUENCE_CREATE, "Create Sequence...",
                   "Create sequence from definition", ActionCategory::kSequence));
    
    add(makeAction(ActionIds::SEQUENCE_ALTER, "Alter Sequence...",
                   "Modify sequence", ActionCategory::kSequence));
    
    add(makeAction(ActionIds::SEQUENCE_DROP, "Drop Sequence...",
                   "Delete sequence", ActionCategory::kSequence));
    
    add(makeAction(ActionIds::SEQUENCE_RECREATE, "Recreate Sequence...",
                   "Drop and recreate sequence", ActionCategory::kSequence));
    
    add(makeAction(ActionIds::SEQUENCE_RENAME, "Rename Sequence...",
                   "Rename sequence", ActionCategory::kSequence));
    
    add(makeAction(ActionIds::SEQUENCE_RESET, "Reset",
                   "Reset sequence to start value", ActionCategory::kSequence));
    
    add(makeAction(ActionIds::SEQUENCE_SET_VALUE, "Set Value...",
                   "Set current sequence value", ActionCategory::kSequence));
    
    add(makeAction(ActionIds::SEQUENCE_PROPERTIES, "Properties...",
                   "Sequence properties", ActionCategory::kSequence));
    
    add(makeAction(ActionIds::SEQUENCE_GENERATE_DDL, "Generate DDL",
                   "Generate CREATE SEQUENCE script", ActionCategory::kSequence));
    
    add(makeAction(ActionIds::SEQUENCE_GENERATE_NEXT, "Generate NEXTVAL",
                   "Generate NEXTVAL expression", ActionCategory::kSequence));
}

// ============================================================================
// Package Actions
// ============================================================================
void ActionCatalog::registerPackageActions() {
    int order = 0;
    auto add = [&](const ActionDefinition& a) {
        ActionDefinition def = a;
        def.sort_order = order++;
        def.requirements.needs_connection = true;
        registerAction(def);
    };
    
    add(makeAction(ActionIds::PACKAGE_NEW, "New Package...",
                   "Create new package", ActionCategory::kPackage));
    
    add(makeAction(ActionIds::PACKAGE_CREATE, "Create Package...",
                   "Create package from definition", ActionCategory::kPackage));
    
    add(makeAction(ActionIds::PACKAGE_ALTER, "Alter Package...",
                   "Modify package", ActionCategory::kPackage));
    
    add(makeAction(ActionIds::PACKAGE_DROP, "Drop Package...",
                   "Delete package", ActionCategory::kPackage));
    
    add(makeAction(ActionIds::PACKAGE_RECREATE, "Recreate Package...",
                   "Drop and recreate package", ActionCategory::kPackage));
    
    add(makeAction(ActionIds::PACKAGE_RENAME, "Rename Package...",
                   "Rename package", ActionCategory::kPackage));
    
    add(makeAction(ActionIds::PACKAGE_COPY, "Copy Package...",
                   "Copy package to another database", ActionCategory::kPackage));
    
    add(makeAction(ActionIds::PACKAGE_PROPERTIES, "Properties...",
                   "Package properties", ActionCategory::kPackage));
    
    add(makeAction(ActionIds::PACKAGE_BODY_EDIT, "Edit Body...",
                   "Edit package body", ActionCategory::kPackage));
    
    add(makeAction(ActionIds::PACKAGE_HEADER_EDIT, "Edit Header...",
                   "Edit package header/spec", ActionCategory::kPackage));
    
    add(makeAction(ActionIds::PACKAGE_DEPENDENCIES, "Dependencies...",
                   "Show package dependencies", ActionCategory::kPackage));
    
    add(makeAction(ActionIds::PACKAGE_PERMISSIONS, "Permissions...",
                   "Manage package permissions", ActionCategory::kPackage));
    
    add(makeAction(ActionIds::PACKAGE_GENERATE_DDL, "Generate DDL",
                   "Generate CREATE PACKAGE script", ActionCategory::kPackage));
}

// ============================================================================
// Exception Actions
// ============================================================================
void ActionCatalog::registerExceptionActions() {
    int order = 0;
    auto add = [&](const ActionDefinition& a) {
        ActionDefinition def = a;
        def.sort_order = order++;
        def.requirements.needs_connection = true;
        registerAction(def);
    };
    
    add(makeAction(ActionIds::EXCEPTION_NEW, "New Exception...",
                   "Create new exception", ActionCategory::kException));
    
    add(makeAction(ActionIds::EXCEPTION_CREATE, "Create Exception...",
                   "Create exception from definition", ActionCategory::kException));
    
    add(makeAction(ActionIds::EXCEPTION_ALTER, "Alter Exception...",
                   "Modify exception", ActionCategory::kException));
    
    add(makeAction(ActionIds::EXCEPTION_DROP, "Drop Exception...",
                   "Delete exception", ActionCategory::kException));
    
    add(makeAction(ActionIds::EXCEPTION_RECREATE, "Recreate Exception...",
                   "Drop and recreate exception", ActionCategory::kException));
    
    add(makeAction(ActionIds::EXCEPTION_RENAME, "Rename Exception...",
                   "Rename exception", ActionCategory::kException));
    
    add(makeAction(ActionIds::EXCEPTION_PROPERTIES, "Properties...",
                   "Exception properties", ActionCategory::kException));
    
    add(makeAction(ActionIds::EXCEPTION_GENERATE_DDL, "Generate DDL",
                   "Generate CREATE EXCEPTION script", ActionCategory::kException));
}

// ============================================================================
// Role Actions
// ============================================================================
void ActionCatalog::registerRoleActions() {
    int order = 0;
    auto add = [&](const ActionDefinition& a) {
        ActionDefinition def = a;
        def.sort_order = order++;
        def.requirements.needs_connection = true;
        registerAction(def);
    };
    
    add(makeAction(ActionIds::ROLE_NEW, "New Role...",
                   "Create new role", ActionCategory::kRole));
    
    add(makeAction(ActionIds::ROLE_CREATE, "Create Role...",
                   "Create role from definition", ActionCategory::kRole));
    
    add(makeAction(ActionIds::ROLE_ALTER, "Alter Role...",
                   "Modify role", ActionCategory::kRole));
    
    add(makeAction(ActionIds::ROLE_DROP, "Drop Role...",
                   "Delete role", ActionCategory::kRole));
    
    add(makeAction(ActionIds::ROLE_RECREATE, "Recreate Role...",
                   "Drop and recreate role", ActionCategory::kRole));
    
    add(makeAction(ActionIds::ROLE_RENAME, "Rename Role...",
                   "Rename role", ActionCategory::kRole));
    
    add(makeAction(ActionIds::ROLE_COPY, "Copy Role...",
                   "Copy role to another database", ActionCategory::kRole));
    
    add(makeAction(ActionIds::ROLE_GRANT, "Grant...",
                   "Grant role to user", ActionCategory::kRole));
    
    add(makeAction(ActionIds::ROLE_REVOKE, "Revoke...",
                   "Revoke role from user", ActionCategory::kRole));
    
    add(makeAction(ActionIds::ROLE_PROPERTIES, "Properties...",
                   "Role properties", ActionCategory::kRole));
    
    add(makeAction(ActionIds::ROLE_MEMBERS, "Members...",
                   "Manage role members", ActionCategory::kRole));
    
    add(makeAction(ActionIds::ROLE_PERMISSIONS, "Permissions...",
                   "Manage role permissions", ActionCategory::kRole));
    
    add(makeAction(ActionIds::ROLE_GENERATE_DDL, "Generate DDL",
                   "Generate CREATE ROLE script", ActionCategory::kRole));
}

// ============================================================================
// User Actions
// ============================================================================
void ActionCatalog::registerUserActions() {
    int order = 0;
    auto add = [&](const ActionDefinition& a) {
        ActionDefinition def = a;
        def.sort_order = order++;
        def.requirements.needs_connection = true;
        registerAction(def);
    };
    
    add(makeAction(ActionIds::USER_NEW, "New User...",
                   "Create new user", ActionCategory::kUser));
    
    add(makeAction(ActionIds::USER_CREATE, "Create User...",
                   "Create user from definition", ActionCategory::kUser));
    
    add(makeAction(ActionIds::USER_ALTER, "Alter User...",
                   "Modify user", ActionCategory::kUser));
    
    add(makeAction(ActionIds::USER_DROP, "Drop User...",
                   "Delete user", ActionCategory::kUser));
    
    add(makeAction(ActionIds::USER_RECREATE, "Recreate User...",
                   "Drop and recreate user", ActionCategory::kUser));
    
    add(makeAction(ActionIds::USER_RENAME, "Rename User...",
                   "Rename user", ActionCategory::kUser));
    
    add(makeAction(ActionIds::USER_COPY, "Copy User...",
                   "Copy user to another database", ActionCategory::kUser));
    
    add(makeAction(ActionIds::USER_GRANT, "Grant...",
                   "Grant privileges to user", ActionCategory::kUser));
    
    add(makeAction(ActionIds::USER_REVOKE, "Revoke...",
                   "Revoke privileges from user", ActionCategory::kUser));
    
    add(makeAction(ActionIds::USER_CHANGE_PASSWORD, "Change Password...",
                   "Change user password", ActionCategory::kUser));
    
    add(makeAction(ActionIds::USER_PROPERTIES, "Properties...",
                   "User properties", ActionCategory::kUser));
    
    add(makeAction(ActionIds::USER_ROLES, "Roles...",
                   "Manage user roles", ActionCategory::kUser));
    
    add(makeAction(ActionIds::USER_PERMISSIONS, "Permissions...",
                   "Manage user permissions", ActionCategory::kUser));
    
    add(makeAction(ActionIds::USER_GENERATE_DDL, "Generate DDL",
                   "Generate CREATE USER script", ActionCategory::kUser));
}

// ============================================================================
// Group Actions
// ============================================================================
void ActionCatalog::registerGroupActions() {
    int order = 0;
    auto add = [&](const ActionDefinition& a) {
        ActionDefinition def = a;
        def.sort_order = order++;
        def.requirements.needs_connection = true;
        registerAction(def);
    };
    
    add(makeAction(ActionIds::GROUP_NEW, "New Group...",
                   "Create new group", ActionCategory::kGroup));
    
    add(makeAction(ActionIds::GROUP_CREATE, "Create Group...",
                   "Create group from definition", ActionCategory::kGroup));
    
    add(makeAction(ActionIds::GROUP_ALTER, "Alter Group...",
                   "Modify group", ActionCategory::kGroup));
    
    add(makeAction(ActionIds::GROUP_DROP, "Drop Group...",
                   "Delete group", ActionCategory::kGroup));
    
    add(makeAction(ActionIds::GROUP_RENAME, "Rename Group...",
                   "Rename group", ActionCategory::kGroup));
    
    add(makeAction(ActionIds::GROUP_MEMBERS, "Members...",
                   "Manage group members", ActionCategory::kGroup));
    
    add(makeAction(ActionIds::GROUP_PROPERTIES, "Properties...",
                   "Group properties", ActionCategory::kGroup));
    
    add(makeAction(ActionIds::GROUP_GENERATE_DDL, "Generate DDL",
                   "Generate CREATE GROUP script", ActionCategory::kGroup));
}

// ============================================================================
// Job Actions
// ============================================================================
void ActionCatalog::registerJobActions() {
    int order = 0;
    auto add = [&](const ActionDefinition& a) {
        ActionDefinition def = a;
        def.sort_order = order++;
        def.requirements.needs_connection = true;
        registerAction(def);
    };
    
    add(makeAction(ActionIds::JOB_NEW, "New Job...",
                   "Create new job", ActionCategory::kJob));
    
    add(makeAction(ActionIds::JOB_CREATE, "Create Job...",
                   "Create job from definition", ActionCategory::kJob));
    
    add(makeAction(ActionIds::JOB_ALTER, "Alter Job...",
                   "Modify job", ActionCategory::kJob));
    
    add(makeAction(ActionIds::JOB_DROP, "Drop Job...",
                   "Delete job", ActionCategory::kJob));
    
    add(makeAction(ActionIds::JOB_RUN, "Run Now",
                   "Execute job immediately", ActionCategory::kJob));
    
    add(makeAction(ActionIds::JOB_STOP, "Stop",
                   "Stop running job", ActionCategory::kJob));
    
    add(makeAction(ActionIds::JOB_ENABLE, "Enable",
                   "Enable scheduled job", ActionCategory::kJob));
    
    add(makeAction(ActionIds::JOB_DISABLE, "Disable",
                   "Disable scheduled job", ActionCategory::kJob));
    
    add(makeAction(ActionIds::JOB_HISTORY, "History...",
                   "View job execution history", ActionCategory::kJob));
    
    add(makeAction(ActionIds::JOB_PROPERTIES, "Properties...",
                   "Job properties", ActionCategory::kJob));
    
    add(makeAction(ActionIds::JOB_SCHEDULE, "Schedule...",
                   "Edit job schedule", ActionCategory::kJob));
    
    add(makeAction(ActionIds::JOB_GENERATE_DDL, "Generate DDL",
                   "Generate job definition script", ActionCategory::kJob));
}

// ============================================================================
// Schedule Actions
// ============================================================================
void ActionCatalog::registerScheduleActions() {
    int order = 0;
    auto add = [&](const ActionDefinition& a) {
        ActionDefinition def = a;
        def.sort_order = order++;
        def.requirements.needs_connection = true;
        registerAction(def);
    };
    
    add(makeAction(ActionIds::SCHEDULE_NEW, "New Schedule...",
                   "Create new schedule", ActionCategory::kSchedule));
    
    add(makeAction(ActionIds::SCHEDULE_CREATE, "Create Schedule...",
                   "Create schedule from definition", ActionCategory::kSchedule));
    
    add(makeAction(ActionIds::SCHEDULE_ALTER, "Alter Schedule...",
                   "Modify schedule", ActionCategory::kSchedule));
    
    add(makeAction(ActionIds::SCHEDULE_DROP, "Drop Schedule...",
                   "Delete schedule", ActionCategory::kSchedule));
    
    add(makeAction(ActionIds::SCHEDULE_ENABLE, "Enable",
                   "Enable schedule", ActionCategory::kSchedule));
    
    add(makeAction(ActionIds::SCHEDULE_DISABLE, "Disable",
                   "Disable schedule", ActionCategory::kSchedule));
    
    add(makeAction(ActionIds::SCHEDULE_PROPERTIES, "Properties...",
                   "Schedule properties", ActionCategory::kSchedule));
    
    add(makeAction(ActionIds::SCHEDULE_GENERATE_DDL, "Generate DDL",
                   "Generate CREATE SCHEDULE script", ActionCategory::kSchedule));
}

// ============================================================================
// Other Object Actions (Collation, Charset, Filter, Shadow, Synonym)
// ============================================================================
void ActionCatalog::registerOtherObjectActions() {
    // These have minimal action sets - just create, alter, drop, properties
}

// ============================================================================
// Data Actions
// ============================================================================
void ActionCatalog::registerDataActions() {
    int order = 0;
    auto add = [&](const ActionDefinition& a) {
        ActionDefinition def = a;
        def.sort_order = order++;
        def.requirements.needs_connection = true;
        registerAction(def);
    };
    
    add(makeAction(ActionIds::DATA_INSERT, "Insert Row",
                   "Insert new row", ActionCategory::kData,
                   nullptr, "Insert"));
    
    add(makeAction(ActionIds::DATA_UPDATE, "Update Row",
                   "Update current row", ActionCategory::kData));
    
    add(makeAction(ActionIds::DATA_DELETE, "Delete Row",
                   "Delete current row", ActionCategory::kData,
                   nullptr, "Delete"));
    
    add(makeAction(ActionIds::DATA_REFRESH, "Refresh",
                   "Refresh data from database", ActionCategory::kData,
                   "view-refresh", "F5"));
    
    add(makeAction(ActionIds::DATA_FILTER, "Filter...",
                   "Filter data", ActionCategory::kData));
    
    add(makeAction(ActionIds::DATA_CLEAR_FILTER, "Clear Filter",
                   "Remove filter", ActionCategory::kData));
    
    add(makeAction(ActionIds::DATA_SORT_ASC, "Sort Ascending",
                   "Sort ascending by column", ActionCategory::kData));
    
    add(makeAction(ActionIds::DATA_SORT_DESC, "Sort Descending",
                   "Sort descending by column", ActionCategory::kData));
    
    add(makeAction(ActionIds::DATA_EXPORT, "Export...",
                   "Export data", ActionCategory::kData));
    
    add(makeAction(ActionIds::DATA_IMPORT, "Import...",
                   "Import data", ActionCategory::kData));
    
    add(makeAction(ActionIds::DATA_PRINT, "Print...",
                   "Print data", ActionCategory::kData));
    
    add(makeAction(ActionIds::DATA_COMMIT, "Commit Changes",
                   "Save changes to database", ActionCategory::kData));
    
    add(makeAction(ActionIds::DATA_ROLLBACK, "Rollback Changes",
                   "Discard changes", ActionCategory::kData));
    
    add(makeAction(ActionIds::DATA_FIRST, "First",
                   "Go to first row", ActionCategory::kData));
    
    add(makeAction(ActionIds::DATA_LAST, "Last",
                   "Go to last row", ActionCategory::kData));
    
    add(makeAction(ActionIds::DATA_NEXT, "Next",
                   "Go to next row", ActionCategory::kData));
    
    add(makeAction(ActionIds::DATA_PREV, "Previous",
                   "Go to previous row", ActionCategory::kData));
    
    add(makeAction(ActionIds::DATA_GOTO, "Go to Row...",
                   "Go to specific row", ActionCategory::kData));
}

// ============================================================================
// Import/Export Actions
// ============================================================================
void ActionCatalog::registerImportExportActions() {
    int order = 0;
    auto add = [&](const ActionDefinition& a) {
        ActionDefinition def = a;
        def.sort_order = order++;
        registerAction(def);
    };
    
    // Import
    add(makeAction(ActionIds::IMPORT_SQL, "Import SQL...",
                   "Import SQL script", ActionCategory::kImportExport,
                   "document-import"));
    
    add(makeAction(ActionIds::IMPORT_CSV, "Import CSV...",
                   "Import CSV data", ActionCategory::kImportExport));
    
    add(makeAction(ActionIds::IMPORT_XML, "Import XML...",
                   "Import XML data", ActionCategory::kImportExport));
    
    add(makeAction(ActionIds::IMPORT_JSON, "Import JSON...",
                   "Import JSON data", ActionCategory::kImportExport));
    
    add(makeAction(ActionIds::IMPORT_EXCEL, "Import Excel...",
                   "Import Excel data", ActionCategory::kImportExport));
    
    // Export
    add(makeAction(ActionIds::EXPORT_SQL, "Export SQL...",
                   "Export to SQL script", ActionCategory::kImportExport,
                   "document-export"));
    
    add(makeAction(ActionIds::EXPORT_CSV, "Export CSV...",
                   "Export to CSV", ActionCategory::kImportExport));
    
    add(makeAction(ActionIds::EXPORT_XML, "Export XML...",
                   "Export to XML", ActionCategory::kImportExport));
    
    add(makeAction(ActionIds::EXPORT_JSON, "Export JSON...",
                   "Export to JSON", ActionCategory::kImportExport));
    
    add(makeAction(ActionIds::EXPORT_EXCEL, "Export Excel...",
                   "Export to Excel", ActionCategory::kImportExport));
    
    add(makeAction(ActionIds::EXPORT_HTML, "Export HTML...",
                   "Export to HTML", ActionCategory::kImportExport));
    
    add(makeAction(ActionIds::EXPORT_PDF, "Export PDF...",
                   "Export to PDF", ActionCategory::kImportExport));
}

// ============================================================================
// Tools Actions
// ============================================================================
void ActionCatalog::registerToolsActions() {
    int order = 0;
    auto add = [&](const ActionDefinition& a) {
        ActionDefinition def = a;
        def.sort_order = order++;
        registerAction(def);
    };
    
    add(makeAction(ActionIds::TOOLS_GENERATE_DDL, "Generate DDL...",
                   "Generate DDL script for objects", ActionCategory::kTools,
                   "tools-generate-ddl"));
    
    add(makeAction(ActionIds::TOOLS_GENERATE_DATA, "Generate Test Data...",
                   "Generate test data", ActionCategory::kTools));
    
    add(makeAction(ActionIds::TOOLS_COMPARE_SCHEMA, "Compare Schemas...",
                   "Compare database schemas", ActionCategory::kTools));
    
    add(makeAction(ActionIds::TOOLS_COMPARE_DATA, "Compare Data...",
                   "Compare table data", ActionCategory::kTools));
    
    add(makeAction(ActionIds::TOOLS_MIGRATE, "Database Migration...",
                   "Migrate between databases", ActionCategory::kTools));
    
    add(makeAction(ActionIds::TOOLS_SCRIPT, "Script Console...",
                   "Open script console", ActionCategory::kTools));
    
    add(makeAction(ActionIds::TOOLS_SCHEDULER, "Job Scheduler...",
                   "Manage scheduled jobs", ActionCategory::kTools));
}

// ============================================================================
// Monitor Actions
// ============================================================================
void ActionCatalog::registerMonitorActions() {
    int order = 0;
    auto add = [&](const ActionDefinition& a) {
        ActionDefinition def = a;
        def.sort_order = order++;
        def.requirements.needs_connection = true;
        registerAction(def);
    };
    
    add(makeAction(ActionIds::MONITOR_CONNECTIONS, "Connections",
                   "Monitor database connections", ActionCategory::kMonitor,
                   "tools-monitor"));
    
    add(makeAction(ActionIds::MONITOR_TRANSACTIONS, "Transactions",
                   "Monitor transactions", ActionCategory::kMonitor));
    
    add(makeAction(ActionIds::MONITOR_STATEMENTS, "Active Statements",
                   "Monitor active SQL statements", ActionCategory::kMonitor));
    
    add(makeAction(ActionIds::MONITOR_LOCKS, "Locks",
                   "Monitor database locks", ActionCategory::kMonitor));
    
    add(makeAction(ActionIds::MONITOR_PERFORMANCE, "Performance",
                   "Monitor performance metrics", ActionCategory::kMonitor));
    
    add(makeAction(ActionIds::MONITOR_LOGS, "Logs",
                   "View database logs", ActionCategory::kMonitor));
    
    add(makeAction(ActionIds::MONITOR_ALERTS, "Alerts",
                   "View system alerts", ActionCategory::kMonitor));
}

// ============================================================================
// Maintenance Actions
// ============================================================================
void ActionCatalog::registerMaintenanceActions() {
    int order = 0;
    auto add = [&](const ActionDefinition& a) {
        ActionDefinition def = a;
        def.sort_order = order++;
        def.requirements.needs_connection = true;
        registerAction(def);
    };
    
    add(makeAction(ActionIds::MAINTENANCE_ANALYZE, "Analyze",
                   "Analyze tables and indexes", ActionCategory::kMaintenance));
    
    add(makeAction(ActionIds::MAINTENANCE_VACUUM, "Vacuum",
                   "Vacuum database", ActionCategory::kMaintenance));
    
    add(makeAction(ActionIds::MAINTENANCE_REINDEX, "Reindex",
                   "Rebuild indexes", ActionCategory::kMaintenance));
    
    add(makeAction(ActionIds::MAINTENANCE_CHECK, "Check",
                   "Check database integrity", ActionCategory::kMaintenance));
    
    add(makeAction(ActionIds::MAINTENANCE_REPAIR, "Repair",
                   "Repair database", ActionCategory::kMaintenance));
}

// ============================================================================
// Navigation Actions
// ============================================================================
void ActionCatalog::registerNavigationActions() {
    int order = 0;
    auto add = [&](const ActionDefinition& a) {
        ActionDefinition def = a;
        def.sort_order = order++;
        registerAction(def);
    };
    
    add(makeAction(ActionIds::NAV_BACK, "Back",
                   "Navigate back", ActionCategory::kNavigation,
                   "nav-back", "Alt+Left"));
    
    add(makeAction(ActionIds::NAV_FORWARD, "Forward",
                   "Navigate forward", ActionCategory::kNavigation,
                   "nav-forward", "Alt+Right"));
    
    add(makeAction(ActionIds::NAV_UP, "Up",
                   "Go to parent", ActionCategory::kNavigation,
                   "nav-up", "Alt+Up"));
    
    add(makeAction(ActionIds::NAV_HOME, "Home",
                   "Go to home", ActionCategory::kNavigation,
                   "nav-home", "Alt+Home"));
    
    add(makeAction(ActionIds::NAV_REFRESH, "Refresh",
                   "Refresh view", ActionCategory::kNavigation,
                   "nav-refresh", "F5"));
    
    add(makeAction(ActionIds::NAV_BOOKMARK, "Add Bookmark",
                   "Bookmark current location", ActionCategory::kNavigation));
    
    add(makeAction(ActionIds::NAV_BOOKMARKS, "Bookmarks",
                   "Manage bookmarks", ActionCategory::kNavigation));
}

// ============================================================================
// Search Actions
// ============================================================================
void ActionCatalog::registerSearchActions() {
    int order = 0;
    auto add = [&](const ActionDefinition& a) {
        ActionDefinition def = a;
        def.sort_order = order++;
        registerAction(def);
    };
    
    add(makeAction(ActionIds::SEARCH_FIND, "Find...",
                   "Find in current editor", ActionCategory::kSearch,
                   "edit-find", "Ctrl+F"));
    
    add(makeAction(ActionIds::SEARCH_FIND_NEXT, "Find Next",
                   "Find next occurrence", ActionCategory::kSearch,
                   nullptr, "F3"));
    
    add(makeAction(ActionIds::SEARCH_FIND_PREV, "Find Previous",
                   "Find previous occurrence", ActionCategory::kSearch,
                   nullptr, "Shift+F3"));
    
    add(makeAction(ActionIds::SEARCH_REPLACE, "Replace...",
                   "Find and replace", ActionCategory::kSearch,
                   "edit-find-replace", "Ctrl+H"));
    
    add(makeAction(ActionIds::SEARCH_ADVANCED, "Advanced Search...",
                   "Advanced metadata search", ActionCategory::kSearch,
                   "edit-find"));
    
    add(makeAction(ActionIds::SEARCH_METADATA, "Search Metadata...",
                   "Search database metadata", ActionCategory::kSearch));
    
    add(makeAction(ActionIds::SEARCH_DATA, "Search Data...",
                   "Search table data", ActionCategory::kSearch));
    
    add(makeAction(ActionIds::SEARCH_SQL, "Search in SQL...",
                   "Search in SQL code", ActionCategory::kSearch));
}

// ============================================================================
// Action Groups
// ============================================================================
void ActionCatalog::registerActionGroups() {
    // File operations
    action_groups_["file.basic"] = ActionGroup{
        "file.basic", "File Operations",
        {ActionIds::FILE_NEW_CONNECTION, ActionIds::FILE_OPEN_SQL, 
         ActionIds::FILE_SAVE_SQL, ActionIds::FILE_SAVE_AS}
    };
    
    action_groups_["file.print"] = ActionGroup{
        "file.print", "Print",
        {ActionIds::FILE_PRINT, ActionIds::FILE_PRINT_PREVIEW},
        true
    };
    
    action_groups_["file.exit"] = ActionGroup{
        "file.exit", "Exit",
        {ActionIds::FILE_EXIT},
        false
    };
    
    // Edit operations
    action_groups_["edit.undo"] = ActionGroup{
        "edit.undo", "Undo/Redo",
        {ActionIds::EDIT_UNDO, ActionIds::EDIT_REDO}
    };
    
    action_groups_["edit.clipboard"] = ActionGroup{
        "edit.clipboard", "Clipboard",
        {ActionIds::EDIT_CUT, ActionIds::EDIT_COPY, ActionIds::EDIT_PASTE}
    };
    
    action_groups_["edit.search"] = ActionGroup{
        "edit.search", "Search",
        {ActionIds::EDIT_FIND, ActionIds::EDIT_FIND_REPLACE, 
         ActionIds::EDIT_FIND_NEXT, ActionIds::EDIT_FIND_PREV}
    };
    
    action_groups_["edit.prefs"] = ActionGroup{
        "edit.prefs", "Preferences",
        {ActionIds::EDIT_PREFERENCES},
        false
    };
    
    // Database operations
    action_groups_["db.connect"] = ActionGroup{
        "db.connect", "Connection",
        {ActionIds::DB_CONNECT, ActionIds::DB_DISCONNECT, ActionIds::DB_RECONNECT}
    };
    
    action_groups_["db.manage"] = ActionGroup{
        "db.manage", "Database Management",
        {ActionIds::DB_NEW, ActionIds::DB_REGISTER, ActionIds::DB_UNREGISTER,
         ActionIds::DB_PROPERTIES}
    };
    
    action_groups_["db.backup"] = ActionGroup{
        "db.backup", "Backup & Restore",
        {ActionIds::DB_BACKUP, ActionIds::DB_RESTORE, ActionIds::DB_VERIFY}
    };
    
    // Query operations
    action_groups_["query.execute"] = ActionGroup{
        "query.execute", "Execute",
        {ActionIds::QUERY_EXECUTE, ActionIds::QUERY_EXECUTE_SELECTION, 
         ActionIds::QUERY_STOP}
    };
    
    action_groups_["query.tools"] = ActionGroup{
        "query.tools", "Query Tools",
        {ActionIds::QUERY_EXPLAIN, ActionIds::QUERY_EXPLAIN_ANALYZE, 
         ActionIds::QUERY_FORMAT}
    };
    
    // Transaction operations
    action_groups_["transaction.basic"] = ActionGroup{
        "transaction.basic", "Transaction Control",
        {ActionIds::TX_START, ActionIds::TX_COMMIT, ActionIds::TX_ROLLBACK}
    };
    
    action_groups_["transaction.options"] = ActionGroup{
        "transaction.options", "Transaction Options",
        {ActionIds::TX_AUTOCOMMIT, ActionIds::TX_ISOLATION, ActionIds::TX_READONLY}
    };
    
    // Object creation
    action_groups_["object.new.table"] = ActionGroup{
        "object.new.table", "Table",
        {ActionIds::TABLE_NEW, ActionIds::TABLE_CREATE, ActionIds::TABLE_ALTER, 
         ActionIds::TABLE_DROP}
    };
    
    action_groups_["object.new.view"] = ActionGroup{
        "object.new.view", "View",
        {ActionIds::VIEW_NEW, ActionIds::VIEW_CREATE, ActionIds::VIEW_ALTER, 
         ActionIds::VIEW_DROP}
    };
    
    action_groups_["object.new.procedure"] = ActionGroup{
        "object.new.procedure", "Procedure",
        {ActionIds::PROCEDURE_NEW, ActionIds::PROCEDURE_CREATE, 
         ActionIds::PROCEDURE_ALTER, ActionIds::PROCEDURE_DROP}
    };
    
    action_groups_["object.new.function"] = ActionGroup{
        "object.new.function", "Function",
        {ActionIds::FUNCTION_NEW, ActionIds::FUNCTION_CREATE, 
         ActionIds::FUNCTION_ALTER, ActionIds::FUNCTION_DROP}
    };
    
    action_groups_["object.new.trigger"] = ActionGroup{
        "object.new.trigger", "Trigger",
        {ActionIds::TRIGGER_NEW, ActionIds::TRIGGER_CREATE, 
         ActionIds::TRIGGER_ALTER, ActionIds::TRIGGER_DROP}
    };
    
    // Window operations
    action_groups_["window.basic"] = ActionGroup{
        "window.basic", "Window",
        {ActionIds::WINDOW_NEW, ActionIds::WINDOW_CLOSE, ActionIds::WINDOW_CLOSE_ALL}
    };
    
    action_groups_["window.navigate"] = ActionGroup{
        "window.navigate", "Navigation",
        {ActionIds::WINDOW_NEXT, ActionIds::WINDOW_PREV}
    };
    
    action_groups_["window.arrange"] = ActionGroup{
        "window.arrange", "Arrange",
        {ActionIds::WINDOW_CASCADE, ActionIds::WINDOW_TILE_H, ActionIds::WINDOW_TILE_V}
    };
    
    // Help operations
    action_groups_["help.docs"] = ActionGroup{
        "help.docs", "Documentation",
        {ActionIds::HELP_DOCS, ActionIds::_HELP_CONTEXT, ActionIds::HELP_SHORTCUTS}
    };
    
    action_groups_["help.about"] = ActionGroup{
        "help.about", "About",
        {ActionIds::HELP_ABOUT},
        false
    };
}

// ============================================================================
// Menu Templates
// ============================================================================
void ActionCatalog::registerMenuTemplates() {
    // File menu
    menu_templates_["file"] = MenuTemplate{
        "file", "&File",
        {"file.basic", "file.print", "file.exit"},
        10
    };
    
    // Edit menu
    menu_templates_["edit"] = MenuTemplate{
        "edit", "&Edit",
        {"edit.undo", "edit.clipboard", "edit.search", "edit.prefs"},
        20
    };
    
    // View menu
    menu_templates_["view"] = MenuTemplate{
        "view", "&View",
        {},  // View actions added individually
        30
    };
    
    // Database menu
    menu_templates_["database"] = MenuTemplate{
        "database", "&Database",
        {"db.connect", "db.manage", "db.backup"},
        40
    };
    
    // Query menu
    menu_templates_["query"] = MenuTemplate{
        "query", "&Query",
        {"query.execute", "query.tools"},
        50
    };
    
    // Transaction menu
    menu_templates_["transaction"] = MenuTemplate{
        "transaction", "&Transaction",
        {"transaction.basic", "transaction.options"},
        60
    };
    
    // Object menu (New submenu contains multiple groups)
    menu_templates_["object"] = MenuTemplate{
        "object", "&Object",
        {},  // Object actions added dynamically based on selection
        70
    };
    
    // Tools menu
    menu_templates_["tools"] = MenuTemplate{
        "tools", "&Tools",
        {},  // Tools added individually
        80
    };
    
    // Window menu
    menu_templates_["window"] = MenuTemplate{
        "window", "&Window",
        {"window.basic", "window.navigate", "window.arrange"},
        90
    };
    
    // Help menu
    menu_templates_["help"] = MenuTemplate{
        "help", "&Help",
        {"help.docs", "help.about"},
        100
    };
}

// ============================================================================
// Public API Implementation
// ============================================================================
void ActionCatalog::registerAction(const ActionDefinition& action) {
    actions_[action.action_id] = action;
}

const ActionDefinition* ActionCatalog::findAction(const std::string& action_id) const {
    auto it = actions_.find(action_id);
    if (it != actions_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<const ActionDefinition*> ActionCatalog::getActionsByCategory(ActionCategory cat) const {
    std::vector<const ActionDefinition*> result;
    for (const auto& [id, action] : actions_) {
        if (action.category == cat) {
            result.push_back(&action);
        }
    }
    std::sort(result.begin(), result.end(), [](const ActionDefinition* a, const ActionDefinition* b) {
        return a->sort_order < b->sort_order;
    });
    return result;
}

std::vector<const ActionDefinition*> ActionCatalog::getAllActions() const {
    std::vector<const ActionDefinition*> result;
    for (const auto& [id, action] : actions_) {
        result.push_back(&action);
    }
    std::sort(result.begin(), result.end(), [](const ActionDefinition* a, const ActionDefinition* b) {
        if (a->category != b->category) {
            return a->category < b->category;
        }
        return a->sort_order < b->sort_order;
    });
    return result;
}

std::vector<const ActionDefinition*> ActionCatalog::searchActions(const std::string& query) const {
    std::vector<const ActionDefinition*> result;
    std::string lower_query = query;
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);
    
    for (const auto& [id, action] : actions_) {
        std::string lower_id = id;
        std::transform(lower_id.begin(), lower_id.end(), lower_id.begin(), ::tolower);
        if (lower_id.find(lower_query) != std::string::npos) {
            result.push_back(&action);
            continue;
        }
        
        std::string lower_name = action.display_name;
        std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
        if (lower_name.find(lower_query) != std::string::npos) {
            result.push_back(&action);
            continue;
        }
        
        std::string lower_desc = action.description;
        std::transform(lower_desc.begin(), lower_desc.end(), lower_desc.begin(), ::tolower);
        if (lower_desc.find(lower_query) != std::string::npos) {
            result.push_back(&action);
        }
    }
    return result;
}

void ActionCatalog::registerActionGroup(const ActionGroup& group) {
    action_groups_[group.group_id] = group;
}

const ActionGroup* ActionCatalog::findActionGroup(const std::string& group_id) const {
    auto it = action_groups_.find(group_id);
    if (it != action_groups_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<const ActionGroup*> ActionCatalog::getActionGroupsForMenu(const std::string& menu_id) const {
    std::vector<const ActionGroup*> result;
    auto menu_it = menu_templates_.find(menu_id);
    if (menu_it == menu_templates_.end()) {
        return result;
    }
    
    for (const auto& group_id : menu_it->second.action_groups) {
        auto group_it = action_groups_.find(group_id);
        if (group_it != action_groups_.end()) {
            result.push_back(&group_it->second);
        }
    }
    return result;
}

void ActionCatalog::registerMenuTemplate(const MenuTemplate& menu) {
    menu_templates_[menu.menu_id] = menu;
}

const MenuTemplate* ActionCatalog::findMenuTemplate(const std::string& menu_id) const {
    auto it = menu_templates_.find(menu_id);
    if (it != menu_templates_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<const MenuTemplate*> ActionCatalog::getAllMenuTemplates() const {
    std::vector<const MenuTemplate*> result;
    for (const auto& [id, menu] : menu_templates_) {
        result.push_back(&menu);
    }
    std::sort(result.begin(), result.end(), [](const MenuTemplate* a, const MenuTemplate* b) {
        return a->sort_order < b->sort_order;
    });
    return result;
}

bool ActionCatalog::canExecute(const std::string& action_id) const {
    const ActionDefinition* action = findAction(action_id);
    if (!action) {
        return false;
    }
    // TODO: Check actual requirements against current state
    return true;
}

std::vector<ActionCategory> ActionCatalog::getCategories() const {
    std::vector<ActionCategory> result;
    for (const auto& [id, action] : actions_) {
        if (std::find(result.begin(), result.end(), action.category) == result.end()) {
            result.push_back(action.category);
        }
    }
    std::sort(result.begin(), result.end());
    return result;
}

std::string ActionCatalog::generateActionId(const std::string& base_name) {
    std::string id = "custom." + base_name;
    std::transform(id.begin(), id.end(), id.begin(), ::tolower);
    for (auto& c : id) {
        if (c == ' ') c = '_';
    }
    
    std::string unique_id = id;
    int suffix = 1;
    while (actions_.find(unique_id) != actions_.end()) {
        unique_id = id + "_" + std::to_string(suffix++);
    }
    return unique_id;
}

}  // namespace scratchrobin::ui
