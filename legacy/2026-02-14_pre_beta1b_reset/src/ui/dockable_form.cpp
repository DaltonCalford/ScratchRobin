/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0
 */
#include "dockable_form.h"

namespace scratchrobin {

DockableForm::DockableForm(wxWindow* parent, const wxString& title)
    : wxPanel(parent, wxID_ANY)
    , document_title_(title) {
}

void DockableForm::OnActivate() {
    is_active_ = true;
}

void DockableForm::OnDeactivate() {
    is_active_ = false;
}

} // namespace scratchrobin
