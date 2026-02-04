/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_MENU_BUILDER_H
#define SCRATCHROBIN_MENU_BUILDER_H

#include <wx/menu.h>

class wxFrame;

namespace scratchrobin {

struct MenuBuildOptions {
    bool includeConnections = true;
    bool includeObjects = true;
    bool includeEdit = true;
    bool includeView = true;
    bool includeAdmin = true;
    bool includeTools = true;  // Beta placeholder features
    bool includeWindow = true;
    bool includeHelp = true;
};

class WindowManager;

wxMenuBar* BuildMenuBar(const MenuBuildOptions& options,
                        WindowManager* window_manager,
                        wxFrame* current_frame);
wxMenuBar* BuildMenuBar(const MenuBuildOptions& options);

} // namespace scratchrobin

#endif // SCRATCHROBIN_MENU_BUILDER_H
