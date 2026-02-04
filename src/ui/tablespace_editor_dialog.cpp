/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "tablespace_editor_dialog.h"

#include <algorithm>
#include <cctype>
#include <sstream>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/dirdlg.h>
#include <wx/filedlg.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

namespace scratchrobin {
namespace {

std::string Trim(std::string value) {
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

bool IsSimpleIdentifier(const std::string& value) {
    if (value.empty()) {
        return false;
    }
    if (!(std::isalpha(static_cast<unsigned char>(value[0])) || value[0] == '_')) {
        return false;
    }
    for (char ch : value) {
        if (!(std::isalnum(static_cast<unsigned char>(ch)) || ch == '_')) {
            return false;
        }
    }
    return true;
}

bool IsQuotedIdentifier(const std::string& value) {
    return value.size() >= 2 && value.front() == '"' && value.back() == '"';
}

std::string QuoteIdentifier(const std::string& value) {
    if (IsSimpleIdentifier(value) || IsQuotedIdentifier(value)) {
        return value;
    }
    std::string out = "\"";
    for (char ch : value) {
        if (ch == '"') {
            out.push_back('"');
        }
        out.push_back(ch);
    }
    out.push_back('"');
    return out;
}

wxChoice* BuildChoice(wxWindow* parent, const std::vector<std::string>& options) {
    auto* choice = new wxChoice(parent, wxID_ANY);
    for (const auto& option : options) {
        choice->Append(option);
    }
    if (!options.empty()) {
        choice->SetSelection(0);
    }
    return choice;
}

} // namespace

TablespaceEditorDialog::TablespaceEditorDialog(wxWindow* parent, TablespaceEditorMode mode)
    : wxDialog(parent, wxID_ANY,
               mode == TablespaceEditorMode::Create ? "Create Tablespace" : "Edit Tablespace",
               wxDefaultPosition, wxSize(550, 520),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      mode_(mode) {
    auto* rootSizer = new wxBoxSizer(wxVERTICAL);

    // Tablespace Name
    auto* nameLabel = new wxStaticText(this, wxID_ANY, "Tablespace Name");
    rootSizer->Add(nameLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    name_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    rootSizer->Add(name_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    // Owner dropdown
    auto* ownerLabel = new wxStaticText(this, wxID_ANY, "Owner");
    rootSizer->Add(ownerLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    owner_choice_ = BuildChoice(this, {"SYSDBA", "SYSTEM", "ADMIN", "USER"});
    owner_choice_->SetSelection(0);
    rootSizer->Add(owner_choice_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    // Location/Path with browse button
    auto* locationLabel = new wxStaticText(this, wxID_ANY, "Location/Path");
    rootSizer->Add(locationLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    
    auto* locationSizer = new wxBoxSizer(wxHORIZONTAL);
    location_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    location_ctrl_->SetHint("/path/to/tablespace/data");
    locationSizer->Add(location_ctrl_, 1, wxEXPAND | wxRIGHT, 6);
    browse_button_ = new wxButton(this, wxID_ANY, "Browse...");
    locationSizer->Add(browse_button_, 0);
    rootSizer->Add(locationSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    // Initial Size
    auto* sizeLabel = new wxStaticText(this, wxID_ANY, "Initial Size");
    rootSizer->Add(sizeLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    
    auto* sizeSizer = new wxBoxSizer(wxHORIZONTAL);
    size_ctrl_ = new wxSpinCtrl(this, wxID_ANY, "100", wxDefaultPosition, wxDefaultSize,
                                wxSP_ARROW_KEYS, 1, 999999, 100);
    sizeSizer->Add(size_ctrl_, 1, wxEXPAND | wxRIGHT, 6);
    size_unit_choice_ = BuildChoice(this, {"MB", "GB", "TB"});
    size_unit_choice_->SetSelection(1); // Default to GB
    sizeSizer->Add(size_unit_choice_, 0);
    rootSizer->Add(sizeSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    // Auto-extend checkbox
    auto* autoextendSizer = new wxBoxSizer(wxVERTICAL);
    autoextend_ctrl_ = new wxCheckBox(this, wxID_ANY, "Auto-extend");
    autoextendSizer->Add(autoextend_ctrl_, 0, wxBOTTOM, 8);

    // Increment size (enabled when auto-extend is on)
    auto* incrementSizer = new wxBoxSizer(wxHORIZONTAL);
    increment_label_ = new wxStaticText(this, wxID_ANY, "  Increment:");
    incrementSizer->Add(increment_label_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    increment_ctrl_ = new wxSpinCtrl(this, wxID_ANY, "10", wxDefaultPosition, wxDefaultSize,
                                     wxSP_ARROW_KEYS, 1, 999999, 10);
    incrementSizer->Add(increment_ctrl_, 1, wxEXPAND | wxRIGHT, 6);
    increment_unit_choice_ = BuildChoice(this, {"MB", "GB", "TB"});
    increment_unit_choice_->SetSelection(0); // Default to MB
    incrementSizer->Add(increment_unit_choice_, 0);
    autoextendSizer->Add(incrementSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 4);

    // Max size (enabled when auto-extend is on)
    auto* maxSizeSizer = new wxBoxSizer(wxHORIZONTAL);
    unlimited_max_ctrl_ = new wxCheckBox(this, wxID_ANY, "  Unlimited Max Size");
    maxSizeSizer->Add(unlimited_max_ctrl_, 0, wxRIGHT, 12);
    autoextendSizer->Add(maxSizeSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 4);

    auto* maxInputSizer = new wxBoxSizer(wxHORIZONTAL);
    max_size_label_ = new wxStaticText(this, wxID_ANY, "  Max Size:");
    maxInputSizer->Add(max_size_label_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    max_size_ctrl_ = new wxSpinCtrl(this, wxID_ANY, "1000", wxDefaultPosition, wxDefaultSize,
                                    wxSP_ARROW_KEYS, 1, 999999, 1000);
    maxInputSizer->Add(max_size_ctrl_, 1, wxEXPAND | wxRIGHT, 6);
    max_size_unit_choice_ = BuildChoice(this, {"MB", "GB", "TB"});
    max_size_unit_choice_->SetSelection(1); // Default to GB
    maxInputSizer->Add(max_size_unit_choice_, 0);
    autoextendSizer->Add(maxInputSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 4);

    rootSizer->Add(autoextendSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    // Add buttons
    rootSizer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 12);
    SetSizerAndFit(rootSizer);
    CentreOnParent();

    // Bind events
    autoextend_ctrl_->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) { UpdateAutoextendFields(); });
    unlimited_max_ctrl_->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) { UpdateAutoextendFields(); });
    browse_button_->Bind(wxEVT_BUTTON, &TablespaceEditorDialog::OnBrowseLocation, this);

    // Initialize field states
    UpdateAutoextendFields();

    // In edit mode, name is read-only
    if (mode_ == TablespaceEditorMode::Edit && name_ctrl_) {
        name_ctrl_->Enable(false);
    }
}

std::string TablespaceEditorDialog::BuildSql() const {
    return mode_ == TablespaceEditorMode::Create ? BuildCreateSql() : BuildAlterSql();
}

std::string TablespaceEditorDialog::tablespace_name() const {
    return Trim(name_ctrl_ ? name_ctrl_->GetValue().ToStdString() : std::string());
}

void TablespaceEditorDialog::SetTablespaceName(const std::string& name) {
    if (name_ctrl_) {
        name_ctrl_->SetValue(name);
    }
}

void TablespaceEditorDialog::SetOwner(const std::string& owner) {
    if (owner_choice_) {
        int index = owner_choice_->FindString(owner);
        if (index != wxNOT_FOUND) {
            owner_choice_->SetSelection(index);
        }
    }
}

void TablespaceEditorDialog::SetLocation(const std::string& location) {
    if (location_ctrl_) {
        location_ctrl_->SetValue(location);
    }
}

void TablespaceEditorDialog::SetSize(const std::string& size) {
    // Parse size string like "100 MB" or "2 GB"
    if (size_ctrl_ && size_unit_choice_) {
        std::istringstream iss(size);
        int value;
        std::string unit;
        if (iss >> value >> unit) {
            size_ctrl_->SetValue(value);
            int unitIndex = size_unit_choice_->FindString(unit);
            if (unitIndex != wxNOT_FOUND) {
                size_unit_choice_->SetSelection(unitIndex);
            }
        }
    }
}

void TablespaceEditorDialog::SetAutoextend(bool enabled, const std::string& increment,
                                           const std::string& maxsize) {
    if (autoextend_ctrl_) {
        autoextend_ctrl_->SetValue(enabled);
    }
    if (enabled && increment_ctrl_ && increment_unit_choice_) {
        std::istringstream iss(increment);
        int value;
        std::string unit;
        if (iss >> value >> unit) {
            increment_ctrl_->SetValue(value);
            int unitIndex = increment_unit_choice_->FindString(unit);
            if (unitIndex != wxNOT_FOUND) {
                increment_unit_choice_->SetSelection(unitIndex);
            }
        }
    }
    if (enabled && max_size_ctrl_ && max_size_unit_choice_ && unlimited_max_ctrl_) {
        if (maxsize == "UNLIMITED") {
            unlimited_max_ctrl_->SetValue(true);
        } else {
            unlimited_max_ctrl_->SetValue(false);
            std::istringstream iss(maxsize);
            int value;
            std::string unit;
            if (iss >> value >> unit) {
                max_size_ctrl_->SetValue(value);
                int unitIndex = max_size_unit_choice_->FindString(unit);
                if (unitIndex != wxNOT_FOUND) {
                    max_size_unit_choice_->SetSelection(unitIndex);
                }
            }
        }
    }
    UpdateAutoextendFields();
}

std::string TablespaceEditorDialog::BuildCreateSql() const {
    std::string name = tablespace_name();
    if (name.empty()) {
        return std::string();
    }

    std::string location = Trim(location_ctrl_ ? location_ctrl_->GetValue().ToStdString() : std::string());
    if (location.empty()) {
        return std::string();
    }

    std::ostringstream sql;
    sql << "CREATE TABLESPACE " << QuoteIdentifier(name) << "\n";

    // Owner
    std::string owner = owner_choice_ ? owner_choice_->GetStringSelection().ToStdString() : "SYSDBA";
    sql << "  OWNER " << QuoteIdentifier(owner) << "\n";

    // Location
    sql << "  LOCATION '" << location << "'\n";

    // Size
    if (size_ctrl_ && size_unit_choice_) {
        int size = size_ctrl_->GetValue();
        std::string unit = size_unit_choice_->GetStringSelection().ToStdString();
        sql << "  SIZE " << size << unit;
    }

    // Auto-extend
    if (autoextend_ctrl_ && autoextend_ctrl_->IsChecked()) {
        sql << "\n  AUTOEXTEND ON";
        if (increment_ctrl_ && increment_unit_choice_) {
            int inc = increment_ctrl_->GetValue();
            std::string unit = increment_unit_choice_->GetStringSelection().ToStdString();
            sql << " NEXT " << inc << unit;
        }
        if (unlimited_max_ctrl_ && unlimited_max_ctrl_->IsChecked()) {
            sql << " MAXSIZE UNLIMITED";
        } else if (max_size_ctrl_ && max_size_unit_choice_) {
            int max = max_size_ctrl_->GetValue();
            std::string unit = max_size_unit_choice_->GetStringSelection().ToStdString();
            sql << " MAXSIZE " << max << unit;
        }
    }

    sql << ";";
    return sql.str();
}

std::string TablespaceEditorDialog::BuildAlterSql() const {
    std::string name = tablespace_name();
    if (name.empty()) {
        return std::string();
    }

    std::ostringstream sql;
    sql << "ALTER TABLESPACE " << QuoteIdentifier(name) << "\n";

    // For alter, we support resizing and changing autoextend settings
    if (size_ctrl_ && size_unit_choice_) {
        int size = size_ctrl_->GetValue();
        std::string unit = size_unit_choice_->GetStringSelection().ToStdString();
        sql << "  RESIZE " << size << unit;
    }

    // Auto-extend settings
    if (autoextend_ctrl_) {
        if (autoextend_ctrl_->IsChecked()) {
            sql << "\n  AUTOEXTEND ON";
            if (increment_ctrl_ && increment_unit_choice_) {
                int inc = increment_ctrl_->GetValue();
                std::string unit = increment_unit_choice_->GetStringSelection().ToStdString();
                sql << " NEXT " << inc << unit;
            }
            if (unlimited_max_ctrl_ && unlimited_max_ctrl_->IsChecked()) {
                sql << " MAXSIZE UNLIMITED";
            } else if (max_size_ctrl_ && max_size_unit_choice_) {
                int max = max_size_ctrl_->GetValue();
                std::string unit = max_size_unit_choice_->GetStringSelection().ToStdString();
                sql << " MAXSIZE " << max << unit;
            }
        } else {
            sql << "\n  AUTOEXTEND OFF";
        }
    }

    sql << ";";
    return sql.str();
}

void TablespaceEditorDialog::UpdateAutoextendFields() {
    bool autoextend = autoextend_ctrl_ ? autoextend_ctrl_->IsChecked() : false;
    bool unlimited = unlimited_max_ctrl_ ? unlimited_max_ctrl_->IsChecked() : false;

    if (increment_label_) increment_label_->Enable(autoextend);
    if (increment_ctrl_) increment_ctrl_->Enable(autoextend);
    if (increment_unit_choice_) increment_unit_choice_->Enable(autoextend);
    if (unlimited_max_ctrl_) unlimited_max_ctrl_->Enable(autoextend);
    
    if (max_size_label_) max_size_label_->Enable(autoextend && !unlimited);
    if (max_size_ctrl_) max_size_ctrl_->Enable(autoextend && !unlimited);
    if (max_size_unit_choice_) max_size_unit_choice_->Enable(autoextend && !unlimited);
}

void TablespaceEditorDialog::OnBrowseLocation(wxCommandEvent& event) {
    wxDirDialog dialog(this, "Select Tablespace Directory", "",
                       wxDD_DEFAULT_STYLE | wxDD_DIR_MUST_EXIST);
    if (dialog.ShowModal() == wxID_OK) {
        if (location_ctrl_) {
            location_ctrl_->SetValue(dialog.GetPath());
        }
    }
}

} // namespace scratchrobin
