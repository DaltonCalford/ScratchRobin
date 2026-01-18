#ifndef SCRATCHROBIN_SQL_EDITOR_FRAME_H
#define SCRATCHROBIN_SQL_EDITOR_FRAME_H

#include <string>
#include <vector>
#include <chrono>

#include <wx/frame.h>

#include "core/config.h"
#include "core/connection_manager.h"

class wxChoice;
class wxGrid;
class wxCheckBox;
class wxButton;
class wxSpinCtrl;
class wxStaticText;
class wxTextCtrl;
class wxNotebook;

namespace scratchrobin {

class WindowManager;
class ResultGridTable;

class SqlEditorFrame : public wxFrame {
public:
    SqlEditorFrame(WindowManager* windowManager,
                   ConnectionManager* connectionManager,
                   const std::vector<ConnectionProfile>* connections,
                   const AppConfig* appConfig);

private:
    void OnExecuteQuery(wxCommandEvent& event);
    void OnCancelQuery(wxCommandEvent& event);
    void OnConnect(wxCommandEvent& event);
    void OnDisconnect(wxCommandEvent& event);
    void OnConnectionChanged(wxCommandEvent& event);
    void OnBegin(wxCommandEvent& event);
    void OnCommit(wxCommandEvent& event);
    void OnRollback(wxCommandEvent& event);
    void OnToggleAutoCommit(wxCommandEvent& event);
    void OnTogglePaging(wxCommandEvent& event);
    void OnPrevPage(wxCommandEvent& event);
    void OnNextPage(wxCommandEvent& event);
    void OnPageSizeChanged(wxCommandEvent& event);
    void OnToggleStream(wxCommandEvent& event);
    void OnExportCsv(wxCommandEvent& event);
    void OnExportJson(wxCommandEvent& event);
    void OnResultSelection(wxCommandEvent& event);
    void OnHistoryLoad(wxCommandEvent& event);
    void OnExplain(wxCommandEvent& event);
    void OnSblr(wxCommandEvent& event);
    void OnClose(wxCloseEvent& event);
    void PopulateConnections();
    const ConnectionProfile* GetSelectedProfile() const;
    bool EnsureConnected(const ConnectionProfile& profile);
    bool ExecuteStatements(const std::string& sql);
    bool ExecuteStatementBatch(const std::vector<std::string>& statements);
    bool ExecutePagedStatement(const std::string& statement, int page_index, bool stream_append);
    void ExecuteNextStatement();
    void StartAsyncQuery(const std::string& sql, bool is_last, bool is_paged, bool stream_append,
                         int result_index, const std::string& statement);
    void HandleQueryResult(bool ok, const QueryResult& result,
                           const std::string& error, bool is_last, bool is_paged, bool stream_append,
                           int result_index, const std::string& statement);
    bool IsPagedStatement(const std::string& statement) const;
    std::string BuildPagedQuery(const std::string& statement, int64_t offset, int64_t limit) const;
    void PopulateGrid(const QueryResult& result, bool append);
    void ResetGrid();
    void UpdateStatus(const std::string& message);
    void UpdateExportControls();
    void UpdateResultControls();
    void UpdateSessionControls();
    void UpdatePagingControls();
    void ClearMessages();
    void AppendMessages(const QueryResult& result, const std::string& error);
    void AppendMessageLine(const std::string& line);
    void AddToHistory(const std::string& sql);
    void UpdateHistoryControls();
    std::string GetExplainTargetSql() const;
    void StartSpecialQuery(const std::string& sql, const std::string& mode);
    void HandleSpecialResult(bool ok, const QueryResult& result, const std::string& error,
                             const std::string& mode);
    std::string ResultToText(const QueryResult& result) const;
    void UpdateResultChoiceSelection(int index);
    void ShowResultAtIndex(int index);

    WindowManager* window_manager_ = nullptr;
    ConnectionManager* connection_manager_ = nullptr;
    const std::vector<ConnectionProfile>* connections_ = nullptr;
    const AppConfig* app_config_ = nullptr;
    int active_profile_index_ = -1;

    wxChoice* connection_choice_ = nullptr;
    wxButton* connect_button_ = nullptr;
    wxButton* disconnect_button_ = nullptr;
    wxCheckBox* auto_commit_check_ = nullptr;
    wxButton* begin_button_ = nullptr;
    wxButton* commit_button_ = nullptr;
    wxButton* rollback_button_ = nullptr;
    wxButton* run_button_ = nullptr;
    wxButton* cancel_button_ = nullptr;
    wxCheckBox* paging_check_ = nullptr;
    wxCheckBox* stream_check_ = nullptr;
    wxButton* prev_page_button_ = nullptr;
    wxButton* next_page_button_ = nullptr;
    wxButton* export_csv_button_ = nullptr;
    wxButton* export_json_button_ = nullptr;
    wxButton* explain_button_ = nullptr;
    wxButton* sblr_button_ = nullptr;
    wxChoice* result_choice_ = nullptr;
    wxChoice* history_choice_ = nullptr;
    wxButton* history_load_button_ = nullptr;
    wxSpinCtrl* page_size_ctrl_ = nullptr;
    wxStaticText* page_label_ = nullptr;
    wxTextCtrl* editor_ = nullptr;
    wxGrid* result_grid_ = nullptr;
    ResultGridTable* result_table_ = nullptr;
    wxStaticText* status_label_ = nullptr;
    wxNotebook* result_notebook_ = nullptr;
    wxTextCtrl* message_log_ = nullptr;
    wxTextCtrl* plan_view_ = nullptr;
    wxTextCtrl* sblr_view_ = nullptr;
    int results_page_index_ = 0;
    int plan_page_index_ = 1;
    int sblr_page_index_ = 2;
    int messages_page_index_ = 3;

    std::string current_statement_;
    int current_page_ = 0;
    int page_size_ = 200;
    bool paging_active_ = false;
    bool query_running_ = false;
    bool stream_append_ = false;
    bool has_result_ = false;
    std::chrono::steady_clock::time_point batch_start_time_;
    std::chrono::steady_clock::time_point statement_start_time_;
    double last_statement_ms_ = 0.0;
    int64_t pending_rows_affected_ = 0;
    size_t pending_statement_index_ = 0;
    std::vector<std::string> pending_statements_;
    QueryResult pending_last_result_;
    QueryResult last_result_;
    std::string pending_last_tag_;
    JobHandle active_query_job_;

    struct ResultEntry {
        std::string statement;
        QueryResult result;
        double elapsedMs = 0.0;
        bool isPaged = false;
        bool ok = true;
        std::string error;
    };

    std::vector<ResultEntry> result_sets_;
    int active_result_index_ = -1;
    int paged_result_index_ = -1;
    std::vector<std::string> statement_history_;
    size_t history_max_items_ = 2000;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_SQL_EDITOR_FRAME_H
