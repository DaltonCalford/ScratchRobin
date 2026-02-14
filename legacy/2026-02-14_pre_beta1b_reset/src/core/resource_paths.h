/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0
 */
#ifndef SCRATCHROBIN_RESOURCE_PATHS_H
#define SCRATCHROBIN_RESOURCE_PATHS_H

#include <string>
#include <wx/string.h>

namespace scratchrobin {

/**
 * @brief Utility class for resolving resource paths.
 * 
 * Supports portable AppImage deployment by checking environment variable
 * SCRATCHROBIN_RESOURCES first, then falling back to local paths.
 */
class ResourcePaths {
public:
    /**
     * @brief Get the base directory for application resources.
     * 
     * Checks SCRATCHROBIN_RESOURCES environment variable first,
     * then falls back to current working directory.
     */
    static std::string GetResourcesDir();
    
    /**
     * @brief Get the path to a translation file.
     * @param locale The locale code (e.g., "en_CA", "fr_CA")
     * @return Full path to the translation JSON file
     */
    static std::string GetTranslationPath(const std::string& locale);
    
    /**
     * @brief Get the path to an icon.
     * @param name Icon name (e.g., "sql", "save")
     * @param size Icon size (16, 24, 32, 48)
     * @param extension File extension (default: "png")
     * @return Full path to the icon file
     */
    static std::string GetIconPath(const std::string& name, int size, 
                                   const std::string& extension = "png");
    
    /**
     * @brief Get the path to an SVG icon.
     * @param name Icon name
     * @return Full path to the SVG file
     */
    static std::string GetSvgIconPath(const std::string& name);
    
    /**
     * @brief Check if a resource file exists.
     * @param relativePath Path relative to resources directory
     * @return true if file exists
     */
    static bool ResourceExists(const std::string& relativePath);
    
    /**
     * @brief Get wxString version of resources directory.
     */
    static wxString GetResourcesDirWx();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_RESOURCE_PATHS_H
