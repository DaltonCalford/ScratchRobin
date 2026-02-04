/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "database_editor_dialog.h"

#include "core/query_types.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <vector>

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/panel.h>
#include <wx/radiobut.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

namespace scratchrobin {
namespace {

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

DatabaseEditorDialog::DatabaseEditorDialog(wxWindow* parent, DatabaseEditorMode mode)
    : wxDialog(parent, wxID_ANY, 
               mode == DatabaseEditorMode::Create ? "Create Database" :
               mode == DatabaseEditorMode::Clone ? "Clone Database" : "Database Properties",
               wxDefaultPosition, 
               mode == DatabaseEditorMode::Properties ? wxSize(500, 500) : wxSize(500, 600),
               wxDEFAULT_DIALOG_STYLE | (mode == DatabaseEditorMode::Properties ? 0 : wxRESIZE_BORDER)),
      mode_(mode) {
    
    if (mode_ == DatabaseEditorMode::Create) {
        BuildCreateLayout();
    } else if (mode_ == DatabaseEditorMode::Clone) {
        BuildCloneLayout();
    } else {
        BuildPropertiesLayout();
    }
    
    CentreOnParent();
}

void DatabaseEditorDialog::BuildCreateLayout() {
    auto* rootSizer = new wxBoxSizer(wxVERTICAL);
    
    auto* formPanel = new wxPanel(this, wxID_ANY);
    auto* formSizer = new wxFlexGridSizer(2, 8, 8);
    formSizer->AddGrowableCol(1, 1);
    
    // Database Name
    formSizer->Add(new wxStaticText(formPanel, wxID_ANY, "Database Name:"), 0, wxALIGN_CENTER_VERTICAL);
    name_ctrl_ = new wxTextCtrl(formPanel, wxID_ANY);
    name_ctrl_->SetHint("Enter database name");
    formSizer->Add(name_ctrl_, 1, wxEXPAND);
    
    // Owner
    formSizer->Add(new wxStaticText(formPanel, wxID_ANY, "Owner:"), 0, wxALIGN_CENTER_VERTICAL);
    owner_choice_ = BuildChoice(formPanel, {"CURRENT_USER", "postgres", "admin"});
    formSizer->Add(owner_choice_, 1, wxEXPAND);
    
    // Encoding
    formSizer->Add(new wxStaticText(formPanel, wxID_ANY, "Encoding:"), 0, wxALIGN_CENTER_VERTICAL);
    encoding_choice_ = BuildChoice(formPanel, {"UTF8", "LATIN1", "LATIN2", "LATIN9", "WIN1250", "WIN1251", "WIN1252", "EUC_JP", "EUC_KR", "EUC_CN", "EUC_TW", "GB18030", "GBK", "BIG5", "SHIFT_JIS", "SQL_ASCII"});
    formSizer->Add(encoding_choice_, 1, wxEXPAND);
    
    // Collation
    formSizer->Add(new wxStaticText(formPanel, wxID_ANY, "Collation:"), 0, wxALIGN_CENTER_VERTICAL);
    collation_choice_ = BuildChoice(formPanel, {
        "en_US.UTF-8", "en_US.utf8", "C", "POSIX",
        "en_GB.UTF-8", "de_DE.UTF-8", "fr_FR.UTF-8", "es_ES.UTF-8",
        "it_IT.UTF-8", "pt_BR.UTF-8", "ja_JP.UTF-8", "ko_KR.UTF-8",
        "zh_CN.UTF-8", "ru_RU.UTF-8"
    });
    formSizer->Add(collation_choice_, 1, wxEXPAND);
    
    // Character Class (LC_CTYPE)
    formSizer->Add(new wxStaticText(formPanel, wxID_ANY, "Character Class:"), 0, wxALIGN_CENTER_VERTICAL);
    character_class_choice_ = BuildChoice(formPanel, {
        "en_US.UTF-8", "en_US.utf8", "C", "POSIX",
        "en_GB.UTF-8", "de_DE.UTF-8", "fr_FR.UTF-8", "es_ES.UTF-8",
        "it_IT.UTF-8", "pt_BR.UTF-8", "ja_JP.UTF-8", "ko_KR.UTF-8",
        "zh_CN.UTF-8", "ru_RU.UTF-8"
    });
    formSizer->Add(character_class_choice_, 1, wxEXPAND);
    
    // Template
    formSizer->Add(new wxStaticText(formPanel, wxID_ANY, "Template:"), 0, wxALIGN_CENTER_VERTICAL);
    template_choice_ = BuildChoice(formPanel, {"template1", "template0"});
    formSizer->Add(template_choice_, 1, wxEXPAND);
    
    // Tablespace
    formSizer->Add(new wxStaticText(formPanel, wxID_ANY, "Tablespace:"), 0, wxALIGN_CENTER_VERTICAL);
    tablespace_choice_ = BuildChoice(formPanel, {"pg_default", "pg_global"});
    formSizer->Add(tablespace_choice_, 1, wxEXPAND);
    
    formPanel->SetSizer(formSizer);
    rootSizer->Add(formPanel, 0, wxEXPAND | wxALL, 12);
    
    // Options section
    auto* optionsSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Options");
    
    // Allow connections checkbox
    allow_connections_ctrl_ = new wxCheckBox(this, wxID_ANY, "Allow connections");
    allow_connections_ctrl_->SetValue(true);
    allow_connections_ctrl_->Bind(wxEVT_CHECKBOX, &DatabaseEditorDialog::OnAllowConnectionsChanged, this);
    optionsSizer->Add(allow_connections_ctrl_, 0, wxALL, 8);
    
    // Connection limit
    auto* limitSizer = new wxBoxSizer(wxHORIZONTAL);
    limitSizer->Add(new wxStaticText(this, wxID_ANY, "Connection limit:"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    connection_limit_ctrl_ = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, 
                                            wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, -1, 1000, -1);
    connection_limit_ctrl_->SetValue(-1);
    limitSizer->Add(connection_limit_ctrl_, 0);
    limitSizer->Add(new wxStaticText(this, wxID_ANY, "(-1 = unlimited)"), 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 4);
    optionsSizer->Add(limitSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    
    rootSizer->Add(optionsSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    rootSizer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 12);
    SetSizerAndFit(rootSizer);
}

void DatabaseEditorDialog::BuildCloneLayout() {
    auto* rootSizer = new wxBoxSizer(wxVERTICAL);
    
    // Source database info
    auto* sourceSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Source Database");
    source_db_label_ = new wxStaticText(this, wxID_ANY, "Not specified");
    source_db_label_->SetFont(source_db_label_->GetFont().Bold());
    sourceSizer->Add(source_db_label_, 0, wxALL, 8);
    rootSizer->Add(sourceSizer, 0, wxEXPAND | wxALL, 12);
    
    // Target database
    auto* targetSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Target Database");
    auto* targetFormSizer = new wxFlexGridSizer(2, 8, 8);
    targetFormSizer->AddGrowableCol(1, 1);
    
    targetFormSizer->Add(new wxStaticText(this, wxID_ANY, "Target Name:"), 0, wxALIGN_CENTER_VERTICAL);
    target_name_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    target_name_ctrl_->SetHint("Enter target database name");
    targetFormSizer->Add(target_name_ctrl_, 1, wxEXPAND);
    
    targetSizer->Add(targetFormSizer, 0, wxEXPAND | wxALL, 8);
    rootSizer->Add(targetSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    // Clone options
    auto* cloneSizer = new wxStaticBoxSizer(wxVERTICAL, this, "Clone Options");
    
    clone_both_radio_ = new wxRadioButton(this, wxID_ANY, 
                                           "Data and Structure", wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
    clone_both_radio_->SetValue(true);
    cloneSizer->Add(clone_both_radio_, 0, wxALL, 4);
    
    clone_structure_radio_ = new wxRadioButton(this, wxID_ANY, "Structure only (schema)");
    cloneSizer->Add(clone_structure_radio_, 0, wxALL, 4);
    
    clone_data_radio_ = new wxRadioButton(this, wxID_ANY, "Data only");
    cloneSizer->Add(clone_data_radio_, 0, wxALL, 4);
    
    rootSizer->Add(cloneSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    // Note about clone process
    auto* noteText = new wxStaticText(this, wxID_ANY, 
        "Note: Cloning will create a new database using the source as a template. "
        "This may take some time for large databases.");
    noteText->SetForegroundColour(wxColour(100, 100, 100));
    noteText->Wrap(450);
    rootSizer->Add(noteText, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    rootSizer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 12);
    SetSizerAndFit(rootSizer);
}

void DatabaseEditorDialog::BuildPropertiesLayout() {
    auto* rootSizer = new wxBoxSizer(wxVERTICAL);
    
    auto* formPanel = new wxPanel(this, wxID_ANY);
    auto* formSizer = new wxFlexGridSizer(2, 8, 8);
    formSizer->AddGrowableCol(1, 1);
    
    // Database Name
    formSizer->Add(new wxStaticText(formPanel, wxID_ANY, "Database Name:"), 0, wxALIGN_CENTER_VERTICAL);
    prop_name_label_ = new wxStaticText(formPanel, wxID_ANY, "-");
    prop_name_label_->SetFont(prop_name_label_->GetFont().Bold());
    formSizer->Add(prop_name_label_, 1, wxEXPAND);
    
    // Owner
    formSizer->Add(new wxStaticText(formPanel, wxID_ANY, "Owner:"), 0, wxALIGN_CENTER_VERTICAL);
    prop_owner_label_ = new wxStaticText(formPanel, wxID_ANY, "-");
    formSizer->Add(prop_owner_label_, 1, wxEXPAND);
    
    // Encoding
    formSizer->Add(new wxStaticText(formPanel, wxID_ANY, "Encoding:"), 0, wxALIGN_CENTER_VERTICAL);
    prop_encoding_label_ = new wxStaticText(formPanel, wxID_ANY, "-");
    formSizer->Add(prop_encoding_label_, 1, wxEXPAND);
    
    // Collation
    formSizer->Add(new wxStaticText(formPanel, wxID_ANY, "Collation:"), 0, wxALIGN_CENTER_VERTICAL);
    prop_collation_label_ = new wxStaticText(formPanel, wxID_ANY, "-");
    formSizer->Add(prop_collation_label_, 1, wxEXPAND);
    
    // Character Class
    formSizer->Add(new wxStaticText(formPanel, wxID_ANY, "Character Class:"), 0, wxALIGN_CENTER_VERTICAL);
    prop_character_class_label_ = new wxStaticText(formPanel, wxID_ANY, "-");
    formSizer->Add(prop_character_class_label_, 1, wxEXPAND);
    
    // Tablespace
    formSizer->Add(new wxStaticText(formPanel, wxID_ANY, "Tablespace:"), 0, wxALIGN_CENTER_VERTICAL);
    prop_tablespace_label_ = new wxStaticText(formPanel, wxID_ANY, "-");
    formSizer->Add(prop_tablespace_label_, 1, wxEXPAND);
    
    // Size
    formSizer->Add(new wxStaticText(formPanel, wxID_ANY, "Size:"), 0, wxALIGN_CENTER_VERTICAL);
    prop_size_label_ = new wxStaticText(formPanel, wxID_ANY, "-");
    formSizer->Add(prop_size_label_, 1, wxEXPAND);
    
    // Created
    formSizer->Add(new wxStaticText(formPanel, wxID_ANY, "Created:"), 0, wxALIGN_CENTER_VERTICAL);
    prop_created_label_ = new wxStaticText(formPanel, wxID_ANY, "-");
    formSizer->Add(prop_created_label_, 1, wxEXPAND);
    
    // Allow connections
    formSizer->Add(new wxStaticText(formPanel, wxID_ANY, "Allow Connections:"), 0, wxALIGN_CENTER_VERTICAL);
    prop_allow_connections_label_ = new wxStaticText(formPanel, wxID_ANY, "-");
    formSizer->Add(prop_allow_connections_label_, 1, wxEXPAND);
    
    // Connection limit
    formSizer->Add(new wxStaticText(formPanel, wxID_ANY, "Connection Limit:"), 0, wxALIGN_CENTER_VERTICAL);
    prop_connection_limit_label_ = new wxStaticText(formPanel, wxID_ANY, "-");
    formSizer->Add(prop_connection_limit_label_, 1, wxEXPAND);
    
    // Active connections
    formSizer->Add(new wxStaticText(formPanel, wxID_ANY, "Active Connections:"), 0, wxALIGN_CENTER_VERTICAL);
    prop_active_connections_label_ = new wxStaticText(formPanel, wxID_ANY, "-");
    formSizer->Add(prop_active_connections_label_, 1, wxEXPAND);
    
    formPanel->SetSizer(formSizer);
    rootSizer->Add(formPanel, 0, wxEXPAND | wxALL, 12);
    
    rootSizer->AddStretchSpacer(1);
    
    wxButton* closeButton = new wxButton(this, wxID_OK, "Close");
    auto* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    btnSizer->AddStretchSpacer(1);
    btnSizer->Add(closeButton, 0, wxALL, 12);
    rootSizer->Add(btnSizer, 0, wxEXPAND);
    
    SetSizer(rootSizer);
}

std::string DatabaseEditorDialog::BuildSql() const {
    if (mode_ == DatabaseEditorMode::Create) {
        return BuildCreateSql();
    } else if (mode_ == DatabaseEditorMode::Clone) {
        return BuildCloneSql();
    }
    return std::string();
}

void DatabaseEditorDialog::SetSourceDatabase(const std::string& name) {
    source_database_ = name;
    if (source_db_label_) {
        source_db_label_->SetLabel(name);
    }
    if (mode_ == DatabaseEditorMode::Clone && target_name_ctrl_) {
        target_name_ctrl_->SetValue(name + "_copy");
    }
}

void DatabaseEditorDialog::LoadProperties(const QueryResult& result) {
    if (result.rows.empty()) {
        return;
    }
    
    const auto& row = result.rows[0];
    
    auto getValue = [&row, &result](const std::string& colName) -> std::string {
        for (size_t i = 0; i < result.columns.size() && i < row.size(); ++i) {
            std::string col = result.columns[i].name;
            std::transform(col.begin(), col.end(), col.begin(), ::tolower);
            if (col == colName && !row[i].isNull) {
                return row[i].text;
            }
        }
        return "-";
    };
    
    auto getBoolValue = [&row, &result](const std::string& colName) -> std::string {
        for (size_t i = 0; i < result.columns.size() && i < row.size(); ++i) {
            std::string col = result.columns[i].name;
            std::transform(col.begin(), col.end(), col.begin(), ::tolower);
            if (col == colName && !row[i].isNull) {
                return row[i].text == "t" || row[i].text == "true" || row[i].text == "1" ? "Yes" : "No";
            }
        }
        return "-";
    };
    
    if (prop_name_label_) prop_name_label_->SetLabel(getValue("database_name"));
    if (prop_owner_label_) prop_owner_label_->SetLabel(getValue("owner"));
    if (prop_encoding_label_) prop_encoding_label_->SetLabel(getValue("encoding"));
    if (prop_collation_label_) prop_collation_label_->SetLabel(getValue("collation"));
    if (prop_character_class_label_) prop_character_class_label_->SetLabel(getValue("character_class"));
    if (prop_tablespace_label_) prop_tablespace_label_->SetLabel(getValue("tablespace"));
    if (prop_size_label_) prop_size_label_->SetLabel(getValue("size"));
    if (prop_created_label_) prop_created_label_->SetLabel(getValue("created"));
    if (prop_allow_connections_label_) prop_allow_connections_label_->SetLabel(getBoolValue("allow_connections"));
    if (prop_connection_limit_label_) prop_connection_limit_label_->SetLabel(getValue("connection_limit"));
}

std::string DatabaseEditorDialog::BuildCreateSql() const {
    std::string name = Trim(name_ctrl_ ? name_ctrl_->GetValue().ToStdString() : std::string());
    if (name.empty()) {
        return std::string();
    }
    
    std::ostringstream sql;
    sql << "CREATE DATABASE " << QuoteIdentifier(name);
    
    // Owner
    if (owner_choice_) {
        std::string owner = owner_choice_->GetStringSelection().ToStdString();
        if (!owner.empty() && owner != "CURRENT_USER") {
            sql << "\n  OWNER " << QuoteIdentifier(owner);
        }
    }
    
    // Encoding
    if (encoding_choice_) {
        std::string encoding = encoding_choice_->GetStringSelection().ToStdString();
        if (!encoding.empty()) {
            sql << "\n  ENCODING '" << EscapeSqlLiteral(encoding) << "'";
        }
    }
    
    // Collation
    if (collation_choice_) {
        std::string collation = collation_choice_->GetStringSelection().ToStdString();
        if (!collation.empty()) {
            sql << "\n  LC_COLLATE '" << EscapeSqlLiteral(collation) << "'";
        }
    }
    
    // Character Class (LC_CTYPE)
    if (character_class_choice_) {
        std::string charClass = character_class_choice_->GetStringSelection().ToStdString();
        if (!charClass.empty()) {
            sql << "\n  LC_CTYPE '" << EscapeSqlLiteral(charClass) << "'";
        }
    }
    
    // Template
    if (template_choice_) {
        std::string tmpl = template_choice_->GetStringSelection().ToStdString();
        if (!tmpl.empty() && tmpl != "template1") {
            sql << "\n  TEMPLATE " << QuoteIdentifier(tmpl);
        }
    }
    
    // Tablespace
    if (tablespace_choice_) {
        std::string tablespace = tablespace_choice_->GetStringSelection().ToStdString();
        if (!tablespace.empty() && tablespace != "pg_default") {
            sql << "\n  TABLESPACE " << QuoteIdentifier(tablespace);
        }
    }
    
    // Allow connections
    if (allow_connections_ctrl_) {
        sql << "\n  ALLOW_CONNECTIONS " << (allow_connections_ctrl_->IsChecked() ? "true" : "false");
    }
    
    // Connection limit
    if (connection_limit_ctrl_) {
        int limit = connection_limit_ctrl_->GetValue();
        sql << "\n  CONNECTION LIMIT " << limit;
    }
    
    sql << ";";
    return sql.str();
}

std::string DatabaseEditorDialog::BuildCloneSql() const {
    std::string target = Trim(target_name_ctrl_ ? target_name_ctrl_->GetValue().ToStdString() : std::string());
    if (target.empty() || source_database_.empty()) {
        return std::string();
    }
    
    std::ostringstream sql;
    sql << "CREATE DATABASE " << QuoteIdentifier(target);
    
    // Use source as template
    sql << "\n  TEMPLATE " << QuoteIdentifier(source_database_);
    
    // Note: Structure-only or data-only cloning would require pg_dump/pg_restore
    // For this implementation, we use TEMPLATE which copies everything
    // The radio button selection is for future enhancement
    
    sql << ";";
    return sql.str();
}

std::string DatabaseEditorDialog::QuoteIdentifier(const std::string& value) const {
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

std::string DatabaseEditorDialog::EscapeSqlLiteral(const std::string& value) const {
    std::string out;
    out.reserve(value.size());
    for (char ch : value) {
        if (ch == '\'') {
            out.push_back('\'');
        }
        out.push_back(ch);
    }
    return out;
}

std::string DatabaseEditorDialog::Trim(const std::string& value) const {
    std::string result = value;
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    result.erase(result.begin(), std::find_if(result.begin(), result.end(), not_space));
    result.erase(std::find_if(result.rbegin(), result.rend(), not_space).base(), result.end());
    return result;
}

bool DatabaseEditorDialog::IsSimpleIdentifier(const std::string& value) const {
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

bool DatabaseEditorDialog::IsQuotedIdentifier(const std::string& value) const {
    return value.size() >= 2 && value.front() == '"' && value.back() == '"';
}

void DatabaseEditorDialog::UpdateConnectionLimitEnabled() {
    if (connection_limit_ctrl_ && allow_connections_ctrl_) {
        connection_limit_ctrl_->Enable(allow_connections_ctrl_->IsChecked());
    }
}

void DatabaseEditorDialog::OnAllowConnectionsChanged(wxCommandEvent&) {
    UpdateConnectionLimitEnabled();
}

} // namespace scratchrobin
