#include "diagram_template_dialog.h"

#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>

namespace scratchrobin {

namespace {

int FindChoiceIndex(wxChoice* choice, const wxString& value) {
    int index = choice->FindString(value);
    return index == wxNOT_FOUND ? 0 : index;
}

} // namespace

DiagramTemplateDialog::DiagramTemplateDialog(wxWindow* parent,
                                             int grid_size,
                                             const std::string& icon_set,
                                             int border_width,
                                             bool border_dashed)
    : wxDialog(parent, wxID_ANY, "Silverston Template", wxDefaultPosition, wxSize(360, 260),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {
    auto* rootSizer = new wxBoxSizer(wxVERTICAL);

    auto* gridLabel = new wxStaticText(this, wxID_ANY, "Grid Size");
    rootSizer->Add(gridLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    grid_choice_ = new wxChoice(this, wxID_ANY);
    grid_choice_->Append("4");
    grid_choice_->Append("8");
    grid_choice_->Append("16");
    grid_choice_->Append("32");
    grid_choice_->SetSelection(FindChoiceIndex(grid_choice_, wxString::Format("%d", grid_size)));
    rootSizer->Add(grid_choice_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    auto* iconLabel = new wxStaticText(this, wxID_ANY, "Icon Set");
    rootSizer->Add(iconLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    icon_choice_ = new wxChoice(this, wxID_ANY);
    icon_choice_->Append("Default");
    icon_choice_->Append("Mono");
    icon_choice_->Append("Accent");
    wxString iconValue = "Default";
    if (icon_set == "mono") {
        iconValue = "Mono";
    } else if (icon_set == "accent") {
        iconValue = "Accent";
    }
    icon_choice_->SetSelection(FindChoiceIndex(icon_choice_, iconValue));
    rootSizer->Add(icon_choice_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    auto* borderLabel = new wxStaticText(this, wxID_ANY, "Border Width");
    rootSizer->Add(borderLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    border_width_ = new wxSpinCtrl(this, wxID_ANY);
    border_width_->SetRange(1, 4);
    border_width_->SetValue(border_width);
    rootSizer->Add(border_width_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    auto* styleLabel = new wxStaticText(this, wxID_ANY, "Border Style");
    rootSizer->Add(styleLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    border_style_ = new wxChoice(this, wxID_ANY);
    border_style_->Append("Solid");
    border_style_->Append("Dashed");
    border_style_->SetSelection(border_dashed ? 1 : 0);
    rootSizer->Add(border_style_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    rootSizer->AddStretchSpacer(1);
    rootSizer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 12);

    SetSizerAndFit(rootSizer);
    CentreOnParent();
}

int DiagramTemplateDialog::grid_size() const {
    long value = 16;
    if (grid_choice_->GetStringSelection().ToLong(&value)) {
        return static_cast<int>(value);
    }
    return 16;
}

std::string DiagramTemplateDialog::icon_set() const {
    wxString selection = icon_choice_->GetStringSelection().Lower();
    return selection.ToStdString();
}

int DiagramTemplateDialog::border_width() const {
    return border_width_->GetValue();
}

bool DiagramTemplateDialog::border_dashed() const {
    return border_style_->GetSelection() == 1;
}

} // namespace scratchrobin
