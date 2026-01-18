#ifndef SCRATCHROBIN_SQL_EDITOR_FRAME_H
#define SCRATCHROBIN_SQL_EDITOR_FRAME_H

#include <string>
#include <vector>
#include <chrono>

#include <wx/frame.h>

#include "core/connection_manager.h"

class wxChoice;
class wxGrid;
class wxCheckBox;
class wxButton;
class wxSpinCtrl;
class wxStaticText;
class wxTextCtrl;

namespace scratchrobin {

class WindowManager;
class ResultGridTable;

class SqlEditorFrame : public wxFrame {
public:
    SqlEditorFrame(WindowManager* windowManager,
                   ConnectionManager* connectionManager,
                   const std::vector<ConnectionProfile>* connections);

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
    void OnClose(wxCloseEvent& event);
    void PopulateConnections();
    const ConnectionProfile* GetSelectedProfile() const;
    bool EnsureConnected(const ConnectionProfile& profile);
    bool ExecuteStatements(const std::string& sql);
    bool ExecuteStatementBatch(const std::vector<std::string>& statements);
    bool ExecutePagedStatement(const std::string& statement, int page_index, bool stream_append);
    void ExecuteNextStatement();
    void StartAsyncQuery(const std::string& sql, bool is_last, bool is_paged, bool stream_append);
    void HandleQueryResult(bool ok, const QueryResult& result,
                           const std::string& error, bool is_last, bool is_paged, bool stream_append);
    bool IsPagedStatement(const std::string& statement) const;
    std::string BuildPagedQuery(const std::string& statement, int64_t offset, int64_t limit) const;
    void PopulateGrid(const QueryResult& result, bool append);
    void ResetGrid();
    void UpdateStatus(const std::string& message);
    void UpdateExportControls();
    void UpdateSessionControls();
    void UpdatePagingControls();
    void ClearMessages();
    void AppendMessages(const QueryResult& result, const std::string& error);
    void AppendMessageLine(const std::string& line);

    WindowManager* window_manager_ = nullptr;
    ConnectionManager* connection_manager_ = nullptr;
    const std::vector<ConnectionProfile>* connections_ = nullptr;
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
    wxSpinCtrl* page_size_ctrl_ = nullptr;
    wxStaticText* page_label_ = nullptr;
    wxTextCtrl* editor_ = nullptr;
    wxGrid* result_grid_ = nullptr;
    ResultGridTable* result_table_ = nullptr;
    wxStaticText* status_label_ = nullptr;
    wxTextCtrl* message_log_ = nullptr;

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
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_SQL_EDITOR_FRAME_H
