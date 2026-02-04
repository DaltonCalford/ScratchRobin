/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_DATABASE_EDITOR_DIALOG_H
#define SCRATCHROBIN_DATABASE_EDITOR_DIALOG_H

#include <string>

#include <wx/dialog.h>

class wxCheckBox;
class wxChoice;
class wxStaticText;
class wxTextCtrl;
class wxSpinCtrl;
class wxRadioButton;

namespace scratchrobin {

struct QueryResult;

enum class DatabaseEditorMode {
    Create,
    Clone,
    Properties
};

class DatabaseEditorDialog : public wxDialog {
public:
    DatabaseEditorDialog(wxWindow* parent, DatabaseEditorMode mode);

    std::string BuildSql() const;
    
    void SetSourceDatabase(const std::string& name);
    void LoadProperties(const QueryResult& result);

private:
    void BuildCreateLayout();
    void BuildCloneLayout();
    void BuildPropertiesLayout();
    void PopulateDropdowns();
    
    std::string BuildCreateSql() const;
    std::string BuildCloneSql() const;
    std::string QuoteIdentifier(const std::string& value) const;
    std::string EscapeSqlLiteral(const std::string& value) const;
    std::string Trim(const std::string& value) const;
    bool IsSimpleIdentifier(const std::string& value) const;
    bool IsQuotedIdentifier(const std::string& value) const;
    
    void UpdateConnectionLimitEnabled();
    void OnAllowConnectionsChanged(wxCommandEvent& event);

    DatabaseEditorMode mode_;
    std::string source_database_;

    // Create mode controls
    wxTextCtrl* name_ctrl_ = nullptr;
    wxChoice* owner_choice_ = nullptr;
    wxChoice* encoding_choice_ = nullptr;
    wxChoice* collation_choice_ = nullptr;
    wxChoice* character_class_choice_ = nullptr;
    wxChoice* template_choice_ = nullptr;
    wxChoice* tablespace_choice_ = nullptr;
    wxCheckBox* allow_connections_ctrl_ = nullptr;
    wxSpinCtrl* connection_limit_ctrl_ = nullptr;

    // Clone mode controls
    wxStaticText* source_db_label_ = nullptr;
    wxTextCtrl* target_name_ctrl_ = nullptr;
    wxRadioButton* clone_data_radio_ = nullptr;
    wxRadioButton* clone_structure_radio_ = nullptr;
    wxRadioButton* clone_both_radio_ = nullptr;

    // Properties mode controls (read-only)
    wxStaticText* prop_name_label_ = nullptr;
    wxStaticText* prop_owner_label_ = nullptr;
    wxStaticText* prop_encoding_label_ = nullptr;
    wxStaticText* prop_collation_label_ = nullptr;
    wxStaticText* prop_character_class_label_ = nullptr;
    wxStaticText* prop_tablespace_label_ = nullptr;
    wxStaticText* prop_size_label_ = nullptr;
    wxStaticText* prop_created_label_ = nullptr;
    wxStaticText* prop_allow_connections_label_ = nullptr;
    wxStaticText* prop_connection_limit_label_ = nullptr;
    wxStaticText* prop_active_connections_label_ = nullptr;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_DATABASE_EDITOR_DIALOG_H
