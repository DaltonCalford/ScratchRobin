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
#include "ui/window_framework/window_types.h"

namespace scratchrobin::ui {

// ============================================================================
// Window Type Identifiers
// Standard window type IDs used throughout the application
// ============================================================================

// Additional window type identifiers not in window_types.h
namespace WindowTypeIds {
    // Main application window - always present
    constexpr const char* kMainWindow = "main_window";
    
    // Navigator/Explorer windows
    constexpr const char* kObjectNavigator = "object_navigator";       // Object browser tree
    constexpr const char* kSearchNavigator = "search_navigator";       // Search results panel
    
    // SQL Editor windows - for executing SQL
    constexpr const char* kSQLScriptEditor = "sql_script_editor";      // Multi-statement script editor
    
    // Results/Output windows
    constexpr const char* kResultsPanel = "results_panel";             // Query results grid
    constexpr const char* kQueryPlanPanel = "query_plan_panel";        // Query execution plan
    constexpr const char* kHistoryPanel = "history_panel";             // SQL history
    
    // Database Object Editors - DDL Editors
    // These correspond to EBNF database object types
    constexpr const char* kTableEditor = "table_editor";               // Table designer/editor
    constexpr const char* kViewEditor = "view_editor";                 // View designer/editor
    constexpr const char* kProcedureEditor = "procedure_editor";       // Stored procedure editor
    constexpr const char* kFunctionEditor = "function_editor";         // Function editor
    constexpr const char* kTriggerEditor = "trigger_editor";           // Trigger editor
    constexpr const char* kSequenceEditor = "sequence_editor";         // Sequence editor
    constexpr const char* kDomainEditor = "domain_editor";             // Domain/UDT editor
    constexpr const char* kIndexEditor = "index_editor";               // Index designer
    constexpr const char* kConstraintEditor = "constraint_editor";     // Constraint editor
    constexpr const char* kGeneratorEditor = "generator_editor";       // Generator/Sequence editor (Firebird)
    constexpr const char* kExceptionEditor = "exception_editor";       // Exception editor
    constexpr const char* kPackageEditor = "package_editor";           // Package editor
    constexpr const char* kRoleEditor = "role_editor";                 // Role editor
    constexpr const char* kUserEditor = "user_editor";                 // User editor
    constexpr const char* kCharsetEditor = "charset_editor";           // Character set editor
    constexpr const char* kCollationEditor = "collation_editor";       // Collation editor
    constexpr const char* kFilterEditor = "filter_editor";             // BLOB filter editor
    constexpr const char* kShadowEditor = "shadow_editor";             // Shadow editor
    constexpr const char* kSynonymEditor = "synonym_editor";           // Synonym editor
    
    // Data Editors - DML Editors
    constexpr const char* kTableDataEditor = "table_data_editor";      // Table data grid editor
    constexpr const char* kBlobEditor = "blob_editor";                 // BLOB content editor
    constexpr const char* kArrayEditor = "array_editor";               // Array editor
    
    // Properties/Inspector windows
    constexpr const char* kObjectProperties = "object_properties";     // Generic object properties
    constexpr const char* kDatabaseProperties = "database_properties"; // Database properties
    constexpr const char* kServerProperties = "server_properties";     // Server properties
    constexpr const char* kColumnProperties = "column_properties";     // Column properties
    constexpr const char* kIndexProperties = "index_properties";       // Index properties
    
    // Tool windows
    constexpr const char* kStatusBar = "status_bar";                   // Status bar
    constexpr const char* kSearchBar = "search_bar";                   // Search/find bar
    
    // Dialog windows (modal)
    constexpr const char* kSettingsDialog = "settings_dialog";         // Application settings
    constexpr const char* kPreferencesDialog = "preferences_dialog";   // User preferences
    constexpr const char* kConnectionDialog = "connection_dialog";     // Connection dialog
    constexpr const char* kAboutDialog = "about_dialog";               // About dialog
    constexpr const char* kPrintDialog = "print_dialog";               // Print dialog
    constexpr const char* kExportDialog = "export_dialog";             // Export dialog
    constexpr const char* kImportDialog = "import_dialog";             // Import dialog
    constexpr const char* kBackupDialog = "backup_dialog";             // Backup dialog
    constexpr const char* kRestoreDialog = "restore_dialog";           // Restore dialog
    constexpr const char* kMonitorDialog = "monitor_dialog";           // Database monitor
    
    // Popup windows (non-resizable, centered, temporary)
    constexpr const char* kMessagePopup = "message_popup";             // Simple message popup
    constexpr const char* kErrorPopup = "error_popup";                 // Error message popup
    constexpr const char* kWarningPopup = "warning_popup";             // Warning popup
    constexpr const char* kInfoPopup = "info_popup";                   // Info popup
    constexpr const char* kConfirmPopup = "confirm_popup";             // Confirmation dialog
    constexpr const char* kInputPopup = "input_popup";                 // Input prompt popup
    constexpr const char* kProgressPopup = "progress_popup";           // Progress indicator
    constexpr const char* kWaitPopup = "wait_popup";                   // Wait/loading popup
    
    // Special windows
    constexpr const char* kSplashScreen = "splash_screen";             // Application splash
    constexpr const char* kWelcomeScreen = "welcome_screen";           // Welcome/start page
    constexpr const char* kTipOfDay = "tip_of_day";                    // Tip of the day
}

// Note: WindowTypeDefinition is defined in window_types.h
// This file extends it with the catalog and type identifiers

// ============================================================================
// Window Type Catalog
// Central registry for all window type definitions
// ============================================================================

class WindowTypeCatalog {
public:
    static WindowTypeCatalog* get();
    
    // Initialize with standard window types
    void initialize();
    
    // Register a window type
    void registerWindowType(const WindowTypeDefinition& def);
    
    // Find a window type definition
    WindowTypeDefinition* findWindowType(const std::string& type_id);
    
    // Get all window types
    std::vector<WindowTypeDefinition*> getAllWindowTypes();
    
    // Get window types by category
    std::vector<WindowTypeDefinition*> getWindowTypesByCategory(WindowCategory category);
    
    // Check if a type is registered
    bool hasWindowType(const std::string& type_id) const;
    
    // Group helpers
    std::vector<WindowTypeDefinition*> getSQLEditors();          // All SQL editor types
    std::vector<WindowTypeDefinition*> getObjectEditors();       // All DDL object editors
    std::vector<WindowTypeDefinition*> getDataEditors();         // All DML data editors
    std::vector<WindowTypeDefinition*> getPropertyWindows();     // All property windows
    std::vector<WindowTypeDefinition*> getToolWindows();         // All tool windows
    std::vector<WindowTypeDefinition*> getDialogs();             // All dialog types
    std::vector<WindowTypeDefinition*> getPopups();              // All popup types
    
    // Is type checks
    bool isSQLEditor(const std::string& type_id);
    bool isObjectEditor(const std::string& type_id);
    bool isDataEditor(const std::string& type_id);
    bool isDialog(const std::string& type_id);
    bool isPopup(const std::string& type_id);
    bool isNavigator(const std::string& type_id);

private:
    WindowTypeCatalog() = default;
    
    std::map<std::string, WindowTypeDefinition> window_types_;
    
    // Registration helpers for different categories
    void registerNavigatorTypes();
    void registerSQLEditorTypes();
    void registerResultsTypes();
    void registerObjectEditorTypes();
    void registerDataEditorTypes();
    void registerPropertyTypes();
    void registerToolTypes();
    void registerDialogTypes();
    void registerPopupTypes();
    void registerSpecialTypes();
    
    // Helper to create standard definitions
    WindowTypeDefinition createNavigatorDef(const std::string& id, const std::string& name);
    WindowTypeDefinition createSQLEditorDef(const std::string& id, const std::string& name);
    WindowTypeDefinition createResultsDef(const std::string& id, const std::string& name);
    WindowTypeDefinition createObjectEditorDef(const std::string& id, const std::string& name, 
                                                const std::string& object_type);
    WindowTypeDefinition createDataEditorDef(const std::string& id, const std::string& name);
    WindowTypeDefinition createPropertyDef(const std::string& id, const std::string& name);
    WindowTypeDefinition createToolDef(const std::string& id, const std::string& name);
    WindowTypeDefinition createDialogDef(const std::string& id, const std::string& name);
    WindowTypeDefinition createPopupDef(const std::string& id, const std::string& name);
};

// ============================================================================
// Database Object Type Registry
// Maps EBNF database object types to window types
// ============================================================================

namespace DatabaseObjectTypes {
    // EBNF database object types
    constexpr const char* kTable = "TABLE";
    constexpr const char* kView = "VIEW";
    constexpr const char* kProcedure = "PROCEDURE";
    constexpr const char* kFunction = "FUNCTION";
    constexpr const char* kTrigger = "TRIGGER";
    constexpr const char* kSequence = "SEQUENCE";
    constexpr const char* kGenerator = "GENERATOR";  // Firebird-specific
    constexpr const char* kDomain = "DOMAIN";
    constexpr const char* kIndex = "INDEX";
    constexpr const char* kConstraint = "CONSTRAINT";
    constexpr const char* kException = "EXCEPTION";
    constexpr const char* kPackage = "PACKAGE";
    constexpr const char* kRole = "ROLE";
    constexpr const char* kUser = "USER";
    constexpr const char* kCharset = "CHARACTER SET";
    constexpr const char* kCollation = "COLLATION";
    constexpr const char* kFilter = "FILTER";
    constexpr const char* kShadow = "SHADOW";
    constexpr const char* kSynonym = "SYNONYM";
}

// Maps database object type to editor window type
class ObjectTypeMapper {
public:
    static ObjectTypeMapper* get();
    
    // Map database object type to window type
    std::string getEditorWindowType(const std::string& object_type);
    
    // Get display name for object type
    std::string getObjectTypeDisplayName(const std::string& object_type);
    
    // Get all known object types
    std::vector<std::string> getAllObjectTypes();
    
    // Register a mapping
    void registerMapping(const std::string& object_type, const std::string& window_type);

private:
    ObjectTypeMapper() { initialize(); }
    void initialize();
    
    std::map<std::string, std::string> type_to_window_;
    std::map<std::string, std::string> type_to_display_;
};

}  // namespace scratchrobin::ui
