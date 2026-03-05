/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "ui/window_framework/window_type_catalog.h"
#include <algorithm>

namespace scratchrobin::ui {

// ============================================================================
// WindowTypeCatalog Implementation
// ============================================================================

WindowTypeCatalog* WindowTypeCatalog::get() {
    static WindowTypeCatalog instance;
    return &instance;
}

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

void WindowTypeCatalog::registerNavigatorTypes() {
    // Project Navigator - Main object tree
    WindowTypeDefinition project_nav;
    project_nav.type_id = WindowTypes::kProjectNavigator;
    project_nav.display_name = "Project Navigator";
    project_nav.description = "Tree view of servers, databases, and database objects";
    project_nav.category = WindowCategory::kNavigator;
    project_nav.dock_capabilities = DockCapability::kLeft | DockCapability::kRight | DockCapability::kTab;
    project_nav.default_dock = wxLEFT;
    project_nav.size_behavior = SizeBehavior::kStretchVertical;
    project_nav.default_size = wxSize(300, 600);
    project_nav.min_size = wxSize(200, 300);
    project_nav.singleton = true;
    project_nav.persist_layout = true;
    project_nav.menu_set_ids = {"navigator"};
    registerWindowType(project_nav);
    
    // Object Navigator - Alternative object browser
    WindowTypeDefinition object_nav;
    object_nav.type_id = WindowTypeIds::kObjectNavigator;
    object_nav.display_name = "Object Navigator";
    object_nav.description = "Alternative object browser with filtering";
    object_nav.category = WindowCategory::kNavigator;
    object_nav.dock_capabilities = DockCapability::kLeft | DockCapability::kRight | DockCapability::kTab;
    object_nav.default_dock = wxLEFT;
    object_nav.size_behavior = SizeBehavior::kStretchVertical;
    object_nav.default_size = wxSize(300, 600);
    object_nav.singleton = false;
    object_nav.persist_layout = true;
    registerWindowType(object_nav);
    
    // Search Navigator
    WindowTypeDefinition search_nav;
    search_nav.type_id = WindowTypeIds::kSearchNavigator;
    search_nav.display_name = "Search Results";
    search_nav.description = "Search results and object finder";
    search_nav.category = WindowCategory::kNavigator;
    search_nav.dock_capabilities = DockCapability::kLeft | DockCapability::kRight | DockCapability::kBottom | DockCapability::kTab;
    search_nav.default_dock = wxBOTTOM;
    search_nav.size_behavior = SizeBehavior::kStretchHorizontal;
    search_nav.default_size = wxSize(800, 200);
    registerWindowType(search_nav);
}

void WindowTypeCatalog::registerSQLEditorTypes() {
    // Standard SQL Editor
    WindowTypeDefinition sql_editor;
    sql_editor.type_id = WindowTypes::kSQLEditor;
    sql_editor.display_name = "SQL Editor";
    sql_editor.description = "Editor for single SQL statements with syntax highlighting";
    sql_editor.category = WindowCategory::kEditor;
    sql_editor.dock_capabilities = DockCapability::kLeft | DockCapability::kRight | 
                                    DockCapability::kTop | DockCapability::kBottom | 
                                    DockCapability::kFloat | DockCapability::kTab;
    sql_editor.default_dock = wxCENTER;
    sql_editor.size_behavior = SizeBehavior::kStretchBoth;
    sql_editor.default_size = wxSize(800, 600);
    sql_editor.min_size = wxSize(400, 300);
    sql_editor.allow_multiple = true;
    sql_editor.max_instances = 20;
    sql_editor.persist_layout = true;
    sql_editor.menu_set_ids = {"sql_editor"};
    sql_editor.merge_menus_with_main = true;
    registerWindowType(sql_editor);
    
    // SQL Script Editor
    WindowTypeDefinition script_editor;
    script_editor.type_id = WindowTypeIds::kSQLScriptEditor;
    script_editor.display_name = "SQL Script";
    script_editor.description = "Editor for multi-statement SQL scripts with execution";
    script_editor.category = WindowCategory::kEditor;
    script_editor.dock_capabilities = DockCapability::kLeft | DockCapability::kRight | 
                                       DockCapability::kTop | DockCapability::kBottom | 
                                       DockCapability::kFloat | DockCapability::kTab;
    script_editor.default_dock = wxCENTER;
    script_editor.size_behavior = SizeBehavior::kStretchBoth;
    script_editor.default_size = wxSize(900, 700);
    script_editor.min_size = wxSize(500, 400);
    script_editor.allow_multiple = true;
    script_editor.max_instances = 10;
    script_editor.persist_layout = true;
    script_editor.menu_set_ids = {"sql_editor", "script"};
    script_editor.merge_menus_with_main = true;
    registerWindowType(script_editor);
    
    // Query Builder
    WindowTypeDefinition query_builder;
    query_builder.type_id = WindowTypes::kQueryBuilder;
    query_builder.display_name = "Query Builder";
    query_builder.description = "Visual query designer with drag-and-drop";
    query_builder.category = WindowCategory::kEditor;
    query_builder.dock_capabilities = DockCapability::kLeft | DockCapability::kRight | 
                                       DockCapability::kTop | DockCapability::kBottom | 
                                       DockCapability::kFloat | DockCapability::kTab;
    query_builder.default_dock = wxCENTER;
    query_builder.size_behavior = SizeBehavior::kStretchBoth;
    query_builder.default_size = wxSize(1000, 700);
    query_builder.min_size = wxSize(600, 500);
    query_builder.allow_multiple = true;
    query_builder.max_instances = 5;
    query_builder.menu_set_ids = {"query_builder"};
    query_builder.merge_menus_with_main = true;
    registerWindowType(query_builder);
}

void WindowTypeCatalog::registerResultsTypes() {
    // Results Panel
    WindowTypeDefinition results;
    results.type_id = WindowTypeIds::kResultsPanel;
    results.display_name = "Results";
    results.description = "Query results in data grid format";
    results.category = WindowCategory::kOutput;
    results.dock_capabilities = DockCapability::kLeft | DockCapability::kRight | 
                                 DockCapability::kBottom | DockCapability::kTab;
    results.default_dock = wxBOTTOM;
    results.size_behavior = SizeBehavior::kStretchHorizontal;
    results.default_size = wxSize(800, 300);
    results.min_size = wxSize(400, 150);
    results.allow_multiple = true;
    results.max_instances = 10;
    results.can_tab = true;
    results.persist_layout = true;
    registerWindowType(results);
    
    // Query Plan Panel
    WindowTypeDefinition query_plan;
    query_plan.type_id = WindowTypeIds::kQueryPlanPanel;
    query_plan.display_name = "Query Plan";
    query_plan.description = "Query execution plan visualization";
    query_plan.category = WindowCategory::kOutput;
    query_plan.dock_capabilities = DockCapability::kLeft | DockCapability::kRight | 
                                    DockCapability::kBottom | DockCapability::kTab;
    query_plan.default_dock = wxBOTTOM;
    query_plan.size_behavior = SizeBehavior::kStretchHorizontal;
    query_plan.default_size = wxSize(800, 300);
    query_plan.allow_multiple = true;
    query_plan.can_tab = true;
    registerWindowType(query_plan);
    
    // Output Panel
    WindowTypeDefinition output;
    output.type_id = WindowTypes::kOutputPanel;
    output.display_name = "Output";
    output.description = "Messages, errors, and execution output";
    output.category = WindowCategory::kOutput;
    output.dock_capabilities = DockCapability::kLeft | DockCapability::kRight | 
                                DockCapability::kBottom | DockCapability::kTab;
    output.default_dock = wxBOTTOM;
    output.size_behavior = SizeBehavior::kStretchHorizontal;
    output.default_size = wxSize(800, 200);
    output.min_size = wxSize(400, 100);
    output.singleton = true;
    output.persist_layout = true;
    registerWindowType(output);
    
    // History Panel
    WindowTypeDefinition history;
    history.type_id = WindowTypeIds::kHistoryPanel;
    history.display_name = "SQL History";
    history.description = "History of executed SQL statements";
    history.category = WindowCategory::kOutput;
    history.dock_capabilities = DockCapability::kLeft | DockCapability::kRight | 
                                 DockCapability::kBottom | DockCapability::kTab;
    history.default_dock = wxBOTTOM;
    history.size_behavior = SizeBehavior::kStretchHorizontal;
    history.default_size = wxSize(800, 200);
    history.singleton = false;
    registerWindowType(history);
}

void WindowTypeCatalog::registerObjectEditorTypes() {
    // Helper lambda to create object editor definitions
    auto registerObjectEditor = [this](const char* id, const char* name, 
                                        const char* object_type, const char* description) {
        WindowTypeDefinition def;
        def.type_id = id;
        def.display_name = name;
        def.description = description;
        def.category = WindowCategory::kEditor;
        def.dock_capabilities = DockCapability::kLeft | DockCapability::kRight | 
                                 DockCapability::kTop | DockCapability::kBottom | 
                                 DockCapability::kFloat | DockCapability::kTab;
        def.default_dock = wxCENTER;
        def.size_behavior = SizeBehavior::kStretchBoth;
        def.default_size = wxSize(800, 600);
        def.min_size = wxSize(500, 400);
        def.allow_multiple = true;
        def.max_instances = 20;
        def.can_tab = true;
        def.persist_layout = true;
        def.menu_set_ids = {"object_editor"};
        def.merge_menus_with_main = true;
        registerWindowType(def);
    };
    
    // Register all database object editors
    registerObjectEditor(WindowTypeIds::kTableEditor, "Table Editor", "TABLE",
                         "Visual table designer with columns, constraints, and indexes");
    
    registerObjectEditor(WindowTypeIds::kViewEditor, "View Editor", "VIEW",
                         "View definition editor with SQL preview");
    
    registerObjectEditor(WindowTypeIds::kProcedureEditor, "Procedure Editor", "PROCEDURE",
                         "Stored procedure editor with syntax highlighting");
    
    registerObjectEditor(WindowTypeIds::kFunctionEditor, "Function Editor", "FUNCTION",
                         "User-defined function editor");
    
    registerObjectEditor(WindowTypeIds::kTriggerEditor, "Trigger Editor", "TRIGGER",
                         "Trigger editor with event selection");
    
    registerObjectEditor(WindowTypeIds::kSequenceEditor, "Sequence Editor", "SEQUENCE",
                         "Sequence/Generator editor");
    
    registerObjectEditor(WindowTypeIds::kGeneratorEditor, "Generator Editor", "GENERATOR",
                         "Firebird generator editor");
    
    registerObjectEditor(WindowTypeIds::kDomainEditor, "Domain Editor", "DOMAIN",
                         "Domain/user-defined type editor");
    
    registerObjectEditor(WindowTypeIds::kIndexEditor, "Index Editor", "INDEX",
                         "Index designer with column selection");
    
    registerObjectEditor(WindowTypeIds::kConstraintEditor, "Constraint Editor", "CONSTRAINT",
                         "Table constraint editor (PK, FK, Check, Unique)");
    
    registerObjectEditor(WindowTypeIds::kExceptionEditor, "Exception Editor", "EXCEPTION",
                         "Database exception editor");
    
    registerObjectEditor(WindowTypeIds::kPackageEditor, "Package Editor", "PACKAGE",
                         "Package editor (header and body)");
    
    registerObjectEditor(WindowTypeIds::kRoleEditor, "Role Editor", "ROLE",
                         "Database role editor with privileges");
    
    registerObjectEditor(WindowTypeIds::kUserEditor, "User Editor", "USER",
                         "Database user editor");
    
    registerObjectEditor(WindowTypeIds::kCharsetEditor, "Character Set Editor", "CHARSET",
                         "Character set editor");
    
    registerObjectEditor(WindowTypeIds::kCollationEditor, "Collation Editor", "COLLATION",
                         "Collation sequence editor");
    
    registerObjectEditor(WindowTypeIds::kFilterEditor, "Filter Editor", "FILTER",
                         "BLOB filter editor");
    
    registerObjectEditor(WindowTypeIds::kShadowEditor, "Shadow Editor", "SHADOW",
                         "Database shadow configuration");
    
    registerObjectEditor(WindowTypeIds::kSynonymEditor, "Synonym Editor", "SYNONYM",
                         "Synonym/alias editor");
}

void WindowTypeCatalog::registerDataEditorTypes() {
    // Table Data Editor
    WindowTypeDefinition table_data;
    table_data.type_id = WindowTypeIds::kTableDataEditor;
    table_data.display_name = "Table Data";
    table_data.description = "Editable data grid for table contents";
    table_data.category = WindowCategory::kEditor;
    table_data.dock_capabilities = DockCapability::kLeft | DockCapability::kRight | 
                                    DockCapability::kTop | DockCapability::kBottom | 
                                    DockCapability::kFloat | DockCapability::kTab;
    table_data.default_dock = wxCENTER;
    table_data.size_behavior = SizeBehavior::kStretchBoth;
    table_data.default_size = wxSize(900, 600);
    table_data.min_size = wxSize(600, 400);
    table_data.allow_multiple = true;
    table_data.max_instances = 15;
    table_data.can_tab = true;
    table_data.menu_set_ids = {"data_editor"};
    registerWindowType(table_data);
    
    // BLOB Editor
    WindowTypeDefinition blob_editor;
    blob_editor.type_id = WindowTypeIds::kBlobEditor;
    blob_editor.display_name = "BLOB Editor";
    blob_editor.description = "Binary/text BLOB content editor";
    blob_editor.category = WindowCategory::kEditor;
    blob_editor.dock_capabilities = DockCapability::kLeft | DockCapability::kRight | 
                                     DockCapability::kTop | DockCapability::kBottom | 
                                     DockCapability::kFloat;
    blob_editor.default_dock = wxCENTER;
    blob_editor.size_behavior = SizeBehavior::kStretchBoth;
    blob_editor.default_size = wxSize(700, 500);
    blob_editor.min_size = wxSize(400, 300);
    blob_editor.allow_multiple = true;
    registerWindowType(blob_editor);
    
    // Array Editor
    WindowTypeDefinition array_editor;
    array_editor.type_id = WindowTypeIds::kArrayEditor;
    array_editor.display_name = "Array Editor";
    array_editor.description = "Array column data editor";
    array_editor.category = WindowCategory::kEditor;
    array_editor.dock_capabilities = DockCapability::kFloat;
    array_editor.default_dock = wxCENTER;
    array_editor.size_behavior = SizeBehavior::kAutoSize;
    array_editor.default_size = wxSize(500, 400);
    array_editor.min_size = wxSize(300, 200);
    array_editor.can_float = true;
    registerWindowType(array_editor);
}

void WindowTypeCatalog::registerPropertyTypes() {
    // Generic Object Properties
    WindowTypeDefinition obj_props;
    obj_props.type_id = WindowTypeIds::kObjectProperties;
    obj_props.display_name = "Properties";
    obj_props.description = "Object properties and metadata";
    obj_props.category = WindowCategory::kInspector;
    obj_props.dock_capabilities = DockCapability::kLeft | DockCapability::kRight | 
                                   DockCapability::kFloat | DockCapability::kTab;
    obj_props.default_dock = wxRIGHT;
    obj_props.size_behavior = SizeBehavior::kStretchVertical;
    obj_props.default_size = wxSize(350, 600);
    obj_props.min_size = wxSize(250, 300);
    obj_props.singleton = false;
    obj_props.can_tab = true;
    registerWindowType(obj_props);
    
    // Database Properties
    WindowTypeDefinition db_props;
    db_props.type_id = WindowTypeIds::kDatabaseProperties;
    db_props.display_name = "Database Properties";
    db_props.description = "Database connection and settings";
    db_props.category = WindowCategory::kInspector;
    db_props.dock_capabilities = DockCapability::kLeft | DockCapability::kRight | 
                                  DockCapability::kFloat;
    db_props.default_dock = wxRIGHT;
    db_props.size_behavior = SizeBehavior::kStretchVertical;
    db_props.default_size = wxSize(400, 600);
    db_props.min_size = wxSize(300, 400);
    db_props.singleton = true;
    registerWindowType(db_props);
}

void WindowTypeCatalog::registerToolTypes() {
    // Icon Bar
    WindowTypeDefinition icon_bar;
    icon_bar.type_id = WindowTypes::kIconBar;
    icon_bar.display_name = "Toolbar";
    icon_bar.description = "Configurable icon toolbar";
    icon_bar.category = WindowCategory::kIconBar;
    icon_bar.dock_capabilities = DockCapability::kLeft | DockCapability::kRight | 
                                  DockCapability::kTop | DockCapability::kBottom;
    icon_bar.default_dock = wxTOP;
    icon_bar.size_behavior = SizeBehavior::kAutoSize;
    icon_bar.default_size = wxSize(800, 40);
    icon_bar.min_size = wxSize(200, 32);
    icon_bar.can_close = false;
    icon_bar.can_float = false;
    icon_bar.singleton = true;
    registerWindowType(icon_bar);
}

void WindowTypeCatalog::registerDialogTypes() {
    auto createDialogDef = [this](const char* id, const char* name, const char* desc,
                                   bool is_modal = true) {
        WindowTypeDefinition def;
        def.type_id = id;
        def.display_name = name;
        def.description = desc;
        def.category = WindowCategory::kDialog;
        def.dock_capabilities = DockCapability::kNone;
        def.size_behavior = SizeBehavior::kAutoSize;
        def.default_size = wxSize(600, 500);
        def.min_size = wxSize(400, 300);
        def.can_close = true;
        def.can_float = true;
        def.singleton = false;
        def.is_modal = is_modal;
        def.is_tool_window = true;
        def.merge_menus_with_main = false;
        registerWindowType(def);
    };
    
    createDialogDef(WindowTypeIds::kSettingsDialog, "Settings", "Application settings dialog");
    createDialogDef(WindowTypeIds::kPreferencesDialog, "Preferences", "User preferences dialog");
    createDialogDef(WindowTypeIds::kConnectionDialog, "Connection", "Database connection dialog");
    createDialogDef(WindowTypeIds::kAboutDialog, "About", "About dialog", false);
    createDialogDef(WindowTypeIds::kExportDialog, "Export", "Data export dialog");
    createDialogDef(WindowTypeIds::kImportDialog, "Import", "Data import dialog");
    createDialogDef(WindowTypeIds::kBackupDialog, "Backup", "Database backup dialog");
    createDialogDef(WindowTypeIds::kRestoreDialog, "Restore", "Database restore dialog");
    createDialogDef(WindowTypeIds::kMonitorDialog, "Monitor", "Database monitor dialog", false);
}

void WindowTypeCatalog::registerPopupTypes() {
    auto createPopupDef = [this](const char* id, const char* name, const char* desc,
                                  wxSize size = wxSize(400, 200)) {
        WindowTypeDefinition def;
        def.type_id = id;
        def.display_name = name;
        def.description = desc;
        def.category = WindowCategory::kPopup;
        def.dock_capabilities = DockCapability::kNone;
        def.size_behavior = SizeBehavior::kFixed;
        def.default_size = size;
        def.min_size = size;
        def.max_size = size;
        def.can_close = true;
        def.can_float = true;
        def.can_tab = false;
        def.singleton = false;
        def.is_popup = true;
        def.is_modal = true;
        def.is_always_on_top = true;
        def.merge_menus_with_main = false;
        def.steal_focus = true;
        registerWindowType(def);
    };
    
    // Message popups
    createPopupDef(WindowTypeIds::kMessagePopup, "Message", "General message popup", wxSize(400, 200));
    createPopupDef(WindowTypeIds::kErrorPopup, "Error", "Error message popup", wxSize(500, 250));
    createPopupDef(WindowTypeIds::kWarningPopup, "Warning", "Warning popup", wxSize(450, 220));
    createPopupDef(WindowTypeIds::kInfoPopup, "Information", "Info popup", wxSize(400, 200));
    
    // Interactive popups
    createPopupDef(WindowTypeIds::kConfirmPopup, "Confirm", "Confirmation dialog", wxSize(400, 180));
    createPopupDef(WindowTypeIds::kInputPopup, "Input", "Input prompt popup", wxSize(450, 150));
    
    // Progress popups
    createPopupDef(WindowTypeIds::kProgressPopup, "Progress", "Progress indicator", wxSize(400, 120));
    createPopupDef(WindowTypeIds::kWaitPopup, "Wait", "Please wait indicator", wxSize(300, 100));
}

void WindowTypeCatalog::registerSpecialTypes() {
    // Splash Screen
    WindowTypeDefinition splash;
    splash.type_id = WindowTypeIds::kSplashScreen;
    splash.display_name = "Splash";
    splash.description = "Application startup splash screen";
    splash.category = WindowCategory::kPopup;
    splash.dock_capabilities = DockCapability::kNone;
    splash.size_behavior = SizeBehavior::kFixed;
    splash.default_size = wxSize(500, 300);
    splash.min_size = wxSize(500, 300);
    splash.max_size = wxSize(500, 300);
    splash.can_close = false;
    splash.singleton = true;
    splash.is_always_on_top = true;
    splash.is_popup = true;
    registerWindowType(splash);
    
    // Welcome Screen
    WindowTypeDefinition welcome;
    welcome.type_id = WindowTypeIds::kWelcomeScreen;
    welcome.display_name = "Welcome";
    welcome.description = "Welcome/start page";
    welcome.category = WindowCategory::kEditor;
    welcome.dock_capabilities = DockCapability::kLeft | DockCapability::kRight | 
                                 DockCapability::kTop | DockCapability::kBottom | 
                                 DockCapability::kTab;
    welcome.default_dock = wxCENTER;
    welcome.size_behavior = SizeBehavior::kStretchBoth;
    welcome.default_size = wxSize(800, 600);
    welcome.singleton = false;
    welcome.can_tab = true;
    registerWindowType(welcome);
}

void WindowTypeCatalog::registerWindowType(const WindowTypeDefinition& def) {
    window_types_[def.type_id] = def;
}

WindowTypeDefinition* WindowTypeCatalog::findWindowType(const std::string& type_id) {
    auto it = window_types_.find(type_id);
    if (it != window_types_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<WindowTypeDefinition*> WindowTypeCatalog::getAllWindowTypes() {
    std::vector<WindowTypeDefinition*> result;
    for (auto& [id, def] : window_types_) {
        result.push_back(&def);
    }
    return result;
}

std::vector<WindowTypeDefinition*> WindowTypeCatalog::getWindowTypesByCategory(WindowCategory category) {
    std::vector<WindowTypeDefinition*> result;
    for (auto& [id, def] : window_types_) {
        if (def.category == category) {
            result.push_back(&def);
        }
    }
    return result;
}

bool WindowTypeCatalog::hasWindowType(const std::string& type_id) const {
    return window_types_.find(type_id) != window_types_.end();
}

std::vector<WindowTypeDefinition*> WindowTypeCatalog::getSQLEditors() {
    std::vector<WindowTypeDefinition*> result;
    for (auto& [id, def] : window_types_) {
        if (def.category == WindowCategory::kEditor && 
            (id.find("sql") != std::string::npos || id.find("query") != std::string::npos)) {
            result.push_back(&def);
        }
    }
    return result;
}

std::vector<WindowTypeDefinition*> WindowTypeCatalog::getObjectEditors() {
    std::vector<WindowTypeDefinition*> result;
    for (auto& [id, def] : window_types_) {
        if (def.category == WindowCategory::kEditor && id.find("_editor") != std::string::npos &&
            id.find("sql") == std::string::npos && id.find("data") == std::string::npos) {
            result.push_back(&def);
        }
    }
    return result;
}

std::vector<WindowTypeDefinition*> WindowTypeCatalog::getDataEditors() {
    return getWindowTypesByCategory(WindowCategory::kEditor);
}

std::vector<WindowTypeDefinition*> WindowTypeCatalog::getPropertyWindows() {
    return getWindowTypesByCategory(WindowCategory::kInspector);
}

std::vector<WindowTypeDefinition*> WindowTypeCatalog::getToolWindows() {
    return getWindowTypesByCategory(WindowCategory::kIconBar);
}

std::vector<WindowTypeDefinition*> WindowTypeCatalog::getDialogs() {
    return getWindowTypesByCategory(WindowCategory::kDialog);
}

std::vector<WindowTypeDefinition*> WindowTypeCatalog::getPopups() {
    return getWindowTypesByCategory(WindowCategory::kPopup);
}

bool WindowTypeCatalog::isSQLEditor(const std::string& type_id) {
    auto* def = findWindowType(type_id);
    return def && def->category == WindowCategory::kEditor &&
           (type_id.find("sql") != std::string::npos || type_id.find("query") != std::string::npos);
}

bool WindowTypeCatalog::isObjectEditor(const std::string& type_id) {
    auto* def = findWindowType(type_id);
    return def && def->category == WindowCategory::kEditor &&
           type_id.find("_editor") != std::string::npos &&
           type_id.find("sql") == std::string::npos;
}

bool WindowTypeCatalog::isDataEditor(const std::string& type_id) {
    return type_id == WindowTypeIds::kTableDataEditor || 
           type_id == WindowTypeIds::kBlobEditor ||
           type_id == WindowTypeIds::kArrayEditor;
}

bool WindowTypeCatalog::isDialog(const std::string& type_id) {
    auto* def = findWindowType(type_id);
    return def && def->category == WindowCategory::kDialog;
}

bool WindowTypeCatalog::isPopup(const std::string& type_id) {
    auto* def = findWindowType(type_id);
    return def && (def->category == WindowCategory::kPopup || def->is_popup);
}

bool WindowTypeCatalog::isNavigator(const std::string& type_id) {
    auto* def = findWindowType(type_id);
    return def && def->category == WindowCategory::kNavigator;
}

// ============================================================================
// ObjectTypeMapper Implementation
// ============================================================================

ObjectTypeMapper* ObjectTypeMapper::get() {
    static ObjectTypeMapper instance;
    return &instance;
}

void ObjectTypeMapper::initialize() {
    // Map database object types to window types
    type_to_window_[DatabaseObjectTypes::kTable] = WindowTypeIds::kTableEditor;
    type_to_window_[DatabaseObjectTypes::kView] = WindowTypeIds::kViewEditor;
    type_to_window_[DatabaseObjectTypes::kProcedure] = WindowTypeIds::kProcedureEditor;
    type_to_window_[DatabaseObjectTypes::kFunction] = WindowTypeIds::kFunctionEditor;
    type_to_window_[DatabaseObjectTypes::kTrigger] = WindowTypeIds::kTriggerEditor;
    type_to_window_[DatabaseObjectTypes::kSequence] = WindowTypeIds::kSequenceEditor;
    type_to_window_[DatabaseObjectTypes::kGenerator] = WindowTypeIds::kGeneratorEditor;
    type_to_window_[DatabaseObjectTypes::kDomain] = WindowTypeIds::kDomainEditor;
    type_to_window_[DatabaseObjectTypes::kIndex] = WindowTypeIds::kIndexEditor;
    type_to_window_[DatabaseObjectTypes::kConstraint] = WindowTypeIds::kConstraintEditor;
    type_to_window_[DatabaseObjectTypes::kException] = WindowTypeIds::kExceptionEditor;
    type_to_window_[DatabaseObjectTypes::kPackage] = WindowTypeIds::kPackageEditor;
    type_to_window_[DatabaseObjectTypes::kRole] = WindowTypeIds::kRoleEditor;
    type_to_window_[DatabaseObjectTypes::kUser] = WindowTypeIds::kUserEditor;
    type_to_window_[DatabaseObjectTypes::kCharset] = WindowTypeIds::kCharsetEditor;
    type_to_window_[DatabaseObjectTypes::kCollation] = WindowTypeIds::kCollationEditor;
    type_to_window_[DatabaseObjectTypes::kFilter] = WindowTypeIds::kFilterEditor;
    type_to_window_[DatabaseObjectTypes::kShadow] = WindowTypeIds::kShadowEditor;
    type_to_window_[DatabaseObjectTypes::kSynonym] = WindowTypeIds::kSynonymEditor;
    type_to_window_[DatabaseObjectTypes::kShadow] = WindowTypeIds::kShadowEditor;
    type_to_window_[DatabaseObjectTypes::kSynonym] = WindowTypeIds::kSynonymEditor;
    
    // Display names
    type_to_display_[DatabaseObjectTypes::kTable] = "Table";
    type_to_display_[DatabaseObjectTypes::kView] = "View";
    type_to_display_[DatabaseObjectTypes::kProcedure] = "Procedure";
    type_to_display_[DatabaseObjectTypes::kFunction] = "Function";
    type_to_display_[DatabaseObjectTypes::kTrigger] = "Trigger";
    type_to_display_[DatabaseObjectTypes::kSequence] = "Sequence";
    type_to_display_[DatabaseObjectTypes::kGenerator] = "Generator";
    type_to_display_[DatabaseObjectTypes::kDomain] = "Domain";
    type_to_display_[DatabaseObjectTypes::kIndex] = "Index";
    type_to_display_[DatabaseObjectTypes::kConstraint] = "Constraint";
    type_to_display_[DatabaseObjectTypes::kException] = "Exception";
    type_to_display_[DatabaseObjectTypes::kPackage] = "Package";
    type_to_display_[DatabaseObjectTypes::kRole] = "Role";
    type_to_display_[DatabaseObjectTypes::kUser] = "User";
    type_to_display_[DatabaseObjectTypes::kCharset] = "Character Set";
    type_to_display_[DatabaseObjectTypes::kCollation] = "Collation";
    type_to_display_[DatabaseObjectTypes::kFilter] = "Filter";
    type_to_display_[DatabaseObjectTypes::kShadow] = "Shadow";
    type_to_display_[DatabaseObjectTypes::kSynonym] = "Synonym";
}

std::string ObjectTypeMapper::getEditorWindowType(const std::string& object_type) {
    auto it = type_to_window_.find(object_type);
    if (it != type_to_window_.end()) {
        return it->second;
    }
    return "";
}

std::string ObjectTypeMapper::getObjectTypeDisplayName(const std::string& object_type) {
    auto it = type_to_display_.find(object_type);
    if (it != type_to_display_.end()) {
        return it->second;
    }
    return object_type;
}

std::vector<std::string> ObjectTypeMapper::getAllObjectTypes() {
    std::vector<std::string> result;
    for (const auto& [type, window] : type_to_window_) {
        result.push_back(type);
    }
    return result;
}

void ObjectTypeMapper::registerMapping(const std::string& object_type, 
                                        const std::string& window_type) {
    type_to_window_[object_type] = window_type;
}

}  // namespace scratchrobin::ui
