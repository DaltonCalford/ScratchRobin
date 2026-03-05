/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <functional>

namespace scratchrobin::ui {

// ============================================================================
// Action Categories - organized by functional area
// ============================================================================
enum class ActionCategory {
    // Application level
    kFile,              // File operations (new, open, save, print)
    kEdit,              // Edit operations (cut, copy, paste, undo)
    kView,              // View operations (zoom, fullscreen, panels)
    kWindow,            // Window management (new, close, arrange)
    kHelp,              // Help operations (docs, about, tips)
    
    // Database operations
    kDatabase,          // Database level (connect, backup, restore)
    kServer,            // Server management (register, properties)
    kSchema,            // Schema operations (create, drop)
    
    // SQL and Query operations
    kQuery,             // SQL execution (execute, explain)
    kTransaction,       // Transaction control (commit, rollback)
    kScript,            // Script operations (run, debug)
    
    // DDL Object operations - organized by object type
    kTable,             // Table specific operations
    kDbView,            // View specific operations (database object)
    kProcedure,         // Procedure specific operations
    kFunction,          // Function specific operations
    kTrigger,           // Trigger specific operations
    kIndex,             // Index specific operations
    kConstraint,        // Constraint specific operations
    kDomain,            // Domain/type operations
    kSequence,          // Sequence/Generator operations
    kPackage,           // Package operations
    kException,         // Exception operations
    kRole,              // Role operations
    kUser,              // User operations
    kGroup,             // Group operations
    kJob,               // Job operations
    kSchedule,          // Schedule operations
    kCollation,         // Collation operations
    kCharset,           // Character set operations
    kFilter,            // Filter operations
    kShadow,            // Shadow operations
    kSynonym,           // Synonym operations
    
    // Data operations
    kData,              // Data editing (insert, update, delete, filter)
    kImportExport,      // Import/Export operations
    
    // Tools and utilities
    kTools,             // General tools
    kMonitor,           // Monitoring tools
    kMaintenance,       // Database maintenance
    kMigration,         // Migration tools
    
    // Navigation
    kNavigation,        // Navigation (back, forward, home)
    kSearch,            // Search operations
    
    // Custom/User defined
    kCustom,            // User-defined actions
};

// Convert category to string for display
std::string actionCategoryToString(ActionCategory cat);
ActionCategory actionCategoryFromString(const std::string& str);

// ============================================================================
// Action Scope - where can this action be used?
// ============================================================================
enum class ActionScope {
    kGlobal,            // Available everywhere
    kWindow,            // Window-specific (SQL editor, Table editor)
    kContext,           // Context menu only
    kDialog,            // Only within specific dialogs
};

// ============================================================================
// Action State Requirements - what must be true to enable this action?
// ============================================================================
struct ActionRequirements {
    bool needs_connection{false};       // Database connection required
    bool needs_transaction{false};      // Active transaction required
    bool needs_selection{false};        // Object selection required
    bool needs_editor{false};           // Active editor required
    bool needs_query{false};            // Query text required
    bool needs_table{false};            // Table selection required
    bool needs_view{false};             // View selection required
    bool needs_object{false};           // Any object selection required
};

// ============================================================================
// Action Definition
// ============================================================================
struct ActionDefinition {
    std::string action_id;              // Unique identifier (e.g., "file.new_connection")
    std::string display_name;           // Human-readable name
    std::string description;            // Detailed description
    std::string tooltip;                // Tooltip text
    std::string help_topic;             // Help file/topic link
    
    ActionCategory category;            // Functional category
    ActionScope scope;                  // Where action can be used
    ActionRequirements requirements;    // Enable/disable conditions
    
    std::string default_icon;           // Default icon ID
    std::string accelerator;            // Keyboard shortcut
    
    bool is_checkable{false};           // Toggle/checkbox action
    bool is_radio{false};               // Radio button action
    bool is_builtin{true};              // Built-in vs custom
    bool is_visible{true};              // Show in action lists
    
    int sort_order{0};                  // Sorting within category
};

// ============================================================================
// Action Group - related actions that should be kept together
// ============================================================================
struct ActionGroup {
    std::string group_id;               // Group identifier
    std::string display_name;           // Group name
    std::vector<std::string> action_ids; // Actions in this group
    bool show_separator_after{true};    // Add separator after group
};

// ============================================================================
// Menu Template - predefined menu structures
// ============================================================================
struct MenuTemplate {
    std::string menu_id;                // Menu identifier
    std::string display_name;           // Menu title (&File, &Edit)
    std::vector<std::string> action_groups; // Action groups in this menu
    int sort_order{0};                  // Position in menu bar
};

// ============================================================================
// Action Catalog - comprehensive action registry
// ============================================================================
class ActionCatalog {
public:
    static ActionCatalog* get();
    
    // Initialize with all built-in actions
    void initialize();
    
    // Action registration
    void registerAction(const ActionDefinition& action);
    const ActionDefinition* findAction(const std::string& action_id) const;
    std::vector<const ActionDefinition*> getActionsByCategory(ActionCategory cat) const;
    std::vector<const ActionDefinition*> getAllActions() const;
    std::vector<const ActionDefinition*> searchActions(const std::string& query) const;
    
    // Action groups
    void registerActionGroup(const ActionGroup& group);
    const ActionGroup* findActionGroup(const std::string& group_id) const;
    std::vector<const ActionGroup*> getActionGroupsForMenu(const std::string& menu_id) const;
    
    // Menu templates
    void registerMenuTemplate(const MenuTemplate& menu);
    const MenuTemplate* findMenuTemplate(const std::string& menu_id) const;
    std::vector<const MenuTemplate*> getAllMenuTemplates() const;
    
    // Check if action can be executed in current context
    bool canExecute(const std::string& action_id) const;
    
    // Get categories with actions
    std::vector<ActionCategory> getCategories() const;
    
    // Generate unique action ID
    std::string generateActionId(const std::string& base_name);

private:
    ActionCatalog() = default;
    
    std::map<std::string, ActionDefinition> actions_;
    std::map<std::string, ActionGroup> action_groups_;
    std::map<std::string, MenuTemplate> menu_templates_;
    
    // Registration methods for each category
    void registerFileActions();
    void registerEditActions();
    void registerViewActions();
    void registerWindowActions();
    void registerHelpActions();
    
    void registerDatabaseActions();
    void registerServerActions();
    void registerSchemaActions();
    
    void registerQueryActions();
    void registerTransactionActions();
    void registerScriptActions();
    
    void registerTableActions();
    void registerDbViewActions();
    void registerProcedureActions();
    void registerFunctionActions();
    void registerTriggerActions();
    void registerIndexActions();
    void registerConstraintActions();
    void registerDomainActions();
    void registerSequenceActions();
    void registerPackageActions();
    void registerExceptionActions();
    void registerRoleActions();
    void registerUserActions();
    void registerGroupActions();
    void registerJobActions();
    void registerScheduleActions();
    void registerOtherObjectActions();
    
    void registerDataActions();
    void registerImportExportActions();
    void registerToolsActions();
    void registerMonitorActions();
    void registerMaintenanceActions();
    void registerNavigationActions();
    void registerSearchActions();
    
    void registerActionGroups();
    void registerMenuTemplates();
};

// ============================================================================
// Action ID Constants - use these instead of string literals
// ============================================================================
namespace ActionIds {
    // File Operations
    constexpr const char* FILE_NEW_CONNECTION = "file.new_connection";
    constexpr const char* FILE_OPEN_SQL = "file.open_sql";
    constexpr const char* FILE_SAVE_SQL = "file.save_sql";
    constexpr const char* FILE_SAVE_AS = "file.save_as";
    constexpr const char* FILE_PRINT = "file.print";
    constexpr const char* FILE_PRINT_PREVIEW = "file.print_preview";
    constexpr const char* FILE_EXIT = "file.exit";
    
    // Edit Operations
    constexpr const char* EDIT_UNDO = "edit.undo";
    constexpr const char* EDIT_REDO = "edit.redo";
    constexpr const char* EDIT_CUT = "edit.cut";
    constexpr const char* EDIT_COPY = "edit.copy";
    constexpr const char* EDIT_PASTE = "edit.paste";
    constexpr const char* EDIT_SELECT_ALL = "edit.select_all";
    constexpr const char* EDIT_FIND = "edit.find";
    constexpr const char* EDIT_FIND_REPLACE = "edit.find_replace";
    constexpr const char* EDIT_FIND_NEXT = "edit.find_next";
    constexpr const char* EDIT_FIND_PREV = "edit.find_prev";
    constexpr const char* EDIT_GOTO_LINE = "edit.goto_line";
    constexpr const char* EDIT_PREFERENCES = "edit.preferences";
    
    // View Operations
    constexpr const char* VIEW_REFRESH = "view.refresh";
    constexpr const char* VIEW_ZOOM_IN = "view.zoom_in";
    constexpr const char* VIEW_ZOOM_OUT = "view.zoom_out";
    constexpr const char* VIEW_ZOOM_RESET = "view.zoom_reset";
    constexpr const char* VIEW_FULLSCREEN = "view.fullscreen";
    constexpr const char* VIEW_STATUS_BAR = "view.status_bar";
    constexpr const char* VIEW_SEARCH_BAR = "view.search_bar";
    constexpr const char* VIEW_PROJECT_NAVIGATOR = "view.project_navigator";
    constexpr const char* VIEW_PROPERTIES = "view.properties";
    constexpr const char* VIEW_OUTPUT = "view.output";
    constexpr const char* VIEW_QUERY_PLAN = "view.query_plan";
    
    // Window Operations
    constexpr const char* WINDOW_NEW = "window.new";
    constexpr const char* WINDOW_CLOSE = "window.close";
    constexpr const char* WINDOW_CLOSE_ALL = "window.close_all";
    constexpr const char* WINDOW_NEXT = "window.next";
    constexpr const char* WINDOW_PREV = "window.prev";
    constexpr const char* WINDOW_CASCADE = "window.cascade";
    constexpr const char* WINDOW_TILE_H = "window.tile_horizontal";
    constexpr const char* WINDOW_TILE_V = "window.tile_vertical";
    
    // Help Operations
    constexpr const char* HELP_DOCS = "help.docs";
    constexpr const char*_HELP_CONTEXT = "help.context";
    constexpr const char* HELP_SHORTCUTS = "help.shortcuts";
    constexpr const char* HELP_TIP_OF_DAY = "help.tip_of_day";
    constexpr const char* HELP_ABOUT = "help.about";
    
    // Database Operations
    constexpr const char* DB_CONNECT = "db.connect";
    constexpr const char* DB_DISCONNECT = "db.disconnect";
    constexpr const char* DB_RECONNECT = "db.reconnect";
    constexpr const char* DB_NEW = "db.new_database";
    constexpr const char* DB_REGISTER = "db.register_database";
    constexpr const char* DB_UNREGISTER = "db.unregister_database";
    constexpr const char* DB_PROPERTIES = "db.properties";
    constexpr const char* DB_BACKUP = "db.backup";
    constexpr const char* DB_RESTORE = "db.restore";
    constexpr const char* DB_VERIFY = "db.verify";
    constexpr const char* DB_COMPACT = "db.compact";
    constexpr const char* DB_SHUTDOWN = "db.shutdown";
    
    // Server Operations
    constexpr const char* SERVER_REGISTER = "server.register";
    constexpr const char* SERVER_UNREGISTER = "server.unregister";
    constexpr const char* SERVER_PROPERTIES = "server.properties";
    constexpr const char* SERVER_START = "server.start";
    constexpr const char* SERVER_STOP = "server.stop";
    
    // Schema Operations
    constexpr const char* SCHEMA_CREATE = "schema.create";
    constexpr const char* SCHEMA_DROP = "schema.drop";
    constexpr const char* SCHEMA_PROPERTIES = "schema.properties";
    
    // Query Operations
    constexpr const char* QUERY_EXECUTE = "query.execute";
    constexpr const char* QUERY_EXECUTE_SELECTION = "query.execute_selection";
    constexpr const char* QUERY_EXECUTE_SCRIPT = "query.execute_script";
    constexpr const char* QUERY_EXPLAIN = "query.explain";
    constexpr const char* QUERY_EXPLAIN_ANALYZE = "query.explain_analyze";
    constexpr const char* QUERY_STOP = "query.stop";
    constexpr const char* QUERY_FORMAT = "query.format";
    constexpr const char* QUERY_COMMENT = "query.comment";
    constexpr const char* QUERY_UNCOMMENT = "query.uncomment";
    constexpr const char* QUERY_TOGGLE_COMMENT = "query.toggle_comment";
    constexpr const char* QUERY_UPPERCASE = "query.uppercase";
    constexpr const char* QUERY_LOWERCASE = "query.lowercase";
    
    // Transaction Operations
    constexpr const char* TX_START = "tx.start";
    constexpr const char* TX_COMMIT = "tx.commit";
    constexpr const char* TX_ROLLBACK = "tx.rollback";
    constexpr const char* TX_SAVEPOINT = "tx.savepoint";
    constexpr const char* TX_ROLLBACK_TO = "tx.rollback_to";
    constexpr const char* TX_AUTOCOMMIT = "tx.autocommit";
    constexpr const char* TX_ISOLATION = "tx.isolation_level";
    constexpr const char* TX_READONLY = "tx.readonly";
    
    // Script Operations
    constexpr const char* SCRIPT_RUN = "script.run";
    constexpr const char* SCRIPT_DEBUG = "script.debug";
    constexpr const char* SCRIPT_STEP_INTO = "script.step_into";
    constexpr const char* SCRIPT_STEP_OVER = "script.step_over";
    constexpr const char* SCRIPT_STEP_OUT = "script.step_out";
    constexpr const char* SCRIPT_CONTINUE = "script.continue";
    constexpr const char* SCRIPT_BREAKPOINT = "script.breakpoint";
    constexpr const char* SCRIPT_STOP_DEBUG = "script.stop_debug";
    
    // Table Operations
    constexpr const char* TABLE_NEW = "table.new";
    constexpr const char* TABLE_CREATE = "table.create";
    constexpr const char* TABLE_ALTER = "table.alter";
    constexpr const char* TABLE_DROP = "table.drop";
    constexpr const char* TABLE_RECREATE = "table.recreate";
    constexpr const char* TABLE_TRUNCATE = "table.truncate";
    constexpr const char* TABLE_RENAME = "table.rename";
    constexpr const char* TABLE_COPY = "table.copy";
    constexpr const char* TABLE_DUPLICATE = "table.duplicate";
    constexpr const char* TABLE_PROPERTIES = "table.properties";
    constexpr const char* TABLE_CONSTRAINTS = "table.constraints";
    constexpr const char* TABLE_INDEXES = "table.indexes";
    constexpr const char* TABLE_TRIGGERS = "table.triggers";
    constexpr const char* TABLE_DEPENDENCIES = "table.dependencies";
    constexpr const char* TABLE_PERMISSIONS = "table.permissions";
    constexpr const char* TABLE_STATISTICS = "table.statistics";
    constexpr const char* TABLE_EXPORT_DATA = "table.export_data";
    constexpr const char* TABLE_IMPORT_DATA = "table.import_data";
    constexpr const char* TABLE_GENERATE_DDL = "table.generate_ddl";
    constexpr const char* TABLE_GENERATE_SELECT = "table.generate_select";
    constexpr const char* TABLE_GENERATE_INSERT = "table.generate_insert";
    constexpr const char* TABLE_GENERATE_UPDATE = "table.generate_update";
    constexpr const char* TABLE_GENERATE_DELETE = "table.generate_delete";
    constexpr const char* TABLE_BROWSE_DATA = "table.browse_data";
    
    // View Object Operations
    constexpr const char* VIEW_NEW = "view.new";
    constexpr const char* VIEW_CREATE = "view.create";
    constexpr const char* VIEW_ALTER = "view.alter";
    constexpr const char* VIEW_DROP = "view.drop";
    constexpr const char* VIEW_REFRESH_DEF = "view.refresh_def";
    constexpr const char* VIEW_RECREATE = "view.recreate";
    constexpr const char* VIEW_RENAME = "view.rename";
    constexpr const char* VIEW_COPY = "view.copy";
    constexpr const char* VIEW_OBJECT_PROPERTIES = "view.object_properties";
    constexpr const char* VIEW_DEPENDENCIES = "view.dependencies";
    constexpr const char* VIEW_PERMISSIONS = "view.permissions";
    constexpr const char* VIEW_GENERATE_DDL = "view.generate_ddl";
    constexpr const char* VIEW_BROWSE_DATA = "view.browse_data";
    
    // Procedure Operations
    constexpr const char* PROCEDURE_NEW = "procedure.new";
    constexpr const char* PROCEDURE_CREATE = "procedure.create";
    constexpr const char* PROCEDURE_ALTER = "procedure.alter";
    constexpr const char* PROCEDURE_DROP = "procedure.drop";
    constexpr const char* PROCEDURE_RECREATE = "procedure.recreate";
    constexpr const char* PROCEDURE_RENAME = "procedure.rename";
    constexpr const char* PROCEDURE_COPY = "procedure.copy";
    constexpr const char* PROCEDURE_PROPERTIES = "procedure.properties";
    constexpr const char* PROCEDURE_EXECUTE = "procedure.execute";
    constexpr const char* PROCEDURE_DEBUG = "procedure.debug";
    constexpr const char* PROCEDURE_DEPENDENCIES = "procedure.dependencies";
    constexpr const char* PROCEDURE_PERMISSIONS = "procedure.permissions";
    constexpr const char* PROCEDURE_GENERATE_DDL = "procedure.generate_ddl";
    constexpr const char* PROCEDURE_GENERATE_CALL = "procedure.generate_call";
    
    // Function Operations
    constexpr const char* FUNCTION_NEW = "function.new";
    constexpr const char* FUNCTION_CREATE = "function.create";
    constexpr const char* FUNCTION_ALTER = "function.alter";
    constexpr const char* FUNCTION_DROP = "function.drop";
    constexpr const char* FUNCTION_RECREATE = "function.recreate";
    constexpr const char* FUNCTION_RENAME = "function.rename";
    constexpr const char* FUNCTION_COPY = "function.copy";
    constexpr const char* FUNCTION_PROPERTIES = "function.properties";
    constexpr const char* FUNCTION_EXECUTE = "function.execute";
    constexpr const char* FUNCTION_DEBUG = "function.debug";
    constexpr const char* FUNCTION_DEPENDENCIES = "function.dependencies";
    constexpr const char* FUNCTION_PERMISSIONS = "function.permissions";
    constexpr const char* FUNCTION_GENERATE_DDL = "function.generate_ddl";
    constexpr const char* FUNCTION_GENERATE_CALL = "function.generate_call";
    
    // Trigger Operations
    constexpr const char* TRIGGER_NEW = "trigger.new";
    constexpr const char* TRIGGER_CREATE = "trigger.create";
    constexpr const char* TRIGGER_ALTER = "trigger.alter";
    constexpr const char* TRIGGER_DROP = "trigger.drop";
    constexpr const char* TRIGGER_ENABLE = "trigger.enable";
    constexpr const char* TRIGGER_DISABLE = "trigger.disable";
    constexpr const char* TRIGGER_RECREATE = "trigger.recreate";
    constexpr const char* TRIGGER_RENAME = "trigger.rename";
    constexpr const char* TRIGGER_COPY = "trigger.copy";
    constexpr const char* TRIGGER_PROPERTIES = "trigger.properties";
    constexpr const char* TRIGGER_GENERATE_DDL = "trigger.generate_ddl";
    
    // Index Operations
    constexpr const char* INDEX_NEW = "index.new";
    constexpr const char* INDEX_CREATE = "index.create";
    constexpr const char* INDEX_ALTER = "index.alter";
    constexpr const char* INDEX_DROP = "index.drop";
    constexpr const char* INDEX_RECREATE = "index.recreate";
    constexpr const char* INDEX_RENAME = "index.rename";
    constexpr const char* INDEX_REBUILD = "index.rebuild";
    constexpr const char* INDEX_STATISTICS = "index.statistics";
    constexpr const char* INDEX_PROPERTIES = "index.properties";
    constexpr const char* INDEX_GENERATE_DDL = "index.generate_ddl";
    
    // Constraint Operations
    constexpr const char* CONSTRAINT_NEW = "constraint.new";
    constexpr const char* CONSTRAINT_CREATE = "constraint.create";
    constexpr const char* CONSTRAINT_ALTER = "constraint.alter";
    constexpr const char* CONSTRAINT_DROP = "constraint.drop";
    constexpr const char* CONSTRAINT_ENABLE = "constraint.enable";
    constexpr const char* CONSTRAINT_DISABLE = "constraint.disable";
    constexpr const char* CONSTRAINT_PROPERTIES = "constraint.properties";
    constexpr const char* CONSTRAINT_GENERATE_DDL = "constraint.generate_ddl";
    
    // Domain Operations
    constexpr const char* DOMAIN_NEW = "domain.new";
    constexpr const char* DOMAIN_CREATE = "domain.create";
    constexpr const char* DOMAIN_ALTER = "domain.alter";
    constexpr const char* DOMAIN_DROP = "domain.drop";
    constexpr const char* DOMAIN_RECREATE = "domain.recreate";
    constexpr const char* DOMAIN_RENAME = "domain.rename";
    constexpr const char* DOMAIN_COPY = "domain.copy";
    constexpr const char* DOMAIN_PROPERTIES = "domain.properties";
    constexpr const char* DOMAIN_DEPENDENCIES = "domain.dependencies";
    constexpr const char* DOMAIN_GENERATE_DDL = "domain.generate_ddl";
    
    // Sequence/Generator Operations
    constexpr const char* SEQUENCE_NEW = "sequence.new";
    constexpr const char* SEQUENCE_CREATE = "sequence.create";
    constexpr const char* SEQUENCE_ALTER = "sequence.alter";
    constexpr const char* SEQUENCE_DROP = "sequence.drop";
    constexpr const char* SEQUENCE_RECREATE = "sequence.recreate";
    constexpr const char* SEQUENCE_RENAME = "sequence.rename";
    constexpr const char* SEQUENCE_RESET = "sequence.reset";
    constexpr const char* SEQUENCE_SET_VALUE = "sequence.set_value";
    constexpr const char* SEQUENCE_PROPERTIES = "sequence.properties";
    constexpr const char* SEQUENCE_GENERATE_DDL = "sequence.generate_ddl";
    constexpr const char* SEQUENCE_GENERATE_NEXT = "sequence.generate_next";
    
    // Package Operations
    constexpr const char* PACKAGE_NEW = "package.new";
    constexpr const char* PACKAGE_CREATE = "package.create";
    constexpr const char* PACKAGE_ALTER = "package.alter";
    constexpr const char* PACKAGE_DROP = "package.drop";
    constexpr const char* PACKAGE_RECREATE = "package.recreate";
    constexpr const char* PACKAGE_RENAME = "package.rename";
    constexpr const char* PACKAGE_COPY = "package.copy";
    constexpr const char* PACKAGE_PROPERTIES = "package.properties";
    constexpr const char* PACKAGE_BODY_EDIT = "package.edit_body";
    constexpr const char* PACKAGE_HEADER_EDIT = "package.edit_header";
    constexpr const char* PACKAGE_DEPENDENCIES = "package.dependencies";
    constexpr const char* PACKAGE_PERMISSIONS = "package.permissions";
    constexpr const char* PACKAGE_GENERATE_DDL = "package.generate_ddl";
    
    // Exception Operations
    constexpr const char* EXCEPTION_NEW = "exception.new";
    constexpr const char* EXCEPTION_CREATE = "exception.create";
    constexpr const char* EXCEPTION_ALTER = "exception.alter";
    constexpr const char* EXCEPTION_DROP = "exception.drop";
    constexpr const char* EXCEPTION_RECREATE = "exception.recreate";
    constexpr const char* EXCEPTION_RENAME = "exception.rename";
    constexpr const char* EXCEPTION_PROPERTIES = "exception.properties";
    constexpr const char* EXCEPTION_GENERATE_DDL = "exception.generate_ddl";
    
    // Role Operations
    constexpr const char* ROLE_NEW = "role.new";
    constexpr const char* ROLE_CREATE = "role.create";
    constexpr const char* ROLE_ALTER = "role.alter";
    constexpr const char* ROLE_DROP = "role.drop";
    constexpr const char* ROLE_RECREATE = "role.recreate";
    constexpr const char* ROLE_RENAME = "role.rename";
    constexpr const char* ROLE_COPY = "role.copy";
    constexpr const char* ROLE_GRANT = "role.grant";
    constexpr const char* ROLE_REVOKE = "role.revoke";
    constexpr const char* ROLE_PROPERTIES = "role.properties";
    constexpr const char* ROLE_MEMBERS = "role.members";
    constexpr const char* ROLE_PERMISSIONS = "role.permissions";
    constexpr const char* ROLE_GENERATE_DDL = "role.generate_ddl";
    
    // User Operations
    constexpr const char* USER_NEW = "user.new";
    constexpr const char* USER_CREATE = "user.create";
    constexpr const char* USER_ALTER = "user.alter";
    constexpr const char* USER_DROP = "user.drop";
    constexpr const char* USER_RECREATE = "user.recreate";
    constexpr const char* USER_RENAME = "user.rename";
    constexpr const char* USER_COPY = "user.copy";
    constexpr const char* USER_GRANT = "user.grant";
    constexpr const char* USER_REVOKE = "user.revoke";
    constexpr const char* USER_CHANGE_PASSWORD = "user.change_password";
    constexpr const char* USER_PROPERTIES = "user.properties";
    constexpr const char* USER_ROLES = "user.roles";
    constexpr const char* USER_PERMISSIONS = "user.permissions";
    constexpr const char* USER_GENERATE_DDL = "user.generate_ddl";
    
    // Group Operations
    constexpr const char* GROUP_NEW = "group.new";
    constexpr const char* GROUP_CREATE = "group.create";
    constexpr const char* GROUP_ALTER = "group.alter";
    constexpr const char* GROUP_DROP = "group.drop";
    constexpr const char* GROUP_RENAME = "group.rename";
    constexpr const char* GROUP_MEMBERS = "group.members";
    constexpr const char* GROUP_PROPERTIES = "group.properties";
    constexpr const char* GROUP_GENERATE_DDL = "group.generate_ddl";
    
    // Job Operations
    constexpr const char* JOB_NEW = "job.new";
    constexpr const char* JOB_CREATE = "job.create";
    constexpr const char* JOB_ALTER = "job.alter";
    constexpr const char* JOB_DROP = "job.drop";
    constexpr const char* JOB_RUN = "job.run";
    constexpr const char* JOB_STOP = "job.stop";
    constexpr const char* JOB_ENABLE = "job.enable";
    constexpr const char* JOB_DISABLE = "job.disable";
    constexpr const char* JOB_HISTORY = "job.history";
    constexpr const char* JOB_PROPERTIES = "job.properties";
    constexpr const char* JOB_SCHEDULE = "job.schedule";
    constexpr const char* JOB_GENERATE_DDL = "job.generate_ddl";
    
    // Schedule Operations
    constexpr const char* SCHEDULE_NEW = "schedule.new";
    constexpr const char* SCHEDULE_CREATE = "schedule.create";
    constexpr const char* SCHEDULE_ALTER = "schedule.alter";
    constexpr const char* SCHEDULE_DROP = "schedule.drop";
    constexpr const char* SCHEDULE_ENABLE = "schedule.enable";
    constexpr const char* SCHEDULE_DISABLE = "schedule.disable";
    constexpr const char* SCHEDULE_PROPERTIES = "schedule.properties";
    constexpr const char* SCHEDULE_GENERATE_DDL = "schedule.generate_ddl";
    
    // Data Operations
    constexpr const char* DATA_INSERT = "data.insert";
    constexpr const char* DATA_UPDATE = "data.update";
    constexpr const char* DATA_DELETE = "data.delete";
    constexpr const char* DATA_REFRESH = "data.refresh";
    constexpr const char* DATA_FILTER = "data.filter";
    constexpr const char* DATA_CLEAR_FILTER = "data.clear_filter";
    constexpr const char* DATA_SORT_ASC = "data.sort_asc";
    constexpr const char* DATA_SORT_DESC = "data.sort_desc";
    constexpr const char* DATA_EXPORT = "data.export";
    constexpr const char* DATA_IMPORT = "data.import";
    constexpr const char* DATA_PRINT = "data.print";
    constexpr const char* DATA_COMMIT = "data.commit";
    constexpr const char* DATA_ROLLBACK = "data.rollback";
    constexpr const char* DATA_FIRST = "data.first";
    constexpr const char* DATA_LAST = "data.last";
    constexpr const char* DATA_NEXT = "data.next";
    constexpr const char* DATA_PREV = "data.prev";
    constexpr const char* DATA_GOTO = "data.goto";
    
    // Import/Export Operations
    constexpr const char* IMPORT_SQL = "import.sql";
    constexpr const char* IMPORT_CSV = "import.csv";
    constexpr const char* IMPORT_XML = "import.xml";
    constexpr const char* IMPORT_JSON = "import.json";
    constexpr const char* IMPORT_EXCEL = "import.excel";
    constexpr const char* EXPORT_SQL = "export.sql";
    constexpr const char* EXPORT_CSV = "export.csv";
    constexpr const char* EXPORT_XML = "export.xml";
    constexpr const char* EXPORT_JSON = "export.json";
    constexpr const char* EXPORT_EXCEL = "export.excel";
    constexpr const char* EXPORT_HTML = "export.html";
    constexpr const char* EXPORT_PDF = "export.pdf";
    
    // Tools Operations
    constexpr const char* TOOLS_GENERATE_DDL = "tools.generate_ddl";
    constexpr const char* TOOLS_GENERATE_DATA = "tools.generate_data";
    constexpr const char* TOOLS_COMPARE_SCHEMA = "tools.compare_schema";
    constexpr const char* TOOLS_COMPARE_DATA = "tools.compare_data";
    constexpr const char* TOOLS_MIGRATE = "tools.migrate";
    constexpr const char* TOOLS_SCRIPT = "tools.script";
    constexpr const char* TOOLS_SCHEDULER = "tools.scheduler";
    
    // Monitor Operations
    constexpr const char* MONITOR_CONNECTIONS = "monitor.connections";
    constexpr const char* MONITOR_TRANSACTIONS = "monitor.transactions";
    constexpr const char* MONITOR_STATEMENTS = "monitor.statements";
    constexpr const char* MONITOR_LOCKS = "monitor.locks";
    constexpr const char* MONITOR_PERFORMANCE = "monitor.performance";
    constexpr const char* MONITOR_LOGS = "monitor.logs";
    constexpr const char* MONITOR_ALERTS = "monitor.alerts";
    
    // Maintenance Operations
    constexpr const char* MAINTENANCE_ANALYZE = "maintenance.analyze";
    constexpr const char* MAINTENANCE_VACUUM = "maintenance.vacuum";
    constexpr const char* MAINTENANCE_REINDEX = "maintenance.reindex";
    constexpr const char* MAINTENANCE_CHECK = "maintenance.check";
    constexpr const char* MAINTENANCE_REPAIR = "maintenance.repair";
    
    // Navigation Operations
    constexpr const char* NAV_BACK = "nav.back";
    constexpr const char* NAV_FORWARD = "nav.forward";
    constexpr const char* NAV_UP = "nav.up";
    constexpr const char* NAV_HOME = "nav.home";
    constexpr const char* NAV_REFRESH = "nav.refresh";
    constexpr const char* NAV_BOOKMARK = "nav.bookmark";
    constexpr const char* NAV_BOOKMARKS = "nav.bookmarks";
    
    // Search Operations
    constexpr const char* SEARCH_FIND = "search.find";
    constexpr const char* SEARCH_FIND_NEXT = "search.find_next";
    constexpr const char* SEARCH_FIND_PREV = "search.find_prev";
    constexpr const char* SEARCH_REPLACE = "search.replace";
    constexpr const char* SEARCH_ADVANCED = "search.advanced";
    constexpr const char* SEARCH_METADATA = "search.metadata";
    constexpr const char* SEARCH_DATA = "search.data";
    constexpr const char* SEARCH_SQL = "search.sql";
}

}  // namespace scratchrobin::ui
