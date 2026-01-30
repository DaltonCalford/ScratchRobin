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
