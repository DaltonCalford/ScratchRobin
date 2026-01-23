#ifndef SCRATCHROBIN_MENU_BUILDER_H
#define SCRATCHROBIN_MENU_BUILDER_H

#include <wx/menu.h>

namespace scratchrobin {

struct MenuBuildOptions {
    bool includeConnections = true;
    bool includeEdit = true;
    bool includeView = true;
    bool includeWindow = true;
    bool includeHelp = true;
};

wxMenuBar* BuildMenuBar(const MenuBuildOptions& options);

} // namespace scratchrobin

#endif // SCRATCHROBIN_MENU_BUILDER_H
