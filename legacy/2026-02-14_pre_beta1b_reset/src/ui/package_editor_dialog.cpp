/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "package_editor_dialog.h"

#include <algorithm>
#include <cctype>
#include <sstream>

#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/choice.h>

namespace scratchrobin {

PackageEditorDialog::PackageEditorDialog(wxWindow* parent, PackageEditorMode mode)
    : wxDialog(parent, wxID_ANY, mode == PackageEditorMode::Create ? "Create Package" : "Edit Package",
               wxDefaultPosition, wxSize(800, 700),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      mode_(mode) {
    BuildLayout();
    CentreOnParent();
}

void PackageEditorDialog::BuildLayout() {
    auto* rootSizer = new wxBoxSizer(wxVERTICAL);

    // Package Name field
    auto* nameLabel = new wxStaticText(this, wxID_ANY, "Package Name");
    rootSizer->Add(nameLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    name_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    name_ctrl_->SetHint("Enter package name");
    rootSizer->Add(name_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    // Schema dropdown
    auto* schemaLabel = new wxStaticText(this, wxID_ANY, "Schema");
    rootSizer->Add(schemaLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    schema_choice_ = new wxChoice(this, wxID_ANY);
    schema_choice_->SetToolTip("Select the schema for this package");
    rootSizer->Add(schema_choice_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    // Notebook with Specification and Body tabs
    notebook_ = new wxNotebook(this, wxID_ANY);

    BuildSpecificationTab(notebook_);
    BuildBodyTab(notebook_);

    rootSizer->Add(notebook_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    // Button sizer
    rootSizer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 12);

    SetSizerAndFit(rootSizer);
}

void PackageEditorDialog::BuildSpecificationTab(wxNotebook* notebook) {
    auto* specPanel = new wxPanel(notebook, wxID_ANY);
    auto* specSizer = new wxBoxSizer(wxVERTICAL);

    auto* specLabel = new wxStaticText(specPanel, wxID_ANY, "Package Specification (Public Declarations)");
    specSizer->Add(specLabel, 0, wxLEFT | wxRIGHT | wxTOP, 8);

    spec_ctrl_ = new wxTextCtrl(specPanel, wxID_ANY, "", wxDefaultPosition,
                                wxSize(-1, -1), wxTE_MULTILINE);
    spec_ctrl_->SetHint(
        "-- Enter public declarations here\n"
        "PROCEDURE proc_name(param TYPE);\n"
        "FUNCTION func_name(param TYPE) RETURN return_type;\n"
        "variable_name TYPE := default_value;\n"
        "CONSTANT constant_name TYPE := value;"
    );
    specSizer->Add(spec_ctrl_, 1, wxEXPAND | wxALL, 8);

    specPanel->SetSizer(specSizer);
    notebook->AddPage(specPanel, "Specification");
}

void PackageEditorDialog::BuildBodyTab(wxNotebook* notebook) {
    auto* bodyPanel = new wxPanel(notebook, wxID_ANY);
    auto* bodySizer = new wxBoxSizer(wxVERTICAL);

    auto* bodyLabel = new wxStaticText(bodyPanel, wxID_ANY, "Package Body (Implementation)");
    bodySizer->Add(bodyLabel, 0, wxLEFT | wxRIGHT | wxTOP, 8);

    body_ctrl_ = new wxTextCtrl(bodyPanel, wxID_ANY, "", wxDefaultPosition,
                                wxSize(-1, -1), wxTE_MULTILINE);
    body_ctrl_->SetHint(
        "-- Enter implementation here\n"
        "PROCEDURE proc_name(param TYPE) IS\n"
        "BEGIN\n"
        "  -- implementation\n"
        "END;\n\n"
        "FUNCTION func_name(param TYPE) RETURN return_type IS\n"
        "BEGIN\n"
        "  -- implementation\n"
        "  RETURN value;\n"
        "END;"
    );
    bodySizer->Add(body_ctrl_, 1, wxEXPAND | wxALL, 8);

    bodyPanel->SetSizer(bodySizer);
    notebook->AddPage(bodyPanel, "Body");
}

std::string PackageEditorDialog::BuildSql() const {
    return mode_ == PackageEditorMode::Create ? BuildCreateSql() : BuildAlterSql();
}

std::string PackageEditorDialog::BuildCreateSql() const {
    std::string name = Trim(package_name());
    if (name.empty()) {
        return std::string();
    }

    std::ostringstream sql;

    // Build specification
    sql << "CREATE PACKAGE " << FormatPackagePath() << " AS\n";

    std::string spec = Trim(GetSpecification());
    if (!spec.empty()) {
        // Add each line with proper indentation
        std::istringstream specStream(spec);
        std::string line;
        while (std::getline(specStream, line)) {
            std::string trimmed = Trim(line);
            if (!trimmed.empty()) {
                sql << "  " << trimmed << "\n";
            }
        }
    }

    sql << "END " << QuoteIdentifier(name) << ";\n";

    // Build body (if provided)
    std::string body = Trim(GetBody());
    if (!body.empty()) {
        sql << "\n";
        sql << "CREATE PACKAGE BODY " << FormatPackagePath() << " AS\n";

        std::istringstream bodyStream(body);
        std::string line;
        while (std::getline(bodyStream, line)) {
            std::string trimmed = Trim(line);
            if (!trimmed.empty()) {
                sql << "  " << trimmed << "\n";
            }
        }

        sql << "END " << QuoteIdentifier(name) << ";";
    }

    return sql.str();
}

std::string PackageEditorDialog::BuildAlterSql() const {
    std::string name = Trim(package_name());
    if (name.empty()) {
        return std::string();
    }

    std::ostringstream sql;

    // For ALTER, we recreate the package (drop and create)
    sql << "-- Drop existing package\n";
    sql << "DROP PACKAGE BODY IF EXISTS " << FormatPackagePath() << ";\n";
    sql << "DROP PACKAGE IF EXISTS " << FormatPackagePath() << ";\n\n";

    // Build specification
    sql << "CREATE PACKAGE " << FormatPackagePath() << " AS\n";

    std::string spec = Trim(GetSpecification());
    if (!spec.empty()) {
        std::istringstream specStream(spec);
        std::string line;
        while (std::getline(specStream, line)) {
            std::string trimmed = Trim(line);
            if (!trimmed.empty()) {
                sql << "  " << trimmed << "\n";
            }
        }
    }

    sql << "END " << QuoteIdentifier(name) << ";\n";

    // Build body (if provided)
    std::string body = Trim(GetBody());
    if (!body.empty()) {
        sql << "\n";
        sql << "CREATE PACKAGE BODY " << FormatPackagePath() << " AS\n";

        std::istringstream bodyStream(body);
        std::string line;
        while (std::getline(bodyStream, line)) {
            std::string trimmed = Trim(line);
            if (!trimmed.empty()) {
                sql << "  " << trimmed << "\n";
            }
        }

        sql << "END " << QuoteIdentifier(name) << ";";
    }

    return sql.str();
}

std::string PackageEditorDialog::FormatPackagePath() const {
    std::string schema = Trim(schema_name());
    std::string name = Trim(package_name());

    if (schema.empty() || schema == "(default)") {
        return QuoteIdentifier(name);
    }

    return QuoteIdentifier(schema) + "." + QuoteIdentifier(name);
}

std::string PackageEditorDialog::package_name() const {
    return Trim(name_ctrl_ ? name_ctrl_->GetValue().ToStdString() : std::string());
}

std::string PackageEditorDialog::schema_name() const {
    if (!schema_choice_) {
        return std::string();
    }
    int selection = schema_choice_->GetSelection();
    if (selection == wxNOT_FOUND) {
        return std::string();
    }
    return Trim(schema_choice_->GetString(selection).ToStdString());
}

void PackageEditorDialog::SetPackageName(const std::string& name) {
    if (name_ctrl_) {
        name_ctrl_->SetValue(name);
        if (mode_ == PackageEditorMode::Edit) {
            name_ctrl_->Enable(false);
        }
    }
}

void PackageEditorDialog::SetSchemaName(const std::string& schema) {
    if (!schema_choice_) {
        return;
    }

    std::string trimmed = Trim(schema);
    for (unsigned int i = 0; i < schema_choice_->GetCount(); ++i) {
        if (Trim(schema_choice_->GetString(i).ToStdString()) == trimmed) {
            schema_choice_->SetSelection(i);
            if (mode_ == PackageEditorMode::Edit) {
                schema_choice_->Enable(false);
            }
            return;
        }
    }

    // If not found, add it and select it
    if (!trimmed.empty()) {
        int idx = schema_choice_->Append(trimmed);
        schema_choice_->SetSelection(idx);
        if (mode_ == PackageEditorMode::Edit) {
            schema_choice_->Enable(false);
        }
    }
}

void PackageEditorDialog::SetAvailableSchemas(const std::vector<std::string>& schemas) {
    if (!schema_choice_) {
        return;
    }

    schema_choice_->Clear();
    schema_choice_->Append("(default)");

    for (const auto& schema : schemas) {
        if (!schema.empty() && Trim(schema) != "(default)") {
            schema_choice_->Append(schema);
        }
    }

    schema_choice_->SetSelection(0);
}

void PackageEditorDialog::SetSpecification(const std::string& spec) {
    if (spec_ctrl_) {
        spec_ctrl_->SetValue(spec);
    }
}

void PackageEditorDialog::SetBody(const std::string& body) {
    if (body_ctrl_) {
        body_ctrl_->SetValue(body);
    }
}

std::string PackageEditorDialog::GetSpecification() const {
    return spec_ctrl_ ? spec_ctrl_->GetValue().ToStdString() : std::string();
}

std::string PackageEditorDialog::GetBody() const {
    return body_ctrl_ ? body_ctrl_->GetValue().ToStdString() : std::string();
}

std::string PackageEditorDialog::Trim(const std::string& value) const {
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    std::string result = value;
    result.erase(result.begin(), std::find_if(result.begin(), result.end(), not_space));
    result.erase(std::find_if(result.rbegin(), result.rend(), not_space).base(), result.end());
    return result;
}

bool PackageEditorDialog::IsSimpleIdentifier(const std::string& value) const {
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

bool PackageEditorDialog::IsQuotedIdentifier(const std::string& value) const {
    return value.size() >= 2 && value.front() == '"' && value.back() == '"';
}

std::string PackageEditorDialog::QuoteIdentifier(const std::string& value) const {
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

} // namespace scratchrobin
