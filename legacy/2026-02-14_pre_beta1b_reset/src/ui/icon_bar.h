/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_ICON_BAR_H
#define SCRATCHROBIN_ICON_BAR_H

#include <wx/toolbar.h>
#include <wx/string.h>

namespace scratchrobin {

// Forward declarations
class DraggableToolBar;
class IconBarHost;

enum class IconBarType {
    Main,
    SqlEditor,
    Monitoring,
    UsersRoles,
    Diagram
};

// Build a standard icon bar
wxToolBar* BuildIconBar(wxFrame* frame, IconBarType type, int iconSize);

// Build a draggable icon bar (new)
DraggableToolBar* BuildDraggableIconBar(wxFrame* frame, IconBarHost* host, 
                                        IconBarType type, int iconSize);

// Get the toolbar name for a type
wxString GetIconBarTypeName(IconBarType type);

} // namespace scratchrobin

#endif // SCRATCHROBIN_ICON_BAR_H
