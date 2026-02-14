/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#include "window_state_manager.h"

#include "main_frame.h"

#include <wx/filename.h>
#include <wx/stdpaths.h>
#include <wx/file.h>
#include <wx/display.h>

#include <fstream>
#include <sstream>

namespace scratchrobin {

namespace {

// JSON helper functions for minimal JSON support
std::string JsonEscape(const std::string& str) {
    std::string result;
    result.reserve(str.size());
    for (char c : str) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:
                if (c >= 0x20 && c <= 0x7E) {
                    result += c;
                } else {
                    char buf[8];
                    std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned char>(c));
                    result += buf;
                }
        }
    }
    return result;
}

std::string JsonBool(bool value) {
    return value ? "true" : "false";
}

std::string JsonInt(int value) {
    return std::to_string(value);
}

std::string JsonRect(const wxRect& rect) {
    std::string json = "{";
    json += "\"x\":" + JsonInt(rect.x) + ",";
    json += "\"y\":" + JsonInt(rect.y) + ",";
    json += "\"width\":" + JsonInt(rect.width) + ",";
    json += "\"height\":" + JsonInt(rect.height);
    json += "}";
    return json;
}

std::string JsonPoint(const wxPoint& point) {
    std::string json = "{";
    json += "\"x\":" + JsonInt(point.x) + ",";
    json += "\"y\":" + JsonInt(point.y);
    json += "}";
    return json;
}

// Simple JSON value extraction
bool ExtractBool(const std::string& json, const std::string& key, bool default_val = false) {
    std::string search = "\"" + key + "\":";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return default_val;
    pos += search.length();
    // Skip whitespace
    while (pos < json.length() && std::isspace(json[pos])) pos++;
    if (pos >= json.length()) return default_val;
    if (json.compare(pos, 4, "true") == 0) return true;
    if (json.compare(pos, 5, "false") == 0) return false;
    return default_val;
}

int ExtractInt(const std::string& json, const std::string& key, int default_val = 0) {
    std::string search = "\"" + key + "\":";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return default_val;
    pos += search.length();
    // Skip whitespace
    while (pos < json.length() && std::isspace(json[pos])) pos++;
    if (pos >= json.length()) return default_val;
    try {
        size_t end;
        int val = std::stoi(json.substr(pos), &end);
        return val;
    } catch (...) {
        return default_val;
    }
}

std::string ExtractString(const std::string& json, const std::string& key, const std::string& default_val = "") {
    std::string search = "\"" + key + "\":";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return default_val;
    pos += search.length();
    // Skip whitespace
    while (pos < json.length() && std::isspace(json[pos])) pos++;
    if (pos >= json.length() || json[pos] != '"') return default_val;
    pos++; // Skip opening quote
    std::string result;
    while (pos < json.length() && json[pos] != '"') {
        if (json[pos] == '\\' && pos + 1 < json.length()) {
            switch (json[pos + 1]) {
                case '"': result += '"'; pos += 2; continue;
                case '\\': result += '\\'; pos += 2; continue;
                case 'b': result += '\b'; pos += 2; continue;
                case 'f': result += '\f'; pos += 2; continue;
                case 'n': result += '\n'; pos += 2; continue;
                case 'r': result += '\r'; pos += 2; continue;
                case 't': result += '\t'; pos += 2; continue;
                default: result += json[pos]; pos++; continue;
            }
        }
        result += json[pos];
        pos++;
    }
    return result;
}

wxRect ExtractRect(const std::string& json, const std::string& key) {
    std::string search = "\"" + key + "\":";
    size_t pos = json.find(search);
    if (pos == std::string::npos) return wxRect(100, 100, 1024, 768);
    pos += search.length();
    // Skip whitespace
    while (pos < json.length() && std::isspace(json[pos])) pos++;
    if (pos >= json.length() || json[pos] != '{') return wxRect(100, 100, 1024, 768);
    
    size_t end = json.find('}', pos);
    if (end == std::string::npos) return wxRect(100, 100, 1024, 768);
    
    std::string rect_json = json.substr(pos, end - pos + 1);
    int x = ExtractInt(rect_json, "x", 100);
    int y = ExtractInt(rect_json, "y", 100);
    int width = ExtractInt(rect_json, "width", 1024);
    int height = ExtractInt(rect_json, "height", 768);
    return wxRect(wxPoint(x, y), wxSize(width, height));
}

} // namespace

WindowStateManager::WindowStateManager(MainFrame* main_frame)
    : main_frame_(main_frame) {
    // Initialize default panel states
    panel_states_["navigator"] = PanelState(true);
    panel_states_["navigator"].dock_proportion = 30;
    panel_states_["document_manager"] = PanelState(false);
    panel_states_["inspector"] = PanelState(true);
    panel_states_["inspector"].dock_proportion = 70;
    
    // Initialize default toolbar states
    toolbar_states_["main"] = ToolbarState();
    toolbar_states_["sql_editor"] = ToolbarState();
}

WindowStateManager::~WindowStateManager() {
    // Auto-save on destruction if we have a main frame
    if (main_frame_) {
        SaveState();
    }
}

bool WindowStateManager::SaveState() {
    if (!EnsureConfigDir()) {
        return false;
    }
    
    // Update state from main frame if available
    if (main_frame_) {
        // Save maximized state
        main_state_.is_maximized = main_frame_->IsMaximized();
        main_state_.is_fullscreen = main_frame_->IsFullScreen();
        
        // Only save position/size if not maximized/fullscreen
        if (!main_state_.is_maximized && !main_state_.is_fullscreen) {
            main_state_.normal_rect = main_frame_->GetRect();
            main_state_.display_index = GetDisplayIndexForPoint(main_state_.normal_rect.GetPosition());
        }
    }
    
    std::string json = SerializeToJson();
    std::string path = GetStateFilePath();
    
    std::ofstream file(path);
    if (!file.is_open()) {
        return false;
    }
    
    file << json;
    return file.good();
}

bool WindowStateManager::RestoreState() {
    std::string path = GetStateFilePath();
    
    std::ifstream file(path);
    if (!file.is_open()) {
        // No saved state, use defaults
        ResetToDefaults();
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    
    if (!DeserializeFromJson(buffer.str())) {
        ResetToDefaults();
        return false;
    }
    
    return true;
}

void WindowStateManager::ResetToDefaults() {
    main_state_ = MainFormState();
    main_state_.normal_rect = wxRect(wxPoint(100, 100), wxSize(1024, 768));
    main_state_.auto_size_mode = AutoSizePolicy::Mode::ADAPTIVE;
    main_state_.display_index = 0;
    
    panel_states_.clear();
    panel_states_["navigator"] = PanelState(true);
    panel_states_["navigator"].dock_proportion = 30;
    panel_states_["document_manager"] = PanelState(false);
    panel_states_["inspector"] = PanelState(true);
    panel_states_["inspector"].dock_proportion = 70;
    
    toolbar_states_.clear();
    toolbar_states_["main"] = ToolbarState();
    toolbar_states_["sql_editor"] = ToolbarState();
    
    layout_preset_ = "default";
}

void WindowStateManager::OnExit() {
    SaveState();
}

void WindowStateManager::OnInit() {
    RestoreState();
}

WindowStateManager::PanelState& WindowStateManager::GetPanelState(const std::string& name) {
    auto it = panel_states_.find(name);
    if (it == panel_states_.end()) {
        panel_states_[name] = PanelState(true);
    }
    return panel_states_[name];
}

const WindowStateManager::PanelState& WindowStateManager::GetPanelState(const std::string& name) const {
    auto it = panel_states_.find(name);
    if (it != panel_states_.end()) {
        return it->second;
    }
    static PanelState default_state;
    return default_state;
}

WindowStateManager::ToolbarState& WindowStateManager::GetToolbarState(const std::string& name) {
    auto it = toolbar_states_.find(name);
    if (it == toolbar_states_.end()) {
        toolbar_states_[name] = ToolbarState();
    }
    return toolbar_states_[name];
}

const WindowStateManager::ToolbarState& WindowStateManager::GetToolbarState(const std::string& name) const {
    auto it = toolbar_states_.find(name);
    if (it != toolbar_states_.end()) {
        return it->second;
    }
    static ToolbarState default_state;
    return default_state;
}

bool WindowStateManager::HasSavedState() const {
    return wxFileExists(GetStateFilePath());
}

bool WindowStateManager::DeleteSavedState() {
    std::string path = GetStateFilePath();
    if (!wxFileExists(path)) {
        return true;
    }
    return wxRemoveFile(path);
}

int WindowStateManager::GetDisplayIndexForPoint(const wxPoint& point) {
    int count = wxDisplay::GetCount();
    for (int i = 0; i < count; ++i) {
        wxDisplay display(i);
        if (display.GetGeometry().Contains(point)) {
            return i;
        }
    }
    return 0; // Default to primary display
}

bool WindowStateManager::IsValidDisplay(int index) {
    return index >= 0 && index < static_cast<int>(wxDisplay::GetCount());
}

wxPoint WindowStateManager::GetSafePosition(const wxRect& rect, int& display_index) {
    // Check if the requested display is still valid
    if (!IsValidDisplay(display_index)) {
        display_index = 0; // Fall back to primary
    }
    
    wxDisplay display(display_index);
    wxRect display_rect = display.GetClientArea();
    
    // Check if the window would be at least partially visible
    wxRect intersection = display_rect.Intersect(rect);
    if (intersection.width < 100 || intersection.height < 100) {
        // Window would be mostly off-screen, move to primary display
        display_index = 0;
        wxDisplay primary_display(0u);
        display_rect = primary_display.GetClientArea();
        
        // Center on primary display
        int x = display_rect.x + (display_rect.width - rect.width) / 2;
        int y = display_rect.y + (display_rect.height - rect.height) / 2;
        
        // Ensure at least top-left is visible
        x = std::max(display_rect.x, x);
        y = std::max(display_rect.y, y);
        
        return wxPoint(x, y);
    }
    
    return rect.GetPosition();
}

std::string WindowStateManager::GetStateFilePath() const {
    wxFileName path(GetConfigDir(), "");
    path.SetFullName("window_state.json");
    return path.GetFullPath().ToStdString();
}

std::string WindowStateManager::GetConfigDir() const {
    wxString config_dir = wxStandardPaths::Get().GetUserConfigDir();
    wxFileName path(config_dir, "");
    path.AppendDir("scratchrobin");
    return path.GetFullPath().ToStdString();
}

bool WindowStateManager::EnsureConfigDir() const {
    wxFileName dir(GetConfigDir(), "");
    if (dir.DirExists()) {
        return true;
    }
    return dir.Mkdir(wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);
}

std::string WindowStateManager::SerializeToJson() const {
    std::string json = "{\n";
    
    // Version
    json += "  \"version\": " + JsonInt(kStateVersion) + ",\n";
    
    // Main form state
    json += "  \"main_form\": {\n";
    json += "    \"is_maximized\": " + JsonBool(main_state_.is_maximized) + ",\n";
    json += "    \"is_fullscreen\": " + JsonBool(main_state_.is_fullscreen) + ",\n";
    json += "    \"normal_rect\": " + JsonRect(main_state_.normal_rect) + ",\n";
    json += "    \"display_index\": " + JsonInt(main_state_.display_index) + ",\n";
    json += "    \"auto_size_mode\": \"" + JsonEscape(AutoSizePolicy::ModeToString(main_state_.auto_size_mode)) + "\"\n";
    json += "  },\n";
    
    // Panels state
    json += "  \"panels\": {\n";
    bool first = true;
    for (const auto& [name, state] : panel_states_) {
        if (!first) json += ",\n";
        first = false;
        json += "    \"" + JsonEscape(name) + "\": {\n";
        json += "      \"is_visible\": " + JsonBool(state.is_visible) + ",\n";
        json += "      \"is_docked\": " + JsonBool(state.is_docked) + ",\n";
        json += "      \"floating_rect\": " + JsonRect(state.floating_rect) + ",\n";
        json += "      \"dock_proportion\": " + JsonInt(state.dock_proportion) + ",\n";
        json += "      \"display_index\": " + JsonInt(state.display_index) + "\n";
        json += "    }";
    }
    json += "\n  },\n";
    
    // Toolbars state
    json += "  \"toolbars\": {\n";
    first = true;
    for (const auto& [name, state] : toolbar_states_) {
        if (!first) json += ",\n";
        first = false;
        json += "    \"" + JsonEscape(name) + "\": {\n";
        json += "      \"is_floating\": " + JsonBool(state.is_floating) + ",\n";
        json += "      \"position\": " + JsonPoint(state.position) + ",\n";
        json += "      \"display_index\": " + JsonInt(state.display_index) + "\n";
        json += "    }";
    }
    json += "\n  },\n";
    
    // Layout preset
    json += "  \"layout_preset\": \"" + JsonEscape(layout_preset_) + "\"\n";
    
    json += "}\n";
    return json;
}

bool WindowStateManager::DeserializeFromJson(const std::string& json) {
    // Check version
    int version = ExtractInt(json, "version", 1);
    if (version < 1 || version > kStateVersion) {
        return false; // Unsupported version
    }
    
    // Parse main form state
    size_t main_form_pos = json.find("\"main_form\"");
    if (main_form_pos != std::string::npos) {
        size_t main_form_start = json.find('{', main_form_pos);
        size_t main_form_end = json.find('}', main_form_start);
        if (main_form_start != std::string::npos && main_form_end != std::string::npos) {
            std::string main_json = json.substr(main_form_start, main_form_end - main_form_start + 1);
            main_state_.is_maximized = ExtractBool(main_json, "is_maximized", false);
            main_state_.is_fullscreen = ExtractBool(main_json, "is_fullscreen", false);
            main_state_.normal_rect = ExtractRect(main_json, "normal_rect");
            main_state_.display_index = ExtractInt(main_json, "display_index", 0);
            std::string mode_str = ExtractString(main_json, "auto_size_mode", "adaptive");
            main_state_.auto_size_mode = AutoSizePolicy::StringToMode(mode_str.c_str());
        }
    }
    
    // Parse panels state
    size_t panels_pos = json.find("\"panels\"");
    if (panels_pos != std::string::npos) {
        size_t panels_start = json.find('{', panels_pos);
        size_t panels_end = json.find('}', panels_start);
        if (panels_start != std::string::npos && panels_end != std::string::npos) {
            std::string panels_section = json.substr(panels_start, panels_end - panels_start + 1);
            
            // Parse each panel
            for (auto& [name, state] : panel_states_) {
                size_t panel_pos = panels_section.find("\"" + name + "\"");
                if (panel_pos != std::string::npos) {
                    size_t panel_start = panels_section.find('{', panel_pos);
                    size_t panel_end = panels_section.find('}', panel_start);
                    if (panel_start != std::string::npos && panel_end != std::string::npos) {
                        std::string panel_json = panels_section.substr(panel_start, panel_end - panel_start + 1);
                        state.is_visible = ExtractBool(panel_json, "is_visible", true);
                        state.is_docked = ExtractBool(panel_json, "is_docked", true);
                        state.dock_proportion = ExtractInt(panel_json, "dock_proportion", 25);
                        state.display_index = ExtractInt(panel_json, "display_index", 0);
                    }
                }
            }
        }
    }
    
    // Parse toolbars state
    size_t toolbars_pos = json.find("\"toolbars\"");
    if (toolbars_pos != std::string::npos) {
        size_t toolbars_start = json.find('{', toolbars_pos);
        size_t toolbars_end = json.find('}', toolbars_start);
        if (toolbars_start != std::string::npos && toolbars_end != std::string::npos) {
            std::string toolbars_section = json.substr(toolbars_start, toolbars_end - toolbars_start + 1);
            
            for (auto& [name, state] : toolbar_states_) {
                size_t toolbar_pos = toolbars_section.find("\"" + name + "\"");
                if (toolbar_pos != std::string::npos) {
                    size_t toolbar_start = toolbars_section.find('{', toolbar_pos);
                    size_t toolbar_end = toolbars_section.find('}', toolbar_start);
                    if (toolbar_start != std::string::npos && toolbar_end != std::string::npos) {
                        std::string toolbar_json = toolbars_section.substr(toolbar_start, toolbar_end - toolbar_start + 1);
                        state.is_floating = ExtractBool(toolbar_json, "is_floating", false);
                        state.display_index = ExtractInt(toolbar_json, "display_index", 0);
                    }
                }
            }
        }
    }
    
    // Parse layout preset
    layout_preset_ = ExtractString(json, "layout_preset", "default");
    
    return true;
}

} // namespace scratchrobin
