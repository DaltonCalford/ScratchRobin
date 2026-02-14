/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0
 */
#include "ui/layout/layout_preset.h"
#include "core/simple_json.h"
#include <fstream>
#include <sstream>

namespace scratchrobin {

// Forward declarations for helper functions
namespace {
    LayoutWindowState ParseWindowState(const JsonValue& obj);
    MonitorConfig ParseMonitorConfig(const JsonValue& obj);
}

// LayoutWindowState implementation
std::string LayoutWindowState::ToJson() const {
    JsonValue obj;
    obj.type = JsonValue::Type::Object;
    obj.object_value["window_id"] = JsonValue{JsonValue::Type::String, false, 0, window_id};
    obj.object_value["window_type"] = JsonValue{JsonValue::Type::String, false, 0, window_type};
    obj.object_value["is_visible"] = JsonValue{JsonValue::Type::Bool, is_visible};
    obj.object_value["is_docked"] = JsonValue{JsonValue::Type::Bool, is_docked};
    obj.object_value["dock_direction"] = JsonValue{JsonValue::Type::Number, false, static_cast<double>(static_cast<int>(dock_direction))};
    obj.object_value["floating_x"] = JsonValue{JsonValue::Type::Number, false, static_cast<double>(floating_rect.x)};
    obj.object_value["floating_y"] = JsonValue{JsonValue::Type::Number, false, static_cast<double>(floating_rect.y)};
    obj.object_value["floating_width"] = JsonValue{JsonValue::Type::Number, false, static_cast<double>(floating_rect.width)};
    obj.object_value["floating_height"] = JsonValue{JsonValue::Type::Number, false, static_cast<double>(floating_rect.height)};
    obj.object_value["dock_proportion"] = JsonValue{JsonValue::Type::Number, false, static_cast<double>(dock_proportion)};
    obj.object_value["dock_row"] = JsonValue{JsonValue::Type::Number, false, static_cast<double>(dock_row)};
    obj.object_value["dock_layer"] = JsonValue{JsonValue::Type::Number, false, static_cast<double>(dock_layer)};
    obj.object_value["is_maximized"] = JsonValue{JsonValue::Type::Bool, is_maximized};
    
    // Simple serialization
    std::string result = "{";
    bool first = true;
    for (const auto& [key, val] : obj.object_value) {
        if (!first) result += ",";
        first = false;
        result += "\"" + key + "\":";
        switch (val.type) {
            case JsonValue::Type::Bool:
                result += val.bool_value ? "true" : "false";
                break;
            case JsonValue::Type::Number:
                result += std::to_string(val.number_value);
                break;
            case JsonValue::Type::String:
                result += "\"" + val.string_value + "\"";
                break;
            default:
                result += "null";
        }
    }
    result += "}";
    return result;
}

LayoutWindowState LayoutWindowState::FromJson(const std::string& json) {
    LayoutWindowState state;
    JsonParser parser(json);
    JsonValue root;
    std::string error;
    
    if (!parser.Parse(&root, &error)) {
        return state;
    }
    
    if (root.type != JsonValue::Type::Object) {
        return state;
    }
    
    if (const auto* val = FindMember(root, "window_id")) {
        GetStringValue(*val, &state.window_id);
    }
    if (const auto* val = FindMember(root, "window_type")) {
        GetStringValue(*val, &state.window_type);
    }
    if (const auto* val = FindMember(root, "is_visible")) {
        GetBoolValue(*val, &state.is_visible);
    }
    if (const auto* val = FindMember(root, "is_docked")) {
        GetBoolValue(*val, &state.is_docked);
    }
    int64_t dir_val = 0;
    if (const auto* val = FindMember(root, "dock_direction")) {
        if (GetInt64Value(*val, &dir_val)) {
            state.dock_direction = static_cast<DockDirection>(dir_val);
        }
    }
    int64_t x = 0, y = 0, w = 0, h = 0;
    if (const auto* val = FindMember(root, "floating_x")) GetInt64Value(*val, &x);
    if (const auto* val = FindMember(root, "floating_y")) GetInt64Value(*val, &y);
    if (const auto* val = FindMember(root, "floating_width")) GetInt64Value(*val, &w);
    if (const auto* val = FindMember(root, "floating_height")) GetInt64Value(*val, &h);
    state.floating_rect = wxRect(static_cast<int>(x), static_cast<int>(y), static_cast<int>(w), static_cast<int>(h));
    
    int64_t prop = 25;
    if (const auto* val = FindMember(root, "dock_proportion")) GetInt64Value(*val, &prop);
    state.dock_proportion = static_cast<int>(prop);
    
    return state;
}

// MonitorConfig implementation
std::string MonitorConfig::ToJson() const {
    JsonValue obj;
    obj.type = JsonValue::Type::Object;
    obj.object_value["index"] = JsonValue{JsonValue::Type::Number, false, static_cast<double>(index)};
    obj.object_value["geometry_x"] = JsonValue{JsonValue::Type::Number, false, static_cast<double>(geometry.x)};
    obj.object_value["geometry_y"] = JsonValue{JsonValue::Type::Number, false, static_cast<double>(geometry.y)};
    obj.object_value["geometry_width"] = JsonValue{JsonValue::Type::Number, false, static_cast<double>(geometry.width)};
    obj.object_value["geometry_height"] = JsonValue{JsonValue::Type::Number, false, static_cast<double>(geometry.height)};
    obj.object_value["is_primary"] = JsonValue{JsonValue::Type::Bool, is_primary};
    obj.object_value["name"] = JsonValue{JsonValue::Type::String, false, 0, name};
    
    std::string result = "{";
    bool first = true;
    for (const auto& [key, val] : obj.object_value) {
        if (!first) result += ",";
        first = false;
        result += "\"" + key + "\":";
        switch (val.type) {
            case JsonValue::Type::Bool:
                result += val.bool_value ? "true" : "false";
                break;
            case JsonValue::Type::Number:
                result += std::to_string(val.number_value);
                break;
            case JsonValue::Type::String:
                result += "\"" + val.string_value + "\"";
                break;
            default:
                result += "null";
        }
    }
    result += "}";
    return result;
}

MonitorConfig MonitorConfig::FromJson(const std::string& json) {
    MonitorConfig config;
    JsonParser parser(json);
    JsonValue root;
    std::string error;
    
    if (!parser.Parse(&root, &error)) {
        return config;
    }
    
    int64_t val = 0;
    if (const auto* v = FindMember(root, "index")) {
        if (GetInt64Value(*v, &val)) config.index = static_cast<int>(val);
    }
    int64_t x = 0, y = 0, w = 1920, h = 1080;
    if (const auto* v = FindMember(root, "geometry_x")) GetInt64Value(*v, &x);
    if (const auto* v = FindMember(root, "geometry_y")) GetInt64Value(*v, &y);
    if (const auto* v = FindMember(root, "geometry_width")) GetInt64Value(*v, &w);
    if (const auto* v = FindMember(root, "geometry_height")) GetInt64Value(*v, &h);
    config.geometry = wxRect(static_cast<int>(x), static_cast<int>(y), static_cast<int>(w), static_cast<int>(h));
    
    if (const auto* v = FindMember(root, "is_primary")) {
        GetBoolValue(*v, &config.is_primary);
    }
    if (const auto* v = FindMember(root, "name")) {
        GetStringValue(*v, &config.name);
    }
    return config;
}

// LayoutPreset implementation
LayoutPreset::LayoutPreset(const std::string& name) : name_(name) {}

void LayoutPreset::SetWindowState(const std::string& window_id, const LayoutWindowState& state) {
    window_states_[window_id] = state;
}

LayoutWindowState LayoutPreset::GetWindowState(const std::string& window_id) const {
    auto it = window_states_.find(window_id);
    if (it != window_states_.end()) {
        return it->second;
    }
    return LayoutWindowState();
}

bool LayoutPreset::HasWindowState(const std::string& window_id) const {
    return window_states_.find(window_id) != window_states_.end();
}

void LayoutPreset::RemoveWindowState(const std::string& window_id) {
    window_states_.erase(window_id);
}

std::vector<std::string> LayoutPreset::GetWindowIds() const {
    std::vector<std::string> ids;
    for (const auto& [id, _] : window_states_) {
        ids.push_back(id);
    }
    return ids;
}

std::string LayoutPreset::ToJson() const {
    std::string result = "{";
    result += "\"name\":\"" + name_ + "\",";
    result += "\"description\":\"" + description_ + "\",";
    result += "\"is_default\":" + std::string(is_default_ ? "true" : "false") + ",";
    result += "\"version\":" + std::to_string(version_) + ",";
    result += "\"main_form_x\":" + std::to_string(main_form_rect_.x) + ",";
    result += "\"main_form_y\":" + std::to_string(main_form_rect_.y) + ",";
    result += "\"main_form_width\":" + std::to_string(main_form_rect_.width) + ",";
    result += "\"main_form_height\":" + std::to_string(main_form_rect_.height) + ",";
    result += "\"main_form_maximized\":" + std::string(main_form_maximized_ ? "true" : "false") + ",";
    result += "\"monitor\":" + monitor_config_.ToJson() + ",";
    result += "\"windows\":[";
    bool first = true;
    for (const auto& [id, state] : window_states_) {
        if (!first) result += ",";
        first = false;
        result += state.ToJson();
    }
    result += "]}";
    return result;
}

LayoutPreset LayoutPreset::FromJson(const std::string& json) {
    LayoutPreset preset;
    JsonParser parser(json);
    JsonValue root;
    std::string error;
    
    if (!parser.Parse(&root, &error)) {
        return preset;
    }
    
    if (root.type != JsonValue::Type::Object) {
        return preset;
    }
    
    if (const auto* val = FindMember(root, "name")) {
        GetStringValue(*val, &preset.name_);
    }
    if (const auto* val = FindMember(root, "description")) {
        GetStringValue(*val, &preset.description_);
    }
    if (const auto* val = FindMember(root, "is_default")) {
        GetBoolValue(*val, &preset.is_default_);
    }
    int64_t version = 1;
    if (const auto* val = FindMember(root, "version")) {
        GetInt64Value(*val, &version);
    }
    preset.version_ = static_cast<int>(version);
    
    int64_t x = 100, y = 100, w = 1280, h = 900;
    if (const auto* val = FindMember(root, "main_form_x")) GetInt64Value(*val, &x);
    if (const auto* val = FindMember(root, "main_form_y")) GetInt64Value(*val, &y);
    if (const auto* val = FindMember(root, "main_form_width")) GetInt64Value(*val, &w);
    if (const auto* val = FindMember(root, "main_form_height")) GetInt64Value(*val, &h);
    preset.main_form_rect_ = wxRect(static_cast<int>(x), static_cast<int>(y), static_cast<int>(w), static_cast<int>(h));
    
    if (const auto* val = FindMember(root, "main_form_maximized")) {
        GetBoolValue(*val, &preset.main_form_maximized_);
    }
    
    if (const auto* val = FindMember(root, "monitor")) {
        if (val->type == JsonValue::Type::Object) {
            // Parse directly from JsonValue
            preset.monitor_config_ = ParseMonitorConfig(*val);
        }
    }
    
    if (const auto* val = FindMember(root, "windows")) {
        if (val->type == JsonValue::Type::Array) {
            for (const auto& window_val : val->array_value) {
                if (window_val.type == JsonValue::Type::Object) {
                    LayoutWindowState state = ParseWindowState(window_val);
                    if (!state.window_id.empty()) {
                        preset.window_states_[state.window_id] = state;
                    }
                }
            }
        }
    }
    
    return preset;
}

bool LayoutPreset::SaveToFile(const std::string& path) const {
    std::ofstream file(path);
    if (!file.is_open()) return false;
    file << ToJson();
    return true;
}

LayoutPreset LayoutPreset::LoadFromFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return LayoutPreset();
    std::stringstream buffer;
    buffer << file.rdbuf();
    return FromJson(buffer.str());
}

// Helper functions for parsing
namespace {
    LayoutWindowState ParseWindowState(const JsonValue& obj) {
        LayoutWindowState state;
        if (const auto* val = FindMember(obj, "window_id")) {
            GetStringValue(*val, &state.window_id);
        }
        if (const auto* val = FindMember(obj, "window_type")) {
            GetStringValue(*val, &state.window_type);
        }
        if (const auto* val = FindMember(obj, "is_visible")) {
            GetBoolValue(*val, &state.is_visible);
        }
        if (const auto* val = FindMember(obj, "is_docked")) {
            GetBoolValue(*val, &state.is_docked);
        }
        int64_t dir_val = 0;
        if (const auto* val = FindMember(obj, "dock_direction")) {
            if (GetInt64Value(*val, &dir_val)) {
                state.dock_direction = static_cast<DockDirection>(dir_val);
            }
        }
        int64_t x = 0, y = 0, w = 0, h = 0;
        if (const auto* val = FindMember(obj, "floating_x")) GetInt64Value(*val, &x);
        if (const auto* val = FindMember(obj, "floating_y")) GetInt64Value(*val, &y);
        if (const auto* val = FindMember(obj, "floating_width")) GetInt64Value(*val, &w);
        if (const auto* val = FindMember(obj, "floating_height")) GetInt64Value(*val, &h);
        state.floating_rect = wxRect(static_cast<int>(x), static_cast<int>(y), static_cast<int>(w), static_cast<int>(h));
        int64_t prop = 25;
        if (const auto* val = FindMember(obj, "dock_proportion")) GetInt64Value(*val, &prop);
        state.dock_proportion = static_cast<int>(prop);
        return state;
    }
    
    MonitorConfig ParseMonitorConfig(const JsonValue& obj) {
        MonitorConfig config;
        int64_t val = 0;
        if (const auto* v = FindMember(obj, "index")) {
            if (GetInt64Value(*v, &val)) config.index = static_cast<int>(val);
        }
        int64_t x = 0, y = 0, w = 1920, h = 1080;
        if (const auto* v = FindMember(obj, "geometry_x")) GetInt64Value(*v, &x);
        if (const auto* v = FindMember(obj, "geometry_y")) GetInt64Value(*v, &y);
        if (const auto* v = FindMember(obj, "geometry_width")) GetInt64Value(*v, &w);
        if (const auto* v = FindMember(obj, "geometry_height")) GetInt64Value(*v, &h);
        config.geometry = wxRect(static_cast<int>(x), static_cast<int>(y), static_cast<int>(w), static_cast<int>(h));
        if (const auto* v = FindMember(obj, "is_primary")) {
            GetBoolValue(*v, &config.is_primary);
        }
        if (const auto* v = FindMember(obj, "name")) {
            GetStringValue(*v, &config.name);
        }
        return config;
    }
}

// Factory methods
LayoutPreset LayoutPreset::CreateDefault() {
    LayoutPreset preset("Default");
    preset.SetDescription("Standard layout with navigator on the left");
    preset.SetDefault(true);
    
    LayoutWindowState navigator;
    navigator.window_id = "navigator";
    navigator.window_type = "navigator";
    navigator.is_visible = true;
    navigator.is_docked = true;
    navigator.dock_direction = DockDirection::Left;
    navigator.dock_proportion = 25;
    preset.SetWindowState("navigator", navigator);
    
    return preset;
}

LayoutPreset LayoutPreset::CreateSingleMonitor() {
    LayoutPreset preset("Single Monitor");
    preset.SetDescription("Optimized for single display");
    return preset;
}

LayoutPreset LayoutPreset::CreateDualMonitor() {
    LayoutPreset preset("Dual Monitor");
    preset.SetDescription("Spreads windows across two displays");
    return preset;
}

LayoutPreset LayoutPreset::CreateWideScreen() {
    LayoutPreset preset("Wide Screen");
    preset.SetDescription("Optimized for ultrawide displays");
    return preset;
}

LayoutPreset LayoutPreset::CreateCompact() {
    LayoutPreset preset("Compact");
    preset.SetDescription("Minimal UI for maximum document space");
    
    LayoutWindowState navigator;
    navigator.window_id = "navigator";
    navigator.window_type = "navigator";
    navigator.is_visible = false;
    preset.SetWindowState("navigator", navigator);
    
    return preset;
}

} // namespace scratchrobin
