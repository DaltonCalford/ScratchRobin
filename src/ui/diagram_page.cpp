/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "diagram_page.h"

#include "diagram_template_dialog.h"

#include <cctype>

#include <wx/button.h>
#include <wx/choice.h>
#include <wx/checkbox.h>
#include <wx/listbox.h>
#include <wx/msgdlg.h>
#include <wx/sizer.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/textdlg.h>
#include <wx/textctrl.h>

namespace scratchrobin {
namespace {

std::vector<std::string> BuildPaletteTypes(DiagramType type) {
    if (type == DiagramType::Silverston) {
        return {"Cluster", "Node", "Database", "Schema", "Table", "Service", "Host", "Network", "Dependency"};
    }
    if (type == DiagramType::Whiteboard) {
        return {"Note", "Group", "Sketch", "Image", "Link"};
    }
    if (type == DiagramType::MindMap) {
        return {"Topic", "Subtopic", "Idea", "Note", "Link"};
    }
    if (type == DiagramType::DataFlow) {
        return {"Process", "Data Store", "External", "Data Flow"};
    }
    return {"Table", "View", "Domain", "Sequence", "Relationship"};
}

std::vector<std::string> BuildTemplateKeys(DiagramType type) {
    if (type == DiagramType::Silverston) {
        return {"default", "infrastructure", "organization", "network"};
    }
    return {"default"};
}

wxArrayString BuildCardinalityChoices() {
    wxArrayString choices;
    choices.Add("1");
    choices.Add("0..1");
    choices.Add("1..N");
    choices.Add("0..N");
    return choices;
}

Cardinality CardinalityFromIndex(int index) {
    switch (index) {
        case 0:
            return Cardinality::One;
        case 1:
            return Cardinality::ZeroOrOne;
        case 2:
            return Cardinality::OneOrMany;
        case 3:
            return Cardinality::ZeroOrMany;
        default:
            return Cardinality::One;
    }
}

int CardinalityToIndex(Cardinality value) {
    switch (value) {
        case Cardinality::One:
            return 0;
        case Cardinality::ZeroOrOne:
            return 1;
        case Cardinality::OneOrMany:
            return 2;
        case Cardinality::ZeroOrMany:
            return 3;
        default:
            return 0;
    }
}

wxArrayString ToTemplateDisplay(const std::vector<std::string>& values) {
    wxArrayString arr;
    for (const auto& value : values) {
        if (value.empty()) {
            arr.Add(value);
        } else {
            std::string display = value;
            display[0] = static_cast<char>(std::toupper(display[0]));
            arr.Add(display);
        }
    }
    return arr;
}

wxArrayString ToWxArray(const std::vector<std::string>& values) {
    wxArrayString arr;
    for (const auto& value : values) {
        arr.Add(value);
    }
    return arr;
}

} // namespace

DiagramPage::DiagramPage(wxWindow* parent)
    : wxPanel(parent, wxID_ANY) {
    BuildLayout();
    PopulatePalette();
    PopulateTemplates();
    UpdateProperties();
}

void DiagramPage::BuildLayout() {
    auto* rootSizer = new wxBoxSizer(wxHORIZONTAL);

    auto* palettePanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(200, -1));
    palettePanel->SetBackgroundColour(wxColour(30, 30, 30));
    auto* paletteSizer = new wxBoxSizer(wxVERTICAL);
    auto* paletteLabel = new wxStaticText(palettePanel, wxID_ANY, "Palette");
    paletteLabel->SetForegroundColour(wxColour(220, 220, 220));
    paletteSizer->Add(paletteLabel, 0, wxALL, 8);

    palette_types_ = BuildPaletteTypes(diagram_type_);
    palette_list_ = new wxListBox(palettePanel, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                  ToWxArray(palette_types_));
    paletteSizer->Add(palette_list_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    palette_add_button_ = new wxButton(palettePanel, wxID_ANY, "Add");
    paletteSizer->Add(palette_add_button_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    palettePanel->SetSizer(paletteSizer);

    auto* canvasPanel = new wxPanel(this, wxID_ANY);
    auto* canvasSizer = new wxBoxSizer(wxVERTICAL);
    auto* topBar = new wxBoxSizer(wxHORIZONTAL);
    auto* typeLabel = new wxStaticText(canvasPanel, wxID_ANY, "Diagram Type:");
    typeLabel->SetForegroundColour(wxColour(220, 220, 220));
    topBar->Add(typeLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);

    diagram_type_choice_ = new wxChoice(canvasPanel, wxID_ANY);
    diagram_type_choice_->Append("ERD");
    diagram_type_choice_->Append("Silverston");
    diagram_type_choice_->Append("Whiteboard");
    diagram_type_choice_->Append("Mind Map");
    diagram_type_choice_->Append("DFD");
    diagram_type_choice_->SetSelection(static_cast<int>(diagram_type_));
    topBar->Add(diagram_type_choice_, 0, wxALIGN_CENTER_VERTICAL);
    
    // Notation selector (ERD only)
    topBar->AddSpacer(12);
    auto* notationLabel = new wxStaticText(canvasPanel, wxID_ANY, "Notation:");
    notationLabel->SetForegroundColour(wxColour(200, 200, 200));
    topBar->Add(notationLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    notation_choice_ = new wxChoice(canvasPanel, wxID_ANY);
    notation_choice_->Append("Crow's Foot");
    notation_choice_->Append("IDEF1X");
    notation_choice_->Append("UML");
    notation_choice_->Append("Chen");
    notation_choice_->SetSelection(0);
    topBar->Add(notation_choice_, 0, wxALIGN_CENTER_VERTICAL);
    
    topBar->AddStretchSpacer(1);
    mode_label_ = new wxStaticText(canvasPanel, wxID_ANY, "Mode: Select");
    mode_label_->SetForegroundColour(wxColour(200, 200, 200));
    topBar->Add(mode_label_, 0, wxALIGN_CENTER_VERTICAL);
    topBar->AddSpacer(16);
    auto* templateLabel = new wxStaticText(canvasPanel, wxID_ANY, "Template:");
    templateLabel->SetForegroundColour(wxColour(200, 200, 200));
    topBar->Add(templateLabel, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    template_choice_ = new wxChoice(canvasPanel, wxID_ANY);
    topBar->Add(template_choice_, 0, wxALIGN_CENTER_VERTICAL);
    template_edit_button_ = new wxButton(canvasPanel, wxID_ANY, "Edit...");
    topBar->Add(template_edit_button_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 6);

    canvasSizer->Add(topBar, 0, wxEXPAND | wxALL, 8);
    canvasSizer->Add(new wxStaticLine(canvasPanel), 0, wxEXPAND | wxLEFT | wxRIGHT, 8);

    canvas_ = new DiagramCanvas(canvasPanel, diagram_type_);
    canvasSizer->Add(canvas_, 1, wxEXPAND | wxALL, 8);
    canvasPanel->SetSizer(canvasSizer);

    auto* propsPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(240, -1));
    propsPanel->SetBackgroundColour(wxColour(30, 30, 30));
    auto* propsSizer = new wxBoxSizer(wxVERTICAL);
    selection_label_ = new wxStaticText(propsPanel, wxID_ANY, "Properties");
    selection_label_->SetForegroundColour(wxColour(220, 220, 220));
    propsSizer->Add(selection_label_, 0, wxALL, 8);

    auto* nameLabel = new wxStaticText(propsPanel, wxID_ANY, "Name");
    nameLabel->SetForegroundColour(wxColour(200, 200, 200));
    propsSizer->Add(nameLabel, 0, wxLEFT | wxRIGHT | wxTOP, 8);
    name_edit_ = new wxTextCtrl(propsPanel, wxID_ANY, "");
    propsSizer->Add(name_edit_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    auto* typeLabelValue = new wxStaticText(propsPanel, wxID_ANY, "Type");
    typeLabelValue->SetForegroundColour(wxColour(200, 200, 200));
    propsSizer->Add(typeLabelValue, 0, wxLEFT | wxRIGHT | wxTOP, 8);
    type_value_ = new wxStaticText(propsPanel, wxID_ANY, "-");
    type_value_->SetForegroundColour(wxColour(220, 220, 220));
    propsSizer->Add(type_value_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

    auto* idLabelValue = new wxStaticText(propsPanel, wxID_ANY, "ID");
    idLabelValue->SetForegroundColour(wxColour(200, 200, 200));
    propsSizer->Add(idLabelValue, 0, wxLEFT | wxRIGHT | wxTOP, 8);
    id_value_ = new wxStaticText(propsPanel, wxID_ANY, "-");
    id_value_->SetForegroundColour(wxColour(220, 220, 220));
    propsSizer->Add(id_value_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

    edge_label_label_ = new wxStaticText(propsPanel, wxID_ANY, "Edge Label");
    edge_label_label_->SetForegroundColour(wxColour(200, 200, 200));
    propsSizer->Add(edge_label_label_, 0, wxLEFT | wxRIGHT | wxTOP, 8);
    edge_label_edit_ = new wxTextCtrl(propsPanel, wxID_ANY, "");
    propsSizer->Add(edge_label_edit_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    auto* labelPosLabel = new wxStaticText(propsPanel, wxID_ANY, "Label Position");
    labelPosLabel->SetForegroundColour(wxColour(200, 200, 200));
    propsSizer->Add(labelPosLabel, 0, wxLEFT | wxRIGHT | wxTOP, 8);
    label_position_choice_ = new wxChoice(propsPanel, wxID_ANY);
    label_position_choice_->Append("Center");
    label_position_choice_->Append("Above");
    label_position_choice_->Append("Below");
    propsSizer->Add(label_position_choice_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    auto* cardLabel = new wxStaticText(propsPanel, wxID_ANY, "Cardinality");
    cardLabel->SetForegroundColour(wxColour(200, 200, 200));
    propsSizer->Add(cardLabel, 0, wxLEFT | wxRIGHT | wxTOP, 8);
    cardinality_source_choice_ = new wxChoice(propsPanel, wxID_ANY);
    cardinality_source_choice_->Append(BuildCardinalityChoices());
    cardinality_target_choice_ = new wxChoice(propsPanel, wxID_ANY);
    cardinality_target_choice_->Append(BuildCardinalityChoices());
    propsSizer->Add(cardinality_source_choice_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 8);
    propsSizer->Add(cardinality_target_choice_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    identifying_check_ = new wxCheckBox(propsPanel, wxID_ANY, "Identifying relationship");
    identifying_check_->SetForegroundColour(wxColour(200, 200, 200));
    propsSizer->Add(identifying_check_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

    propsSizer->AddStretchSpacer(1);
    propsPanel->SetSizer(propsSizer);

    rootSizer->Add(palettePanel, 0, wxEXPAND);
    rootSizer->Add(canvasPanel, 1, wxEXPAND);
    rootSizer->Add(propsPanel, 0, wxEXPAND);
    SetSizer(rootSizer);

    diagram_type_choice_->Bind(wxEVT_CHOICE, &DiagramPage::OnDiagramTypeChanged, this);
    notation_choice_->Bind(wxEVT_CHOICE, &DiagramPage::OnNotationChanged, this);
    template_choice_->Bind(wxEVT_CHOICE, &DiagramPage::OnTemplateChanged, this);
    template_edit_button_->Bind(wxEVT_BUTTON, &DiagramPage::OnTemplateEdit, this);
    palette_add_button_->Bind(wxEVT_BUTTON, &DiagramPage::OnPaletteAdd, this);
    palette_list_->Bind(wxEVT_LISTBOX_DCLICK, &DiagramPage::OnPaletteActivate, this);
    canvas_->Bind(EVT_DIAGRAM_SELECTION_CHANGED, &DiagramPage::OnSelectionChanged, this);
    name_edit_->Bind(wxEVT_TEXT, &DiagramPage::OnNameEdited, this);
    edge_label_edit_->Bind(wxEVT_TEXT, &DiagramPage::OnEdgeLabelEdited, this);
    label_position_choice_->Bind(wxEVT_CHOICE, &DiagramPage::OnLabelPositionChanged, this);
    cardinality_source_choice_->Bind(wxEVT_CHOICE, &DiagramPage::OnCardinalityChanged, this);
    cardinality_target_choice_->Bind(wxEVT_CHOICE, &DiagramPage::OnCardinalityChanged, this);
    identifying_check_->Bind(wxEVT_CHECKBOX, &DiagramPage::OnIdentifyingChanged, this);
    canvas_->Bind(wxEVT_CHAR_HOOK, &DiagramPage::OnCanvasKey, this);
}

void DiagramPage::PopulatePalette() {
    palette_types_ = BuildPaletteTypes(diagram_type_);
    palette_list_->Clear();
    palette_list_->Append(ToWxArray(palette_types_));
    if (!palette_types_.empty()) {
        palette_list_->SetSelection(0);
    }
}

void DiagramPage::PopulateTemplates() {
    template_keys_ = BuildTemplateKeys(diagram_type_);
    template_choice_->Clear();
    template_choice_->Append(ToTemplateDisplay(template_keys_));
    if (!template_keys_.empty()) {
        template_choice_->SetSelection(0);
        canvas_->SetTemplateKey(template_keys_.front());
    }
    template_edit_button_->Enable(diagram_type_ == DiagramType::Silverston);
}

void DiagramPage::UpdateProperties() {
    const DiagramNode* selected_node = canvas_->GetSelectedNode();
    const DiagramEdge* selected_edge = canvas_->GetSelectedEdge();
    if (!selected_node && !selected_edge) {
        selection_label_->SetLabel("Properties");
        name_edit_->ChangeValue("");
        name_edit_->Enable(false);
        type_value_->SetLabel("-");
        id_value_->SetLabel("-");
        edge_label_edit_->ChangeValue("");
        edge_label_edit_->Enable(false);
        edge_label_label_->Enable(false);
        label_position_choice_->Enable(false);
        cardinality_source_choice_->Enable(false);
        cardinality_target_choice_->Enable(false);
        identifying_check_->Enable(false);
        return;
    }

    if (selected_node) {
        selection_label_->SetLabel("Properties: " + selected_node->name);
        name_edit_->Enable(true);
        if (name_edit_->GetValue() != selected_node->name) {
            name_edit_->ChangeValue(selected_node->name);
        }
        type_value_->SetLabel(selected_node->type);
        id_value_->SetLabel(selected_node->id);
        edge_label_edit_->ChangeValue("");
        edge_label_edit_->Enable(false);
        edge_label_label_->Enable(false);
        label_position_choice_->Enable(false);
        cardinality_source_choice_->Enable(false);
        cardinality_target_choice_->Enable(false);
        identifying_check_->Enable(false);
        return;
    }

    selection_label_->SetLabel("Relationship Properties");
    name_edit_->ChangeValue("");
    name_edit_->Enable(false);
    type_value_->SetLabel("Edge");
    id_value_->SetLabel(selected_edge->id);
    edge_label_label_->Enable(true);
    edge_label_edit_->Enable(true);
    if (edge_label_edit_->GetValue() != selected_edge->label) {
        edge_label_edit_->ChangeValue(selected_edge->label);
    }
    label_position_choice_->Enable(true);
    if (selected_edge->label_offset > 0) {
        label_position_choice_->SetSelection(1);
    } else if (selected_edge->label_offset < 0) {
        label_position_choice_->SetSelection(2);
    } else {
        label_position_choice_->SetSelection(0);
    }
    bool erd_edge = diagram_type_ == DiagramType::Erd;
    cardinality_source_choice_->Enable(erd_edge);
    cardinality_target_choice_->Enable(erd_edge);
    identifying_check_->Enable(erd_edge);
    if (erd_edge) {
        cardinality_source_choice_->SetSelection(CardinalityToIndex(selected_edge->source_cardinality));
        cardinality_target_choice_->SetSelection(CardinalityToIndex(selected_edge->target_cardinality));
        identifying_check_->SetValue(selected_edge->identifying);
    }
}

void DiagramPage::OnDiagramTypeChanged(wxCommandEvent&) {
    int selection = diagram_type_choice_->GetSelection();
    switch (selection) {
        case 0:
            diagram_type_ = DiagramType::Erd;
            break;
        case 1:
            diagram_type_ = DiagramType::Silverston;
            break;
        case 2:
            diagram_type_ = DiagramType::Whiteboard;
            break;
        case 3:
            diagram_type_ = DiagramType::MindMap;
            break;
        case 4:
            diagram_type_ = DiagramType::DataFlow;
            break;
        default:
            diagram_type_ = DiagramType::Erd;
            break;
    }
    canvas_->SetDiagramType(diagram_type_);
    
    // Enable/disable notation selector based on diagram type
    notation_choice_->Enable(diagram_type_ == DiagramType::Erd);
    
    relationship_mode_ = false;
    relationship_source_id_.clear();
    relationship_kind_.clear();
    mode_label_->SetLabel("Mode: Select");
    PopulatePalette();
    PopulateTemplates();
    UpdateProperties();
}

void DiagramPage::OnNotationChanged(wxCommandEvent&) {
    if (diagram_type_ != DiagramType::Erd) {
        return;
    }
    
    int selection = notation_choice_->GetSelection();
    ErdNotation notation = static_cast<ErdNotation>(selection);
    canvas_->SetNotation(notation);
}

void DiagramPage::OnCardinalityChanged(wxCommandEvent& event) {
    auto* selected = canvas_->GetSelectedEdgeMutable();
    if (!selected) {
        return;
    }
    if (event.GetEventObject() == cardinality_source_choice_) {
        selected->source_cardinality = CardinalityFromIndex(cardinality_source_choice_->GetSelection());
    } else if (event.GetEventObject() == cardinality_target_choice_) {
        selected->target_cardinality = CardinalityFromIndex(cardinality_target_choice_->GetSelection());
    }
    canvas_->Refresh();
}

void DiagramPage::OnIdentifyingChanged(wxCommandEvent&) {
    auto* selected = canvas_->GetSelectedEdgeMutable();
    if (!selected) {
        return;
    }
    selected->identifying = identifying_check_->GetValue();
    canvas_->Refresh();
}

void DiagramPage::OnLabelPositionChanged(wxCommandEvent&) {
    auto* selected = canvas_->GetSelectedEdgeMutable();
    if (!selected) {
        return;
    }
    int choice = label_position_choice_->GetSelection();
    if (choice == 1) {
        selected->label_offset = 1;
    } else if (choice == 2) {
        selected->label_offset = -1;
    } else {
        selected->label_offset = 0;
    }
    canvas_->Refresh();
}

void DiagramPage::OnCanvasKey(wxKeyEvent& event) {
    int key = event.GetKeyCode();
    
    // Undo/Redo (Phase 3.3)
    if (event.ControlDown() && (key == 'Z' || key == 'z')) {
        if (event.ShiftDown()) {
            canvas_->Redo();
        } else {
            canvas_->Undo();
        }
        return;
    }
    if (event.ControlDown() && (key == 'Y' || key == 'y')) {
        canvas_->Redo();
        return;
    }
    
    if (key == 'L' || key == 'l') {
        StartRelationshipMode();
        return;
    }
    if (key == WXK_TAB) {
        if (event.ShiftDown()) {
            canvas_->SelectPreviousNode();
        } else {
            canvas_->SelectNextNode();
        }
        return;
    }
    if (key == WXK_ESCAPE && relationship_mode_) {
        relationship_mode_ = false;
        relationship_source_id_.clear();
        relationship_kind_.clear();
        mode_label_->SetLabel("Mode: Select");
        return;
    }
    event.Skip();
}

void DiagramPage::StartRelationshipMode() {
    const DiagramNode* source = canvas_->GetSelectedNode();
    if (!source) {
        wxMessageBox("Select a source object first.", "Relationship", wxOK | wxICON_INFORMATION, this);
        return;
    }
    relationship_mode_ = true;
    relationship_source_id_ = source->id;
    if (diagram_type_ == DiagramType::Silverston) {
        relationship_kind_ = "Dependency";
    } else if (diagram_type_ == DiagramType::DataFlow) {
        relationship_kind_ = "Flow";
    } else if (diagram_type_ == DiagramType::MindMap || diagram_type_ == DiagramType::Whiteboard) {
        relationship_kind_ = "Link";
    } else {
        relationship_kind_ = "Relationship";
    }
    mode_label_->SetLabel("Mode: Select target");
}

void DiagramPage::OnTemplateChanged(wxCommandEvent&) {
    int selection = template_choice_->GetSelection();
    if (selection == wxNOT_FOUND || selection >= static_cast<int>(template_keys_.size())) {
        return;
    }
    canvas_->SetTemplateKey(template_keys_[static_cast<size_t>(selection)]);
}

void DiagramPage::OnTemplateEdit(wxCommandEvent&) {
    if (diagram_type_ != DiagramType::Silverston) {
        return;
    }
    DiagramTemplateDialog dialog(this,
                                 canvas_->grid_size(),
                                 canvas_->icon_set(),
                                 canvas_->border_width(),
                                 canvas_->border_dashed());
    if (dialog.ShowModal() == wxID_OK) {
        canvas_->SetGridSize(dialog.grid_size());
        canvas_->SetIconSet(dialog.icon_set());
        canvas_->SetBorderWidth(dialog.border_width());
        canvas_->SetBorderDashed(dialog.border_dashed());
    }
}

void DiagramPage::OnPaletteAdd(wxCommandEvent&) {
    int selection = palette_list_->GetSelection();
    if (selection == wxNOT_FOUND || selection >= static_cast<int>(palette_types_.size())) {
        return;
    }
    const std::string& type = palette_types_[static_cast<size_t>(selection)];
    if ((diagram_type_ == DiagramType::Erd && type == "Relationship") ||
        (diagram_type_ == DiagramType::Silverston && type == "Dependency") ||
        (diagram_type_ == DiagramType::DataFlow && type == "Data Flow") ||
        ((diagram_type_ == DiagramType::Whiteboard || diagram_type_ == DiagramType::MindMap) && type == "Link")) {
        StartRelationshipMode();
        return;
    }
    std::string name = type + " " + std::to_string(canvas_->model().nodes().size() + 1);
    canvas_->AddNode(type, name);
    UpdateProperties();
}

void DiagramPage::OnPaletteActivate(wxCommandEvent& event) {
    OnPaletteAdd(event);
}

void DiagramPage::OnSelectionChanged(wxCommandEvent& event) {
    if (relationship_mode_ && event.GetString() == "node") {
        const DiagramNode* target = canvas_->GetSelectedNode();
        if (target && target->id != relationship_source_id_) {
            wxTextEntryDialog label_dialog(this, "Relationship label (optional)", relationship_kind_ + " Label");
            if (diagram_type_ == DiagramType::Silverston) {
                label_dialog.SetValue("depends_on");
            } else if (diagram_type_ == DiagramType::DataFlow) {
                label_dialog.SetValue("flow");
            } else {
                label_dialog.SetValue("FK");
            }
            if (label_dialog.ShowModal() == wxID_OK) {
                std::string label = label_dialog.GetValue().ToStdString();
                canvas_->AddEdge(relationship_source_id_, target->id, label);
            }
        }
        relationship_mode_ = false;
        relationship_source_id_.clear();
        relationship_kind_.clear();
        mode_label_->SetLabel("Mode: Select");
    }
    UpdateProperties();
}

void DiagramPage::OnNameEdited(wxCommandEvent&) {
    auto* selected = canvas_->GetSelectedNodeMutable();
    if (!selected) {
        return;
    }
    std::string value = name_edit_->GetValue().ToStdString();
    if (value == selected->name) {
        return;
    }
    selected->name = value;
    canvas_->Refresh();
    UpdateProperties();
}

void DiagramPage::OnEdgeLabelEdited(wxCommandEvent&) {
    auto* selected = canvas_->GetSelectedEdgeMutable();
    if (!selected) {
        return;
    }
    std::string value = edge_label_edit_->GetValue().ToStdString();
    if (value == selected->label) {
        return;
    }
    selected->label = value;
    canvas_->Refresh();
    UpdateProperties();
}

} // namespace scratchrobin
