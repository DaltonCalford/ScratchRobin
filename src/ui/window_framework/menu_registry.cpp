/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "ui/window_framework/menu_registry.h"
#include "ui/window_framework/action_registry.h"
#include <algorithm>
#include <fstream>
#include <chrono>

namespace scratchrobin::ui {

// ============================================================================
// MenuItem helpers
// ============================================================================

// isSeparator() and isSubmenu() are defined inline in header

// ============================================================================
// MenuDefinition implementation
// ============================================================================

MenuItem* MenuDefinition::findItem(const std::string& item_id) {
    for (auto& item : items) {
        if (item.item_id == item_id) return &item;
        // Search in children if submenu
        if (!item.children.empty()) {
            for (auto& child : item.children) {
                if (child.item_id == item_id) return &child;
            }
        }
    }
    return nullptr;
}

const MenuItem* MenuDefinition::findItem(const std::string& item_id) const {
    for (const auto& item : items) {
        if (item.item_id == item_id) return &item;
        if (!item.children.empty()) {
            for (const auto& child : item.children) {
                if (child.item_id == item_id) return &child;
            }
        }
    }
    return nullptr;
}

bool MenuDefinition::moveItemUp(const std::string& item_id) {
    for (size_t i = 1; i < items.size(); i++) {
        if (items[i].item_id == item_id) {
            std::swap(items[i], items[i-1]);
            return true;
        }
    }
    return false;
}

bool MenuDefinition::moveItemDown(const std::string& item_id) {
    for (size_t i = 0; i < items.size() - 1; i++) {
        if (items[i].item_id == item_id) {
            std::swap(items[i], items[i+1]);
            return true;
        }
    }
    return false;
}

void MenuDefinition::removeItem(const std::string& item_id) {
    items.erase(
        std::remove_if(items.begin(), items.end(),
            [&item_id](const MenuItem& item) { return item.item_id == item_id; }),
        items.end()
    );
}

void MenuDefinition::addItem(const MenuItem& item, int position) {
    if (position >= 0 && position <= (int)items.size()) {
        items.insert(items.begin() + position, item);
    } else {
        items.push_back(item);
    }
}

// ============================================================================
// WindowTypeMenuSet implementation
// ============================================================================

bool WindowTypeMenuSet::hasMenu(const std::string& menu_id) const {
    return std::find(menu_ids.begin(), menu_ids.end(), menu_id) != menu_ids.end();
}

void WindowTypeMenuSet::addMenu(const std::string& menu_id) {
    if (!hasMenu(menu_id)) {
        menu_ids.push_back(menu_id);
    }
}

void WindowTypeMenuSet::removeMenu(const std::string& menu_id) {
    menu_ids.erase(
        std::remove(menu_ids.begin(), menu_ids.end(), menu_id),
        menu_ids.end()
    );
}

// ============================================================================
// MenuRegistry implementation
// ============================================================================

MenuRegistry* MenuRegistry::get() {
    static MenuRegistry instance;
    return &instance;
}

void MenuRegistry::initialize() {
    registerBuiltinMenus();
    createStandardMenus();
    loadFromConfig();
}

void MenuRegistry::registerBuiltinMenus() {
    registerFileMenu();
    registerEditMenu();
    registerViewMenu();
    registerDatabaseMenu();
    registerQueryMenu();
    registerTransactionMenu();
    registerToolsMenu();
    registerWindowMenu();
    registerHelpMenu();
    
    // Save defaults
    default_menus_ = menus_;
}

void MenuRegistry::registerFileMenu() {
    MenuDefinition menu;
    menu.menu_id = "file";
    menu.display_name = "&File";
    menu.sort_order = 10;
    
    registerMenuItem(menu, "file.new_connection", "&New Connection...", "Ctrl+N");
    registerMenuItem(menu, "file.open_sql", "&Open SQL Script...", "Ctrl+O");
    registerSeparator(menu);
    registerMenuItem(menu, "file.save_sql", "&Save", "Ctrl+S");
    registerMenuItem(menu, "file.save_as", "Save &As...", "Ctrl+Shift+S");
    registerSeparator(menu);
    
    // Import/Export submenu
    MenuItem import_export;
    import_export.item_id = "file.import_export";
    import_export.type = MenuItemType::kSubmenu;
    import_export.custom_label = "Import/Export";
    
    MenuItem imp;
    imp.item_id = "file.import_export.import";
    imp.action_id = "tools.import_sql";
    imp.custom_label = "&Import SQL...";
    import_export.children.push_back(imp);
    
    MenuItem exp;
    exp.item_id = "file.import_export.export";
    exp.action_id = "tools.export_sql";
    exp.custom_label = "&Export SQL...";
    import_export.children.push_back(exp);
    
    menu.items.push_back(import_export);
    registerSeparator(menu);
    
    registerMenuItem(menu, "file.exit", "E&xit", "Ctrl+Q");
    
    registerMenu(menu);
}

void MenuRegistry::registerEditMenu() {
    MenuDefinition menu;
    menu.menu_id = "edit";
    menu.display_name = "&Edit";
    menu.sort_order = 20;
    
    registerMenuItem(menu, "edit.undo", "&Undo", "Ctrl+Z");
    registerMenuItem(menu, "edit.redo", "&Redo", "Ctrl+Shift+Z");
    registerSeparator(menu);
    registerMenuItem(menu, "edit.cut", "Cu&t", "Ctrl+X");
    registerMenuItem(menu, "edit.copy", "&Copy", "Ctrl+C");
    registerMenuItem(menu, "edit.paste", "&Paste", "Ctrl+V");
    registerSeparator(menu);
    registerMenuItem(menu, "edit.preferences", "Prefere&nces...");
    
    registerMenu(menu);
}

void MenuRegistry::registerViewMenu() {
    MenuDefinition menu;
    menu.menu_id = "view";
    menu.display_name = "&View";
    menu.sort_order = 30;
    
    registerMenuItem(menu, "view.refresh", "&Refresh", "F5");
    registerSeparator(menu);
    registerMenuItem(menu, "view.zoom_in", "Zoom &In", "Ctrl++");
    registerMenuItem(menu, "view.zoom_out", "Zoom &Out", "Ctrl+-");
    registerSeparator(menu);
    registerMenuItem(menu, "view.fullscreen", "&Fullscreen", "F11");
    
    registerMenu(menu);
}

void MenuRegistry::registerDatabaseMenu() {
    MenuDefinition menu;
    menu.menu_id = "database";
    menu.display_name = "&Database";
    menu.sort_order = 40;
    
    registerMenuItem(menu, "db.connect", "&Connect");
    registerMenuItem(menu, "db.disconnect", "&Disconnect");
    registerSeparator(menu);
    registerMenuItem(menu, "db.new_database", "&New Database...");
    registerSeparator(menu);
    registerMenuItem(menu, "db.new_table", "New &Table...");
    registerMenuItem(menu, "db.edit_table", "&Edit Table...");
    registerMenuItem(menu, "db.drop_table", "&Drop Table...");
    registerSeparator(menu);
    registerMenuItem(menu, "db.backup", "&Backup Database...");
    registerMenuItem(menu, "db.restore", "&Restore Database...");
    
    registerMenu(menu);
}

void MenuRegistry::registerQueryMenu() {
    MenuDefinition menu;
    menu.menu_id = "query";
    menu.display_name = "&Query";
    menu.sort_order = 50;
    
    registerMenuItem(menu, "query.execute", "&Execute", "F9");
    registerMenuItem(menu, "query.execute_selection", "Execute &Selection", "Ctrl+F9");
    registerSeparator(menu);
    registerMenuItem(menu, "query.explain", "&Explain Query");
    registerMenuItem(menu, "query.stop", "&Stop Query");
    
    registerMenu(menu);
}

void MenuRegistry::registerTransactionMenu() {
    MenuDefinition menu;
    menu.menu_id = "transaction";
    menu.display_name = "&Transaction";
    menu.sort_order = 60;
    
    registerMenuItem(menu, "tx.start", "&Start Transaction");
    registerMenuItem(menu, "tx.commit", "&Commit");
    registerMenuItem(menu, "tx.rollback", "&Rollback");
    registerSeparator(menu);
    registerMenuItem(menu, "tx.autocommit", "&Auto Commit");
    
    registerMenu(menu);
}

void MenuRegistry::registerToolsMenu() {
    MenuDefinition menu;
    menu.menu_id = "tools";
    menu.display_name = "&Tools";
    menu.sort_order = 70;
    
    registerMenuItem(menu, "tools.generate_ddl", "&Generate DDL...");
    registerSeparator(menu);
    registerMenuItem(menu, "edit.preferences", "&Preferences...");
    
    registerMenu(menu);
}

void MenuRegistry::registerWindowMenu() {
    MenuDefinition menu;
    menu.menu_id = "window";
    menu.display_name = "&Window";
    menu.sort_order = 80;
    
    registerMenuItem(menu, "window.new", "&New Window");
    registerMenuItem(menu, "window.close", "&Close Window", "Ctrl+W");
    
    registerMenu(menu);
}

void MenuRegistry::registerHelpMenu() {
    MenuDefinition menu;
    menu.menu_id = "help";
    menu.display_name = "&Help";
    menu.sort_order = 90;
    
    registerMenuItem(menu, "help.docs", "&Documentation", "F1");
    registerSeparator(menu);
    registerMenuItem(menu, "help.about", "&About...");
    
    registerMenu(menu);
}

void MenuRegistry::createStandardMenus() {
    // Register window type menu sets
    
    // Main window - always present
    WindowTypeMenuSet main;
    main.window_type_id = "main";
    main.display_name = "Main Window";
    main.menu_ids = {"file", "edit", "view", "window", "help"};
    main.merge_with_main = false;  // Main is the base
    registerWindowTypeMenuSet(main);
    
    // SQL Editor
    WindowTypeMenuSet sql;
    sql.window_type_id = "sql_editor";
    sql.display_name = "SQL Editor";
    sql.menu_ids = {"query", "transaction"};
    sql.merge_with_main = true;
    registerWindowTypeMenuSet(sql);
    
    // Settings/Preferences dialog
    WindowTypeMenuSet settings;
    settings.window_type_id = "settings";
    settings.display_name = "Settings";
    settings.menu_ids = {};  // Settings doesn't add menus, may remove some
    settings.merge_with_main = true;
    registerWindowTypeMenuSet(settings);
    
    // Database browser/navigator
    WindowTypeMenuSet navigator;
    navigator.window_type_id = "navigator";
    navigator.display_name = "Navigator";
    navigator.menu_ids = {"database"};
    navigator.merge_with_main = true;
    registerWindowTypeMenuSet(navigator);
    
    // Object metadata/editor
    WindowTypeMenuSet object_editor;
    object_editor.window_type_id = "object_editor";
    object_editor.display_name = "Object Editor";
    object_editor.menu_ids = {"database", "tools"};
    object_editor.merge_with_main = true;
    registerWindowTypeMenuSet(object_editor);
}

void MenuRegistry::registerMenuItem(MenuDefinition& menu, const std::string& action_id, 
                                     const std::string& custom_label,
                                     const std::string& custom_accel) {
    MenuItem item;
    item.item_id = menu.menu_id + "_" + action_id;
    item.type = MenuItemType::kAction;
    item.action_id = action_id;
    item.custom_label = custom_label;
    item.custom_accel = custom_accel;
    
    // Get default icon from action
    Action* action = ActionRegistry::get()->findAction(action_id);
    if (action) {
        item.icon_id = action->default_icon_name;
        if (item.custom_label.empty()) {
            item.custom_label = action->display_name;
        }
    }
    
    menu.items.push_back(item);
}

void MenuRegistry::registerSeparator(MenuDefinition& menu) {
    MenuItem sep;
    sep.item_id = menu.menu_id + "_sep_" + std::to_string(menu.items.size());
    sep.type = MenuItemType::kSeparator;
    menu.items.push_back(sep);
}

void MenuRegistry::registerSubmenu(MenuDefinition& parent, const std::string& submenu_id,
                                    const std::string& display_name,
                                    const std::vector<std::pair<std::string, std::string>>& actions) {
    MenuItem submenu;
    submenu.item_id = parent.menu_id + "_" + submenu_id;
    submenu.type = MenuItemType::kSubmenu;
    submenu.custom_label = display_name;
    
    for (const auto& [action_id, label] : actions) {
        MenuItem item;
        item.item_id = submenu.item_id + "_" + action_id;
        item.type = MenuItemType::kAction;
        item.action_id = action_id;
        item.custom_label = label;
        submenu.children.push_back(item);
    }
    
    parent.items.push_back(submenu);
}

void MenuRegistry::registerMenu(const MenuDefinition& menu) {
    menus_[menu.menu_id] = menu;
}

void MenuRegistry::unregisterMenu(const std::string& menu_id) {
    menus_.erase(menu_id);
}

MenuDefinition* MenuRegistry::findMenu(const std::string& menu_id) {
    auto it = menus_.find(menu_id);
    if (it != menus_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<MenuDefinition*> MenuRegistry::getAllMenus() {
    std::vector<MenuDefinition*> result;
    for (auto& [id, menu] : menus_) {
        result.push_back(&menu);
    }
    std::sort(result.begin(), result.end(), [](MenuDefinition* a, MenuDefinition* b) {
        return a->sort_order < b->sort_order;
    });
    return result;
}

std::vector<MenuDefinition*> MenuRegistry::searchMenus(const std::string& query) {
    std::vector<MenuDefinition*> result;
    std::string lower_query = query;
    std::transform(lower_query.begin(), lower_query.end(), lower_query.begin(), ::tolower);
    
    for (auto& [id, menu] : menus_) {
        std::string lower_name = menu.display_name;
        std::transform(lower_name.begin(), lower_name.end(), lower_name.begin(), ::tolower);
        if (lower_name.find(lower_query) != std::string::npos) {
            result.push_back(&menu);
        }
    }
    return result;
}

void MenuRegistry::registerWindowTypeMenuSet(const WindowTypeMenuSet& menu_set) {
    window_type_menus_[menu_set.window_type_id] = menu_set;
}

void MenuRegistry::unregisterWindowTypeMenuSet(const std::string& window_type_id) {
    window_type_menus_.erase(window_type_id);
}

WindowTypeMenuSet* MenuRegistry::findWindowTypeMenuSet(const std::string& window_type_id) {
    auto it = window_type_menus_.find(window_type_id);
    if (it != window_type_menus_.end()) {
        return &it->second;
    }
    return nullptr;
}

std::vector<WindowTypeMenuSet*> MenuRegistry::getAllWindowTypeMenuSets() {
    std::vector<WindowTypeMenuSet*> result;
    for (auto& [id, menu_set] : window_type_menus_) {
        result.push_back(&menu_set);
    }
    return result;
}

std::vector<MenuDefinition*> MenuRegistry::getMenusForWindowType(const std::string& window_type_id) {
    std::vector<MenuDefinition*> result;
    WindowTypeMenuSet* menu_set = findWindowTypeMenuSet(window_type_id);
    if (!menu_set) return result;
    
    for (const auto& menu_id : menu_set->menu_ids) {
        MenuDefinition* menu = findMenu(menu_id);
        if (menu) {
            result.push_back(menu);
        }
    }
    return result;
}

std::string MenuRegistry::generateMenuId(const std::string& base_name) {
    std::string id = "custom." + base_name;
    std::transform(id.begin(), id.end(), id.begin(), ::tolower);
    for (auto& c : id) {
        if (c == ' ') c = '_';
    }
    
    std::string unique_id = id;
    int suffix = 1;
    while (findMenu(unique_id) != nullptr) {
        unique_id = id + "_" + std::to_string(suffix++);
    }
    return unique_id;
}

std::string MenuRegistry::generateItemId(const std::string& menu_id, const std::string& base_name) {
    return menu_id + "_" + base_name + "_" + std::to_string(std::chrono::system_clock::now().time_since_epoch().count());
}

bool MenuRegistry::exportMenusToFile(const std::string& file_path) {
    // TODO: Implement JSON export
    return false;
}

bool MenuRegistry::importMenusFromFile(const std::string& file_path) {
    // TODO: Implement JSON import
    return false;
}

void MenuRegistry::saveToConfig() {
    // TODO: Implement
}

void MenuRegistry::loadFromConfig() {
    // TODO: Implement
}

void MenuRegistry::resetToDefaults() {
    menus_ = default_menus_;
}

}  // namespace scratchrobin::ui
