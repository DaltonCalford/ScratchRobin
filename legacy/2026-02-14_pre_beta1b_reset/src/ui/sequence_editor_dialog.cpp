/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "sequence_editor_dialog.h"

#include <algorithm>
#include <cctype>
#include <limits>
#include <sstream>

#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/statline.h>
#include <wx/textctrl.h>

namespace scratchrobin {

namespace {

constexpr int64_t kMaxInt64 = std::numeric_limits<int64_t>::max();
constexpr int64_t kMinInt64 = std::numeric_limits<int64_t>::min();

} // namespace

SequenceEditorDialog::SequenceEditorDialog(wxWindow* parent, SequenceEditorMode mode)
    : wxDialog(parent, wxID_ANY, mode == SequenceEditorMode::Create ? "Create Sequence" : "Edit Sequence",
               wxDefaultPosition, wxSize(500, 600),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      mode_(mode) {
    auto* rootSizer = new wxBoxSizer(wxVERTICAL);

    // Sequence Name
    auto* nameLabel = new wxStaticText(this, wxID_ANY, "Sequence Name");
    rootSizer->Add(nameLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    name_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    rootSizer->Add(name_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    // Schema
    auto* schemaLabel = new wxStaticText(this, wxID_ANY, "Schema");
    rootSizer->Add(schemaLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    schema_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    schema_ctrl_->SetHint("Leave empty for default schema");
    rootSizer->Add(schema_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    // Data Type
    auto* dataTypeLabel = new wxStaticText(this, wxID_ANY, "Data Type");
    rootSizer->Add(dataTypeLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    data_type_choice_ = new wxChoice(this, wxID_ANY);
    data_type_choice_->Append("INTEGER");
    data_type_choice_->Append("BIGINT");
    data_type_choice_->Append("SMALLINT");
    data_type_choice_->SetSelection(1); // Default to BIGINT
    rootSizer->Add(data_type_choice_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    // Start Value
    auto* startValueLabel = new wxStaticText(this, wxID_ANY, "Start Value");
    rootSizer->Add(startValueLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    start_value_ctrl_ = new wxSpinCtrl(this, wxID_ANY);
    start_value_ctrl_->SetRange(static_cast<int>(kMinInt64), static_cast<int>(kMaxInt64));
    start_value_ctrl_->SetValue(1);
    rootSizer->Add(start_value_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    // Increment By
    auto* incrementLabel = new wxStaticText(this, wxID_ANY, "Increment By");
    rootSizer->Add(incrementLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    increment_by_ctrl_ = new wxSpinCtrl(this, wxID_ANY);
    increment_by_ctrl_->SetRange(static_cast<int>(kMinInt64), static_cast<int>(kMaxInt64));
    increment_by_ctrl_->SetValue(1);
    rootSizer->Add(increment_by_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    // Min Value
    auto* minValueSizer = new wxBoxSizer(wxHORIZONTAL);
    use_min_value_ctrl_ = new wxCheckBox(this, wxID_ANY, "Use Min Value");
    minValueSizer->Add(use_min_value_ctrl_, 0, wxALIGN_CENTER_VERTICAL);
    rootSizer->Add(minValueSizer, 0, wxLEFT | wxRIGHT | wxTOP, 12);

    min_value_ctrl_ = new wxSpinCtrl(this, wxID_ANY);
    min_value_ctrl_->SetRange(static_cast<int>(kMinInt64), static_cast<int>(kMaxInt64));
    min_value_ctrl_->SetValue(1);
    min_value_ctrl_->Enable(false);
    rootSizer->Add(min_value_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    // Max Value
    auto* maxValueSizer = new wxBoxSizer(wxHORIZONTAL);
    use_max_value_ctrl_ = new wxCheckBox(this, wxID_ANY, "Use Max Value");
    maxValueSizer->Add(use_max_value_ctrl_, 0, wxALIGN_CENTER_VERTICAL);
    rootSizer->Add(maxValueSizer, 0, wxLEFT | wxRIGHT | wxTOP, 12);

    max_value_ctrl_ = new wxSpinCtrl(this, wxID_ANY);
    max_value_ctrl_->SetRange(static_cast<int>(kMinInt64), static_cast<int>(kMaxInt64));
    max_value_ctrl_->SetValue(1000000);
    max_value_ctrl_->Enable(false);
    rootSizer->Add(max_value_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    // Cache Size
    auto* cacheLabel = new wxStaticText(this, wxID_ANY, "Cache Size");
    rootSizer->Add(cacheLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    cache_size_ctrl_ = new wxSpinCtrl(this, wxID_ANY);
    cache_size_ctrl_->SetRange(1, 1000000);
    cache_size_ctrl_->SetValue(20);
    rootSizer->Add(cache_size_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    // Checkboxes
    cycle_ctrl_ = new wxCheckBox(this, wxID_ANY, "Cycle (restart after reaching limit)");
    rootSizer->Add(cycle_ctrl_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

    ordered_ctrl_ = new wxCheckBox(this, wxID_ANY, "Is Ordered (guarantee sequence order)");
    rootSizer->Add(ordered_ctrl_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

    // Edit mode specific fields
    if (mode_ == SequenceEditorMode::Edit) {
        rootSizer->Add(new wxStaticLine(this, wxID_ANY), 0, wxEXPAND | wxLEFT | wxRIGHT | wxTOP, 12);

        // Current Value (read-only display)
        current_value_label_ = new wxStaticText(this, wxID_ANY, "Current Value");
        rootSizer->Add(current_value_label_, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        current_value_display_ = new wxStaticText(this, wxID_ANY, "N/A");
        rootSizer->Add(current_value_display_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

        // Reset option
        reset_sequence_ctrl_ = new wxCheckBox(this, wxID_ANY, "Reset Sequence (RESTART WITH start value)");
        rootSizer->Add(reset_sequence_ctrl_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);
    }

    rootSizer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL | wxHELP), 0, wxEXPAND | wxALL, 12);
    SetSizerAndFit(rootSizer);
    CentreOnParent();

    // Bind events
    use_min_value_ctrl_->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) {
        min_value_ctrl_->Enable(use_min_value_ctrl_->IsChecked());
    });

    use_max_value_ctrl_->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent&) {
        max_value_ctrl_->Enable(use_max_value_ctrl_->IsChecked());
    });
}

std::string SequenceEditorDialog::BuildSql() const {
    return mode_ == SequenceEditorMode::Create ? BuildCreateSql() : BuildAlterSql();
}

std::string SequenceEditorDialog::sequence_name() const {
    if (!name_ctrl_) {
        return std::string();
    }
    wxString value = name_ctrl_->GetValue();
    return std::string(value.mb_str());
}

void SequenceEditorDialog::SetSequenceName(const std::string& name) {
    if (name_ctrl_) {
        name_ctrl_->SetValue(name);
        if (mode_ == SequenceEditorMode::Edit) {
            name_ctrl_->Enable(false);
        }
    }
}

void SequenceEditorDialog::SetSchema(const std::string& schema) {
    if (schema_ctrl_) {
        schema_ctrl_->SetValue(schema);
    }
}

void SequenceEditorDialog::SetDataType(const std::string& data_type) {
    if (!data_type_choice_) {
        return;
    }
    wxString wx_type(data_type.c_str(), wxConvUTF8);
    for (size_t i = 0; i < data_type_choice_->GetCount(); ++i) {
        if (data_type_choice_->GetString(i).IsSameAs(wx_type, false)) {
            data_type_choice_->SetSelection(static_cast<int>(i));
            break;
        }
    }
}

void SequenceEditorDialog::SetStartValue(int64_t value) {
    if (start_value_ctrl_) {
        start_value_ctrl_->SetValue(static_cast<int>(value));
    }
}

void SequenceEditorDialog::SetIncrementBy(int64_t value) {
    if (increment_by_ctrl_) {
        increment_by_ctrl_->SetValue(static_cast<int>(value));
    }
}

void SequenceEditorDialog::SetMinValue(int64_t value) {
    if (min_value_ctrl_) {
        min_value_ctrl_->SetValue(static_cast<int>(value));
        if (use_min_value_ctrl_) {
            use_min_value_ctrl_->SetValue(true);
            min_value_ctrl_->Enable(true);
        }
    }
}

void SequenceEditorDialog::SetMaxValue(int64_t value) {
    if (max_value_ctrl_) {
        max_value_ctrl_->SetValue(static_cast<int>(value));
        if (use_max_value_ctrl_) {
            use_max_value_ctrl_->SetValue(true);
            max_value_ctrl_->Enable(true);
        }
    }
}

void SequenceEditorDialog::SetCacheSize(int value) {
    if (cache_size_ctrl_) {
        cache_size_ctrl_->SetValue(value);
    }
}

void SequenceEditorDialog::SetCycle(bool cycle) {
    if (cycle_ctrl_) {
        cycle_ctrl_->SetValue(cycle);
    }
}

void SequenceEditorDialog::SetIsOrdered(bool ordered) {
    if (ordered_ctrl_) {
        ordered_ctrl_->SetValue(ordered);
    }
}

void SequenceEditorDialog::SetCurrentValue(int64_t value) {
    if (current_value_display_) {
        std::ostringstream oss;
        oss << value;
        current_value_display_->SetLabel(oss.str());
    }
}

std::string SequenceEditorDialog::BuildCreateSql() const {
    std::string name = sequence_name();
    if (name.empty()) {
        return std::string();
    }

    std::ostringstream sql;
    sql << "CREATE SEQUENCE ";

    // Add schema prefix if specified
    if (schema_ctrl_) {
        std::string schema(schema_ctrl_->GetValue().mb_str());
        if (!schema.empty()) {
            sql << QuoteIdentifier(schema) << ".";
        }
    }
    sql << QuoteIdentifier(name) << "\n";

    // Data Type
    if (data_type_choice_) {
        sql << "  AS " << data_type_choice_->GetStringSelection().mb_str() << "\n";
    }

    // Start Value
    if (start_value_ctrl_) {
        sql << "  START WITH " << start_value_ctrl_->GetValue() << "\n";
    }

    // Increment By
    if (increment_by_ctrl_) {
        sql << "  INCREMENT BY " << increment_by_ctrl_->GetValue() << "\n";
    }

    // Min Value
    if (use_min_value_ctrl_ && use_min_value_ctrl_->IsChecked() && min_value_ctrl_) {
        sql << "  MINVALUE " << min_value_ctrl_->GetValue() << "\n";
    } else {
        sql << "  NO MINVALUE\n";
    }

    // Max Value
    if (use_max_value_ctrl_ && use_max_value_ctrl_->IsChecked() && max_value_ctrl_) {
        sql << "  MAXVALUE " << max_value_ctrl_->GetValue() << "\n";
    } else {
        sql << "  NO MAXVALUE\n";
    }

    // Cache Size
    if (cache_size_ctrl_) {
        sql << "  CACHE " << cache_size_ctrl_->GetValue() << "\n";
    }

    // Cycle
    if (cycle_ctrl_ && cycle_ctrl_->IsChecked()) {
        sql << "  CYCLE\n";
    } else {
        sql << "  NO CYCLE\n";
    }

    // Ordered (Firebird specific - ORDERED keyword)
    if (ordered_ctrl_ && ordered_ctrl_->IsChecked()) {
        sql << "  ORDERED\n";
    }

    sql << ";";
    return sql.str();
}

std::string SequenceEditorDialog::BuildAlterSql() const {
    std::string name = sequence_name();
    if (name.empty()) {
        return std::string();
    }

    std::ostringstream sql;
    sql << "ALTER SEQUENCE ";

    // Add schema prefix if specified
    if (schema_ctrl_) {
        std::string schema(schema_ctrl_->GetValue().mb_str());
        if (!schema.empty()) {
            sql << QuoteIdentifier(schema) << ".";
        }
    }
    sql << QuoteIdentifier(name);

    // Check if reset is requested
    if (reset_sequence_ctrl_ && reset_sequence_ctrl_->IsChecked() && start_value_ctrl_) {
        sql << " RESTART WITH " << start_value_ctrl_->GetValue();
    }

    sql << ";";
    return sql.str();
}

std::string SequenceEditorDialog::QuoteIdentifier(const std::string& value) const {
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

bool SequenceEditorDialog::IsSimpleIdentifier(const std::string& value) const {
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

bool SequenceEditorDialog::IsQuotedIdentifier(const std::string& value) const {
    return value.size() >= 2 && value.front() == '"' && value.back() == '"';
}

} // namespace scratchrobin
