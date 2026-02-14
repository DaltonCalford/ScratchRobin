/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
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
class wxStaticBox;
class wxTimer;
class wxScrolledWindow;
class wxListBox;
class wxBoxSizer;
class wxTimerEvent;

namespace scratchrobin {

class WindowManager;
class ResultGridTable;
class MetadataModel;

class SqlEditorFrame : public wxFrame {
public:
    SqlEditorFrame(WindowManager* windowManager,
                   ConnectionManager* connectionManager,
                   const std::vector<ConnectionProfile>* connections,
                   const AppConfig* appConfig,
                   MetadataModel* metadataModel);
    void LoadStatement(const std::string& sql);

private:
    void OnExecuteQuery(wxCommandEvent& event);
    void OnCancelQuery(wxCommandEvent& event);
    void OnCopy(wxCommandEvent& event);
    void OnNewSqlEditor(wxCommandEvent& event);
    void OnNewDiagram(wxCommandEvent& event);
    void OnOpenMonitoring(wxCommandEvent& event);
    void OnOpenUsersRoles(wxCommandEvent& event);
    void OnOpenJobScheduler(wxCommandEvent& event);
    void OnOpenDomainManager(wxCommandEvent& event);
    void OnOpenSchemaManager(wxCommandEvent& event);
    void OnOpenTableDesigner(wxCommandEvent& event);
    void OnOpenIndexDesigner(wxCommandEvent& event);
    void OnConnect(wxCommandEvent& event);
    void OnDisconnect(wxCommandEvent& event);
    void OnConnectionChanged(wxCommandEvent& event);
    void OnBegin(wxCommandEvent& event);
    void OnCommit(wxCommandEvent& event);
    void OnRollback(wxCommandEvent& event);
    void OnSavepoint(wxCommandEvent& event);
    void OnRollbackToSavepoint(wxCommandEvent& event);
    void OnReleaseSavepoint(wxCommandEvent& event);
    void OnToggleAutoCommit(wxCommandEvent& event);
    void OnIsolationLevelChanged(wxCommandEvent& event);
    void UpdateTransactionUI();
    bool ConfirmCloseWithTransaction();
    void OnTogglePaging(wxCommandEvent& event);
    void OnPrevPage(wxCommandEvent& event);
    void OnNextPage(wxCommandEvent& event);
    void OnPageSizeChanged(wxCommandEvent& event);
    void OnToggleStream(wxCommandEvent& event);
    void OnRowLimitChanged(wxCommandEvent& event);
    void OnExportCsv(wxCommandEvent& event);
    void OnExportJson(wxCommandEvent& event);
    void OnApplyStreamSettings(wxCommandEvent& event);
    void OnResultSelection(wxCommandEvent& event);
    void OnHistoryLoad(wxCommandEvent& event);
    void OnExplain(wxCommandEvent& event);
    void OnSblr(wxCommandEvent& event);
    void OnEditPreparedParams(wxCommandEvent& event);
    void OnPrepareStatement(wxCommandEvent& event);
    void OnExecutePrepared(wxCommandEvent& event);
    void OnSubscribeNotifications(wxCommandEvent& event);
    void OnUnsubscribeNotifications(wxCommandEvent& event);
    void OnFetchNotification(wxCommandEvent& event);
    void OnToggleNotificationPolling(wxCommandEvent& event);
    void OnNotificationTimer(wxTimerEvent& event);
    void OnFetchStatus(wxCommandEvent& event);
    void OnClearStatus(wxCommandEvent& event);
    void OnToggleStatusPolling(wxCommandEvent& event);
    void OnStatusTimer(wxTimerEvent& event);
    void OnStatusHistorySelection(wxCommandEvent& event);
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
    bool IsDdlStatement(const std::string& statement) const;
    std::string BuildPagedQuery(const std::string& statement, int64_t offset, int64_t limit) const;
    void ApplyRowLimit(QueryResult* result, bool* limit_hit);
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
    void AppendNotificationLine(const std::string& line);
    void DisplayStatusSnapshot(const StatusSnapshot& snapshot);
    void SetStatusMessage(const std::string& message);
    void ApplyStatusDefaults(const ConnectionProfile* profile, bool restartTimer);
    std::string BuildStatusJson(const StatusSnapshot& snapshot,
                                const std::string& category,
                                bool diff_only) const;
    void ClearStatusCards();
    void BuildStatusCards(const StatusSnapshot& snapshot);
    std::string SelectedStatusCategory() const;
    void UpdateStatusCategoryChoices(const StatusSnapshot& snapshot);
    void AddStatusHistory(const StatusSnapshot& snapshot);
    void RefreshStatusHistory();
    void ShowHistorySnapshot(size_t index);
    void PersistStatusPreferences();
    void UpdateDiffOptionControls();
    std::string FormatNotificationPayload(const NotificationEvent& ev) const;
    bool ShouldDisplayNotification(const NotificationEvent& ev) const;
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
    MetadataModel* metadata_model_ = nullptr;
    int active_profile_index_ = -1;

    wxChoice* connection_choice_ = nullptr;
    wxButton* connect_button_ = nullptr;
    wxButton* disconnect_button_ = nullptr;
    wxCheckBox* auto_commit_check_ = nullptr;
    wxButton* begin_button_ = nullptr;
    wxButton* commit_button_ = nullptr;
    wxButton* rollback_button_ = nullptr;
    wxButton* savepoint_button_ = nullptr;
    wxChoice* savepoint_choice_ = nullptr;
    std::vector<std::string> savepoints_;
    wxButton* run_button_ = nullptr;
    wxButton* cancel_button_ = nullptr;
    wxButton* copy_button_ = nullptr;
    wxCheckBox* paging_check_ = nullptr;
    wxCheckBox* stream_check_ = nullptr;
    wxSpinCtrl* stream_window_ctrl_ = nullptr;
    wxSpinCtrl* stream_chunk_ctrl_ = nullptr;
    wxButton* stream_apply_button_ = nullptr;
    wxStaticText* stream_metrics_label_ = nullptr;
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
    wxSpinCtrl* row_limit_ctrl_ = nullptr;
    wxStaticText* page_label_ = nullptr;
    wxTextCtrl* editor_ = nullptr;
    wxGrid* result_grid_ = nullptr;
    ResultGridTable* result_table_ = nullptr;
    wxStaticText* status_label_ = nullptr;
    wxStaticText* progress_label_ = nullptr;
    wxStaticText* transaction_indicator_ = nullptr;
    wxChoice* isolation_choice_ = nullptr;
    wxNotebook* result_notebook_ = nullptr;
    wxTextCtrl* message_log_ = nullptr;
    wxTextCtrl* plan_view_ = nullptr;
    wxTextCtrl* sblr_view_ = nullptr;
    wxTextCtrl* notifications_log_ = nullptr;
    wxTextCtrl* notification_channel_ctrl_ = nullptr;
    wxTextCtrl* notification_filter_ctrl_ = nullptr;
    wxButton* notification_subscribe_button_ = nullptr;
    wxButton* notification_unsubscribe_button_ = nullptr;
    wxButton* notification_fetch_button_ = nullptr;
    wxCheckBox* notification_poll_check_ = nullptr;
    wxSpinCtrl* notification_poll_interval_ctrl_ = nullptr;
    wxTimer* notification_timer_ = nullptr;
    bool notification_fetch_pending_ = false;
    wxButton* notification_clear_button_ = nullptr;
    wxCheckBox* notification_autoscroll_check_ = nullptr;
    wxStaticText* stream_status_label_ = nullptr;
    wxStaticText* prepared_status_label_ = nullptr;
    wxButton* prepared_edit_button_ = nullptr;
    wxButton* prepared_prepare_button_ = nullptr;
    wxButton* prepared_execute_button_ = nullptr;
    wxChoice* status_type_choice_ = nullptr;
    wxButton* status_fetch_button_ = nullptr;
    wxButton* status_clear_button_ = nullptr;
    wxCheckBox* status_poll_check_ = nullptr;
    wxSpinCtrl* status_poll_interval_ctrl_ = nullptr;
    wxStaticText* status_message_label_ = nullptr;
    wxScrolledWindow* status_cards_panel_ = nullptr;
    wxBoxSizer* status_cards_sizer_ = nullptr;
    wxButton* status_copy_button_ = nullptr;
    wxButton* status_save_button_ = nullptr;
    wxChoice* status_category_choice_ = nullptr;
    wxCheckBox* status_diff_check_ = nullptr;
    wxCheckBox* status_diff_ignore_unchanged_check_ = nullptr;
    wxCheckBox* status_diff_ignore_empty_check_ = nullptr;
    wxListBox* status_history_list_ = nullptr;
    wxTimer* status_timer_ = nullptr;
    bool status_fetch_pending_ = false;
    StatusSnapshot last_status_;
    StatusSnapshot previous_status_;
    bool has_status_ = false;
    std::vector<std::string> status_category_order_;
    std::string status_category_preference_;
    size_t status_history_limit_ = 50;
    struct StatusHistoryEntry {
        std::string label;
        StatusSnapshot snapshot;
    };
    std::vector<StatusHistoryEntry> status_history_;

    PreparedStatementHandlePtr active_prepared_;
    std::vector<PreparedParameter> prepared_params_;
    int results_page_index_ = 0;
    int plan_page_index_ = 1;
    int sblr_page_index_ = 2;
    int messages_page_index_ = 3;

    std::string current_statement_;
    int current_page_ = 0;
    int page_size_ = 200;
    int row_limit_ = 200;
    bool row_limit_hit_ = false;
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
    size_t pending_query_length_ = 0;
    JobHandle active_query_job_;
    bool pending_metadata_refresh_ = false;
    
    // Transaction tracking
    std::chrono::steady_clock::time_point transaction_start_time_;
    int transaction_statement_count_ = 0;
    bool transaction_failed_ = false;

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
