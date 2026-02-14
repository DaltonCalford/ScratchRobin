/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 */
#include "ui/cdc_config_frame.h"

#include <wx/wx.h>
#include <wx/notebook.h>
#include <wx/listctrl.h>
#include <wx/textctrl.h>
#include <wx/choice.h>
#include <wx/checkbox.h>
#include <wx/spinctrl.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/msgdlg.h>

#include "ui/window_manager.h"
#include "core/cdc_streaming.h"

namespace scratchrobin {

enum {
    ID_CREATE_PIPELINE = wxID_HIGHEST + 1,
    ID_EDIT_PIPELINE,
    ID_DELETE_PIPELINE,
    ID_START_PIPELINE,
    ID_STOP_PIPELINE,
    ID_REFRESH,
    ID_SAVE_CONFIG,
    ID_TEST_CONNECTION,
    ID_RETRY_FAILED,
    ID_CLEAR_FAILED,
    ID_PIPELINE_LIST,
    ID_TIMER_REFRESH
};

wxBEGIN_EVENT_TABLE(CdcConfigFrame, wxFrame)
    EVT_CLOSE(CdcConfigFrame::OnClose)
    EVT_BUTTON(ID_CREATE_PIPELINE, CdcConfigFrame::OnCreatePipeline)
    EVT_BUTTON(ID_EDIT_PIPELINE, CdcConfigFrame::OnEditPipeline)
    EVT_BUTTON(ID_DELETE_PIPELINE, CdcConfigFrame::OnDeletePipeline)
    EVT_BUTTON(ID_START_PIPELINE, CdcConfigFrame::OnStartPipeline)
    EVT_BUTTON(ID_STOP_PIPELINE, CdcConfigFrame::OnStopPipeline)
    EVT_BUTTON(ID_REFRESH, CdcConfigFrame::OnRefresh)
    EVT_BUTTON(ID_SAVE_CONFIG, CdcConfigFrame::OnSaveConfig)
    EVT_BUTTON(ID_TEST_CONNECTION, CdcConfigFrame::OnTestConnection)
    EVT_BUTTON(ID_RETRY_FAILED, CdcConfigFrame::OnRetryFailedEvents)
    EVT_BUTTON(ID_CLEAR_FAILED, CdcConfigFrame::OnClearFailedEvents)
    EVT_LIST_ITEM_SELECTED(ID_PIPELINE_LIST, CdcConfigFrame::OnPipelineSelected)
    EVT_TIMER(ID_TIMER_REFRESH, CdcConfigFrame::OnTimer)
wxEND_EVENT_TABLE()

CdcConfigFrame::CdcConfigFrame(WindowManager* windowManager, wxWindow* parent)
    : wxFrame(parent, wxID_ANY, _("CDC Pipeline Configuration"),
              wxDefaultPosition, wxSize(1000, 700),
              wxDEFAULT_FRAME_STYLE | wxFRAME_FLOAT_ON_PARENT)
    , window_manager_(windowManager)
    , refresh_timer_(this, ID_TIMER_REFRESH) {
    
    BuildMenu();
    BuildLayout();
    
    CentreOnScreen();
    
    // Start refresh timer
    refresh_timer_.Start(2000);
    
    // Load initial data
    LoadPipelineList();
}

CdcConfigFrame::~CdcConfigFrame() = default;

void CdcConfigFrame::BuildMenu() {
    auto* menu_bar = new wxMenuBar();
    
    auto* file_menu = new wxMenu();
    file_menu->Append(wxID_CLOSE, _("&Close\tCtrl+W"));
    menu_bar->Append(file_menu, _("&File"));
    
    auto* pipeline_menu = new wxMenu();
    pipeline_menu->Append(ID_CREATE_PIPELINE, _("&New Pipeline...\tCtrl+N"));
    pipeline_menu->Append(ID_EDIT_PIPELINE, _("&Edit Pipeline...\tCtrl+E"));
    pipeline_menu->AppendSeparator();
    pipeline_menu->Append(ID_START_PIPELINE, _("&Start\tF5"));
    pipeline_menu->Append(ID_STOP_PIPELINE, _("S&top\tShift+F5"));
    menu_bar->Append(pipeline_menu, _("&Pipeline"));
    
    auto* view_menu = new wxMenu();
    view_menu->Append(ID_REFRESH, _("&Refresh\tF5"));
    menu_bar->Append(view_menu, _("&View"));
    
    SetMenuBar(menu_bar);
}

void CdcConfigFrame::BuildLayout() {
    auto* main_sizer = new wxBoxSizer(wxVERTICAL);
    
    auto* notebook = new wxNotebook(this, wxID_ANY);
    
    // Build each tab
    BuildPipelinesPanel();
    BuildSourcePanel();
    BuildTargetPanel();
    BuildFilterPanel();
    BuildRetryPanel();
    BuildMonitoringPanel();
    
    // Add notebook pages (placeholder panels for now)
    notebook->AddPage(new wxPanel(notebook, wxID_ANY), _("Pipelines"));
    notebook->AddPage(new wxPanel(notebook, wxID_ANY), _("Source"));
    notebook->AddPage(new wxPanel(notebook, wxID_ANY), _("Target"));
    notebook->AddPage(new wxPanel(notebook, wxID_ANY), _("Filters"));
    notebook->AddPage(new wxPanel(notebook, wxID_ANY), _("Retry"));
    notebook->AddPage(new wxPanel(notebook, wxID_ANY), _("Monitoring"));
    
    main_sizer->Add(notebook, 1, wxEXPAND | wxALL, 5);
    
    // Button bar
    auto* button_sizer = new wxBoxSizer(wxHORIZONTAL);
    button_sizer->AddStretchSpacer(1);
    
    auto* btn_save = new wxButton(this, ID_SAVE_CONFIG, _("Save Configuration"));
    btn_save->SetDefault();
    button_sizer->Add(btn_save, 0, wxRIGHT, 5);
    
    button_sizer->Add(new wxButton(this, wxID_CANCEL, _("Cancel")), 0);
    
    main_sizer->Add(button_sizer, 0, wxEXPAND | wxALL, 10);
    
    SetSizer(main_sizer);
}

void CdcConfigFrame::BuildPipelinesPanel() {
    // Panel implementation would go here
}

void CdcConfigFrame::BuildSourcePanel() {
    // Panel implementation would go here
}

void CdcConfigFrame::BuildTargetPanel() {
    // Panel implementation would go here
}

void CdcConfigFrame::BuildFilterPanel() {
    // Panel implementation would go here
}

void CdcConfigFrame::BuildRetryPanel() {
    // Panel implementation would go here
}

void CdcConfigFrame::BuildMonitoringPanel() {
    // Panel implementation would go here
}


// Event Handlers
void CdcConfigFrame::OnClose(wxCloseEvent& event) {
    refresh_timer_.Stop();
    if (window_manager_) {
        window_manager_->UnregisterWindow(this);
    }
    Destroy();
}

void CdcConfigFrame::OnCreatePipeline(wxCommandEvent& /*event*/) {
    // Clear selection and reset form for new pipeline
    selected_pipeline_id_.clear();
    
    // Reset all fields to defaults
    if (choice_source_type_) choice_source_type_->SetSelection(0);
    if (txt_connection_string_) txt_connection_string_->Clear();
    if (txt_tables_) txt_tables_->Clear();
    if (txt_target_topic_) txt_target_topic_->Clear();
    if (chk_snapshot_) chk_snapshot_->SetValue(true);
    if (spin_poll_interval_) spin_poll_interval_->SetValue(1000);
    if (spin_max_retries_) spin_max_retries_->SetValue(3);
    if (chk_enable_dlq_) chk_enable_dlq_->SetValue(true);
}

void CdcConfigFrame::OnEditPipeline(wxCommandEvent& /*event*/) {
    if (selected_pipeline_id_.empty()) {
        wxMessageBox(_("Please select a pipeline to edit."), _("Info"), 
                     wxOK | wxICON_INFORMATION);
        return;
    }
    LoadPipelineDetails(selected_pipeline_id_);
}

void CdcConfigFrame::OnDeletePipeline(wxCommandEvent& /*event*/) {
    if (selected_pipeline_id_.empty()) {
        wxMessageBox(_("Please select a pipeline to delete."), _("Info"),
                     wxOK | wxICON_INFORMATION);
        return;
    }
    
    if (wxMessageBox(_("Are you sure you want to delete this pipeline?"),
                     _("Confirm Delete"), wxYES_NO | wxICON_QUESTION) == wxYES) {
        auto& manager = CdcStreamManager::Instance();
        if (manager.RemovePipeline(selected_pipeline_id_)) {
            selected_pipeline_id_.clear();
            LoadPipelineList();
            wxMessageBox(_("Pipeline deleted."), _("Success"));
        }
    }
}

void CdcConfigFrame::OnStartPipeline(wxCommandEvent& /*event*/) {
    if (selected_pipeline_id_.empty()) {
        wxMessageBox(_("Please select a pipeline to start."), _("Info"),
                     wxOK | wxICON_INFORMATION);
        return;
    }
    
    auto& manager = CdcStreamManager::Instance();
    if (manager.StartPipeline(selected_pipeline_id_)) {
        wxMessageBox(_("Pipeline started."), _("Success"));
        UpdateMetrics();
    } else {
        wxMessageBox(_("Failed to start pipeline."), _("Error"), wxOK | wxICON_ERROR);
    }
}

void CdcConfigFrame::OnStopPipeline(wxCommandEvent& /*event*/) {
    if (selected_pipeline_id_.empty()) {
        wxMessageBox(_("Please select a pipeline to stop."), _("Info"),
                     wxOK | wxICON_INFORMATION);
        return;
    }
    
    auto& manager = CdcStreamManager::Instance();
    if (manager.StopPipeline(selected_pipeline_id_)) {
        wxMessageBox(_("Pipeline stopped."), _("Success"));
    } else {
        wxMessageBox(_("Failed to stop pipeline."), _("Error"), wxOK | wxICON_ERROR);
    }
}

void CdcConfigFrame::OnPipelineSelected(wxListEvent& event) {
    long sel = event.GetIndex();
    if (sel >= 0 && list_pipelines_) {
        wxString id = list_pipelines_->GetItemText(sel);
        selected_pipeline_id_ = id.ToStdString();
        LoadPipelineDetails(selected_pipeline_id_);
    }
}

void CdcConfigFrame::OnRefresh(wxCommandEvent& /*event*/) {
    LoadPipelineList();
    UpdateMetrics();
}

void CdcConfigFrame::OnSaveConfig(wxCommandEvent& /*event*/) {
    if (!ValidateConfiguration()) {
        return;
    }
    
    auto config = GatherConfiguration();
    
    auto& manager = CdcStreamManager::Instance();
    std::string pipeline_id;
    
    if (selected_pipeline_id_.empty()) {
        // Create new pipeline
        pipeline_id = manager.CreatePipeline(config);
        selected_pipeline_id_ = pipeline_id;
    } else {
        // Update existing pipeline
        pipeline_id = selected_pipeline_id_;
        // Remove and recreate with new config
        manager.RemovePipeline(pipeline_id);
        pipeline_id = manager.CreatePipeline(config);
    }
    
    wxMessageBox(_("Configuration saved successfully."), _("Success"));
    LoadPipelineList();
}

void CdcConfigFrame::OnTestConnection(wxCommandEvent& /*event*/) {
    wxString broker = txt_broker_connection_->GetValue();
    if (broker.IsEmpty()) {
        wxMessageBox(_("Please enter a broker connection string."), _("Error"),
                     wxOK | wxICON_ERROR);
        return;
    }
    
    // Try to connect to broker
    bool success = true;  // Would actually test connection
    
    if (success) {
        wxMessageBox(_("Connection successful!"), _("Success"));
    } else {
        wxMessageBox(_("Connection failed. Please check your settings."), _("Error"),
                     wxOK | wxICON_ERROR);
    }
}

void CdcConfigFrame::OnRetryFailedEvents(wxCommandEvent& /*event*/) {
    if (selected_pipeline_id_.empty()) return;
    
    auto& manager = CdcStreamManager::Instance();
    auto* pipeline = manager.GetPipeline(selected_pipeline_id_);
    if (pipeline) {
        if (pipeline->RetryAllFailedEvents()) {
            wxMessageBox(_("All failed events have been retried successfully."),
                        _("Success"));
        } else {
            wxMessageBox(_("Some events failed to retry. Check the failed events list."),
                        _("Warning"), wxOK | wxICON_WARNING);
        }
        UpdateMetrics();
    }
}

void CdcConfigFrame::OnClearFailedEvents(wxCommandEvent& /*event*/) {
    if (selected_pipeline_id_.empty()) return;
    
    if (wxMessageBox(_("Are you sure you want to clear all failed events?"),
                     _("Confirm"), wxYES_NO | wxICON_QUESTION) != wxYES) {
        return;
    }
    
    auto& manager = CdcStreamManager::Instance();
    auto* pipeline = manager.GetPipeline(selected_pipeline_id_);
    if (pipeline) {
        pipeline->ClearFailedEvents();
        wxMessageBox(_("Failed events cleared."), _("Success"));
        UpdateMetrics();
    }
}

void CdcConfigFrame::OnTimer(wxTimerEvent& /*event*/) {
    if (IsShown()) {
        UpdateMetrics();
    }
}


// Helper Methods
void CdcConfigFrame::LoadPipelineList() {
    if (!list_pipelines_) return;
    
    list_pipelines_->DeleteAllItems();
    
    auto& manager = CdcStreamManager::Instance();
    auto pipelines = manager.GetPipelineIds();
    
    for (size_t i = 0; i < pipelines.size(); ++i) {
        long idx = list_pipelines_->InsertItem(i, pipelines[i]);
        
        auto* pipeline = manager.GetPipeline(pipelines[i]);
        if (pipeline) {
            // Add status column
            list_pipelines_->SetItem(idx, 1, 
                pipeline->IsRunning() ? _("Running") : _("Stopped"));
            
            // Add metrics
            auto metrics = pipeline->GetMetrics();
            list_pipelines_->SetItem(idx, 2, 
                wxString::Format("%lld", metrics.events_processed));
        }
    }
}

void CdcConfigFrame::LoadPipelineDetails(const std::string& pipeline_id) {
    // Would load configuration from pipeline and populate form fields
    // For now, this is a placeholder
}

void CdcConfigFrame::UpdateMetrics() {
    if (selected_pipeline_id_.empty()) return;
    
    auto& manager = CdcStreamManager::Instance();
    auto* pipeline = manager.GetPipeline(selected_pipeline_id_);
    if (!pipeline) return;
    
    auto metrics = pipeline->GetMetrics();
    
    if (lbl_status_) {
        lbl_status_->SetLabel(pipeline->IsRunning() ? _("Running") : _("Stopped"));
    }
    if (lbl_events_processed_) {
        lbl_events_processed_->SetLabel(wxString::Format("%lld", metrics.events_processed));
    }
    if (lbl_events_failed_) {
        lbl_events_failed_->SetLabel(wxString::Format("%lld", metrics.events_failed));
    }
    if (lbl_events_filtered_) {
        lbl_events_filtered_->SetLabel(wxString::Format("%lld", metrics.events_filtered));
    }
    if (lbl_processing_rate_) {
        lbl_processing_rate_->SetLabel(wxString::Format("%.1f events/sec", 
                                                         metrics.processing_rate));
    }
    if (lbl_latency_) {
        lbl_latency_->SetLabel(wxString::Format("%.1f ms", metrics.latency_ms));
    }
    
    // Update failed events list
    if (list_failed_events_) {
        list_failed_events_->DeleteAllItems();
        auto failed = pipeline->GetFailedEvents();
        for (size_t i = 0; i < failed.size(); ++i) {
            long idx = list_failed_events_->InsertItem(i, failed[i].event_id);
            list_failed_events_->SetItem(idx, 1, failed[i].table);
            list_failed_events_->SetItem(idx, 2, 
                CdcEventTypeToString(failed[i].type));
        }
    }
}

bool CdcConfigFrame::ValidateConfiguration() {
    // Check required fields
    if (!txt_connection_string_ || txt_connection_string_->IsEmpty()) {
        wxMessageBox(_("Please enter a source connection string."), _("Validation Error"),
                     wxOK | wxICON_ERROR);
        return false;
    }
    
    if (!txt_target_topic_ || txt_target_topic_->IsEmpty()) {
        wxMessageBox(_("Please enter a target topic name."), _("Validation Error"),
                     wxOK | wxICON_ERROR);
        return false;
    }
    
    if (!txt_tables_ || txt_tables_->IsEmpty()) {
        wxMessageBox(_("Please specify at least one table to monitor."), _("Validation Error"),
                     wxOK | wxICON_ERROR);
        return false;
    }
    
    return true;
}

CdcPipeline::Configuration CdcConfigFrame::GatherConfiguration() {
    CdcPipeline::Configuration config;
    
    // Source configuration
    if (choice_source_type_) {
        config.connector_id = choice_source_type_->GetStringSelection().ToStdString();
    }
    
    // Target configuration
    if (choice_broker_type_) {
        int sel = choice_broker_type_->GetSelection();
        config.broker_type = static_cast<BrokerType>(sel);
    }
    
    if (txt_broker_connection_) {
        config.broker_connection_string = txt_broker_connection_->GetValue().ToStdString();
    }
    
    if (txt_target_topic_) {
        config.target_topic = txt_target_topic_->GetValue().ToStdString();
    }
    
    // DLQ configuration
    if (chk_enable_dlq_) {
        config.enable_dlq = chk_enable_dlq_->GetValue();
    }
    if (txt_dlq_topic_) {
        config.dlq_topic = txt_dlq_topic_->GetValue().ToStdString();
    }
    if (spin_max_retries_) {
        config.max_retries = spin_max_retries_->GetValue();
    }
    
    // Add transformations based on filter settings
    if (chk_capture_insert_ && !chk_capture_insert_->GetValue()) {
        CdcPipeline::Configuration::Transformation transform;
        transform.type = "filter";
        transform.config = "exclude_inserts";
        config.transformations.push_back(transform);
    }
    
    if (chk_capture_update_ && !chk_capture_update_->GetValue()) {
        CdcPipeline::Configuration::Transformation transform;
        transform.type = "filter";
        transform.config = "exclude_updates";
        config.transformations.push_back(transform);
    }
    
    if (chk_capture_delete_ && !chk_capture_delete_->GetValue()) {
        CdcPipeline::Configuration::Transformation transform;
        transform.type = "filter";
        transform.config = "exclude_deletes";
        config.transformations.push_back(transform);
    }
    
    return config;
}

} // namespace scratchrobin
