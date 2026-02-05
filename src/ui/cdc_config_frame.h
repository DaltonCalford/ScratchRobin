/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_CDC_CONFIG_FRAME_H
#define SCRATCHROBIN_CDC_CONFIG_FRAME_H

#include <memory>
#include <vector>

#include <wx/frame.h>
#include <wx/timer.h>

#include "core/cdc_streaming.h"

class wxNotebook;
class wxListCtrl;
class wxListEvent;
class wxTextCtrl;
class wxChoice;
class wxCheckBox;
class wxSpinCtrl;
class wxButton;
class wxStaticText;

namespace scratchrobin {

class WindowManager;
class CdcPipeline;

/**
 * @brief CDC Pipeline Configuration Frame
 * 
 * Provides UI for configuring and monitoring CDC pipelines including:
 * - Source database configuration
 * - Target message broker setup
 * - Table/column filtering
 * - Error handling and retry configuration
 * - Pipeline monitoring and metrics
 */
class CdcConfigFrame : public wxFrame {
public:
    CdcConfigFrame(WindowManager* windowManager,
                   wxWindow* parent = nullptr);
    
    ~CdcConfigFrame();

private:
    void BuildMenu();
    void BuildLayout();
    void BuildPipelinesPanel();
    void BuildSourcePanel();
    void BuildTargetPanel();
    void BuildFilterPanel();
    void BuildRetryPanel();
    void BuildMonitoringPanel();
    
    // Event handlers
    void OnClose(wxCloseEvent& event);
    void OnCreatePipeline(wxCommandEvent& event);
    void OnEditPipeline(wxCommandEvent& event);
    void OnDeletePipeline(wxCommandEvent& event);
    void OnStartPipeline(wxCommandEvent& event);
    void OnStopPipeline(wxCommandEvent& event);
    void OnPipelineSelected(wxListEvent& event);
    void OnRefresh(wxCommandEvent& event);
    void OnSaveConfig(wxCommandEvent& event);
    void OnTestConnection(wxCommandEvent& event);
    void OnRetryFailedEvents(wxCommandEvent& event);
    void OnClearFailedEvents(wxCommandEvent& event);
    void OnTimer(wxTimerEvent& event);
    
    // Helper methods
    void LoadPipelineList();
    void LoadPipelineDetails(const std::string& pipeline_id);
    void UpdateMetrics();
    bool ValidateConfiguration();
    CdcPipeline::Configuration GatherConfiguration();
    
    WindowManager* window_manager_ = nullptr;
    
    // Currently selected pipeline
    std::string selected_pipeline_id_;
    
    // Auto-refresh timer
    wxTimer refresh_timer_;
    
    // UI Elements - Pipelines Panel
    wxListCtrl* list_pipelines_ = nullptr;
    wxButton* btn_create_ = nullptr;
    wxButton* btn_edit_ = nullptr;
    wxButton* btn_delete_ = nullptr;
    wxButton* btn_start_ = nullptr;
    wxButton* btn_stop_ = nullptr;
    
    // UI Elements - Source Panel
    wxChoice* choice_source_type_ = nullptr;
    wxTextCtrl* txt_connection_string_ = nullptr;
    wxTextCtrl* txt_tables_ = nullptr;
    wxTextCtrl* txt_schemas_ = nullptr;
    wxCheckBox* chk_snapshot_ = nullptr;
    wxSpinCtrl* spin_poll_interval_ = nullptr;
    
    // UI Elements - Target Panel
    wxChoice* choice_broker_type_ = nullptr;
    wxTextCtrl* txt_broker_connection_ = nullptr;
    wxTextCtrl* txt_target_topic_ = nullptr;
    wxChoice* choice_message_format_ = nullptr;
    wxButton* btn_test_connection_ = nullptr;
    
    // UI Elements - Filter Panel
    wxCheckBox* chk_capture_insert_ = nullptr;
    wxCheckBox* chk_capture_update_ = nullptr;
    wxCheckBox* chk_capture_delete_ = nullptr;
    wxTextCtrl* txt_column_filter_ = nullptr;
    wxTextCtrl* txt_row_filter_ = nullptr;
    
    // UI Elements - Retry Panel
    wxSpinCtrl* spin_max_retries_ = nullptr;
    wxSpinCtrl* spin_retry_delay_ = nullptr;
    wxCheckBox* chk_exponential_backoff_ = nullptr;
    wxSpinCtrl* spin_max_backoff_ = nullptr;
    wxCheckBox* chk_enable_dlq_ = nullptr;
    wxTextCtrl* txt_dlq_topic_ = nullptr;
    wxButton* btn_retry_failed_ = nullptr;
    wxButton* btn_clear_failed_ = nullptr;
    
    // UI Elements - Monitoring Panel
    wxStaticText* lbl_status_ = nullptr;
    wxStaticText* lbl_events_processed_ = nullptr;
    wxStaticText* lbl_events_failed_ = nullptr;
    wxStaticText* lbl_events_filtered_ = nullptr;
    wxStaticText* lbl_processing_rate_ = nullptr;
    wxStaticText* lbl_latency_ = nullptr;
    wxListCtrl* list_failed_events_ = nullptr;
    
    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_CDC_CONFIG_FRAME_H
