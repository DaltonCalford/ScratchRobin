/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_AUDIT_RETENTION_POLICY_DIALOG_H
#define SCRATCHROBIN_AUDIT_RETENTION_POLICY_DIALOG_H

#include <string>

#include <wx/dialog.h>

class wxTextCtrl;
class wxChoice;
class wxCheckBox;

namespace scratchrobin {

class AuditRetentionPolicyDialog : public wxDialog {
public:
    enum class Mode {
        Create,
        Edit
    };

    explicit AuditRetentionPolicyDialog(wxWindow* parent, Mode mode);

    void SetPolicyId(const std::string& id);
    void SetCategory(const std::string& category);
    void SetSeverityMin(const std::string& severity);
    void SetRetentionPeriod(const std::string& period);
    void SetArchiveAfter(const std::string& period);
    void SetDeleteAfter(const std::string& period);
    void SetStorageClass(const std::string& storage_class);
    void SetActive(bool active);

    std::string GetStatement() const;

private:
    void BuildLayout();
    void UpdateStatementPreview();
    std::string BuildStatement() const;

    Mode mode_;
    std::string statement_;

    wxTextCtrl* policy_id_ctrl_ = nullptr;
    wxTextCtrl* category_ctrl_ = nullptr;
    wxTextCtrl* severity_min_ctrl_ = nullptr;
    wxTextCtrl* retention_period_ctrl_ = nullptr;
    wxTextCtrl* archive_after_ctrl_ = nullptr;
    wxTextCtrl* delete_after_ctrl_ = nullptr;
    wxChoice* storage_class_choice_ = nullptr;
    wxCheckBox* active_ctrl_ = nullptr;
    wxTextCtrl* preview_ctrl_ = nullptr;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_AUDIT_RETENTION_POLICY_DIALOG_H
