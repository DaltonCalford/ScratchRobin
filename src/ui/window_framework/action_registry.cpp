/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "ui/window_framework/action_registry.h"
#include "ui/window_framework/window_types.h"
#include "res/icon_manager.h"
#include "core/app_config.h"
#include <algorithm>
#include <fstream>

namespace scratchrobin::ui {

ActionRegistry* ActionRegistry::get() {
    static ActionRegistry instance;
    return &instance;
}

void ActionRegistry::initialize() {
    registerBuiltinActions();
    registerBuiltinIcons();
    registerStandardToolbars();
    loadFromConfig();
}

void ActionRegistry::registerBuiltinActions() {
    registerFileActions();
    registerEditActions();
    registerViewActions();
    registerDatabaseActions();
    registerTransactionActions();
    registerQueryActions();
    registerToolsActions();
    registerWindowActions();
    registerHelpActions();
}

void ActionRegistry::registerFileActions() {
    Action a;
    a.action_id = "file.new_connection"; a.display_name = "New Connection..."; 
    a.description = "Create a new database connection"; a.tooltip = "Create new connection";
    a.category = ActionCategory::kFile; a.default_icon_name = res::IconIds::DOCUMENT_NEW;
    a.accelerator = "Ctrl+N"; registerAction(a);
    
    a = Action();
    a.action_id = "file.open_sql"; a.display_name = "Open SQL Script...";
    a.description = "Open a SQL script file"; a.tooltip = "Open SQL file";
    a.category = ActionCategory::kFile; a.default_icon_name = res::IconIds::DOCUMENT_OPEN;
    a.accelerator = "Ctrl+O"; registerAction(a);
    
    a = Action();
    a.action_id = "file.save_sql"; a.display_name = "Save SQL Script";
    a.description = "Save current SQL script"; a.tooltip = "Save SQL";
    a.category = ActionCategory::kFile; a.default_icon_name = res::IconIds::DOCUMENT_SAVE;
    a.accelerator = "Ctrl+S"; registerAction(a);
    
    a = Action();
    a.action_id = "file.save_as"; a.display_name = "Save As...";
    a.description = "Save SQL script with new name"; a.tooltip = "Save as new file";
    a.category = ActionCategory::kFile; a.default_icon_name = res::IconIds::DOCUMENT_SAVE_AS;
    a.accelerator = "Ctrl+Shift+S"; registerAction(a);
    
    a = Action();
    a.action_id = "file.exit"; a.display_name = "Exit";
    a.description = "Exit the application"; a.tooltip = "Exit application";
    a.category = ActionCategory::kFile; a.default_icon_name = res::IconIds::WIN_CLOSE;
    a.accelerator = "Ctrl+Q"; registerAction(a);
}

void ActionRegistry::registerEditActions() {
    Action a;
    a.action_id = "edit.undo"; a.display_name = "Undo";
    a.description = "Undo last action"; a.tooltip = "Undo";
    a.category = ActionCategory::kEdit; a.default_icon_name = res::IconIds::EDIT_UNDO;
    a.accelerator = "Ctrl+Z"; registerAction(a);
    
    a = Action();
    a.action_id = "edit.redo"; a.display_name = "Redo";
    a.description = "Redo last undone action"; a.tooltip = "Redo";
    a.category = ActionCategory::kEdit; a.default_icon_name = res::IconIds::EDIT_REDO;
    a.accelerator = "Ctrl+Shift+Z"; registerAction(a);
    
    a = Action();
    a.action_id = "edit.cut"; a.display_name = "Cut";
    a.description = "Cut selection to clipboard"; a.tooltip = "Cut";
    a.category = ActionCategory::kEdit; a.default_icon_name = res::IconIds::EDIT_CUT;
    a.accelerator = "Ctrl+X"; registerAction(a);
    
    a = Action();
    a.action_id = "edit.copy"; a.display_name = "Copy";
    a.description = "Copy selection to clipboard"; a.tooltip = "Copy";
    a.category = ActionCategory::kEdit; a.default_icon_name = res::IconIds::EDIT_COPY;
    a.accelerator = "Ctrl+C"; registerAction(a);
    
    a = Action();
    a.action_id = "edit.paste"; a.display_name = "Paste";
    a.description = "Paste from clipboard"; a.tooltip = "Paste";
    a.category = ActionCategory::kEdit; a.default_icon_name = res::IconIds::EDIT_PASTE;
    a.accelerator = "Ctrl+V"; registerAction(a);
    
    a = Action();
    a.action_id = "edit.preferences"; a.display_name = "Preferences...";
    a.description = "Edit application preferences"; a.tooltip = "Preferences";
    a.category = ActionCategory::kEdit; a.default_icon_name = res::IconIds::TOOLS_PREFERENCES;
    registerAction(a);
}

void ActionRegistry::registerViewActions() {
    Action a;
    a.action_id = "view.refresh"; a.display_name = "Refresh";
    a.description = "Refresh current view"; a.tooltip = "Refresh";
    a.category = ActionCategory::kView; a.default_icon_name = res::IconIds::VIEW_REFRESH;
    a.accelerator = "F5"; registerAction(a);
    
    a = Action();
    a.action_id = "view.zoom_in"; a.display_name = "Zoom In";
    a.category = ActionCategory::kView; a.default_icon_name = res::IconIds::VIEW_ZOOM_IN;
    a.accelerator = "Ctrl++"; registerAction(a);
    
    a = Action();
    a.action_id = "view.zoom_out"; a.display_name = "Zoom Out";
    a.category = ActionCategory::kView; a.default_icon_name = res::IconIds::VIEW_ZOOM_OUT;
    a.accelerator = "Ctrl+-"; registerAction(a);
    
    a = Action();
    a.action_id = "view.fullscreen"; a.display_name = "Fullscreen";
    a.category = ActionCategory::kView; a.default_icon_name = res::IconIds::VIEW_FULLSCREEN;
    a.accelerator = "F11"; registerAction(a);
}

void ActionRegistry::registerDatabaseActions() {
    Action a;
    a.action_id = "db.connect"; a.display_name = "Connect";
    a.description = "Connect to database"; a.tooltip = "Connect to database";
    a.category = ActionCategory::kDatabase; a.default_icon_name = res::IconIds::DB_CONNECT;
    registerAction(a);
    
    a = Action();
    a.action_id = "db.disconnect"; a.display_name = "Disconnect";
    a.description = "Disconnect from database"; a.tooltip = "Disconnect from database";
    a.category = ActionCategory::kDatabase; a.default_icon_name = res::IconIds::DB_DISCONNECT;
    registerAction(a);
    
    a = Action();
    a.action_id = "db.new_table"; a.display_name = "New Table...";
    a.description = "Create a new table"; a.tooltip = "Create new table";
    a.category = ActionCategory::kDatabase; a.default_icon_name = res::IconIds::TABLE_NEW;
    a.requires_connection = true; registerAction(a);
    
    a = Action();
    a.action_id = "db.edit_table"; a.display_name = "Edit Table...";
    a.description = "Edit table structure"; a.tooltip = "Edit table";
    a.category = ActionCategory::kDatabase; a.default_icon_name = res::IconIds::TABLE_EDIT;
    a.requires_connection = true; registerAction(a);
    
    a = Action();
    a.action_id = "db.drop_table"; a.display_name = "Drop Table...";
    a.description = "Delete a table"; a.tooltip = "Delete table";
    a.category = ActionCategory::kDatabase; a.default_icon_name = res::IconIds::TABLE_DELETE;
    a.requires_connection = true; registerAction(a);
    
    a = Action();
    a.action_id = "db.new_database"; a.display_name = "New Database...";
    a.description = "Create a new database"; a.tooltip = "Create database";
    a.category = ActionCategory::kDatabase; a.default_icon_name = res::IconIds::DB_NEW;
    registerAction(a);
    
    a = Action();
    a.action_id = "db.backup"; a.display_name = "Backup Database...";
    a.description = "Backup database to file"; a.tooltip = "Backup database";
    a.category = ActionCategory::kDatabase; a.default_icon_name = res::IconIds::DB_BACKUP;
    a.requires_connection = true; registerAction(a);
    
    a = Action();
    a.action_id = "db.restore"; a.display_name = "Restore Database...";
    a.description = "Restore database from backup"; a.tooltip = "Restore database";
    a.category = ActionCategory::kDatabase; a.default_icon_name = res::IconIds::DB_RESTORE;
    a.requires_connection = true; registerAction(a);
}

void ActionRegistry::registerTransactionActions() {
    Action a;
    a.action_id = "tx.start"; a.display_name = "Start Transaction";
    a.description = "Begin a new transaction"; a.tooltip = "Start transaction";
    a.category = ActionCategory::kTransaction; a.default_icon_name = res::IconIds::TX_START;
    a.requires_connection = true; registerAction(a);
    
    a = Action();
    a.action_id = "tx.commit"; a.display_name = "Commit";
    a.description = "Commit current transaction"; a.tooltip = "Commit transaction";
    a.category = ActionCategory::kTransaction; a.default_icon_name = res::IconIds::TX_COMMIT;
    a.requires_connection = true; a.requires_transaction = true; registerAction(a);
    
    a = Action();
    a.action_id = "tx.rollback"; a.display_name = "Rollback";
    a.description = "Rollback current transaction"; a.tooltip = "Rollback transaction";
    a.category = ActionCategory::kTransaction; a.default_icon_name = res::IconIds::TX_ROLLBACK;
    a.requires_connection = true; a.requires_transaction = true; registerAction(a);
    
    a = Action();
    a.action_id = "tx.autocommit"; a.display_name = "Auto Commit";
    a.description = "Toggle auto-commit mode"; a.tooltip = "Toggle auto-commit";
    a.category = ActionCategory::kTransaction; a.default_icon_name = res::IconIds::TX_AUTOCOMMIT;
    a.requires_connection = true; registerAction(a);
}

void ActionRegistry::registerQueryActions() {
    Action a;
    a.action_id = "query.execute"; a.display_name = "Execute";
    a.description = "Execute SQL query"; a.tooltip = "Execute query (F9)";
    a.category = ActionCategory::kQuery; a.default_icon_name = res::IconIds::QUERY_EXECUTE;
    a.requires_connection = true; a.accelerator = "F9"; registerAction(a);
    
    a = Action();
    a.action_id = "query.execute_selection"; a.display_name = "Execute Selection";
    a.description = "Execute selected SQL"; a.tooltip = "Execute selection (Ctrl+F9)";
    a.category = ActionCategory::kQuery; a.default_icon_name = res::IconIds::QUERY_EXECUTE_SELECTION;
    a.requires_connection = true; a.accelerator = "Ctrl+F9"; registerAction(a);
    
    a = Action();
    a.action_id = "query.explain"; a.display_name = "Explain Query";
    a.description = "Show query execution plan"; a.tooltip = "Explain query plan";
    a.category = ActionCategory::kQuery; a.default_icon_name = res::IconIds::QUERY_EXPLAIN;
    a.requires_connection = true; registerAction(a);
    
    a = Action();
    a.action_id = "query.stop"; a.display_name = "Stop Query";
    a.description = "Stop executing query"; a.tooltip = "Stop query";
    a.category = ActionCategory::kQuery; a.default_icon_name = res::IconIds::QUERY_STOP;
    a.requires_connection = true; registerAction(a);
}

void ActionRegistry::registerToolsActions() {
    Action a;
    a.action_id = "tools.import_sql"; a.display_name = "Import SQL...";
    a.description = "Import SQL from file"; a.tooltip = "Import SQL";
    a.category = ActionCategory::kTools; a.default_icon_name = res::IconIds::DOCUMENT_IMPORT;
    a.requires_connection = true; registerAction(a);
    
    a = Action();
    a.action_id = "tools.export_sql"; a.display_name = "Export SQL...";
    a.description = "Export to SQL file"; a.tooltip = "Export SQL";
    a.category = ActionCategory::kTools; a.default_icon_name = res::IconIds::DOCUMENT_EXPORT;
    a.requires_connection = true; registerAction(a);
    
    a = Action();
    a.action_id = "tools.generate_ddl"; a.display_name = "Generate DDL";
    a.description = "Generate DDL script"; a.tooltip = "Generate DDL";
    a.category = ActionCategory::kTools; a.default_icon_name = res::IconIds::TOOLS_GENERATE_DDL;
    a.requires_connection = true; registerAction(a);
}

void ActionRegistry::registerWindowActions() {
    Action a;
    a.action_id = "window.new"; a.display_name = "New Window";
    a.description = "Open new application window"; a.tooltip = "New window";
    a.category = ActionCategory::kWindow; a.default_icon_name = res::IconIds::WIN_NEW;
    registerAction(a);
    
    a = Action();
    a.action_id = "window.close"; a.display_name = "Close Window";
    a.description = "Close current window"; a.tooltip = "Close window";
    a.category = ActionCategory::kWindow; a.default_icon_name = res::IconIds::WIN_CLOSE;
    a.accelerator = "Ctrl+W"; registerAction(a);
}

void ActionRegistry::registerHelpActions() {
    Action a;
    a.action_id = "help.docs"; a.display_name = "Documentation";
    a.description = "Open documentation"; a.tooltip = "Documentation (F1)";
    a.category = ActionCategory::kHelp; a.default_icon_name = res::IconIds::HELP_CONTENTS;
    a.accelerator = "F1"; registerAction(a);
    
    a = Action();
    a.action_id = "help.about"; a.display_name = "About";
    a.description = "About this application"; a.tooltip = "About";
    a.category = ActionCategory::kHelp; a.default_icon_name = res::IconIds::HELP_ABOUT;
    registerAction(a);
}

void ActionRegistry::registerBuiltinIcons() {
    res::IconManager::get()->initialize();
}

void ActionRegistry::registerStandardToolbars() {
    createStandardToolbar();
    createDatabaseToolbar();
    createTransactionToolbar();
    createQueryToolbar();
    createNavigationToolbar();
    
    // Save defaults for reset
    default_toolbars_ = toolbars_;
}

void ActionRegistry::createStandardToolbar() {
    ToolbarDefinition tb;
    tb.toolbar_id = "standard";
    tb.display_name = "Standard";
    tb.description = "Standard file and edit operations";
    tb.is_default = true;
    tb.is_visible = true;
    tb.preferred_dock = wxTOP;
    tb.icon_size = 24;
    tb.show_text_labels = false;
    tb.is_expanded = true;
    
    auto addItem = [&](const std::string& action_id) {
        Action* action = findAction(action_id);
        if (!action) return;
        ToolbarItem item;
        item.item_id = "std_" + action_id;
        item.action_id = action_id;
        item.icon_id = action->default_icon_name;
        tb.items.push_back(item);
    };
    
    addItem("file.new_connection");
    addItem("file.open_sql");
    addItem("file.save_sql");
    tb.items.push_back(ToolbarItem()); tb.items.back().item_id = "std_sep1"; tb.items.back().is_separator = true;
    addItem("edit.cut");
    addItem("edit.copy");
    addItem("edit.paste");
    tb.items.push_back(ToolbarItem()); tb.items.back().item_id = "std_sep2"; tb.items.back().is_separator = true;
    addItem("edit.undo");
    addItem("edit.redo");
    
    registerToolbar(tb);
}

void ActionRegistry::createDatabaseToolbar() {
    ToolbarDefinition tb;
    tb.toolbar_id = "database";
    tb.display_name = "Database";
    tb.description = "Database connection and operations";
    tb.is_default = true;
    tb.is_visible = true;
    tb.preferred_dock = wxTOP;
    tb.icon_size = 24;
    tb.show_text_labels = false;
    tb.is_expanded = true;
    
    auto addItem = [&](const std::string& action_id) {
        Action* action = findAction(action_id);
        if (!action) return;
        ToolbarItem item;
        item.item_id = "db_" + action_id;
        item.action_id = action_id;
        item.icon_id = action->default_icon_name;
        tb.items.push_back(item);
    };
    
    addItem("db.connect");
    addItem("db.disconnect");
    tb.items.push_back(ToolbarItem()); tb.items.back().item_id = "db_sep1"; tb.items.back().is_separator = true;
    addItem("db.new_table");
    addItem("db.edit_table");
    addItem("db.drop_table");
    tb.items.push_back(ToolbarItem()); tb.items.back().item_id = "db_sep2"; tb.items.back().is_separator = true;
    addItem("db.backup");
    addItem("db.restore");
    
    registerToolbar(tb);
}

void ActionRegistry::createTransactionToolbar() {
    ToolbarDefinition tb;
    tb.toolbar_id = "transaction";
    tb.display_name = "Transaction";
    tb.description = "Transaction control operations";
    tb.is_default = true;
    tb.is_visible = true;
    tb.preferred_dock = wxTOP;
    tb.icon_size = 24;
    tb.show_text_labels = false;
    tb.is_expanded = false;  // Collapsed by default
    
    auto addItem = [&](const std::string& action_id) {
        Action* action = findAction(action_id);
        if (!action) return;
        ToolbarItem item;
        item.item_id = "tx_" + action_id;
        item.action_id = action_id;
        item.icon_id = action->default_icon_name;
        tb.items.push_back(item);
    };
    
    addItem("tx.start");
    addItem("tx.commit");
    addItem("tx.rollback");
    tb.items.push_back(ToolbarItem()); tb.items.back().item_id = "tx_sep1"; tb.items.back().is_separator = true;
    addItem("tx.autocommit");
    
    registerToolbar(tb);
}

void ActionRegistry::createQueryToolbar() {
    ToolbarDefinition tb;
    tb.toolbar_id = "query";
    tb.display_name = "Query";
    tb.description = "SQL query execution tools";
    tb.is_default = true;
    tb.is_visible = true;
    tb.preferred_dock = wxTOP;
    tb.icon_size = 24;
    tb.show_text_labels = false;
    tb.is_expanded = true;
    
    auto addItem = [&](const std::string& action_id) {
        Action* action = findAction(action_id);
        if (!action) return;
        ToolbarItem item;
        item.item_id = "qry_" + action_id;
        item.action_id = action_id;
        item.icon_id = action->default_icon_name;
        tb.items.push_back(item);
    };
    
    addItem("query.execute");
    addItem("query.execute_selection");
    tb.items.push_back(ToolbarItem()); tb.items.back().item_id = "qry_sep1"; tb.items.back().is_separator = true;
    addItem("query.explain");
    addItem("query.stop");
    
    registerToolbar(tb);
}

void ActionRegistry::createNavigationToolbar() {
    ToolbarDefinition tb;
    tb.toolbar_id = "navigation";
    tb.display_name = "Navigation";
    tb.description = "Navigation controls";
    tb.is_default = true;
    tb.is_visible = false;  // Hidden by default
    tb.preferred_dock = wxLEFT;
    tb.icon_size = 24;
    tb.show_text_labels = true;
    tb.is_expanded = true;
    
    auto addItem = [&](const std::string& action_id) {
        Action* action = findAction(action_id);
        if (!action) return;
        ToolbarItem item;
        item.item_id = "nav_" + action_id;
        item.action_id = action_id;
        item.icon_id = action->default_icon_name;
        tb.items.push_back(item);
    };
    
    addItem("view.refresh");
    addItem("view.zoom_in");
    addItem("view.zoom_out");
    
    registerToolbar(tb);
}

void ActionRegistry::registerAction(const Action& action) {
    actions_[action.action_id] = action;
}

void ActionRegistry::unregisterAction(const std::string& action_id) {
    actions_.erase(action_id);
}

Action* ActionRegistry::findAction(const std::string& action_id) {
    auto it = actions_.find(action_id);
    if (it != actions_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<Action*> ActionRegistry::getActionsByCategory(ActionCategory cat) {
    std::vector<Action*> result;
    for (auto& [id, action] : actions_) {
        if (action.category == cat) {
            result.push_back(&action);
        }
    }
    std::sort(result.begin(), result.end(), [](Action* a, Action* b) {
        return a->display_name < b->display_name;
    });
    return result;
}

std::vector<Action*> ActionRegistry::getAllActions() {
    std::vector<Action*> result;
    for (auto& [id, action] : actions_) {
        result.push_back(&action);
    }
    std::sort(result.begin(), result.end(), [](Action* a, Action* b) {
        if (a->category != b->category) {
            return a->category < b->category;
        }
        return a->display_name < b->display_name;
    });
    return result;
}

std::vector<ActionCategory> ActionRegistry::getCategoriesWithActions() {
    std::vector<ActionCategory> result;
    for (const auto& [id, action] : actions_) {
        if (std::find(result.begin(), result.end(), action.category) == result.end()) {
            result.push_back(action.category);
        }
    }
    std::sort(result.begin(), result.end());
    return result;
}

std::vector<Action*> ActionRegistry::searchActions(const std::string& query) {
    std::vector<Action*> result;
    std::string lower_query = query;
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);
    
    for (auto& [id, action] : actions_) {
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

std::string ActionRegistry::generateActionId(const std::string& base_name) {
    std::string id = "custom." + base_name;
    std::transform(id.begin(), id.end(), id.begin(), ::tolower);
    for (auto& c : id) {
        if (c == ' ') c = '_';
    }
    
    std::string unique_id = id;
    int suffix = 1;
    while (!isActionIdAvailable(unique_id)) {
        unique_id = id + "_" + std::to_string(suffix++);
    }
    return unique_id;
}

bool ActionRegistry::isActionIdAvailable(const std::string& action_id) const {
    return actions_.find(action_id) == actions_.end();
}

wxBitmapBundle ActionRegistry::getIconBitmap(const std::string& icon_id, const wxSize& size) {
    return res::IconManager::get()->getBitmapBundle(icon_id, size);
}

std::vector<std::string> ActionRegistry::getAvailableIcons() {
    std::vector<std::string> result;
    res::IconManager* mgr = res::IconManager::get();
    for (auto* icon : mgr->getAllIcons()) {
        result.push_back(icon->id);
    }
    return result;
}

std::vector<std::string> ActionRegistry::searchIcons(const std::string& query) {
    std::vector<std::string> result;
    res::IconManager* mgr = res::IconManager::get();
    for (auto* icon : mgr->searchIcons(query)) {
        result.push_back(icon->id);
    }
    return result;
}

void ActionRegistry::registerToolbar(const ToolbarDefinition& toolbar) {
    toolbars_[toolbar.toolbar_id] = toolbar;
}

void ActionRegistry::unregisterToolbar(const std::string& toolbar_id) {
    toolbars_.erase(toolbar_id);
}

ToolbarDefinition* ActionRegistry::findToolbar(const std::string& toolbar_id) {
    auto it = toolbars_.find(toolbar_id);
    if (it != toolbars_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<ToolbarDefinition*> ActionRegistry::getAllToolbars() {
    std::vector<ToolbarDefinition*> result;
    for (auto& [id, toolbar] : toolbars_) {
        result.push_back(&toolbar);
    }
    std::sort(result.begin(), result.end(), [](ToolbarDefinition* a, ToolbarDefinition* b) {
        return a->display_name < b->display_name;
    });
    return result;
}

std::vector<ToolbarDefinition*> ActionRegistry::searchToolbars(const std::string& query) {
    std::vector<ToolbarDefinition*> result;
    std::string lower_query = query;
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);
    
    for (auto& [id, toolbar] : toolbars_) {
        std::string lower_name = toolbar.display_name;
        std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
        if (lower_name.find(lower_query) != std::string::npos) {
            result.push_back(&toolbar);
        }
    }
    return result;
}

std::string ActionRegistry::generateToolbarId(const std::string& base_name) {
    std::string id = "custom." + base_name;
    std::transform(id.begin(), id.end(), id.begin(), ::tolower);
    for (auto& c : id) {
        if (c == ' ') c = '_';
    }
    
    std::string unique_id = id;
    int suffix = 1;
    while (findToolbar(unique_id) != nullptr) {
        unique_id = id + "_" + std::to_string(suffix++);
    }
    return unique_id;
}

void ActionRegistry::addItemToToolbar(const std::string& toolbar_id, const ToolbarItem& item, int position) {
    ToolbarDefinition* toolbar = findToolbar(toolbar_id);
    if (!toolbar) return;
    
    if (position >= 0 && position <= (int)toolbar->items.size()) {
        toolbar->items.insert(toolbar->items.begin() + position, item);
    } else {
        toolbar->items.push_back(item);
    }
}

void ActionRegistry::removeItemFromToolbar(const std::string& toolbar_id, const std::string& item_id) {
    ToolbarDefinition* toolbar = findToolbar(toolbar_id);
    if (!toolbar) return;
    
    toolbar->items.erase(
        std::remove_if(toolbar->items.begin(), toolbar->items.end(),
            [&item_id](const ToolbarItem& item) { return item.item_id == item_id; }),
        toolbar->items.end()
    );
}

void ActionRegistry::moveItemInToolbar(const std::string& toolbar_id, const std::string& item_id, int new_position) {
    ToolbarDefinition* toolbar = findToolbar(toolbar_id);
    if (!toolbar) return;
    
    auto it = std::find_if(toolbar->items.begin(), toolbar->items.end(),
        [&item_id](const ToolbarItem& item) { return item.item_id == item_id; });
    
    if (it == toolbar->items.end()) return;
    
    ToolbarItem item = *it;
    toolbar->items.erase(it);
    
    if (new_position < 0 || new_position > (int)toolbar->items.size()) {
        new_position = toolbar->items.size();
    }
    toolbar->items.insert(toolbar->items.begin() + new_position, item);
}

ToolbarItem* ActionRegistry::findToolbarItem(const std::string& toolbar_id, const std::string& item_id) {
    ToolbarDefinition* toolbar = findToolbar(toolbar_id);
    if (!toolbar) return nullptr;
    
    for (auto& item : toolbar->items) {
        if (item.item_id == item_id) {
            return &item;
        }
    }
    return nullptr;
}

bool ActionRegistry::exportToolbarsToFile(const std::string& file_path) {
    // TODO: Implement JSON export using nlohmann/json or similar
    return false;
}

bool ActionRegistry::importToolbarsFromFile(const std::string& file_path) {
    // TODO: Implement JSON import using nlohmann/json or similar
    return false;
}

void ActionRegistry::loadFromConfig() {
    // TODO: Load from AppConfig
}

void ActionRegistry::saveToConfig() {
    // TODO: Save to AppConfig
}

void ActionRegistry::executeAction(const std::string& action_id) {
    Action* action = findAction(action_id);
    if (action && action->callback) {
        action->callback();
    }
}

bool ActionRegistry::canExecute(const std::string& action_id) {
    return findAction(action_id) != nullptr;
}

void ActionRegistry::resetToDefaults() {
    toolbars_ = default_toolbars_;
    // Remove custom actions
    auto it = actions_.begin();
    while (it != actions_.end()) {
        if (!it->second.is_builtin) {
            it = actions_.erase(it);
        } else {
            ++it;
        }
    }
}

}  // namespace scratchrobin::ui
