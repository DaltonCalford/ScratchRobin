/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0
 */
#include "resource_paths.h"

#include <cstdlib>
#include <filesystem>
#include <wx/filename.h>
#include <wx/stdpaths.h>

namespace scratchrobin {

std::string ResourcePaths::GetResourcesDir() {
    // Check for AppImage/portable mode environment variable
    const char* envResources = std::getenv("SCRATCHROBIN_RESOURCES");
    if (envResources && std::filesystem::exists(envResources)) {
        return envResources;
    }
    
    // Check relative to executable location (for portable installs)
    wxString exePath = wxStandardPaths::Get().GetExecutablePath();
    wxFileName exeDir(exePath);
    exeDir.SetFullName("");
    
    // Try exe_dir/../share/scratchrobin (standard Linux layout)
    wxFileName shareDir(exeDir);
    shareDir.RemoveLastDir();
    shareDir.AppendDir("share");
    shareDir.AppendDir("scratchrobin");
    if (shareDir.DirExists()) {
        return shareDir.GetPath().ToStdString();
    }
    
    // Try exe_dir/assets (development layout)
    wxFileName assetsDir(exeDir);
    assetsDir.AppendDir("assets");
    if (assetsDir.DirExists()) {
        return exeDir.GetPath().ToStdString();
    }
    
    // Fallback to current working directory
    return ".";
}

wxString ResourcePaths::GetResourcesDirWx() {
    return wxString::FromUTF8(GetResourcesDir());
}

std::string ResourcePaths::GetTranslationPath(const std::string& locale) {
    std::filesystem::path base(GetResourcesDir());
    base /= "translations";
    base /= locale + ".json";
    return base.string();
}

std::string ResourcePaths::GetIconPath(const std::string& name, int size,
                                       const std::string& extension) {
    std::filesystem::path base(GetResourcesDir());
    base /= "assets";
    base /= "icons";
    base /= name + "@" + std::to_string(size) + "." + extension;
    return base.string();
}

std::string ResourcePaths::GetSvgIconPath(const std::string& name) {
    std::filesystem::path base(GetResourcesDir());
    base /= "assets";
    base /= "icons";
    base /= name + ".svg";
    return base.string();
}

bool ResourcePaths::ResourceExists(const std::string& relativePath) {
    std::filesystem::path base(GetResourcesDir());
    base /= relativePath;
    return std::filesystem::exists(base);
}

} // namespace scratchrobin
