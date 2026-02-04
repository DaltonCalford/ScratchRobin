/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_VIEW_EDITOR_DIALOG_H
#define SCRATCHROBIN_VIEW_EDITOR_DIALOG_H

#include <string>
#include <vector>

#include <wx/dialog.h>

class wxCheckBox;
class wxChoice;
class wxGrid;
class wxNotebook;
class wxStaticText;
class wxTextCtrl;

namespace scratchrobin {

enum class ViewEditorMode {
    Create,
    Edit
};

struct ViewColumnInfo {
    std::string name;
    std::string data_type;
    bool is_nullable = true;
    std::string default_value;
};

struct ViewDependencyInfo {
    std::string name;
    std::string type;  // "TABLE" or "VIEW"
    std::string schema;
};

class ViewEditorDialog : public wxDialog {
public:
    ViewEditorDialog(wxWindow* parent, ViewEditorMode mode);

    std::string BuildSql() const;
    std::string view_name() const;

    void SetViewName(const std::string& name);
    void SetSchema(const std::string& schema);
    void SetViewDefinition(const std::string& definition);
    void SetColumns(const std::vector<ViewColumnInfo>& columns);
    void SetDependencies(const std::vector<ViewDependencyInfo>& dependencies);

private:
    void BuildLayout();
    void BuildDefinitionTab(wxNotebook* notebook);
    void BuildColumnsTab(wxNotebook* notebook);
    void BuildDependenciesTab(wxNotebook* notebook);

    std::string BuildCreateSql() const;
    std::string BuildAlterSql() const;
    std::string FormatViewPath(const std::string& value) const;
    void UpdateViewTypeFields();
    void UpdateAlterActionFields();
    void RefreshColumnsFromDefinition();
    void ParseDependenciesFromDefinition();

    ViewEditorMode mode_;

    // Top fields
    wxTextCtrl* name_ctrl_ = nullptr;
    wxTextCtrl* schema_ctrl_ = nullptr;
    wxCheckBox* if_not_exists_ctrl_ = nullptr;
    wxCheckBox* or_replace_ctrl_ = nullptr;
    wxChoice* view_type_choice_ = nullptr;
    wxChoice* check_option_choice_ = nullptr;
    wxCheckBox* is_updatable_ctrl_ = nullptr;

    // Notebook tabs
    wxNotebook* notebook_ = nullptr;

    // Definition tab
    wxTextCtrl* definition_ctrl_ = nullptr;

    // Columns tab
    wxGrid* columns_grid_ = nullptr;

    // Dependencies tab
    wxGrid* dependencies_grid_ = nullptr;

    // Edit mode fields
    wxChoice* alter_action_choice_ = nullptr;
    wxStaticText* alter_value_label_ = nullptr;
    wxTextCtrl* alter_value_ctrl_ = nullptr;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_VIEW_EDITOR_DIALOG_H
