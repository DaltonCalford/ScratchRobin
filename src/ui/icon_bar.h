#ifndef SCRATCHROBIN_ICON_BAR_H
#define SCRATCHROBIN_ICON_BAR_H

#include <wx/toolbar.h>

namespace scratchrobin {

enum class IconBarType {
    Main,
    SqlEditor,
    Monitoring,
    UsersRoles,
    Diagram
};

wxToolBar* BuildIconBar(wxFrame* frame, IconBarType type, int iconSize);

} // namespace scratchrobin

#endif // SCRATCHROBIN_ICON_BAR_H
