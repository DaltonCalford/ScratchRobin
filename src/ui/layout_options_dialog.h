/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_LAYOUT_OPTIONS_DIALOG_H
#define SCRATCHROBIN_LAYOUT_OPTIONS_DIALOG_H

#include "diagram/layout_engine.h"

#include <wx/dialog.h>

class wxChoice;
class wxSpinCtrl;
class wxCheckBox;
class wxRadioBox;

namespace scratchrobin {

class LayoutOptionsDialog : public wxDialog {
public:
    LayoutOptionsDialog(wxWindow* parent, diagram::LayoutOptions& options);

    bool IsConfirmed() const { return confirmed_; }

private:
    void BuildLayout();
    void OnAlgorithmChanged(wxCommandEvent& event);
    void OnOk(wxCommandEvent& event);
    void UpdateControlsForAlgorithm();

    diagram::LayoutOptions& options_;
    bool confirmed_ = false;
    
    // Algorithm selection
    wxChoice* algorithm_choice_ = nullptr;
    
    // Direction
    wxRadioBox* direction_radio_ = nullptr;
    
    // Spacing
    wxSpinCtrl* node_spacing_spin_ = nullptr;
    wxSpinCtrl* level_spacing_spin_ = nullptr;
    wxSpinCtrl* edge_spacing_spin_ = nullptr;
    
    // Sugiyama options
    wxCheckBox* minimize_crossings_chk_ = nullptr;
    wxSpinCtrl* iterations_spin_ = nullptr;
    
    // Force-directed specific
    wxSpinCtrl* repulsion_spin_ = nullptr;
    
    // Orthogonal options
    wxCheckBox* use_ports_chk_ = nullptr;
    
    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_LAYOUT_OPTIONS_DIALOG_H
