/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "query_history_dialog.h"

#include <wx/sizer.h>
#include <wx/button.h>

namespace scratchrobin::ui {

wxBEGIN_EVENT_TABLE(QueryHistoryDialog, wxDialog)
wxEND_EVENT_TABLE()

QueryHistoryDialog::QueryHistoryDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, _("Query History"), wxDefaultPosition,
               wxSize(600, 400)) {
  CreateControls();
  SetupLayout();
  CentreOnParent();
}

QueryHistoryDialog::~QueryHistoryDialog() = default;

void QueryHistoryDialog::CreateControls() {
  // TODO: Implement
}

void QueryHistoryDialog::SetupLayout() {
  wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
  sizer->Add(new wxButton(this, wxID_OK, _("&Close")), 0, wxALIGN_CENTER | wxALL, 10);
  SetSizer(sizer);
}

}  // namespace scratchrobin::ui
