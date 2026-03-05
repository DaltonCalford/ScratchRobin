/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#include "query_plan_visualizer.h"

#include <wx/sizer.h>
#include <wx/button.h>

namespace scratchrobin::ui {

wxBEGIN_EVENT_TABLE(QueryPlanVisualizer, wxDialog)
wxEND_EVENT_TABLE()

QueryPlanVisualizer::QueryPlanVisualizer(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, _("Query Plan Visualizer"), wxDefaultPosition,
               wxSize(800, 600)) {
  CreateControls();
  SetupLayout();
  CentreOnParent();
}

QueryPlanVisualizer::~QueryPlanVisualizer() = default;

void QueryPlanVisualizer::CreateControls() {
  // TODO: Implement
}

void QueryPlanVisualizer::SetupLayout() {
  wxBoxSizer* sizer = new wxBoxSizer(wxVERTICAL);
  sizer->Add(new wxButton(this, wxID_OK, _("&Close")), 0, wxALIGN_CENTER | wxALL, 10);
  SetSizer(sizer);
}

}  // namespace scratchrobin::ui
