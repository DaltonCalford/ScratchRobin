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
