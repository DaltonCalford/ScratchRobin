#ifndef SCRATCHROBIN_JOB_EDITOR_DIALOG_H
#define SCRATCHROBIN_JOB_EDITOR_DIALOG_H

#include <string>

#include <wx/dialog.h>

class wxChoice;
class wxCheckBox;
class wxStaticText;
class wxTextCtrl;

namespace scratchrobin {

enum class JobEditorMode {
    Create,
    Edit
};

class JobEditorDialog : public wxDialog {
public:
    JobEditorDialog(wxWindow* parent, JobEditorMode mode);

    std::string BuildSql() const;
    std::string job_name() const;

    void SetJobName(const std::string& name);
    void SetScheduleKind(const std::string& kind);
    void SetScheduleValue(const std::string& value);
    void SetScheduleStarts(const std::string& value);
    void SetScheduleEnds(const std::string& value);
    void SetState(const std::string& state);
    void SetOnCompletion(const std::string& value);
    void SetDescription(const std::string& description);
    void SetRunAs(const std::string& role);
    void SetTimeoutSeconds(const std::string& value);
    void SetMaxRetries(const std::string& value);
    void SetRetryBackoffSeconds(const std::string& value);
    void SetJobType(const std::string& type);
    void SetJobBody(const std::string& value);
    void SetDependsOn(const std::string& value);
    void SetJobClass(const std::string& value);
    void SetPartition(const std::string& strategy, const std::string& value);

private:
    std::string BuildCreateSql() const;
    std::string BuildAlterSql() const;
    std::string BuildScheduleClause() const;
    std::string BuildDependsClause() const;
    std::string BuildPartitionClause() const;
    std::string BuildOnCompletionClause() const;
    std::string BuildJobBodyClause() const;
    void UpdateScheduleFields();
    void UpdateJobBodyFields();

    JobEditorMode mode_;
    wxTextCtrl* name_ctrl_ = nullptr;
    wxChoice* create_mode_choice_ = nullptr;
    wxChoice* schedule_kind_choice_ = nullptr;
    wxTextCtrl* schedule_value_ctrl_ = nullptr;
    wxTextCtrl* schedule_starts_ctrl_ = nullptr;
    wxTextCtrl* schedule_ends_ctrl_ = nullptr;
    wxChoice* state_choice_ = nullptr;
    wxChoice* on_completion_choice_ = nullptr;
    wxTextCtrl* description_ctrl_ = nullptr;
    wxTextCtrl* run_as_ctrl_ = nullptr;
    wxTextCtrl* timeout_ctrl_ = nullptr;
    wxTextCtrl* max_retries_ctrl_ = nullptr;
    wxTextCtrl* retry_backoff_ctrl_ = nullptr;
    wxTextCtrl* depends_on_ctrl_ = nullptr;
    wxTextCtrl* job_class_ctrl_ = nullptr;
    wxChoice* partition_kind_choice_ = nullptr;
    wxTextCtrl* partition_value_ctrl_ = nullptr;
    wxCheckBox* edit_body_check_ = nullptr;
    wxChoice* job_type_choice_ = nullptr;
    wxStaticText* job_body_label_ = nullptr;
    wxTextCtrl* job_body_ctrl_ = nullptr;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_JOB_EDITOR_DIALOG_H
