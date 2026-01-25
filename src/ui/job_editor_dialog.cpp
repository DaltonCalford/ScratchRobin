#include "job_editor_dialog.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <vector>

#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/sizer.h>
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

std::string EscapeSqlLiteral(const std::string& value) {
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

bool EqualsNoCase(const std::string& left, const std::string& right) {
    if (left.size() != right.size()) {
        return false;
    }
    for (size_t i = 0; i < left.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(left[i])) !=
            std::tolower(static_cast<unsigned char>(right[i]))) {
            return false;
        }
    }
    return true;
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

bool IsQuotedLiteral(const std::string& value) {
    return value.size() >= 2 && value.front() == '\'' && value.back() == '\'';
}

std::string QuoteIdentifier(const std::string& value) {
    if (IsSimpleIdentifier(value)) {
        return value;
    }
    if (IsQuotedIdentifier(value)) {
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

std::vector<std::string> SplitCommaList(const std::string& value) {
    std::vector<std::string> parts;
    std::stringstream stream(value);
    std::string token;
    while (std::getline(stream, token, ',')) {
        token = Trim(token);
        if (!token.empty()) {
            parts.push_back(token);
        }
    }
    return parts;
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

JobEditorDialog::JobEditorDialog(wxWindow* parent, JobEditorMode mode)
    : wxDialog(parent, wxID_ANY, mode == JobEditorMode::Create ? "Create Job" : "Edit Job",
               wxDefaultPosition, wxSize(560, 740),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      mode_(mode) {
    auto* rootSizer = new wxBoxSizer(wxVERTICAL);

    auto* nameLabel = new wxStaticText(this, wxID_ANY, "Job Name");
    rootSizer->Add(nameLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    name_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    rootSizer->Add(name_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    if (mode_ == JobEditorMode::Create) {
        auto* createModeLabel = new wxStaticText(this, wxID_ANY, "Create Mode");
        rootSizer->Add(createModeLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        create_mode_choice_ = BuildChoice(this, {"CREATE", "CREATE OR ALTER", "RECREATE"});
        rootSizer->Add(create_mode_choice_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    }

    auto* scheduleLabel = new wxStaticText(this, wxID_ANY, "Schedule");
    rootSizer->Add(scheduleLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    schedule_kind_choice_ = BuildChoice(this, {"CRON", "AT", "EVERY"});
    rootSizer->Add(schedule_kind_choice_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    schedule_value_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    schedule_value_ctrl_->SetHint("CRON: 0 * * * * | AT: 2026-01-09 10:00:00 | EVERY: 1 HOUR");
    rootSizer->Add(schedule_value_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    auto* startsLabel = new wxStaticText(this, wxID_ANY, "Starts At (EVERY only)");
    rootSizer->Add(startsLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    schedule_starts_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    schedule_starts_ctrl_->SetHint("2026-01-09 10:00:00");
    rootSizer->Add(schedule_starts_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    auto* endsLabel = new wxStaticText(this, wxID_ANY, "Ends At (EVERY only)");
    rootSizer->Add(endsLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    schedule_ends_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    schedule_ends_ctrl_->SetHint("2026-01-10 10:00:00");
    rootSizer->Add(schedule_ends_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    auto* stateLabel = new wxStaticText(this, wxID_ANY, "State");
    rootSizer->Add(stateLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    if (mode_ == JobEditorMode::Edit) {
        state_choice_ = BuildChoice(this, {"Unchanged", "ENABLED", "DISABLED", "PAUSED"});
    } else {
        state_choice_ = BuildChoice(this, {"ENABLED", "DISABLED", "PAUSED"});
    }
    rootSizer->Add(state_choice_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    auto* completionLabel = new wxStaticText(this, wxID_ANY, "On Completion");
    rootSizer->Add(completionLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    if (mode_ == JobEditorMode::Edit) {
        on_completion_choice_ = BuildChoice(this, {"Unchanged", "PRESERVE", "DROP"});
    } else {
        on_completion_choice_ = BuildChoice(this, {"Default", "PRESERVE", "DROP"});
    }
    rootSizer->Add(on_completion_choice_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    auto* descLabel = new wxStaticText(this, wxID_ANY, "Description");
    rootSizer->Add(descLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    description_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    rootSizer->Add(description_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    auto* runAsLabel = new wxStaticText(this, wxID_ANY, "Run As Role");
    rootSizer->Add(runAsLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    run_as_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    rootSizer->Add(run_as_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    auto* timeoutLabel = new wxStaticText(this, wxID_ANY, "Timeout (duration)");
    rootSizer->Add(timeoutLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    timeout_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    rootSizer->Add(timeout_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    auto* retryLabel = new wxStaticText(this, wxID_ANY, "Max Retries");
    rootSizer->Add(retryLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    max_retries_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    rootSizer->Add(max_retries_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    auto* backoffLabel = new wxStaticText(this, wxID_ANY, "Retry Backoff (duration)");
    rootSizer->Add(backoffLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    retry_backoff_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    rootSizer->Add(retry_backoff_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    auto* dependsLabel = new wxStaticText(this, wxID_ANY, "Depends On (comma-separated)");
    rootSizer->Add(dependsLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    depends_on_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    depends_on_ctrl_->SetHint("job_a, job_b (use NONE in edit to clear)");
    rootSizer->Add(depends_on_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    auto* classLabel = new wxStaticText(this, wxID_ANY, "Job Class");
    rootSizer->Add(classLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    job_class_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    rootSizer->Add(job_class_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    auto* partitionLabel = new wxStaticText(this, wxID_ANY, "Partition");
    rootSizer->Add(partitionLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    partition_kind_choice_ = BuildChoice(this, {"None", "ALL_SHARDS", "SINGLE_SHARD", "SHARD_SET", "DYNAMIC"});
    rootSizer->Add(partition_kind_choice_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    partition_value_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    partition_value_ctrl_->SetHint("UUID or shard expression");
    rootSizer->Add(partition_value_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    auto* bodyLabel = new wxStaticText(this, wxID_ANY, "Job Body");
    rootSizer->Add(bodyLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    if (mode_ == JobEditorMode::Edit) {
        edit_body_check_ = new wxCheckBox(this, wxID_ANY, "Update job body");
        rootSizer->Add(edit_body_check_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 6);
    }
    job_type_choice_ = BuildChoice(this, {"SQL", "PROCEDURE", "EXTERNAL"});
    rootSizer->Add(job_type_choice_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    job_body_label_ = new wxStaticText(this, wxID_ANY, "Job SQL");
    rootSizer->Add(job_body_label_, 0, wxLEFT | wxRIGHT | wxTOP, 6);
    job_body_ctrl_ = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxSize(-1, 160),
                                    wxTE_MULTILINE);
    rootSizer->Add(job_body_ctrl_, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    rootSizer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 12);
    SetSizerAndFit(rootSizer);
    CentreOnParent();

    if (schedule_kind_choice_) {
        schedule_kind_choice_->Bind(wxEVT_CHOICE,
            [this](wxCommandEvent&) { UpdateScheduleFields(); });
    }
    if (job_type_choice_) {
        job_type_choice_->Bind(wxEVT_CHOICE,
            [this](wxCommandEvent&) { UpdateJobBodyFields(); });
    }
    if (edit_body_check_) {
        edit_body_check_->Bind(wxEVT_CHECKBOX,
            [this](wxCommandEvent&) { UpdateJobBodyFields(); });
    }
    UpdateScheduleFields();
    UpdateJobBodyFields();
}

std::string JobEditorDialog::BuildSql() const {
    return mode_ == JobEditorMode::Create ? BuildCreateSql() : BuildAlterSql();
}

std::string JobEditorDialog::job_name() const {
    return Trim(name_ctrl_->GetValue().ToStdString());
}

void JobEditorDialog::SetJobName(const std::string& name) {
    name_ctrl_->SetValue(name);
    if (mode_ == JobEditorMode::Edit) {
        name_ctrl_->Enable(false);
    }
}

void JobEditorDialog::SetScheduleKind(const std::string& kind) {
    wxString upper = kind;
    upper.MakeUpper();
    int selection = schedule_kind_choice_->FindString(upper);
    if (selection != wxNOT_FOUND) {
        schedule_kind_choice_->SetSelection(selection);
        UpdateScheduleFields();
    }
}

void JobEditorDialog::SetScheduleValue(const std::string& value) {
    schedule_value_ctrl_->SetValue(value);
}

void JobEditorDialog::SetScheduleStarts(const std::string& value) {
    if (schedule_starts_ctrl_) {
        schedule_starts_ctrl_->SetValue(value);
    }
}

void JobEditorDialog::SetScheduleEnds(const std::string& value) {
    if (schedule_ends_ctrl_) {
        schedule_ends_ctrl_->SetValue(value);
    }
}

void JobEditorDialog::SetState(const std::string& state) {
    wxString upper = state;
    upper.MakeUpper();
    int selection = state_choice_->FindString(upper);
    if (selection != wxNOT_FOUND) {
        state_choice_->SetSelection(selection);
    }
}

void JobEditorDialog::SetOnCompletion(const std::string& value) {
    if (!on_completion_choice_) {
        return;
    }
    wxString upper = value;
    upper.MakeUpper();
    int selection = on_completion_choice_->FindString(upper);
    if (selection != wxNOT_FOUND) {
        on_completion_choice_->SetSelection(selection);
    }
}

void JobEditorDialog::SetDescription(const std::string& description) {
    description_ctrl_->SetValue(description);
}

void JobEditorDialog::SetRunAs(const std::string& role) {
    run_as_ctrl_->SetValue(role);
}

void JobEditorDialog::SetTimeoutSeconds(const std::string& value) {
    timeout_ctrl_->SetValue(value);
}

void JobEditorDialog::SetMaxRetries(const std::string& value) {
    max_retries_ctrl_->SetValue(value);
}

void JobEditorDialog::SetRetryBackoffSeconds(const std::string& value) {
    retry_backoff_ctrl_->SetValue(value);
}

void JobEditorDialog::SetJobType(const std::string& type) {
    if (!job_type_choice_) {
        return;
    }
    wxString upper = type;
    upper.MakeUpper();
    int selection = job_type_choice_->FindString(upper);
    if (selection != wxNOT_FOUND) {
        job_type_choice_->SetSelection(selection);
        UpdateJobBodyFields();
    }
}

void JobEditorDialog::SetJobBody(const std::string& value) {
    if (job_body_ctrl_) {
        job_body_ctrl_->SetValue(value);
    }
}

void JobEditorDialog::SetDependsOn(const std::string& value) {
    if (depends_on_ctrl_) {
        depends_on_ctrl_->SetValue(value);
    }
}

void JobEditorDialog::SetJobClass(const std::string& value) {
    if (job_class_ctrl_) {
        job_class_ctrl_->SetValue(value);
    }
}

void JobEditorDialog::SetPartition(const std::string& strategy, const std::string& value) {
    if (!partition_kind_choice_) {
        return;
    }
    wxString upper = strategy;
    upper.MakeUpper();
    int selection = partition_kind_choice_->FindString(upper);
    if (selection != wxNOT_FOUND) {
        partition_kind_choice_->SetSelection(selection);
    }
    if (partition_value_ctrl_) {
        partition_value_ctrl_->SetValue(value);
    }
}

std::string JobEditorDialog::BuildScheduleClause() const {
    std::string value = Trim(schedule_value_ctrl_->GetValue().ToStdString());
    if (value.empty()) {
        return std::string();
    }

    std::string kind = schedule_kind_choice_->GetStringSelection().ToStdString();
    if (kind == "CRON") {
        return "SCHEDULE = CRON '" + EscapeSqlLiteral(value) + "'";
    }
    if (kind == "AT") {
        return "SCHEDULE = AT '" + EscapeSqlLiteral(value) + "'";
    }
    if (kind == "EVERY") {
        std::string clause = "SCHEDULE = EVERY " + value;
        std::string starts = Trim(schedule_starts_ctrl_ ? schedule_starts_ctrl_->GetValue().ToStdString()
                                                        : std::string());
        if (!starts.empty()) {
            clause += " STARTS '" + EscapeSqlLiteral(starts) + "'";
        }
        std::string ends = Trim(schedule_ends_ctrl_ ? schedule_ends_ctrl_->GetValue().ToStdString()
                                                    : std::string());
        if (!ends.empty()) {
            clause += " ENDS '" + EscapeSqlLiteral(ends) + "'";
        }
        return clause;
    }
    return std::string();
}

std::string JobEditorDialog::BuildCreateSql() const {
    std::string name = job_name();
    if (name.empty()) {
        return std::string();
    }
    std::ostringstream sql;
    std::string create_mode = create_mode_choice_
        ? create_mode_choice_->GetStringSelection().ToStdString()
        : "CREATE";
    if (create_mode == "CREATE OR ALTER") {
        sql << "CREATE OR ALTER JOB " << QuoteIdentifier(name) << "\n";
    } else if (create_mode == "RECREATE") {
        sql << "RECREATE JOB " << QuoteIdentifier(name) << "\n";
    } else {
        sql << "CREATE JOB " << QuoteIdentifier(name) << "\n";
    }
    std::string schedule_clause = BuildScheduleClause();
    if (!schedule_clause.empty()) {
        sql << "  " << schedule_clause << "\n";
    } else {
        return std::string();
    }
    std::string state = state_choice_ ? state_choice_->GetStringSelection().ToStdString() : "";
    if (!state.empty()) {
        sql << "  STATE = " << state << "\n";
    }
    std::string on_completion = BuildOnCompletionClause();
    if (!on_completion.empty()) {
        sql << "  " << on_completion << "\n";
    }
    std::string description = Trim(description_ctrl_->GetValue().ToStdString());
    if (!description.empty()) {
        sql << "  DESCRIPTION = '" << EscapeSqlLiteral(description) << "'\n";
    }
    std::string run_as = Trim(run_as_ctrl_->GetValue().ToStdString());
    if (!run_as.empty()) {
        sql << "  RUN AS " << QuoteIdentifier(run_as) << "\n";
    }
    std::string max_retries = Trim(max_retries_ctrl_->GetValue().ToStdString());
    if (!max_retries.empty()) {
        sql << "  MAX_RETRIES = " << max_retries << "\n";
    }
    std::string retry_backoff = Trim(retry_backoff_ctrl_->GetValue().ToStdString());
    if (!retry_backoff.empty()) {
        sql << "  RETRY_BACKOFF = " << retry_backoff << "\n";
    }
    std::string timeout = Trim(timeout_ctrl_->GetValue().ToStdString());
    if (!timeout.empty()) {
        sql << "  TIMEOUT = " << timeout << "\n";
    }
    std::string depends_clause = BuildDependsClause();
    if (!depends_clause.empty()) {
        sql << "  " << depends_clause << "\n";
    }
    std::string class_name = Trim(job_class_ctrl_ ? job_class_ctrl_->GetValue().ToStdString()
                                                  : std::string());
    if (!class_name.empty()) {
        sql << "  CLASS = " << QuoteIdentifier(class_name) << "\n";
    }
    std::string partition_clause = BuildPartitionClause();
    if (!partition_clause.empty()) {
        sql << "  " << partition_clause << "\n";
    }
    std::string body_clause = BuildJobBodyClause();
    if (body_clause.empty()) {
        return std::string();
    }
    sql << "  " << body_clause << "\n";
    sql << ";";
    return sql.str();
}

std::string JobEditorDialog::BuildAlterSql() const {
    std::string name = job_name();
    if (name.empty()) {
        return std::string();
    }
    std::ostringstream sql;
    sql << "ALTER JOB " << QuoteIdentifier(name);

    bool appended = false;
    auto append_clause = [&](const std::string& clause) {
        if (clause.empty()) {
            return;
        }
        sql << "\n  SET " << clause;
        appended = true;
    };

    std::string schedule_clause = BuildScheduleClause();
    append_clause(schedule_clause);

    std::string state = state_choice_ ? state_choice_->GetStringSelection().ToStdString() : "";
    if (state != "Unchanged" && !state.empty()) {
        append_clause("STATE = " + state);
    }

    std::string on_completion = BuildOnCompletionClause();
    append_clause(on_completion);

    std::string description = Trim(description_ctrl_->GetValue().ToStdString());
    if (!description.empty()) {
        append_clause("DESCRIPTION = '" + EscapeSqlLiteral(description) + "'");
    }

    std::string run_as = Trim(run_as_ctrl_->GetValue().ToStdString());
    if (!run_as.empty()) {
        append_clause("RUN AS " + QuoteIdentifier(run_as));
    }

    std::string max_retries = Trim(max_retries_ctrl_->GetValue().ToStdString());
    if (!max_retries.empty()) {
        append_clause("MAX_RETRIES = " + max_retries);
    }

    std::string retry_backoff = Trim(retry_backoff_ctrl_->GetValue().ToStdString());
    if (!retry_backoff.empty()) {
        append_clause("RETRY_BACKOFF = " + retry_backoff);
    }

    std::string timeout = Trim(timeout_ctrl_->GetValue().ToStdString());
    if (!timeout.empty()) {
        append_clause("TIMEOUT = " + timeout);
    }

    std::string depends_clause = BuildDependsClause();
    append_clause(depends_clause);

    std::string class_name = Trim(job_class_ctrl_ ? job_class_ctrl_->GetValue().ToStdString()
                                                  : std::string());
    if (!class_name.empty()) {
        append_clause("CLASS = " + QuoteIdentifier(class_name));
    }

    std::string partition_clause = BuildPartitionClause();
    append_clause(partition_clause);

    std::string body_clause = BuildJobBodyClause();
    append_clause(body_clause);

    if (!appended) {
        return std::string();
    }
    sql << ";";
    return sql.str();
}

std::string JobEditorDialog::BuildDependsClause() const {
    if (!depends_on_ctrl_) {
        return std::string();
    }
    std::string raw = Trim(depends_on_ctrl_->GetValue().ToStdString());
    if (raw.empty()) {
        return std::string();
    }
    if (EqualsNoCase(raw, "NONE")) {
        if (mode_ == JobEditorMode::Create) {
            return std::string();
        }
        return "DEPENDS ON NONE";
    }
    auto parts = SplitCommaList(raw);
    if (parts.empty()) {
        return std::string();
    }
    std::ostringstream out;
    out << "DEPENDS ON ";
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) {
            out << ", ";
        }
        out << QuoteIdentifier(parts[i]);
    }
    return out.str();
}

std::string JobEditorDialog::BuildPartitionClause() const {
    if (!partition_kind_choice_) {
        return std::string();
    }
    std::string kind = partition_kind_choice_->GetStringSelection().ToStdString();
    if (kind.empty() || kind == "None") {
        return std::string();
    }
    if (kind == "ALL_SHARDS") {
        return "PARTITION BY ALL_SHARDS";
    }
    std::string value = Trim(partition_value_ctrl_ ? partition_value_ctrl_->GetValue().ToStdString()
                                                   : std::string());
    if (value.empty()) {
        return std::string();
    }
    if (IsQuotedLiteral(value)) {
        return "PARTITION BY " + kind + " " + value;
    }
    return "PARTITION BY " + kind + " '" + EscapeSqlLiteral(value) + "'";
}

std::string JobEditorDialog::BuildOnCompletionClause() const {
    if (!on_completion_choice_) {
        return std::string();
    }
    std::string value = on_completion_choice_->GetStringSelection().ToStdString();
    if (value.empty()) {
        return std::string();
    }
    if ((mode_ == JobEditorMode::Edit && value == "Unchanged") ||
        (mode_ == JobEditorMode::Create && value == "Default")) {
        return std::string();
    }
    return "ON COMPLETION " + value;
}

std::string JobEditorDialog::BuildJobBodyClause() const {
    bool include = mode_ == JobEditorMode::Create;
    if (mode_ == JobEditorMode::Edit) {
        include = edit_body_check_ && edit_body_check_->IsChecked();
    }
    if (!include || !job_body_ctrl_ || !job_type_choice_) {
        return std::string();
    }
    std::string body = Trim(job_body_ctrl_->GetValue().ToStdString());
    if (body.empty()) {
        return std::string();
    }
    std::string type = job_type_choice_->GetStringSelection().ToStdString();
    if (type == "SQL") {
        return "AS '" + EscapeSqlLiteral(body) + "'";
    }
    if (type == "PROCEDURE") {
        std::string proc = body;
        if (proc.find('"') == std::string::npos && proc.find('.') == std::string::npos) {
            proc = QuoteIdentifier(proc);
        }
        if (proc.find('(') == std::string::npos) {
            proc += "()";
        }
        return "CALL " + proc;
    }
    return "EXEC '" + EscapeSqlLiteral(body) + "'";
}

void JobEditorDialog::UpdateScheduleFields() {
    bool every = schedule_kind_choice_ &&
        schedule_kind_choice_->GetStringSelection().ToStdString() == "EVERY";
    if (schedule_starts_ctrl_) {
        schedule_starts_ctrl_->Enable(every);
    }
    if (schedule_ends_ctrl_) {
        schedule_ends_ctrl_->Enable(every);
    }
}

void JobEditorDialog::UpdateJobBodyFields() {
    bool allow_edit = mode_ == JobEditorMode::Create;
    if (mode_ == JobEditorMode::Edit && edit_body_check_) {
        allow_edit = edit_body_check_->IsChecked();
    }
    if (job_type_choice_) {
        job_type_choice_->Enable(allow_edit);
    }
    if (job_body_ctrl_) {
        job_body_ctrl_->Enable(allow_edit);
    }

    if (!job_body_label_ || !job_body_ctrl_ || !job_type_choice_) {
        return;
    }
    std::string type = job_type_choice_->GetStringSelection().ToStdString();
    if (type == "PROCEDURE") {
        job_body_label_->SetLabel("Procedure Name");
        job_body_ctrl_->SetHint("schema.proc");
    } else if (type == "EXTERNAL") {
        job_body_label_->SetLabel("External Command");
        job_body_ctrl_->SetHint("/path/to/command");
    } else {
        job_body_label_->SetLabel("Job SQL");
        job_body_ctrl_->SetHint("SELECT ...");
    }
}

} // namespace scratchrobin
