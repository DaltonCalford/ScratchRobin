/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_AI_ASSISTANT_PANEL_H
#define SCRATCHROBIN_AI_ASSISTANT_PANEL_H

#include <wx/wx.h>
#include <wx/splitter.h>

#include <memory>

namespace scratchrobin {

class AiChatSession;
class AiProvider;

// AI Assistant panel for natural language database assistance
class AiAssistantPanel : public wxPanel {
public:
    AiAssistantPanel(wxWindow* parent);
    ~AiAssistantPanel();

    // Query optimization interface
    void LoadQuery(const wxString& query);
    void OptimizeCurrentQuery();
    
    // Schema design interface
    void StartSchemaDesign();
    
    // Natural language to SQL
    void ConvertToSql(const wxString& natural_language);
    
    // Migration assistance
    void StartMigrationAssistance();
    
    // Documentation generation
    void GenerateDocumentation();

private:
    void CreateControls();
    void BindEvents();
    
    void OnSend(wxCommandEvent& event);
    void OnClear(wxCommandEvent& event);
    void OnOptimizeQuery(wxCommandEvent& event);
    void OnDesignSchema(wxCommandEvent& event);
    void OnConvertToSql(wxCommandEvent& event);
    void OnGenerateCode(wxCommandEvent& event);
    void OnExportChat(wxCommandEvent& event);
    void OnSettings(wxCommandEvent& event);
    
    void AddUserMessage(const wxString& message);
    void AddAssistantMessage(const wxString& message);
    void ShowLoading(bool loading);
    
    void ProcessRequest(const wxString& prompt, const wxString& type);
    
    // UI controls
    wxSplitterWindow* splitter_;
    wxTextCtrl* chat_display_;
    wxTextCtrl* input_text_;
    wxButton* send_button_;
    wxButton* clear_button_;
    wxChoice* mode_choice_;
    wxStaticText* status_text_;
    wxGauge* progress_gauge_;
    
    // Chat session
    std::unique_ptr<AiChatSession> chat_session_;
    
    enum {
        ID_SEND = wxID_HIGHEST + 1,
        ID_CLEAR,
        ID_OPTIMIZE_QUERY,
        ID_DESIGN_SCHEMA,
        ID_CONVERT_TO_SQL,
        ID_GENERATE_CODE,
        ID_EXPORT_CHAT,
        ID_SETTINGS
    };
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_AI_ASSISTANT_PANEL_H
