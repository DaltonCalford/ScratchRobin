/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "layout_options_dialog.h"

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/radiobox.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>

namespace scratchrobin {

wxBEGIN_EVENT_TABLE(LayoutOptionsDialog, wxDialog)
    EVT_CHOICE(wxID_ANY, LayoutOptionsDialog::OnAlgorithmChanged)
    EVT_BUTTON(wxID_OK, LayoutOptionsDialog::OnOk)
wxEND_EVENT_TABLE()

LayoutOptionsDialog::LayoutOptionsDialog(wxWindow* parent, diagram::LayoutOptions& options)
    : wxDialog(parent, wxID_ANY, "Layout Options",
               wxDefaultPosition, wxSize(450, 500)),
      options_(options) {
    BuildLayout();
    UpdateControlsForAlgorithm();
}

void LayoutOptionsDialog::BuildLayout() {
    auto* root = new wxBoxSizer(wxVERTICAL);
    
    // Algorithm selection
    auto* algoSizer = new wxBoxSizer(wxHORIZONTAL);
    algoSizer->Add(new wxStaticText(this, wxID_ANY, "Algorithm:"), 0,
                   wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    algorithm_choice_ = new wxChoice(this, wxID_ANY);
    algorithm_choice_->Append("Sugiyama (Hierarchical)");
    algorithm_choice_->Append("Force-Directed");
    algorithm_choice_->Append("Orthogonal");
    algorithm_choice_->SetSelection(static_cast<int>(options_.algorithm));
    algoSizer->Add(algorithm_choice_, 1, wxEXPAND);
    root->Add(algoSizer, 0, wxEXPAND | wxALL, 12);
    
    // Direction
    wxArrayString directions;
    directions.Add("Top to Bottom");
    directions.Add("Bottom to Top");
    directions.Add("Left to Right");
    directions.Add("Right to Left");
    direction_radio_ = new wxRadioBox(this, wxID_ANY, "Direction",
                                       wxDefaultPosition, wxDefaultSize, directions, 2);
    direction_radio_->SetSelection(static_cast<int>(options_.direction));
    root->Add(direction_radio_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    // Spacing section
    auto* spacingBox = new wxStaticBoxSizer(wxVERTICAL, this, "Spacing");
    
    auto* nodeSpacingSizer = new wxBoxSizer(wxHORIZONTAL);
    nodeSpacingSizer->Add(new wxStaticText(this, wxID_ANY, "Node Spacing:"), 0,
                          wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    node_spacing_spin_ = new wxSpinCtrl(this, wxID_ANY, wxEmptyString,
                                        wxDefaultPosition, wxSize(80, -1));
    node_spacing_spin_->SetRange(50, 500);
    node_spacing_spin_->SetValue(static_cast<int>(options_.node_spacing));
    nodeSpacingSizer->Add(node_spacing_spin_, 0);
    nodeSpacingSizer->Add(new wxStaticText(this, wxID_ANY, " pixels"), 0,
                          wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
    spacingBox->Add(nodeSpacingSizer, 0, wxEXPAND | wxALL, 8);
    
    auto* levelSpacingSizer = new wxBoxSizer(wxHORIZONTAL);
    levelSpacingSizer->Add(new wxStaticText(this, wxID_ANY, "Level Spacing:"), 0,
                           wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    level_spacing_spin_ = new wxSpinCtrl(this, wxID_ANY, wxEmptyString,
                                         wxDefaultPosition, wxSize(80, -1));
    level_spacing_spin_->SetRange(50, 500);
    level_spacing_spin_->SetValue(static_cast<int>(options_.level_spacing));
    levelSpacingSizer->Add(level_spacing_spin_, 0);
    levelSpacingSizer->Add(new wxStaticText(this, wxID_ANY, " pixels"), 0,
                           wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
    spacingBox->Add(levelSpacingSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    
    auto* paddingSizer = new wxBoxSizer(wxHORIZONTAL);
    paddingSizer->Add(new wxStaticText(this, wxID_ANY, "Padding:"), 0,
                      wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    edge_spacing_spin_ = new wxSpinCtrl(this, wxID_ANY, wxEmptyString,
                                        wxDefaultPosition, wxSize(80, -1));
    edge_spacing_spin_->SetRange(10, 200);
    edge_spacing_spin_->SetValue(static_cast<int>(options_.padding));
    paddingSizer->Add(edge_spacing_spin_, 0);
    paddingSizer->Add(new wxStaticText(this, wxID_ANY, " pixels"), 0,
                      wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
    spacingBox->Add(paddingSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    
    root->Add(spacingBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    // Sugiyama options
    auto* sgBox = new wxStaticBoxSizer(wxVERTICAL, this, "Sugiyama Options");
    
    minimize_crossings_chk_ = new wxCheckBox(this, wxID_ANY, "Minimize edge crossings");
    minimize_crossings_chk_->SetValue(options_.minimize_crossings);
    sgBox->Add(minimize_crossings_chk_, 0, wxALL, 8);
    
    auto* iterSizer = new wxBoxSizer(wxHORIZONTAL);
    iterSizer->Add(new wxStaticText(this, wxID_ANY, "Max Iterations:"), 0,
                   wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    iterations_spin_ = new wxSpinCtrl(this, wxID_ANY, wxEmptyString,
                                      wxDefaultPosition, wxSize(80, -1));
    iterations_spin_->SetRange(1, 100);
    iterations_spin_->SetValue(options_.max_iterations);
    iterSizer->Add(iterations_spin_, 0);
    sgBox->Add(iterSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    
    root->Add(sgBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    // Force-directed specific options
    auto* fdBox = new wxStaticBoxSizer(wxVERTICAL, this, "Force-Directed Options");
    
    auto* fdIterSizer = new wxBoxSizer(wxHORIZONTAL);
    fdIterSizer->Add(new wxStaticText(this, wxID_ANY, "Iterations:"), 0,
                     wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    wxSpinCtrl* fd_iterations_spin = new wxSpinCtrl(this, wxID_ANY, wxEmptyString,
                                                    wxDefaultPosition, wxSize(80, -1));
    fd_iterations_spin->SetRange(10, 1000);
    fd_iterations_spin->SetValue(options_.fd_iterations);
    fdIterSizer->Add(fd_iterations_spin, 0);
    fdBox->Add(fdIterSizer, 0, wxEXPAND | wxALL, 8);
    
    auto* repulsionSizer = new wxBoxSizer(wxHORIZONTAL);
    repulsionSizer->Add(new wxStaticText(this, wxID_ANY, "Repulsion:"), 0,
                        wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    repulsion_spin_ = new wxSpinCtrl(this, wxID_ANY, wxEmptyString,
                                     wxDefaultPosition, wxSize(80, -1));
    repulsion_spin_->SetRange(100, 5000);
    repulsion_spin_->SetValue(static_cast<int>(options_.repulsion_force));
    repulsionSizer->Add(repulsion_spin_, 0);
    fdBox->Add(repulsionSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    
    auto* attractionSizer = new wxBoxSizer(wxHORIZONTAL);
    attractionSizer->Add(new wxStaticText(this, wxID_ANY, "Attraction:"), 0,
                         wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    wxSpinCtrl* attraction_spin = new wxSpinCtrl(this, wxID_ANY, wxEmptyString,
                                                 wxDefaultPosition, wxSize(80, -1));
    attraction_spin->SetRange(1, 100);
    attraction_spin->SetValue(static_cast<int>(options_.attraction_force * 1000));
    attractionSizer->Add(attraction_spin, 0);
    fdBox->Add(attractionSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    
    root->Add(fdBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    // Orthogonal options
    auto* orthoBox = new wxStaticBoxSizer(wxVERTICAL, this, "Orthogonal Options");
    use_ports_chk_ = new wxCheckBox(this, wxID_ANY, "Use connection ports");
    use_ports_chk_->SetValue(options_.use_ports);
    orthoBox->Add(use_ports_chk_, 0, wxALL, 8);
    root->Add(orthoBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    // Buttons
    auto* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    btnSizer->AddStretchSpacer(1);
    btnSizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0, wxRIGHT, 8);
    btnSizer->Add(new wxButton(this, wxID_OK, "Apply"), 0);
    root->Add(btnSizer, 0, wxEXPAND | wxALL, 12);
    
    SetSizer(root);
}

void LayoutOptionsDialog::UpdateControlsForAlgorithm() {
    int algo = algorithm_choice_->GetSelection();
    bool isSugiyama = (algo == 0);
    bool isForceDirected = (algo == 1);
    bool isOrthogonal = (algo == 2);
    
    // Enable/disable algorithm-specific controls
    direction_radio_->Enable(isSugiyama || isOrthogonal);
    
    // Sugiyama options
    minimize_crossings_chk_->Enable(isSugiyama);
    iterations_spin_->Enable(isSugiyama);
}

void LayoutOptionsDialog::OnAlgorithmChanged(wxCommandEvent&) {
    UpdateControlsForAlgorithm();
}

void LayoutOptionsDialog::OnOk(wxCommandEvent& event) {
    // Save options
    options_.algorithm = static_cast<diagram::LayoutAlgorithm>(algorithm_choice_->GetSelection());
    options_.direction = static_cast<diagram::LayoutOptions::Direction>(direction_radio_->GetSelection());
    options_.node_spacing = node_spacing_spin_->GetValue();
    options_.level_spacing = level_spacing_spin_->GetValue();
    options_.padding = edge_spacing_spin_->GetValue();
    options_.minimize_crossings = minimize_crossings_chk_->GetValue();
    options_.max_iterations = iterations_spin_->GetValue();
    options_.repulsion_force = repulsion_spin_->GetValue();
    options_.use_ports = use_ports_chk_->GetValue();
    
    confirmed_ = true;
    event.Skip();
}

} // namespace scratchrobin
