// Copyright (c) 2026, Dennis C. Alfonso
// Licensed under the MIT License. See LICENSE file in the project root.
#pragma once

#include "toolbar_definition.h"
#include <map>
#include <memory>
#include <functional>
#include <wx/wx.h>

namespace scratchrobin {
namespace ui {

class MainFrame;

namespace toolbar {

/**
 * @brief Callback for toolbar item actions
 */
using ToolbarActionHandler = std::function<void(const std::string& action_id)>;

/**
 * @brief Callback to check if action is enabled
 */
using ToolbarActionValidator = std::function<bool(const std::string& action_id)>;

/**
 * @brief Manages toolbar definitions, presets, and customization
 */
class ToolbarManager {
public:
    ToolbarManager();
    ~ToolbarManager();

    // Singleton access
    static ToolbarManager& GetInstance();
    static void DestroyInstance();

    // Initialization
    void Initialize();
    void Shutdown();

    // Toolbar definitions
    void RegisterToolbar(const ToolbarDefinition& definition);
    void UnregisterToolbar(const std::string& toolbar_id);
    ToolbarDefinition* GetToolbar(const std::string& toolbar_id);
    const ToolbarDefinition* GetToolbar(const std::string& toolbar_id) const;
    std::vector<ToolbarDefinition*> GetAllToolbars();
    std::vector<ToolbarDefinition*> GetToolbarsForPosition(ToolbarPosition position);
    std::vector<ToolbarDefinition*> GetToolbarsForScope(const std::string& context);

    // Toolbar items
    bool AddItem(const std::string& toolbar_id, const ToolbarItem& item, size_t index = SIZE_MAX);
    bool RemoveItem(const std::string& toolbar_id, const std::string& item_id);
    bool MoveItem(const std::string& toolbar_id, const std::string& item_id, size_t new_index);
    bool UpdateItem(const std::string& toolbar_id, const ToolbarItem& item);

    // Item templates (palette)
    void RegisterItemTemplate(const ToolbarItemTemplate& tmpl);
    std::vector<ToolbarItemTemplate> GetItemTemplates() const;
    std::vector<ToolbarItemTemplate> GetItemTemplatesByCategory(const std::string& category) const;
    std::vector<std::string> GetItemCategories() const;

    // Custom toolbars
    ToolbarDefinition* CreateCustomToolbar(const std::string& name);
    bool DeleteCustomToolbar(const std::string& toolbar_id);
    bool RenameCustomToolbar(const std::string& toolbar_id, const std::string& new_name);

    // Presets
    void RegisterPreset(const ToolbarPreset& preset);
    bool ApplyPreset(const std::string& preset_id);
    ToolbarPreset* GetPreset(const std::string& preset_id);
    std::vector<ToolbarPreset> GetAllPresets() const;

    // Default toolbars
    void CreateDefaultToolbars();
    void ResetToDefaults();

    // Action handlers
    void SetActionHandler(const std::string& action_id, ToolbarActionHandler handler);
    void SetActionValidator(const std::string& action_id, ToolbarActionValidator validator);
    void ExecuteAction(const std::string& action_id);
    bool CanExecuteAction(const std::string& action_id);

    // Persistence
    bool SaveToolbars(const std::string& path);
    bool LoadToolbars(const std::string& path);
    std::string SerializeToolbar(const ToolbarDefinition& toolbar) const;
    ToolbarDefinition DeserializeToolbar(const std::string& json) const;

    // Events
    wxEvtHandler* GetEventHandler() { return event_handler_.get(); }
    void NotifyToolbarChanged(const std::string& toolbar_id);
    void NotifyItemChanged(const std::string& toolbar_id, const std::string& item_id);

private:
    std::map<std::string, ToolbarDefinition> toolbars_;
    std::map<std::string, ToolbarItemTemplate> templates_;
    std::map<std::string, ToolbarPreset> presets_;
    std::map<std::string, ToolbarActionHandler> action_handlers_;
    std::map<std::string, ToolbarActionValidator> action_validators_;
    std::unique_ptr<wxEvtHandler> event_handler_;
    int custom_toolbar_counter_ = 0;

    static ToolbarManager* instance_;

    void RegisterDefaultTemplates();
    void RegisterDefaultPresets();
};

// Event types
wxDECLARE_EVENT(EVT_TOOLBAR_CHANGED, wxCommandEvent);
wxDECLARE_EVENT(EVT_TOOLBAR_ITEM_CHANGED, wxCommandEvent);

} // namespace toolbar
} // namespace ui
} // namespace scratchrobin
