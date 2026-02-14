/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0
 */
#include "ui/layout/dockable_window.h"

namespace scratchrobin {

// DockableFormBase implementation
DockableFormBase::DockableFormBase(wxWindow* parent, const std::string& id, const std::string& title)
    : wxPanel(parent, wxID_ANY)
    , window_id_(id)
    , window_title_(title) {
}

void DockableFormBase::Activate() {
    if (!is_active_) {
        is_active_ = true;
        OnActivated();
    }
}

void DockableFormBase::Deactivate() {
    if (is_active_) {
        is_active_ = false;
        OnDeactivated();
    }
}

} // namespace scratchrobin
