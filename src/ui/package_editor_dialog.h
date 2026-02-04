/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_PACKAGE_EDITOR_DIALOG_H
#define SCRATCHROBIN_PACKAGE_EDITOR_DIALOG_H

#include <string>
#include <vector>

#include <wx/dialog.h>

class wxChoice;
class wxNotebook;
class wxStaticText;
class wxTextCtrl;

namespace scratchrobin {

enum class PackageEditorMode {
    Create,
    Edit
};

class PackageEditorDialog : public wxDialog {
public:
    PackageEditorDialog(wxWindow* parent, PackageEditorMode mode);

    std::string BuildSql() const;
    std::string package_name() const;
    std::string schema_name() const;

    void SetPackageName(const std::string& name);
    void SetSchemaName(const std::string& schema);
    void SetAvailableSchemas(const std::vector<std::string>& schemas);

    void SetSpecification(const std::string& spec);
    void SetBody(const std::string& body);

    std::string GetSpecification() const;
    std::string GetBody() const;

private:
    std::string BuildCreateSql() const;
    std::string BuildAlterSql() const;
    std::string FormatPackagePath() const;
    std::string QuoteIdentifier(const std::string& value) const;
    std::string Trim(const std::string& value) const;
    bool IsSimpleIdentifier(const std::string& value) const;
    bool IsQuotedIdentifier(const std::string& value) const;

    void BuildLayout();
    void BuildSpecificationTab(wxNotebook* notebook);
    void BuildBodyTab(wxNotebook* notebook);

    PackageEditorMode mode_;

    // Top controls
    wxTextCtrl* name_ctrl_ = nullptr;
    wxChoice* schema_choice_ = nullptr;

    // Notebook
    wxNotebook* notebook_ = nullptr;

    // Specification tab controls
    wxTextCtrl* spec_ctrl_ = nullptr;

    // Body tab controls
    wxTextCtrl* body_ctrl_ = nullptr;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_PACKAGE_EDITOR_DIALOG_H
