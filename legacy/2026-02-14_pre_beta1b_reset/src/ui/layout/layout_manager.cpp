/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0
 */
#include "ui/layout/layout_manager.h"
#include "ui/main_frame.h"
#include <wx/stdpaths.h>
#include <wx/filename.h>
#include <wx/toolbar.h>
#include <wx/dir.h>
#include <wx/filefn.h>

namespace scratchrobin {

// MenuMerger implementation
void MenuMerger::MergeMenuBar(wxMenuBar* main_menu, wxMenuBar* form_menu) {
    if (!main_menu || !form_menu) return;
    
    // Store original menus to restore later
    for (size_t i = 0; i < form_menu->GetMenuCount(); ++i) {
        wxString title = form_menu->GetMenuLabel(i);
        wxMenu* menu = form_menu->GetMenu(i);
        
        // Check if menu already exists in main
        int existing = main_menu->FindMenu(title);
        if (existing == wxNOT_FOUND) {
            // Add new menu
            main_menu->Append(menu, title);
            merged_menus_[i] = menu;
        }
    }
}

void MenuMerger::UnmergeMenuBar(wxMenuBar* main_menu, wxMenuBar* form_menu) {
    if (!main_menu) return;
    
    // Remove merged menus by finding their position
    for (const auto& [_, menu] : merged_menus_) {
        // Find position by comparing menu pointers
        size_t count = main_menu->GetMenuCount();
        for (size_t i = 0; i < count; ++i) {
            if (main_menu->GetMenu(i) == menu) {
                main_menu->Remove(static_cast<int>(i));
                break;
            }
        }
    }
    merged_menus_.clear();
}

void MenuMerger::MergeToolBar(wxToolBar* main_toolbar, wxToolBar* form_toolbar) {
    if (!main_toolbar || !form_toolbar) return;
    // Toolbars are typically separate in modern UIs
}

void MenuMerger::UnmergeToolBar(wxToolBar* main_toolbar, wxToolBar* form_toolbar) {
    // Nothing to do for now
}

// LayoutManager implementation
LayoutManager::LayoutManager(MainFrame* main_frame) : main_frame_(main_frame) {
}

LayoutManager::~LayoutManager() {
    Shutdown();
}

void LayoutManager::Initialize() {
    if (!main_frame_) return;
    
    // Initialize AUI manager
    aui_manager_.SetManagedWindow(main_frame_);
    
    // Restore previous state
    RestoreState();
}

void LayoutManager::Shutdown() {
    if (auto_save_) {
        SaveState();
    }
    
    // Unregister all windows
    windows_.clear();
    
    // Uninitialize AUI
    aui_manager_.UnInit();
}

void LayoutManager::RegisterWindow(IDockableWindow* window) {
    if (!window) return;
    
    std::string id = window->GetWindowId();
    windows_[id] = window;
    
    // Create default pane info
    wxAuiPaneInfo info;
    info.Name(wxString::FromUTF8(id));
    info.Caption(wxString::FromUTF8(window->GetWindowTitle()));
    info.Floatable(window->CanFloat());
    info.CloseButton(window->CanClose());
    
    if (auto* nav = dynamic_cast<INavigatorWindow*>(window)) {
        info.Left();
        info.Layer(1);
        info.Row(0);
        info.Position(0);
        info.BestSize(300, 600);
        info.MinSize(150, 200);
    } else if (auto* doc = dynamic_cast<IDocumentWindow*>(window)) {
        info.Center();
        info.Layer(0);
    } else {
        info.Float();
    }
    
    pane_infos_[id] = info;
    
    // Add to AUI manager
    if (wxWindow* w = window->GetWindow()) {
        aui_manager_.AddPane(w, info);
    }
    
    NotifyObservers({LayoutChangeEvent::WindowRegistered, id, ""});
}

void LayoutManager::UnregisterWindow(const std::string& window_id) {
    auto it = windows_.find(window_id);
    if (it != windows_.end()) {
        aui_manager_.DetachPane(it->second->GetWindow());
        windows_.erase(it);
        pane_infos_.erase(window_id);
        NotifyObservers({LayoutChangeEvent::WindowUnregistered, window_id, ""});
    }
}

IDockableWindow* LayoutManager::GetWindow(const std::string& window_id) const {
    auto it = windows_.find(window_id);
    if (it != windows_.end()) {
        return it->second;
    }
    return nullptr;
}

std::vector<IDockableWindow*> LayoutManager::GetAllWindows() const {
    std::vector<IDockableWindow*> result;
    for (const auto& [_, window] : windows_) {
        result.push_back(window);
    }
    return result;
}

std::vector<IDockableWindow*> LayoutManager::GetVisibleWindows() const {
    std::vector<IDockableWindow*> result;
    for (const auto& [_, window] : windows_) {
        if (IsWindowVisible(window->GetWindowId())) {
            result.push_back(window);
        }
    }
    return result;
}

void LayoutManager::DockWindow(const std::string& window_id, DockDirection direction, int proportion) {
    auto* window = GetWindow(window_id);
    if (!window || !window->CanDock()) return;
    
    auto it = pane_infos_.find(window_id);
    if (it == pane_infos_.end()) return;
    
    wxAuiPaneInfo& info = it->second;
    
    switch (direction) {
        case DockDirection::Left:
            info.Left();
            break;
        case DockDirection::Right:
            info.Right();
            break;
        case DockDirection::Top:
            info.Top();
            break;
        case DockDirection::Bottom:
            info.Bottom();
            break;
        case DockDirection::Center:
            info.Center();
            break;
        default:
            break;
    }
    
    info.Dock();
    info.Show();
    
    aui_manager_.Update();
    NotifyObservers({LayoutChangeEvent::WindowDocked, window_id, ""});
}

void LayoutManager::FloatWindow(const std::string& window_id, const wxPoint& pos, const wxSize& size) {
    auto* window = GetWindow(window_id);
    if (!window || !window->CanFloat()) return;
    
    auto it = pane_infos_.find(window_id);
    if (it == pane_infos_.end()) return;
    
    wxAuiPaneInfo& info = it->second;
    info.Float();
    
    if (pos != wxDefaultPosition) {
        info.FloatingPosition(pos);
    }
    if (size != wxDefaultSize) {
        info.FloatingSize(size);
    }
    
    info.Show();
    aui_manager_.Update();
    NotifyObservers({LayoutChangeEvent::WindowFloated, window_id, ""});
}

void LayoutManager::ShowWindow(const std::string& window_id, bool show) {
    auto it = pane_infos_.find(window_id);
    if (it != pane_infos_.end()) {
        it->second.Show(show);
        aui_manager_.Update();
    }
}

void LayoutManager::HideWindow(const std::string& window_id) {
    ShowWindow(window_id, false);
}

void LayoutManager::CloseWindow(const std::string& window_id) {
    auto* window = GetWindow(window_id);
    if (window && window->OnCloseRequest()) {
        HideWindow(window_id);
        NotifyObservers({LayoutChangeEvent::WindowClosed, window_id, ""});
    }
}

bool LayoutManager::IsWindowVisible(const std::string& window_id) const {
    auto it = pane_infos_.find(window_id);
    if (it != pane_infos_.end()) {
        return it->second.IsShown();
    }
    return false;
}

bool LayoutManager::IsWindowDocked(const std::string& window_id) const {
    auto it = pane_infos_.find(window_id);
    if (it != pane_infos_.end()) {
        return it->second.IsDocked();
    }
    return false;
}

void LayoutManager::SetActiveWindow(const std::string& window_id) {
    // Deactivate previous
    if (!active_window_id_.empty()) {
        if (auto* prev = GetWindow(active_window_id_)) {
            prev->OnDeactivate();
        }
    }
    
    active_window_id_ = window_id;
    
    // Activate new
    if (auto* window = GetWindow(window_id)) {
        window->OnActivate();
    }
}

IDockableWindow* LayoutManager::GetActiveWindow() const {
    return GetWindow(active_window_id_);
}

void LayoutManager::LoadPreset(const std::string& name) {
    LayoutPreset preset = LoadPresetFromFile(name);
    LoadPreset(preset);
}

void LayoutManager::LoadPreset(const LayoutPreset& preset) {
    // Apply main form state
    if (main_frame_) {
        if (preset.IsMainFormMaximized()) {
            main_frame_->Maximize();
        } else {
            main_frame_->SetSize(preset.GetMainFormRect());
        }
    }
    
    // Apply window states
    for (const auto& id : preset.GetWindowIds()) {
        LayoutWindowState state = preset.GetWindowState(id);
        ApplyWindowState(state);
    }
    
    current_preset_name_ = preset.GetName();
    aui_manager_.Update();
    NotifyObservers({LayoutChangeEvent::LayoutLoaded, "", preset.GetName()});
}

void LayoutManager::ApplyWindowState(const LayoutWindowState& state) {
    auto it = pane_infos_.find(state.window_id);
    if (it == pane_infos_.end()) return;
    
    wxAuiPaneInfo& info = it->second;
    
    info.Show(state.is_visible);
    
    if (state.is_docked) {
        DockWindow(state.window_id, state.dock_direction, state.dock_proportion);
    } else {
        FloatWindow(state.window_id, 
                    wxPoint(state.floating_rect.x, state.floating_rect.y),
                    wxSize(state.floating_rect.width, state.floating_rect.height));
    }
}

void LayoutManager::SaveCurrentAsPreset(const std::string& name) {
    SaveCurrentAsPreset(name, "");
}

void LayoutManager::SaveCurrentAsPreset(const std::string& name, const std::string& description) {
    LayoutPreset preset(name);
    preset.SetDescription(description);
    
    // Save main form state
    if (main_frame_) {
        preset.SetMainFormRect(main_frame_->GetRect());
        preset.SetMainFormMaximized(main_frame_->IsMaximized());
    }
    
    // Save window states
    for (const auto& [id, window] : windows_) {
        auto it = pane_infos_.find(id);
        if (it != pane_infos_.end()) {
            const wxAuiPaneInfo& info = it->second;
            LayoutWindowState state;
            state.window_id = id;
            state.window_type = window->GetWindowType();
            state.is_visible = info.IsShown();
            state.is_docked = info.IsDocked();
            state.dock_proportion = info.dock_proportion;
            
            if (!info.IsDocked()) {
                state.floating_rect = wxRect(info.floating_pos, info.floating_size);
            }
            
            preset.SetWindowState(id, state);
        }
    }
    
    SavePresetToFile(preset);
    NotifyObservers({LayoutChangeEvent::LayoutSaved, "", name});
}

void LayoutManager::DeletePreset(const std::string& name) {
    wxString filename = wxString::FromUTF8(GetPresetsDirectory()) + wxFileName::GetPathSeparator() + 
                        wxString::FromUTF8(name) + ".json";
    if (wxFileExists(filename)) {
        wxRemoveFile(filename);
    }
}

std::vector<LayoutPreset> LayoutManager::GetPresets() const {
    std::vector<LayoutPreset> presets;
    
    // Add built-in presets
    presets.push_back(LayoutPreset::CreateDefault());
    presets.push_back(LayoutPreset::CreateSingleMonitor());
    presets.push_back(LayoutPreset::CreateDualMonitor());
    presets.push_back(LayoutPreset::CreateWideScreen());
    presets.push_back(LayoutPreset::CreateCompact());
    
    // Load custom presets from directory
    wxString dir = wxString::FromUTF8(GetPresetsDirectory());
    if (wxDirExists(dir)) {
        wxDir wx_dir(dir);
        wxString filename;
        bool cont = wx_dir.GetFirst(&filename, "*.json");
        while (cont) {
            wxString path = dir + wxFileName::GetPathSeparator() + filename;
            LayoutPreset preset = LayoutPreset::LoadFromFile(path.ToStdString());
            if (!preset.GetName().empty()) {
                presets.push_back(preset);
            }
            cont = wx_dir.GetNext(&filename);
        }
    }
    
    return presets;
}

LayoutPreset LayoutManager::GetPreset(const std::string& name) const {
    for (const auto& preset : GetPresets()) {
        if (preset.GetName() == name) {
            return preset;
        }
    }
    return LayoutPreset();
}

bool LayoutManager::HasPreset(const std::string& name) const {
    return !GetPreset(name).GetName().empty();
}

void LayoutManager::SetDefaultPreset(const std::string& name) {
    // Save to config
}

std::string LayoutManager::GetDefaultPreset() const {
    return "Default";
}

void LayoutManager::ApplyDefaultLayout() {
    LoadPreset(LayoutPreset::CreateDefault());
}

void LayoutManager::ApplySingleMonitorLayout() {
    LoadPreset(LayoutPreset::CreateSingleMonitor());
}

void LayoutManager::ApplyDualMonitorLayout() {
    LoadPreset(LayoutPreset::CreateDualMonitor());
}

void LayoutManager::ApplyWideScreenLayout() {
    LoadPreset(LayoutPreset::CreateWideScreen());
}

void LayoutManager::ApplyCompactLayout() {
    LoadPreset(LayoutPreset::CreateCompact());
}

void LayoutManager::SaveState() {
    SaveCurrentAsPreset("_last_session", "Auto-saved session");
}

void LayoutManager::RestoreState() {
    if (HasPreset("_last_session")) {
        LoadPreset("_last_session");
    } else {
        ApplyDefaultLayout();
    }
}

void LayoutManager::AddObserver(LayoutChangeCallback callback) {
    observers_.push_back(callback);
}

void LayoutManager::RemoveObserver(LayoutChangeCallback callback) {
    observers_.erase(std::remove_if(observers_.begin(), observers_.end(),
        [&callback](const LayoutChangeCallback& cb) {
            return cb.target_type() == callback.target_type();
        }), observers_.end());
}

void LayoutManager::NotifyObservers(const LayoutChangeEvent& event) {
    for (auto& observer : observers_) {
        observer(event);
    }
}

std::string LayoutManager::GetPresetsDirectory() const {
    wxString config_dir = wxStandardPaths::Get().GetUserConfigDir();
    config_dir += wxFileName::GetPathSeparator();
    config_dir += ".scratchrobin";
    
    // Create parent .scratchrobin directory if needed
    if (!wxDirExists(config_dir)) {
        wxMkdir(config_dir, wxS_DIR_DEFAULT);
    }
    
    config_dir += wxFileName::GetPathSeparator();
    config_dir += "layouts";
    
    // Create layouts subdirectory if needed
    if (!wxDirExists(config_dir)) {
        wxMkdir(config_dir, wxS_DIR_DEFAULT);
    }
    
    return config_dir.ToStdString();
}

void LayoutManager::SavePresetToFile(const LayoutPreset& preset) {
    std::string path = GetPresetsDirectory() + "/" + preset.GetName() + ".json";
    preset.SaveToFile(path);
}

LayoutPreset LayoutManager::LoadPresetFromFile(const std::string& name) {
    std::string path = GetPresetsDirectory() + "/" + name + ".json";
    if (wxFileExists(wxString::FromUTF8(path))) {
        return LayoutPreset::LoadFromFile(path);
    }
    return LayoutPreset();
}

} // namespace scratchrobin
