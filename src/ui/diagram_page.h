#ifndef SCRATCHROBIN_DIAGRAM_PAGE_H
#define SCRATCHROBIN_DIAGRAM_PAGE_H

#include "diagram_canvas.h"

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

private:
    void BuildLayout();
    void PopulatePalette();
    void PopulateTemplates();
    void UpdateProperties();

    void OnDiagramTypeChanged(wxCommandEvent& event);
    void OnTemplateChanged(wxCommandEvent& event);
    void OnTemplateEdit(wxCommandEvent& event);
    void OnPaletteAdd(wxCommandEvent& event);
    void OnPaletteActivate(wxCommandEvent& event);
    void OnSelectionChanged(wxCommandEvent& event);
    void OnNameEdited(wxCommandEvent& event);
    void OnEdgeLabelEdited(wxCommandEvent& event);
    void OnCardinalityChanged(wxCommandEvent& event);
    void OnIdentifyingChanged(wxCommandEvent& event);
    void OnLabelPositionChanged(wxCommandEvent& event);
    void OnCanvasKey(wxKeyEvent& event);

    void StartRelationshipMode();

    DiagramType diagram_type_ = DiagramType::Erd;
    bool relationship_mode_ = false;
    std::string relationship_source_id_;
    std::string relationship_kind_;
    wxChoice* diagram_type_choice_ = nullptr;
    wxChoice* template_choice_ = nullptr;
    wxButton* template_edit_button_ = nullptr;
    wxStaticText* mode_label_ = nullptr;
    wxListBox* palette_list_ = nullptr;
    wxButton* palette_add_button_ = nullptr;
    DiagramCanvas* canvas_ = nullptr;
    wxStaticText* selection_label_ = nullptr;
    wxTextCtrl* name_edit_ = nullptr;
    wxTextCtrl* edge_label_edit_ = nullptr;
    wxStaticText* edge_label_label_ = nullptr;
    wxChoice* label_position_choice_ = nullptr;
    wxChoice* cardinality_source_choice_ = nullptr;
    wxChoice* cardinality_target_choice_ = nullptr;
    wxCheckBox* identifying_check_ = nullptr;
    wxStaticText* type_value_ = nullptr;
    wxStaticText* id_value_ = nullptr;
    std::vector<std::string> palette_types_;
    std::vector<std::string> template_keys_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_DIAGRAM_PAGE_H
