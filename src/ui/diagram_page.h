/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_DIAGRAM_PAGE_H
#define SCRATCHROBIN_DIAGRAM_PAGE_H

#include "diagram_canvas.h"
#include "diagram/diagram_serialization.h"

#include <vector>

#include <wx/panel.h>

class wxChoice;
class wxListBox;
class wxTextCtrl;
class wxStaticText;
class wxButton;
class wxCheckBox;

namespace scratchrobin {

class DiagramPage : public wxPanel {
public:
    explicit DiagramPage(wxWindow* parent);
    bool SaveToFile(const std::string& path, std::string* error);
    bool LoadFromFile(const std::string& path, std::string* error);
    const std::string& file_path() const { return file_path_; }
    void set_file_path(const std::string& path) { file_path_ = path; }
    DiagramType diagram_type() const { return diagram_type_; }

private:
    void BuildLayout();
    void PopulatePalette();
    void PopulateTemplates();
    void UpdateProperties();

    void OnDiagramTypeChanged(wxCommandEvent& event);
    void OnNotationChanged(wxCommandEvent& event);
    void OnTemplateChanged(wxCommandEvent& event);
    void OnTemplateEdit(wxCommandEvent& event);
    void OnPaletteAdd(wxCommandEvent& event);
    void OnPaletteActivate(wxCommandEvent& event);
    void OnSelectionChanged(wxCommandEvent& event);
    void OnNameEdited(wxCommandEvent& event);
    void OnEdgeLabelEdited(wxCommandEvent& event);
    void OnEdgeTypeEdited(wxCommandEvent& event);
    void OnAttributesEdited(wxCommandEvent& event);
    void OnDomainWizard(wxCommandEvent& event);
    void OnParentIdEdited(wxCommandEvent& event);
    void OnTraceRefsEdited(wxCommandEvent& event);
    void OnOpenTrace(wxCommandEvent& event);
    void OnCardinalityChanged(wxCommandEvent& event);
    void OnIdentifyingChanged(wxCommandEvent& event);
    void OnLabelPositionChanged(wxCommandEvent& event);
    void OnCanvasKey(wxKeyEvent& event);

    void StartRelationshipMode();
    void SyncDocFromCanvas();
    void ApplyDocToCanvas();
    void SetDiagramTypeInternal(DiagramType type);

    DiagramType diagram_type_ = DiagramType::Erd;
    bool relationship_mode_ = false;
    std::string relationship_source_id_;
    std::string relationship_kind_;
    wxChoice* diagram_type_choice_ = nullptr;
    wxChoice* notation_choice_ = nullptr;  // ERD notation selector (Phase 3.2)
    wxChoice* template_choice_ = nullptr;
    wxButton* template_edit_button_ = nullptr;
    wxStaticText* mode_label_ = nullptr;
    wxListBox* palette_list_ = nullptr;
    wxButton* palette_add_button_ = nullptr;
    DiagramCanvas* canvas_ = nullptr;
    wxStaticText* selection_label_ = nullptr;
    wxTextCtrl* name_edit_ = nullptr;
    wxTextCtrl* edge_label_edit_ = nullptr;
    wxTextCtrl* edge_type_edit_ = nullptr;
    wxStaticText* edge_label_label_ = nullptr;
    wxTextCtrl* attributes_edit_ = nullptr;
    wxButton* domain_wizard_button_ = nullptr;
    wxChoice* label_position_choice_ = nullptr;
    wxChoice* cardinality_source_choice_ = nullptr;
    wxChoice* cardinality_target_choice_ = nullptr;
    wxCheckBox* identifying_check_ = nullptr;
    wxTextCtrl* parent_id_edit_ = nullptr;
    wxTextCtrl* trace_refs_edit_ = nullptr;
    wxButton* open_trace_button_ = nullptr;
    wxStaticText* type_value_ = nullptr;
    wxStaticText* id_value_ = nullptr;
    std::vector<std::string> palette_types_;
    std::vector<std::string> template_keys_;
    diagram::DiagramDocument doc_;
    std::string file_path_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_DIAGRAM_PAGE_H
