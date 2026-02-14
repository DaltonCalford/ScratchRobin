// Copyright (c) 2026, Dennis C. Alfonso
// Licensed under the MIT License. See LICENSE file in the project root.
#include "toolbar_manager.h"
#include "../../core/simple_json.h"
#include <algorithm>
#include <fstream>

namespace scratchrobin {
namespace ui {
namespace toolbar {

// Event definitions
wxDEFINE_EVENT(EVT_TOOLBAR_CHANGED, wxCommandEvent);
wxDEFINE_EVENT(EVT_TOOLBAR_ITEM_CHANGED, wxCommandEvent);

// Singleton instance
ToolbarManager* ToolbarManager::instance_ = nullptr;

ToolbarManager& ToolbarManager::GetInstance() {
    if (!instance_) {
        instance_ = new ToolbarManager();
    }
    return *instance_;
}

void ToolbarManager::DestroyInstance() {
    delete instance_;
    instance_ = nullptr;
}

ToolbarManager::ToolbarManager() : event_handler_(std::make_unique<wxEvtHandler>()) {}

ToolbarManager::~ToolbarManager() = default;

void ToolbarManager::Initialize() {
    RegisterDefaultTemplates();
    RegisterDefaultPresets();
    CreateDefaultToolbars();
}

void ToolbarManager::Shutdown() {
    toolbars_.clear();
    templates_.clear();
    presets_.clear();
    action_handlers_.clear();
    action_validators_.clear();
}

void ToolbarManager::RegisterToolbar(const ToolbarDefinition& definition) {
    toolbars_[definition.id] = definition;
    NotifyToolbarChanged(definition.id);
}

void ToolbarManager::UnregisterToolbar(const std::string& toolbar_id) {
    toolbars_.erase(toolbar_id);
    NotifyToolbarChanged(toolbar_id);
}

ToolbarDefinition* ToolbarManager::GetToolbar(const std::string& toolbar_id) {
    auto it = toolbars_.find(toolbar_id);
    return (it != toolbars_.end()) ? &it->second : nullptr;
}

const ToolbarDefinition* ToolbarManager::GetToolbar(const std::string& toolbar_id) const {
    auto it = toolbars_.find(toolbar_id);
    return (it != toolbars_.end()) ? &it->second : nullptr;
}

std::vector<ToolbarDefinition*> ToolbarManager::GetAllToolbars() {
    std::vector<ToolbarDefinition*> result;
    for (auto& [id, def] : toolbars_) {
        result.push_back(&def);
    }
    return result;
}

std::vector<ToolbarDefinition*> ToolbarManager::GetToolbarsForPosition(ToolbarPosition position) {
    std::vector<ToolbarDefinition*> result;
    for (auto& [id, def] : toolbars_) {
        if (def.position == position && def.is_visible) {
            result.push_back(&def);
        }
    }
    return result;
}

std::vector<ToolbarDefinition*> ToolbarManager::GetToolbarsForScope(const std::string& context) {
    std::vector<ToolbarDefinition*> result;
    for (auto& [id, def] : toolbars_) {
        if (def.scope == ToolbarScope::Global) {
            result.push_back(&def);
        } else if (def.scope == ToolbarScope::DocumentType || 
                   def.scope == ToolbarScope::ConnectionType) {
            for (const auto& ctx : def.scope_contexts) {
                if (ctx == context) {
                    result.push_back(&def);
                    break;
                }
            }
        }
    }
    return result;
}

bool ToolbarManager::AddItem(const std::string& toolbar_id, const ToolbarItem& item, size_t index) {
    auto* toolbar = GetToolbar(toolbar_id);
    if (!toolbar || !toolbar->CanModify()) {
        return false;
    }

    if (index >= toolbar->items.size()) {
        toolbar->items.push_back(item);
    } else {
        toolbar->items.insert(toolbar->items.begin() + index, item);
    }
    NotifyItemChanged(toolbar_id, item.id);
    return true;
}

bool ToolbarManager::RemoveItem(const std::string& toolbar_id, const std::string& item_id) {
    auto* toolbar = GetToolbar(toolbar_id);
    if (!toolbar || !toolbar->CanModify()) {
        return false;
    }

    auto it = std::remove_if(toolbar->items.begin(), toolbar->items.end(),
        [&item_id](const ToolbarItem& item) { return item.id == item_id; });
    
    if (it != toolbar->items.end()) {
        toolbar->items.erase(it, toolbar->items.end());
        NotifyItemChanged(toolbar_id, item_id);
        return true;
    }
    return false;
}

bool ToolbarManager::MoveItem(const std::string& toolbar_id, const std::string& item_id, size_t new_index) {
    auto* toolbar = GetToolbar(toolbar_id);
    if (!toolbar || !toolbar->CanModify()) {
        return false;
    }

    auto it = std::find_if(toolbar->items.begin(), toolbar->items.end(),
        [&item_id](const ToolbarItem& item) { return item.id == item_id; });
    
    if (it == toolbar->items.end()) {
        return false;
    }

    // Remove from current position
    ToolbarItem item = *it;
    toolbar->items.erase(it);

    // Insert at new position
    if (new_index >= toolbar->items.size()) {
        toolbar->items.push_back(item);
    } else {
        toolbar->items.insert(toolbar->items.begin() + new_index, item);
    }
    
    NotifyItemChanged(toolbar_id, item_id);
    return true;
}

bool ToolbarManager::UpdateItem(const std::string& toolbar_id, const ToolbarItem& item) {
    auto* toolbar = GetToolbar(toolbar_id);
    if (!toolbar || !toolbar->CanModify()) {
        return false;
    }

    for (auto& existing : toolbar->items) {
        if (existing.id == item.id) {
            existing = item;
            NotifyItemChanged(toolbar_id, item.id);
            return true;
        }
    }
    return false;
}

void ToolbarManager::RegisterItemTemplate(const ToolbarItemTemplate& tmpl) {
    templates_[tmpl.id] = tmpl;
}

std::vector<ToolbarItemTemplate> ToolbarManager::GetItemTemplates() const {
    std::vector<ToolbarItemTemplate> result;
    for (const auto& [id, tmpl] : templates_) {
        result.push_back(tmpl);
    }
    return result;
}

std::vector<ToolbarItemTemplate> ToolbarManager::GetItemTemplatesByCategory(const std::string& category) const {
    std::vector<ToolbarItemTemplate> result;
    for (const auto& [id, tmpl] : templates_) {
        if (tmpl.category == category) {
            result.push_back(tmpl);
        }
    }
    return result;
}

std::vector<std::string> ToolbarManager::GetItemCategories() const {
    std::vector<std::string> categories;
    for (const auto& [id, tmpl] : templates_) {
        if (std::find(categories.begin(), categories.end(), tmpl.category) == categories.end()) {
            categories.push_back(tmpl.category);
        }
    }
    return categories;
}

ToolbarDefinition* ToolbarManager::CreateCustomToolbar(const std::string& name) {
    std::string id = "custom." + std::to_string(++custom_toolbar_counter_);
    
    ToolbarDefinition def;
    def.id = id;
    def.name = name;
    def.description = "Custom user-defined toolbar";
    def.position = ToolbarPosition::Top;
    def.is_locked = false;
    def.scope = ToolbarScope::Global;
    
    toolbars_[id] = def;
    NotifyToolbarChanged(id);
    return &toolbars_[id];
}

bool ToolbarManager::DeleteCustomToolbar(const std::string& toolbar_id) {
    auto* toolbar = GetToolbar(toolbar_id);
    if (!toolbar || !toolbar->IsCustom()) {
        return false;
    }
    
    toolbars_.erase(toolbar_id);
    NotifyToolbarChanged(toolbar_id);
    return true;
}

bool ToolbarManager::RenameCustomToolbar(const std::string& toolbar_id, const std::string& new_name) {
    auto* toolbar = GetToolbar(toolbar_id);
    if (!toolbar || !toolbar->IsCustom()) {
        return false;
    }
    
    toolbar->name = new_name;
    NotifyToolbarChanged(toolbar_id);
    return true;
}

void ToolbarManager::RegisterPreset(const ToolbarPreset& preset) {
    presets_[preset.id] = preset;
}

bool ToolbarManager::ApplyPreset(const std::string& preset_id) {
    auto* preset = GetPreset(preset_id);
    if (!preset) {
        return false;
    }
    
    // Hide all toolbars first
    for (auto& [id, def] : toolbars_) {
        def.is_visible = false;
    }
    
    // Show toolbars in preset
    for (const auto& toolbar_id : preset->toolbar_ids) {
        auto* toolbar = GetToolbar(toolbar_id);
        if (toolbar) {
            toolbar->is_visible = true;
        }
    }
    
    NotifyToolbarChanged("");
    return true;
}

ToolbarPreset* ToolbarManager::GetPreset(const std::string& preset_id) {
    auto it = presets_.find(preset_id);
    return (it != presets_.end()) ? &it->second : nullptr;
}

std::vector<ToolbarPreset> ToolbarManager::GetAllPresets() const {
    std::vector<ToolbarPreset> result;
    for (const auto& [id, preset] : presets_) {
        result.push_back(preset);
    }
    return result;
}

void ToolbarManager::CreateDefaultToolbars() {
    // Main Standard Toolbar
    {
        ToolbarDefinition def;
        def.id = "main.standard";
        def.name = "Standard";
        def.description = "Main standard toolbar";
        def.position = ToolbarPosition::Top;
        def.is_locked = true;
        def.scope = ToolbarScope::Global;
        
        def.items.push_back(ToolbarItem{"new", "file.new", ToolbarItemType::Icon, "file_new", "New", "Create new file"});
        def.items.push_back(ToolbarItem{"open", "file.open", ToolbarItemType::Icon, "file_open", "Open", "Open file"});
        def.items.push_back(ToolbarItem{"save", "file.save", ToolbarItemType::Icon, "file_save", "Save", "Save file"});
        def.items.push_back(ToolbarItem{"sep1", "", ToolbarItemType::Separator});
        def.items.push_back(ToolbarItem{"cut", "edit.cut", ToolbarItemType::Icon, "edit_cut", "Cut", "Cut selection"});
        def.items.push_back(ToolbarItem{"copy", "edit.copy", ToolbarItemType::Icon, "edit_copy", "Copy", "Copy selection"});
        def.items.push_back(ToolbarItem{"paste", "edit.paste", ToolbarItemType::Icon, "edit_paste", "Paste", "Paste from clipboard"});
        def.items.push_back(ToolbarItem{"sep2", "", ToolbarItemType::Separator});
        def.items.push_back(ToolbarItem{"undo", "edit.undo", ToolbarItemType::Icon, "edit_undo", "Undo", "Undo last action"});
        def.items.push_back(ToolbarItem{"redo", "edit.redo", ToolbarItemType::Icon, "edit_redo", "Redo", "Redo last undone action"});
        
        RegisterToolbar(def);
    }
    
    // SQL Editor Toolbar
    {
        ToolbarDefinition def;
        def.id = "sql.editor";
        def.name = "SQL Editor";
        def.description = "SQL editing and execution";
        def.position = ToolbarPosition::Top;
        def.row = 1;
        def.is_locked = true;
        def.scope = ToolbarScope::DocumentType;
        def.scope_contexts = {"sql"};
        
        def.items.push_back(ToolbarItem{"execute", "sql.execute", ToolbarItemType::Icon, "sql_execute", "Execute", "Execute SQL (F5)"});
        def.items.push_back(ToolbarItem{"execute_selection", "sql.execute_selection", ToolbarItemType::Icon, "sql_execute_selection", "Execute Selection", "Execute selected SQL"});
        def.items.push_back(ToolbarItem{"explain", "sql.explain", ToolbarItemType::Icon, "sql_explain", "Explain", "Explain query plan"});
        def.items.push_back(ToolbarItem{"sep1", "", ToolbarItemType::Separator});
        def.items.push_back(ToolbarItem{"format", "sql.format", ToolbarItemType::Icon, "sql_format", "Format", "Format SQL"});
        def.items.push_back(ToolbarItem{"comment", "sql.comment", ToolbarItemType::Icon, "sql_comment", "Comment", "Toggle comment"});
        def.items.push_back(ToolbarItem{"sep2", "", ToolbarItemType::Separator});
        def.items.push_back(ToolbarItem{"find", "edit.find", ToolbarItemType::Icon, "edit_find", "Find", "Find in document"});
        def.items.push_back(ToolbarItem{"replace", "edit.replace", ToolbarItemType::Icon, "edit_replace", "Replace", "Find and replace"});
        
        RegisterToolbar(def);
    }
}

void ToolbarManager::ResetToDefaults() {
    // Clear custom toolbars
    std::vector<std::string> to_remove;
    for (const auto& [id, def] : toolbars_) {
        if (def.IsCustom()) {
            to_remove.push_back(id);
        }
    }
    for (const auto& id : to_remove) {
        toolbars_.erase(id);
    }
    custom_toolbar_counter_ = 0;
    
    // Recreate defaults
    CreateDefaultToolbars();
    ApplyPreset("standard");
}

void ToolbarManager::SetActionHandler(const std::string& action_id, ToolbarActionHandler handler) {
    action_handlers_[action_id] = handler;
}

void ToolbarManager::SetActionValidator(const std::string& action_id, ToolbarActionValidator validator) {
    action_validators_[action_id] = validator;
}

void ToolbarManager::ExecuteAction(const std::string& action_id) {
    auto it = action_handlers_.find(action_id);
    if (it != action_handlers_.end() && it->second) {
        it->second(action_id);
    }
}

bool ToolbarManager::CanExecuteAction(const std::string& action_id) {
    auto it = action_validators_.find(action_id);
    if (it != action_validators_.end() && it->second) {
        return it->second(action_id);
    }
    return true; // Default to enabled
}

bool ToolbarManager::SaveToolbars(const std::string& path) {
    std::ofstream file(path);
    if (!file.is_open()) {
        return false;
    }
    
    file << "{\"toolbars\":[";
    bool first = true;
    for (const auto& [id, def] : toolbars_) {
        if (!first) file << ",";
        first = false;
        file << SerializeToolbar(def);
    }
    file << "],\"counter\":" << custom_toolbar_counter_ << "}";
    
    return true;
}

bool ToolbarManager::LoadToolbars(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return false;
    }
    
    std::string json((std::istreambuf_iterator<char>(file)),
                      std::istreambuf_iterator<char>());
    
    // Parse and load toolbars
    scratchrobin::JsonValue root;
    std::string parse_error;
    scratchrobin::JsonParser parser(json);
    if (!parser.Parse(&root, &parse_error)) {
        return false;
    }
    if (root.type != scratchrobin::JsonValue::Type::Object) {
        return false;
    }
    
    auto counter_it = root.object_value.find("counter");
    if (counter_it != root.object_value.end() && counter_it->second.type == scratchrobin::JsonValue::Type::Number) {
        custom_toolbar_counter_ = static_cast<int>(counter_it->second.number_value);
    }
    
    auto toolbars_it = root.object_value.find("toolbars");
    if (toolbars_it != root.object_value.end() && toolbars_it->second.type == scratchrobin::JsonValue::Type::Array) {
        for (const auto& tb : toolbars_it->second.array_value) {
            if (tb.type == scratchrobin::JsonValue::Type::String) {
                auto def = DeserializeToolbar(tb.string_value);
                if (!def.id.empty()) {
                    toolbars_[def.id] = def;
                }
            }
        }
    }
    
    return true;
}

std::string ToolbarManager::SerializeToolbar(const ToolbarDefinition& toolbar) const {
    std::string json = "{";
    json += "\"id\":\"" + toolbar.id + "\",";
    json += "\"name\":\"" + toolbar.name + "\",";
    json += "\"description\":\"" + toolbar.description + "\",";
    json += "\"position\":" + std::to_string(static_cast<int>(toolbar.position)) + ",";
    json += "\"row\":" + std::to_string(toolbar.row) + ",";
    json += "\"visible\":" + std::string(toolbar.is_visible ? "true" : "false") + ",";
    json += "\"locked\":" + std::string(toolbar.is_locked ? "true" : "false") + ",";
    json += "\"items\":[";
    
    bool first = true;
    for (const auto& item : toolbar.items) {
        if (!first) json += ",";
        first = false;
        json += "{";
        json += "\"id\":\"" + item.id + "\",";
        json += "\"action_id\":\"" + item.action_id + "\",";
        json += "\"type\":" + std::to_string(static_cast<int>(item.type)) + ",";
        json += "\"icon\":\"" + item.icon_name + "\",";
        json += "\"label\":\"" + item.label + "\",";
        json += "\"tooltip\":\"" + item.tooltip + "\"";
        json += "}";
    }
    json += "]}";
    return json;
}

ToolbarDefinition ToolbarManager::DeserializeToolbar(const std::string& json) const {
    ToolbarDefinition def;
    scratchrobin::JsonValue root;
    std::string parse_error;
    scratchrobin::JsonParser parser(json);
    if (!parser.Parse(&root, &parse_error)) {
        return def;
    }
    
    if (root.type != scratchrobin::JsonValue::Type::Object) {
        return def;
    }
    
    auto get_string = [](const scratchrobin::JsonValue& obj, const std::string& key, std::string& out) {
        auto it = obj.object_value.find(key);
        if (it != obj.object_value.end() && it->second.type == scratchrobin::JsonValue::Type::String) {
            out = it->second.string_value;
            return true;
        }
        return false;
    };
    
    auto get_number = [](const scratchrobin::JsonValue& obj, const std::string& key, double& out) {
        auto it = obj.object_value.find(key);
        if (it != obj.object_value.end() && it->second.type == scratchrobin::JsonValue::Type::Number) {
            out = it->second.number_value;
            return true;
        }
        return false;
    };
    
    auto get_bool = [](const scratchrobin::JsonValue& obj, const std::string& key, bool& out) {
        auto it = obj.object_value.find(key);
        if (it != obj.object_value.end() && it->second.type == scratchrobin::JsonValue::Type::Bool) {
            out = it->second.bool_value;
            return true;
        }
        return false;
    };
    
    get_string(root, "id", def.id);
    get_string(root, "name", def.name);
    get_string(root, "description", def.description);
    
    double pos_value;
    if (get_number(root, "position", pos_value)) {
        def.position = static_cast<ToolbarPosition>(static_cast<int>(pos_value));
    }
    
    double row_value;
    if (get_number(root, "row", row_value)) {
        def.row = static_cast<int>(row_value);
    }
    
    get_bool(root, "visible", def.is_visible);
    get_bool(root, "locked", def.is_locked);
    
    auto items_it = root.object_value.find("items");
    if (items_it != root.object_value.end() && items_it->second.type == scratchrobin::JsonValue::Type::Array) {
        for (const auto& item_json : items_it->second.array_value) {
            if (item_json.type != scratchrobin::JsonValue::Type::Object) continue;
            
            ToolbarItem item;
            get_string(item_json, "id", item.id);
            get_string(item_json, "action_id", item.action_id);
            
            double type_value;
            if (get_number(item_json, "type", type_value)) {
                item.type = static_cast<ToolbarItemType>(static_cast<int>(type_value));
            }
            
            get_string(item_json, "icon", item.icon_name);
            get_string(item_json, "label", item.label);
            get_string(item_json, "tooltip", item.tooltip);
            
            def.items.push_back(item);
        }
    }
    
    return def;
}

void ToolbarManager::NotifyToolbarChanged(const std::string& toolbar_id) {
    wxCommandEvent event(EVT_TOOLBAR_CHANGED);
    event.SetString(wxString(toolbar_id));
    event_handler_->ProcessEvent(event);
}

void ToolbarManager::NotifyItemChanged(const std::string& toolbar_id, const std::string& item_id) {
    wxCommandEvent event(EVT_TOOLBAR_ITEM_CHANGED);
    event.SetString(wxString(toolbar_id + ":" + item_id));
    event_handler_->ProcessEvent(event);
}

void ToolbarManager::RegisterDefaultTemplates() {
    // File actions
    RegisterItemTemplate({"file.new", "file.new", "file_new", "New", "Create new file", ToolbarItemType::Icon, "File"});
    RegisterItemTemplate({"file.open", "file.open", "file_open", "Open", "Open file", ToolbarItemType::Icon, "File"});
    RegisterItemTemplate({"file.save", "file.save", "file_save", "Save", "Save file", ToolbarItemType::Icon, "File"});
    RegisterItemTemplate({"file.save_all", "file.save_all", "file_save_all", "Save All", "Save all files", ToolbarItemType::Icon, "File"});
    RegisterItemTemplate({"file.print", "file.print", "file_print", "Print", "Print document", ToolbarItemType::Icon, "File"});
    
    // Edit actions
    RegisterItemTemplate({"edit.undo", "edit.undo", "edit_undo", "Undo", "Undo last action", ToolbarItemType::Icon, "Edit"});
    RegisterItemTemplate({"edit.redo", "edit.redo", "edit_redo", "Redo", "Redo last action", ToolbarItemType::Icon, "Edit"});
    RegisterItemTemplate({"edit.cut", "edit.cut", "edit_cut", "Cut", "Cut selection", ToolbarItemType::Icon, "Edit"});
    RegisterItemTemplate({"edit.copy", "edit.copy", "edit_copy", "Copy", "Copy selection", ToolbarItemType::Icon, "Edit"});
    RegisterItemTemplate({"edit.paste", "edit.paste", "edit_paste", "Paste", "Paste from clipboard", ToolbarItemType::Icon, "Edit"});
    RegisterItemTemplate({"edit.find", "edit.find", "edit_find", "Find", "Find text", ToolbarItemType::Icon, "Edit"});
    RegisterItemTemplate({"edit.replace", "edit.replace", "edit_replace", "Replace", "Find and replace", ToolbarItemType::Icon, "Edit"});
    
    // SQL actions
    RegisterItemTemplate({"sql.execute", "sql.execute", "sql_execute", "Execute", "Execute SQL (F5)", ToolbarItemType::Icon, "SQL"});
    RegisterItemTemplate({"sql.explain", "sql.explain", "sql_explain", "Explain", "Explain query plan", ToolbarItemType::Icon, "SQL"});
    RegisterItemTemplate({"sql.format", "sql.format", "sql_format", "Format", "Format SQL", ToolbarItemType::Icon, "SQL"});
    RegisterItemTemplate({"sql.comment", "sql.comment", "sql_comment", "Comment", "Toggle comment", ToolbarItemType::Icon, "SQL"});
    
    // Custom items
    RegisterItemTemplate({"separator", "", "", "Separator", "Visual separator", ToolbarItemType::Separator, "Custom", true});
    RegisterItemTemplate({"spacer", "", "", "Spacer", "Flexible spacer", ToolbarItemType::Spacer, "Custom", true});
    RegisterItemTemplate({"fixed_spacer", "", "", "Fixed Spacer", "Fixed-width spacer", ToolbarItemType::FixedSpacer, "Custom", true});
}

void ToolbarManager::RegisterDefaultPresets() {
    // Standard preset
    ToolbarPreset standard;
    standard.id = "standard";
    standard.name = "Standard";
    standard.description = "Default toolbar layout";
    standard.is_builtin = true;
    standard.toolbar_ids = {"main.standard", "sql.editor"};
    RegisterPreset(standard);
    
    // Minimal preset
    ToolbarPreset minimal;
    minimal.id = "minimal";
    minimal.name = "Minimal";
    minimal.description = "Minimal toolbar layout";
    minimal.is_builtin = true;
    minimal.toolbar_ids = {"main.standard"};
    RegisterPreset(minimal);
    
    // Advanced preset (includes all)
    ToolbarPreset advanced;
    advanced.id = "advanced";
    advanced.name = "Advanced";
    advanced.description = "All available toolbars";
    advanced.is_builtin = true;
    advanced.toolbar_ids = {"main.standard", "sql.editor"}; // Add more as they're created
    RegisterPreset(advanced);
}

} // namespace toolbar
} // namespace ui
} // namespace scratchrobin
