/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 */
#include "reporting_frame.h"

#include "menu_builder.h"
#include "menu_ids.h"
#include "window_manager.h"
#include "core/project.h"
#include "core/result_exporter.h"

#include <ctime>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/listbox.h>
#include <wx/msgdlg.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/splitter.h>
#include <wx/statline.h>
#include <wx/stattext.h>
#include <wx/textdlg.h>
#include <wx/textctrl.h>

namespace scratchrobin {

namespace {

constexpr int kRunQueryId = wxID_HIGHEST + 2100;
constexpr int kSaveQueryId = wxID_HIGHEST + 2101;
constexpr int kScheduleQueryId = wxID_HIGHEST + 2102;
constexpr int kRefreshPreviewId = wxID_HIGHEST + 2103;
constexpr int kAddAlertId = wxID_HIGHEST + 2104;
constexpr int kScheduleAlertId = wxID_HIGHEST + 2105;
constexpr int kScheduleTickId = wxID_HIGHEST + 2106;

std::string Trim(std::string value) {
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

std::string ToLowerCopy(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

std::string NormalizeBackendName(const std::string& raw) {
    std::string value = ToLowerCopy(Trim(raw));
    if (value.empty() || value == "network" || value == "scratchbird") {
        return "native";
    }
    if (value == "postgres" || value == "pg") {
        return "postgresql";
    }
    if (value == "mariadb") {
        return "mysql";
    }
    if (value == "fb") {
        return "firebird";
    }
    return value;
}

std::string ProfileLabel(const ConnectionProfile& profile) {
    if (!profile.name.empty()) {
        return profile.name;
    }
    if (!profile.database.empty()) {
        return profile.database;
    }
    std::string label = profile.host.empty() ? "localhost" : profile.host;
    if (profile.port != 0) {
        label += ":" + std::to_string(profile.port);
    }
    return label;
}

std::string NormalizeSqlForCache(const std::string& sql) {
    std::string trimmed = Trim(sql);
    std::string out;
    out.reserve(trimmed.size());
    bool in_space = false;
    for (char c : trimmed) {
        if (std::isspace(static_cast<unsigned char>(c))) {
            if (!in_space) {
                out.push_back(' ');
                in_space = true;
            }
        } else {
            out.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
            in_space = false;
        }
    }
    return out;
}

std::string JsonEscape(const std::string& value) {
    std::string out;
    out.reserve(value.size() + 8);
    for (char c : value) {
        switch (c) {
            case '\"': out += "\\\""; break;
            case '\\': out += "\\\\"; break;
            case '\n': out += "\\n"; break;
            case '\r': out += "\\r"; break;
            case '\t': out += "\\t"; break;
            default: out += c; break;
        }
    }
    return out;
}

std::string SerializeQueryResultToJson(const QueryResult& result, size_t max_rows) {
    std::ostringstream out;
    out << "{";
    out << "\"columns\":[";
    for (size_t i = 0; i < result.columns.size(); ++i) {
        if (i) out << ",";
        out << "{";
        out << "\"name\":\"" << JsonEscape(result.columns[i].name) << "\",";
        out << "\"type\":\"" << JsonEscape(result.columns[i].type) << "\"";
        out << "}";
    }
    out << "],";
    out << "\"rows\":[";
    size_t rows_to_write = std::min(max_rows, result.rows.size());
    for (size_t r = 0; r < rows_to_write; ++r) {
        if (r) out << ",";
        out << "[";
        for (size_t c = 0; c < result.rows[r].size(); ++c) {
            if (c) out << ",";
            const auto& cell = result.rows[r][c];
            if (cell.isNull) {
                out << "null";
            } else {
                out << "\"" << JsonEscape(cell.text) << "\"";
            }
        }
        out << "]";
    }
    out << "],";
    out << "\"rows_affected\":" << result.rowsAffected << ",";
    out << "\"command_tag\":\"" << JsonEscape(result.commandTag) << "\",";
    out << "\"truncated\":" << (result.rows.size() > rows_to_write ? "true" : "false");
    out << "}";
    return out.str();
}

std::string PromptScheduleSpec(wxWindow* parent, const wxString& title) {
    wxTextEntryDialog dialog(parent,
                             "Enter schedule (examples: \"every 15 minutes\", \"hourly\", \"daily\")",
                             title);
    if (dialog.ShowModal() != wxID_OK) {
        return {};
    }
    return Trim(dialog.GetValue().ToStdString());
}

wxPanel* BuildPreviewTile(wxWindow* parent, const wxString& title) {
    auto* panel = new wxPanel(parent, wxID_ANY);
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    auto* label = new wxStaticText(panel, wxID_ANY, title);
    label->SetForegroundColour(wxColour(200, 200, 200));
    sizer->Add(label, 0, wxALL, 6);
    sizer->AddStretchSpacer(1);
    panel->SetSizer(sizer);
    panel->SetBackgroundColour(wxColour(36, 36, 44));
    return panel;
}

} // namespace

wxBEGIN_EVENT_TABLE(ReportingFrame, wxFrame)
    EVT_BUTTON(kRunQueryId, ReportingFrame::OnRunQuery)
    EVT_BUTTON(kSaveQueryId, ReportingFrame::OnSaveQuery)
    EVT_BUTTON(kScheduleQueryId, ReportingFrame::OnScheduleQuery)
    EVT_BUTTON(kRefreshPreviewId, ReportingFrame::OnRefreshPreview)
    EVT_BUTTON(kAddAlertId, ReportingFrame::OnAddAlert)
    EVT_BUTTON(kScheduleAlertId, ReportingFrame::OnScheduleAlert)
    EVT_TIMER(kScheduleTickId, ReportingFrame::OnScheduleTick)
wxEND_EVENT_TABLE()

ReportingFrame::ReportingFrame(WindowManager* windowManager,
                               ConnectionManager* connectionManager,
                               std::vector<ConnectionProfile>* connections,
                               const AppConfig* appConfig)
    : wxFrame(nullptr, wxID_ANY, "Reporting & Analytics", wxDefaultPosition, wxSize(1200, 780)),
      window_manager_(windowManager),
      connection_manager_(connectionManager),
      connections_(connections),
      app_config_(appConfig),
      schedule_timer_(this, kScheduleTickId) {
    BuildMenu();
    BuildLayout();
    PopulateConnections();
    schedule_timer_.Start(60000);
    if (window_manager_) {
        window_manager_->RegisterWindow(this);
    }
}

void ReportingFrame::BuildMenu() {
    // Child windows use minimal menu bar (File + Help only)
    auto* menu_bar = scratchrobin::BuildMinimalMenuBar(this);
    SetMenuBar(menu_bar);
}

void ReportingFrame::BuildLayout() {
    auto* rootSizer = new wxBoxSizer(wxVERTICAL);

    auto* headerPanel = new wxPanel(this, wxID_ANY);
    auto* headerSizer = new wxBoxSizer(wxHORIZONTAL);
    headerSizer->Add(new wxStaticText(headerPanel, wxID_ANY, "Connection:"), 0,
                     wxALIGN_CENTER_VERTICAL | wxRIGHT, 6);
    connection_choice_ = new wxChoice(headerPanel, wxID_ANY);
    headerSizer->Add(connection_choice_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 12);
    auto* runBtn = new wxButton(headerPanel, kRunQueryId, "Run");
    auto* saveBtn = new wxButton(headerPanel, kSaveQueryId, "Save");
    auto* scheduleBtn = new wxButton(headerPanel, kScheduleQueryId, "Schedule");
    auto* refreshBtn = new wxButton(headerPanel, kRefreshPreviewId, "Refresh Preview");
    headerSizer->Add(runBtn, 0, wxRIGHT, 6);
    headerSizer->Add(saveBtn, 0, wxRIGHT, 6);
    headerSizer->Add(scheduleBtn, 0, wxRIGHT, 6);
    headerSizer->Add(refreshBtn, 0);
    headerSizer->AddStretchSpacer(1);
    status_label_ = new wxStaticText(headerPanel, wxID_ANY, "Idle");
    status_label_->SetForegroundColour(wxColour(160, 160, 170));
    headerSizer->Add(status_label_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 12);
    headerPanel->SetSizer(headerSizer);
    rootSizer->Add(headerPanel, 0, wxEXPAND | wxALL, 8);

    auto* notebook = new wxNotebook(this, wxID_ANY);

    // Query Builder Tab
    auto* queryPanel = new wxPanel(notebook, wxID_ANY);
    auto* querySizer = new wxBoxSizer(wxHORIZONTAL);

    auto* queryLeft = new wxPanel(queryPanel, wxID_ANY);
    auto* queryLeftSizer = new wxBoxSizer(wxVERTICAL);
    queryLeftSizer->Add(new wxStaticText(queryLeft, wxID_ANY, "Collections"), 0, wxALL, 6);
    auto* collectionList = new wxListBox(queryLeft, wxID_ANY);
    collectionList->Append("Core");
    collectionList->Append("Analytics");
    collectionList->Append("Operational");
    queryLeftSizer->Add(collectionList, 1, wxEXPAND | wxALL, 6);
    queryLeftSizer->Add(new wxStaticText(queryLeft, wxID_ANY, "Tables"), 0, wxALL, 6);
    auto* tableList = new wxListBox(queryLeft, wxID_ANY);
    tableList->Append("orders");
    tableList->Append("customers");
    tableList->Append("line_items");
    queryLeftSizer->Add(tableList, 1, wxEXPAND | wxALL, 6);
    queryLeft->SetSizer(queryLeftSizer);

    auto* queryCenter = new wxPanel(queryPanel, wxID_ANY);
    auto* queryCenterSizer = new wxBoxSizer(wxVERTICAL);
    queryCenterSizer->Add(new wxStaticText(queryCenter, wxID_ANY, "Query Builder"), 0, wxALL, 6);
    sql_editor_ = new wxTextCtrl(queryCenter, wxID_ANY,
                                 "SELECT *\nFROM orders\nWHERE created_at >= CURRENT_DATE - 30;",
                                 wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
    queryCenterSizer->Add(sql_editor_, 1, wxEXPAND | wxALL, 6);
    queryCenterSizer->Add(new wxStaticLine(queryCenter), 0, wxEXPAND | wxLEFT | wxRIGHT, 6);
    queryCenterSizer->Add(new wxStaticText(queryCenter, wxID_ANY, "Chart Preview Grid"), 0, wxALL, 6);
    auto* previewGrid = new wxGridSizer(2, 2, 8, 8);
    previewGrid->Add(BuildPreviewTile(queryCenter, "Line Chart"), 1, wxEXPAND);
    previewGrid->Add(BuildPreviewTile(queryCenter, "Bar Chart"), 1, wxEXPAND);
    previewGrid->Add(BuildPreviewTile(queryCenter, "Table Preview"), 1, wxEXPAND);
    previewGrid->Add(BuildPreviewTile(queryCenter, "Metric Card"), 1, wxEXPAND);
    queryCenterSizer->Add(previewGrid, 1, wxEXPAND | wxALL, 6);
    queryCenter->SetSizer(queryCenterSizer);

    auto* queryRight = new wxPanel(queryPanel, wxID_ANY);
    auto* queryRightSizer = new wxBoxSizer(wxVERTICAL);
    queryRightSizer->Add(new wxStaticText(queryRight, wxID_ANY, "Properties"), 0, wxALL, 6);
    queryRightSizer->Add(new wxTextCtrl(queryRight, wxID_ANY, "Name: Monthly Orders"), 0,
                         wxEXPAND | wxALL, 6);
    queryRightSizer->Add(new wxTextCtrl(queryRight, wxID_ANY, "Visualization: Line"), 0,
                         wxEXPAND | wxALL, 6);
    queryRightSizer->Add(new wxTextCtrl(queryRight, wxID_ANY, "Parameters: None"), 0,
                         wxEXPAND | wxALL, 6);
    queryRightSizer->AddStretchSpacer(1);
    queryRight->SetSizer(queryRightSizer);

    querySizer->Add(queryLeft, 0, wxEXPAND | wxALL, 4);
    querySizer->Add(queryCenter, 1, wxEXPAND | wxALL, 4);
    querySizer->Add(queryRight, 0, wxEXPAND | wxALL, 4);
    queryPanel->SetSizer(querySizer);
    notebook->AddPage(queryPanel, "Query Builder", true);

    // Dashboard Editor Tab
    auto* dashboardPanel = new wxPanel(notebook, wxID_ANY);
    auto* dashboardSizer = new wxBoxSizer(wxHORIZONTAL);

    auto* dashboardLeft = new wxPanel(dashboardPanel, wxID_ANY);
    auto* dashboardLeftSizer = new wxBoxSizer(wxVERTICAL);
    dashboardLeftSizer->Add(new wxStaticText(dashboardLeft, wxID_ANY, "Dashboards"), 0, wxALL, 6);
    auto* dashboardList = new wxListBox(dashboardLeft, wxID_ANY);
    dashboardList->Append("Executive Overview");
    dashboardList->Append("Sales Health");
    dashboardList->Append("Inventory Snapshot");
    dashboardLeftSizer->Add(dashboardList, 1, wxEXPAND | wxALL, 6);
    dashboardLeft->SetSizer(dashboardLeftSizer);

    auto* dashboardCenter = new wxPanel(dashboardPanel, wxID_ANY);
    auto* dashboardCenterSizer = new wxBoxSizer(wxVERTICAL);
    dashboardCenterSizer->Add(new wxStaticText(dashboardCenter, wxID_ANY, "Dashboard Layout"), 0, wxALL, 6);
    auto* dashboardGrid = new wxGridSizer(2, 2, 10, 10);
    dashboardGrid->Add(BuildPreviewTile(dashboardCenter, "Revenue Trend"), 1, wxEXPAND);
    dashboardGrid->Add(BuildPreviewTile(dashboardCenter, "Pipeline"), 1, wxEXPAND);
    dashboardGrid->Add(BuildPreviewTile(dashboardCenter, "Region Map"), 1, wxEXPAND);
    dashboardGrid->Add(BuildPreviewTile(dashboardCenter, "Top Products"), 1, wxEXPAND);
    dashboardCenterSizer->Add(dashboardGrid, 1, wxEXPAND | wxALL, 6);
    dashboardCenter->SetSizer(dashboardCenterSizer);

    auto* dashboardRight = new wxPanel(dashboardPanel, wxID_ANY);
    auto* dashboardRightSizer = new wxBoxSizer(wxVERTICAL);
    dashboardRightSizer->Add(new wxStaticText(dashboardRight, wxID_ANY, "Filters & Properties"), 0, wxALL, 6);
    dashboardRightSizer->Add(new wxTextCtrl(dashboardRight, wxID_ANY, "Owner: analytics"), 0,
                             wxEXPAND | wxALL, 6);
    dashboardRightSizer->Add(new wxTextCtrl(dashboardRight, wxID_ANY, "Refresh: 30 min"), 0,
                             wxEXPAND | wxALL, 6);
    dashboardRightSizer->Add(new wxTextCtrl(dashboardRight, wxID_ANY, "Sharing: internal"), 0,
                             wxEXPAND | wxALL, 6);
    dashboardRightSizer->AddStretchSpacer(1);
    dashboardRight->SetSizer(dashboardRightSizer);

    dashboardSizer->Add(dashboardLeft, 0, wxEXPAND | wxALL, 4);
    dashboardSizer->Add(dashboardCenter, 1, wxEXPAND | wxALL, 4);
    dashboardSizer->Add(dashboardRight, 0, wxEXPAND | wxALL, 4);
    dashboardPanel->SetSizer(dashboardSizer);
    notebook->AddPage(dashboardPanel, "Dashboard Editor", false);

    // Alerts & Rules Tab
    auto* alertPanel = new wxPanel(notebook, wxID_ANY);
    auto* alertSizer = new wxBoxSizer(wxHORIZONTAL);

    auto* alertLeft = new wxPanel(alertPanel, wxID_ANY);
    auto* alertLeftSizer = new wxBoxSizer(wxVERTICAL);
    alertLeftSizer->Add(new wxStaticText(alertLeft, wxID_ANY, "Alerts"), 0, wxALL, 6);
    auto* alertList = new wxListBox(alertLeft, wxID_ANY);
    alertList->Append("High Order Volume");
    alertList->Append("Low Inventory");
    alertList->Append("Failed Payments");
    alertLeftSizer->Add(alertList, 1, wxEXPAND | wxALL, 6);
    alertLeft->SetSizer(alertLeftSizer);

    auto* alertCenter = new wxPanel(alertPanel, wxID_ANY);
    auto* alertCenterSizer = new wxBoxSizer(wxVERTICAL);
    alertCenterSizer->Add(new wxStaticText(alertCenter, wxID_ANY, "Rule Builder"), 0, wxALL, 6);
    auto* ruleEditor = new wxTextCtrl(alertCenter, wxID_ANY,
                                      "WHEN total_orders > 500\nTHEN notify #ops",
                                      wxDefaultPosition, wxDefaultSize, wxTE_MULTILINE);
    alertCenterSizer->Add(ruleEditor, 1, wxEXPAND | wxALL, 6);
    alertCenterSizer->Add(new wxButton(alertCenter, kAddAlertId, "Add Rule"), 0,
                          wxLEFT | wxRIGHT | wxBOTTOM, 6);
    alertCenter->SetSizer(alertCenterSizer);

    auto* alertRight = new wxPanel(alertPanel, wxID_ANY);
    auto* alertRightSizer = new wxBoxSizer(wxVERTICAL);
    alertRightSizer->Add(new wxStaticText(alertRight, wxID_ANY, "Schedule"), 0, wxALL, 6);
    alertRightSizer->Add(new wxTextCtrl(alertRight, wxID_ANY, "Every 15 minutes"), 0,
                         wxEXPAND | wxALL, 6);
    alertRightSizer->Add(new wxButton(alertRight, kScheduleAlertId, "Schedule Alert"), 0,
                         wxLEFT | wxRIGHT | wxBOTTOM, 6);
    alertRightSizer->AddStretchSpacer(1);
    alertRight->SetSizer(alertRightSizer);

    alertSizer->Add(alertLeft, 0, wxEXPAND | wxALL, 4);
    alertSizer->Add(alertCenter, 1, wxEXPAND | wxALL, 4);
    alertSizer->Add(alertRight, 0, wxEXPAND | wxALL, 4);
    alertPanel->SetSizer(alertSizer);
    notebook->AddPage(alertPanel, "Alerts & Rules", false);

    rootSizer->Add(notebook, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    SetSizer(rootSizer);
}

void ReportingFrame::PopulateConnections() {
    if (!connection_choice_) {
        return;
    }
    connection_choice_->Clear();
    if (!connections_ || connections_->empty()) {
        connection_choice_->Append("ScratchBird (default)");
        connection_choice_->SetSelection(0);
        return;
    }
    for (const auto& profile : *connections_) {
        connection_choice_->Append(ProfileLabel(profile));
    }
    connection_choice_->SetSelection(0);
}

const ConnectionProfile* ReportingFrame::GetSelectedProfile() const {
    if (!connections_ || !connection_choice_) {
        return nullptr;
    }
    int index = connection_choice_->GetSelection();
    if (index < 0 || static_cast<size_t>(index) >= connections_->size()) {
        return nullptr;
    }
    return &(*connections_)[static_cast<size_t>(index)];
}

bool ReportingFrame::EnsureConnected(const ConnectionProfile& profile) {
    if (!connection_manager_) {
        return false;
    }
    if (connection_manager_->IsConnected()) {
        return true;
    }
    return connection_manager_->Connect(profile);
}

void ReportingFrame::SetStatus(const wxString& message) {
    if (status_label_) {
        status_label_->SetLabel(message);
    }
}

void ReportingFrame::OnScheduleTick(wxTimerEvent&) {
    auto project = ProjectManager::Instance().GetCurrentProject();
    if (!project) {
        return;
    }
    size_t executed = project->ExecuteDueReportingSchedules();
    if (executed > 0) {
        SetStatus(wxString::Format("Executed %zu scheduled task(s)", executed));
    }
}

void ReportingFrame::OnRunQuery(wxCommandEvent&) {
    std::string sql;
    if (sql_editor_) {
        sql = Trim(sql_editor_->GetValue().ToStdString());
    }
    if (sql.empty()) {
        wxMessageBox("Enter a query to run.", "Reporting",
                     wxOK | wxICON_INFORMATION, this);
        return;
    }

    auto project = ProjectManager::Instance().GetCurrentProject();
    std::string cache_key;
    if (project) {
        if (const auto* profile = GetSelectedProfile()) {
            cache_key = NormalizeSqlForCache(sql) + "|" + ProfileLabel(*profile);
        } else if (connection_manager_ && connection_manager_->IsConnected()) {
            cache_key = NormalizeSqlForCache(sql) + "|active";
        } else {
            cache_key = NormalizeSqlForCache(sql);
        }
        if (!cache_key.empty()) {
            auto cached = project->GetReportingCache(cache_key);
            if (cached.has_value()) {
                SetStatus("Cache hit");
                project->RecordReportingAudit(Project::AuditEvent{
                    std::time(nullptr),
                    "designer",
                    "run_cache",
                    "report:query",
                    "",
                    true,
                    "Cache hit",
                    {{"panel", "query_builder"}}
                });
                wxMessageBox(wxString::Format("Cached result (%lld rows).",
                                              static_cast<long long>(cached->rows_returned)),
                             "Reporting", wxOK | wxICON_INFORMATION, this);
                return;
            }
        }
    }

    if (project) {
        Project::GovernanceContext context;
        context.action = "run";
        context.role = "designer";
        std::string reason;
        if (!project->CanExecuteReportingAction(context, &reason)) {
            project->RecordReportingAudit(Project::AuditEvent{
                std::time(nullptr),
                context.role,
                "run",
                "report:query",
                "",
                false,
                reason,
                {{"panel", "query_builder"}}
            });
            wxMessageBox("Execution blocked: " + reason, "Reporting",
                         wxOK | wxICON_WARNING, this);
            return;
        }
        project->RecordReportingAudit(Project::AuditEvent{
            std::time(nullptr),
            context.role,
            "run",
            "report:query",
            "",
            true,
            "Allowed",
            {{"panel", "query_builder"}}
        });
    }
    const ConnectionProfile* profile = GetSelectedProfile();
    if (profile) {
        if (NormalizeBackendName(profile->backend) != "native") {
            wxMessageBox("Reporting execution is supported for ScratchBird connections.",
                         "Reporting", wxOK | wxICON_WARNING, this);
            return;
        }
        if (!EnsureConnected(*profile)) {
            wxMessageBox(connection_manager_ ? connection_manager_->LastError() : "Connection failed.",
                         "Reporting", wxOK | wxICON_WARNING, this);
            return;
        }
    } else if (!connection_manager_ || !connection_manager_->IsConnected()) {
        wxMessageBox("No connection profile selected.", "Reporting",
                     wxOK | wxICON_WARNING, this);
        return;
    }
    SetStatus("Running...");
    connection_manager_->ExecuteQueryAsync(sql,
        [this, project, cache_key](bool ok, QueryResult result, const std::string& error) {
            CallAfter([this, project, cache_key, ok,
                       result = std::move(result), error]() mutable {
                if (!ok) {
                    SetStatus("Run failed");
                    wxMessageBox(error.empty() ? "Query execution failed." : error,
                                 "Reporting", wxOK | wxICON_ERROR, this);
                    return;
                }
                if (project && !cache_key.empty()) {
                    Project::ReportingCacheEntry entry;
                    entry.key = cache_key;
                    entry.payload_json = SerializeQueryResultToJson(result, 100);
                    entry.rows_returned = result.stats.rowsReturned > 0
                                              ? result.stats.rowsReturned
                                              : static_cast<int64_t>(result.rows.size());
                    entry.ttl_seconds = 15 * 60;
                    entry.source_id = "report:query";
                    project->StoreReportingCache(entry);
                }
                SetStatus("Completed");
                wxMessageBox(wxString::Format("Query executed (%lld rows).",
                                              static_cast<long long>(result.stats.rowsReturned)),
                             "Reporting", wxOK | wxICON_INFORMATION, this);
            });
        });
}

void ReportingFrame::OnSaveQuery(wxCommandEvent&) {
    auto project = ProjectManager::Instance().GetCurrentProject();
    if (project) {
        Project::GovernanceContext context;
        context.action = "save";
        context.role = "designer";
        std::string reason;
        if (!project->CanExecuteReportingAction(context, &reason)) {
            project->RecordReportingAudit(Project::AuditEvent{
                std::time(nullptr),
                context.role,
                "save",
                "report:query",
                "",
                false,
                reason,
                {{"panel", "query_builder"}}
            });
            wxMessageBox("Save blocked: " + reason, "Reporting",
                         wxOK | wxICON_WARNING, this);
            return;
        }
        project->RecordReportingAudit(Project::AuditEvent{
            std::time(nullptr),
            context.role,
            "save",
            "report:query",
            "",
            true,
            "Allowed",
            {{"panel", "query_builder"}}
        });
    }
    wxMessageBox("Save query stub.", "Reporting", wxOK | wxICON_INFORMATION, this);
}

void ReportingFrame::OnScheduleQuery(wxCommandEvent&) {
    auto project = ProjectManager::Instance().GetCurrentProject();
    if (project) {
        std::string schedule_spec = PromptScheduleSpec(this, "Schedule Query");
        if (schedule_spec.empty()) {
            return;
        }
        Project::GovernanceContext context;
        context.action = "schedule";
        context.role = "operator";
        std::string reason;
        if (!project->ScheduleReportingAction("schedule", "report:query", context, &reason)) {
            wxMessageBox("Scheduling blocked: " + reason, "Reporting",
                         wxOK | wxICON_WARNING, this);
            return;
        }
        project->AddReportingSchedule("report:query", "report:query", schedule_spec);
    }
    wxMessageBox("Query schedule recorded.", "Reporting",
                 wxOK | wxICON_INFORMATION, this);
}

void ReportingFrame::OnRefreshPreview(wxCommandEvent&) {
    auto project = ProjectManager::Instance().GetCurrentProject();
    if (project) {
        Project::GovernanceContext context;
        context.action = "refresh";
        context.role = "designer";
        std::string reason;
        if (!project->CanExecuteReportingAction(context, &reason)) {
            project->RecordReportingAudit(Project::AuditEvent{
                std::time(nullptr),
                context.role,
                "refresh",
                "report:preview",
                "",
                false,
                reason,
                {{"panel", "chart_preview"}}
            });
            wxMessageBox("Refresh blocked: " + reason, "Reporting",
                         wxOK | wxICON_WARNING, this);
            return;
        }
        project->RecordReportingAudit(Project::AuditEvent{
            std::time(nullptr),
            context.role,
            "refresh",
            "report:preview",
            "",
            true,
            "Allowed",
            {{"panel", "chart_preview"}}
        });
    }
    wxMessageBox("Preview refresh stub.", "Reporting", wxOK | wxICON_INFORMATION, this);
}

void ReportingFrame::OnAddAlert(wxCommandEvent&) {
    auto project = ProjectManager::Instance().GetCurrentProject();
    if (project) {
        Project::GovernanceContext context;
        context.action = "create";
        context.role = "designer";
        std::string reason;
        if (!project->CanExecuteReportingAction(context, &reason)) {
            project->RecordReportingAudit(Project::AuditEvent{
                std::time(nullptr),
                context.role,
                "create",
                "report:alert",
                "",
                false,
                reason,
                {{"panel", "alerts"}}
            });
            wxMessageBox("Alert creation blocked: " + reason, "Reporting",
                         wxOK | wxICON_WARNING, this);
            return;
        }
        project->RecordReportingAudit(Project::AuditEvent{
            std::time(nullptr),
            context.role,
            "create",
            "report:alert",
            "",
            true,
            "Allowed",
            {{"panel", "alerts"}}
        });
    }
    wxMessageBox("Add alert stub.", "Reporting", wxOK | wxICON_INFORMATION, this);
}

void ReportingFrame::OnScheduleAlert(wxCommandEvent&) {
    auto project = ProjectManager::Instance().GetCurrentProject();
    if (project) {
        std::string schedule_spec = PromptScheduleSpec(this, "Schedule Alert");
        if (schedule_spec.empty()) {
            return;
        }
        Project::GovernanceContext context;
        context.action = "schedule";
        context.role = "operator";
        std::string reason;
        if (!project->ScheduleReportingAction("schedule", "report:alert", context, &reason)) {
            wxMessageBox("Alert scheduling blocked: " + reason, "Reporting",
                         wxOK | wxICON_WARNING, this);
            return;
        }
        project->AddReportingSchedule("report:alert", "report:alert", schedule_spec);
    }
    wxMessageBox("Alert schedule recorded.", "Reporting",
                 wxOK | wxICON_INFORMATION, this);
}

} // namespace scratchrobin
