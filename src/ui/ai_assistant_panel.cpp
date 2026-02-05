/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#include "ai_assistant_panel.h"

#include "ai_settings_dialog.h"
#include "../core/ai_assistant.h"
#include "../core/ai_providers.h"

#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/stattext.h>
#include <wx/gauge.h>
#include <wx/menu.h>
#include <wx/msgdlg.h>
#include <wx/filedlg.h>

namespace scratchrobin {

AiAssistantPanel::AiAssistantPanel(wxWindow* parent)
    : wxPanel(parent, wxID_ANY)
    , chat_session_(std::make_unique<AiChatSession>("main")) {
    CreateControls();
    BindEvents();
}

AiAssistantPanel::~AiAssistantPanel() = default;

void AiAssistantPanel::CreateControls() {
    auto* main_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Toolbar with mode selection
    auto* toolbar_sizer = new wxBoxSizer(wxHORIZONTAL);
    
    toolbar_sizer->Add(new wxStaticText(this, wxID_ANY, "Mode:"), 
                       0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    
    mode_choice_ = new wxChoice(this, wxID_ANY);
    mode_choice_->Append("General Chat");
    mode_choice_->Append("Query Optimization");
    mode_choice_->Append("Schema Design");
    mode_choice_->Append("Natural Language to SQL");
    mode_choice_->Append("Code Generation");
    mode_choice_->Append("Migration Help");
    mode_choice_->SetSelection(0);
    toolbar_sizer->Add(mode_choice_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    
    auto* optimize_btn = new wxButton(this, ID_OPTIMIZE_QUERY, "Optimize Query");
    toolbar_sizer->Add(optimize_btn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    
    auto* schema_btn = new wxButton(this, ID_DESIGN_SCHEMA, "Design Schema");
    toolbar_sizer->Add(schema_btn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    
    auto* convert_btn = new wxButton(this, ID_CONVERT_TO_SQL, "To SQL");
    toolbar_sizer->Add(convert_btn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    
    auto* code_btn = new wxButton(this, ID_GENERATE_CODE, "Generate Code");
    toolbar_sizer->Add(code_btn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    
    auto* export_btn = new wxButton(this, ID_EXPORT_CHAT, "Export");
    toolbar_sizer->Add(export_btn, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 5);
    
    toolbar_sizer->AddStretchSpacer(1);
    
    auto* settings_btn = new wxButton(this, ID_SETTINGS, "Settings");
    toolbar_sizer->Add(settings_btn, 0, wxALIGN_CENTER_VERTICAL);
    
    main_sizer->Add(toolbar_sizer, 0, wxEXPAND | wxALL, 5);
    
    // Chat display area
    splitter_ = new wxSplitterWindow(this, wxID_ANY, 
                                     wxDefaultPosition, wxDefaultSize,
                                     wxSP_LIVE_UPDATE);
    
    chat_display_ = new wxTextCtrl(splitter_, wxID_ANY, wxEmptyString,
                                   wxDefaultPosition, wxDefaultSize,
                                   wxTE_MULTILINE | wxTE_READONLY | wxTE_RICH2);
    chat_display_->SetFont(wxFont(10, wxFONTFAMILY_TELETYPE, wxFONTSTYLE_NORMAL, 
                                  wxFONTWEIGHT_NORMAL));
    
    // Input area
    auto* input_panel = new wxPanel(splitter_, wxID_ANY);
    auto* input_sizer = new wxBoxSizer(wxVERTICAL);
    
    input_text_ = new wxTextCtrl(input_panel, wxID_ANY, wxEmptyString,
                                 wxDefaultPosition, wxDefaultSize,
                                 wxTE_MULTILINE);
    input_text_->SetMinSize(wxSize(-1, 80));
    input_sizer->Add(input_text_, 1, wxEXPAND | wxBOTTOM, 5);
    
    auto* button_sizer = new wxBoxSizer(wxHORIZONTAL);
    
    send_button_ = new wxButton(input_panel, ID_SEND, "Send");
    send_button_->SetDefault();
    button_sizer->Add(send_button_, 0, wxRIGHT, 5);
    
    clear_button_ = new wxButton(input_panel, ID_CLEAR, "Clear Chat");
    button_sizer->Add(clear_button_, 0);
    
    button_sizer->AddStretchSpacer(1);
    
    status_text_ = new wxStaticText(input_panel, wxID_ANY, "Ready");
    button_sizer->Add(status_text_, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 10);
    
    progress_gauge_ = new wxGauge(input_panel, wxID_ANY, 100, 
                                  wxDefaultPosition, wxSize(100, 16));
    progress_gauge_->SetValue(0);
    button_sizer->Add(progress_gauge_, 0, wxALIGN_CENTER_VERTICAL);
    
    input_sizer->Add(button_sizer, 0, wxEXPAND);
    input_panel->SetSizer(input_sizer);
    
    splitter_->SplitHorizontally(chat_display_, input_panel, -150);
    splitter_->SetMinimumPaneSize(100);
    
    main_sizer->Add(splitter_, 1, wxEXPAND | wxALL, 5);
    
    SetSizer(main_sizer);
    
    // Welcome message
    chat_display_->AppendText("Welcome to ScratchRobin AI Assistant\n");
    chat_display_->AppendText("================================\n\n");
    chat_display_->AppendText("I can help you with:\n");
    chat_display_->AppendText("  - Query optimization\n");
    chat_display_->AppendText("  - Schema design suggestions\n");
    chat_display_->AppendText("  - Converting natural language to SQL\n");
    chat_display_->AppendText("  - Generating code from database schemas\n");
    chat_display_->AppendText("  - Migration planning and compatibility\n\n");
    chat_display_->AppendText("Select a mode from the dropdown or use the toolbar buttons.\n\n");
}

void AiAssistantPanel::BindEvents() {
    send_button_->Bind(wxEVT_BUTTON, &AiAssistantPanel::OnSend, this);
    clear_button_->Bind(wxEVT_BUTTON, &AiAssistantPanel::OnClear, this);
    
    Bind(wxEVT_BUTTON, &AiAssistantPanel::OnOptimizeQuery, this, ID_OPTIMIZE_QUERY);
    Bind(wxEVT_BUTTON, &AiAssistantPanel::OnDesignSchema, this, ID_DESIGN_SCHEMA);
    Bind(wxEVT_BUTTON, &AiAssistantPanel::OnConvertToSql, this, ID_CONVERT_TO_SQL);
    Bind(wxEVT_BUTTON, &AiAssistantPanel::OnGenerateCode, this, ID_GENERATE_CODE);
    Bind(wxEVT_BUTTON, &AiAssistantPanel::OnExportChat, this, ID_EXPORT_CHAT);
    Bind(wxEVT_BUTTON, &AiAssistantPanel::OnSettings, this, ID_SETTINGS);
    
    // Bind thread event for async AI responses
    Bind(wxEVT_THREAD, &AiAssistantPanel::OnThreadEvent, this);
    
    input_text_->Bind(wxEVT_CHAR, [this](wxKeyEvent& event) {
        if (event.GetKeyCode() == WXK_RETURN && event.ControlDown()) {
            wxCommandEvent dummy_event;
            OnSend(dummy_event);
        } else {
            event.Skip();
        }
    });
}

void AiAssistantPanel::OnSend(wxCommandEvent& /*event*/) {
    wxString prompt = input_text_->GetValue().Trim();
    if (prompt.IsEmpty()) return;
    
    AddUserMessage(prompt);
    input_text_->Clear();
    
    wxString mode = mode_choice_->GetStringSelection();
    ProcessRequest(prompt, mode);
}

void AiAssistantPanel::OnClear(wxCommandEvent& /*event*/) {
    chat_display_->Clear();
    chat_session_->ClearHistory();
    
    chat_display_->AppendText("Chat cleared. How can I help you?\n\n");
}

void AiAssistantPanel::OnOptimizeQuery(wxCommandEvent& /*event*/) {
    mode_choice_->SetSelection(1);  // Query Optimization
    input_text_->SetValue("Please paste your SQL query to optimize");
    input_text_->SetFocus();
}

void AiAssistantPanel::OnDesignSchema(wxCommandEvent& /*event*/) {
    mode_choice_->SetSelection(2);  // Schema Design
    input_text_->SetValue("I need a schema for... (describe your requirements)");
    input_text_->SetFocus();
}

void AiAssistantPanel::OnConvertToSql(wxCommandEvent& /*event*/) {
    mode_choice_->SetSelection(3);  // Natural Language to SQL
    input_text_->SetValue("Find all... (describe what you want to query)");
    input_text_->SetFocus();
}

void AiAssistantPanel::OnGenerateCode(wxCommandEvent& /*event*/) {
    mode_choice_->SetSelection(4);  // Code Generation
    input_text_->SetValue("Generate code to... (describe your needs)");
    input_text_->SetFocus();
}

void AiAssistantPanel::OnExportChat(wxCommandEvent& /*event*/) {
    wxFileDialog dlg(this, "Export Chat", wxEmptyString, 
                     "chat_export.md", "Markdown files (*.md)|*.md",
                     wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
    
    if (dlg.ShowModal() == wxID_OK) {
        chat_session_->ExportHistory(dlg.GetPath().ToStdString());
        wxMessageBox("Chat exported successfully!", "Export", wxOK | wxICON_INFORMATION);
    }
}

void AiAssistantPanel::OnSettings(wxCommandEvent& /*event*/) {
    AiSettingsDialog dlg(this);
    if (dlg.ShowModal() == wxID_OK) {
        // Settings saved and AI assistant reinitialized
        AddAssistantMessage("AI provider settings updated successfully.");
    }
}

void AiAssistantPanel::AddUserMessage(const wxString& message) {
    chat_display_->AppendText("\n[You]: " + message + "\n");
    
    ChatMessage chat_msg;
    chat_msg.id = std::to_string(std::time(nullptr));
    chat_msg.role = "user";
    chat_msg.content = message.ToStdString();
    chat_msg.timestamp = std::time(nullptr);
    chat_session_->AddMessage(chat_msg);
}

void AiAssistantPanel::AddAssistantMessage(const wxString& message) {
    chat_display_->AppendText("\n[Assistant]: " + message + "\n");
    
    ChatMessage chat_msg;
    chat_msg.id = std::to_string(std::time(nullptr));
    chat_msg.role = "assistant";
    chat_msg.content = message.ToStdString();
    chat_msg.timestamp = std::time(nullptr);
    chat_session_->AddMessage(chat_msg);
}

void AiAssistantPanel::ShowLoading(bool loading) {
    if (loading) {
        send_button_->Disable();
        status_text_->SetLabel("Processing...");
        progress_gauge_->Pulse();
    } else {
        send_button_->Enable();
        status_text_->SetLabel("Ready");
        progress_gauge_->SetValue(0);
    }
}

void AiAssistantPanel::ProcessRequest(const wxString& prompt, const wxString& type) {
    ShowLoading(true);
    
    // Get the AI assistant manager
    AiAssistantManager& manager = AiAssistantManager::Instance();
    AiProvider* provider = manager.GetActiveProvider();
    
    // If no provider is available or not initialized, fall back to simulated responses
    if (!provider || !provider->IsAvailable()) {
        ProcessSimulatedRequest(prompt, type);
        return;
    }
    
    // Build the AI request
    AiRequest request;
    request.system_message = "You are a helpful database assistant. Provide clear, accurate SQL and database advice.";
    request.prompt = prompt.ToStdString();
    
    // Add conversation history as context
    auto messages = chat_session_->GetMessages();
    for (const auto& msg : messages) {
        request.context.push_back({msg.role, msg.content});
    }
    
    // Set model parameters
    AiProviderConfig config = manager.GetConfig();
    request.max_tokens = config.max_tokens;
    request.temperature = config.temperature;
    
    // Run the request asynchronously
    std::thread([this, request, type, provider]() {
        AiResponse response = provider->SendRequest(request);
        
        // Queue the result back to the main thread
        wxThreadEvent* event = new wxThreadEvent(wxEVT_THREAD);
        event->SetString(wxString::FromUTF8(response.content));
        event->SetInt(response.success ? 1 : 0);
        wxQueueEvent(this, event);
    }).detach();
}

void AiAssistantPanel::ProcessSimulatedRequest(const wxString& prompt, const wxString& type) {
    wxString response;
    
    if (type == "Query Optimization") {
        response = "Here's an optimized version of your query:\n\n"
                   "```sql\n"
                   "-- Optimized query\n"
                   "SELECT c.customer_id, c.name, SUM(o.total) as total_spent\n"
                   "FROM customers c\n"
                   "JOIN orders o ON c.customer_id = o.customer_id\n"
                   "WHERE o.status = 'completed'\n"
                   "GROUP BY c.customer_id, c.name\n"
                   "ORDER BY total_spent DESC\n"
                   "LIMIT 100;\n"
                   "```\n\n"
                   "Optimizations applied:\n"
                   "- Added LIMIT clause for pagination\n"
                   "- Consider adding index on orders(customer_id, status)\n"
                   "- Estimated improvement: 45% faster execution";
    } else if (type == "Schema Design") {
        response = "Based on your requirements, here's a suggested schema:\n\n"
                   "```sql\n"
                   "CREATE TABLE users (\n"
                   "    user_id SERIAL PRIMARY KEY,\n"
                   "    email VARCHAR(255) UNIQUE NOT NULL,\n"
                   "    created_at TIMESTAMP DEFAULT NOW()\n";
                   ");\n\n"
                   "CREATE TABLE orders (\n"
                   "    order_id SERIAL PRIMARY KEY,\n"
                   "    user_id INTEGER REFERENCES users(user_id),\n"
                   "    total DECIMAL(10,2),\n"
                   "    status VARCHAR(50)\n"
                   ");\n"
                   "```\n\n"
                   "Recommended indexes:\n"
                   "- CREATE INDEX idx_orders_user ON orders(user_id);\n"
                   "- CREATE INDEX idx_orders_status ON orders(status);";
    } else if (type == "Natural Language to SQL") {
        response = "Here's the SQL for your request:\n\n"
                   "```sql\n"
                   "SELECT p.product_name, p.price, c.category_name\n"
                   "FROM products p\n"
                   "JOIN categories c ON p.category_id = c.category_id\n"
                   "WHERE p.price > 100\n"
                   "  AND p.stock_quantity > 0\n"
                   "ORDER BY p.price DESC;\n"
                   "```\n\n"
                   "This query:\n"
                   "- Joins products with categories\n"
                   "- Filters for items over $100 with stock\n"
                   "- Sorts by price highest first";
    } else if (type == "Code Generation") {
        response = "Here's Python code for database operations:\n\n"
                   "```python\n"
                   "import psycopg2\n\n"
                   "def get_customer_orders(customer_id):\n"
                   "    conn = psycopg2.connect('dbname=mydb')\n"
                   "    cur = conn.cursor()\n"
                   "    cur.execute(\n"
                   "        '''SELECT * FROM orders WHERE customer_id = %s''',\n"
                   "        (customer_id,)\n"
                   "    )\n"
                   "    return cur.fetchall()\n"
                   "```\n\n"
                   "Dependencies: pip install psycopg2-binary";
    } else {
        response = "I'm here to help with your database needs.\n\n"
                   "I can assist with:\n"
                   "- Writing and optimizing SQL queries\n"
                   "- Designing database schemas\n"
                   "- Explaining query execution plans\n"
                   "- Generating code for database operations\n"
                   "- Migration planning\n\n"
                   "What would you like to work on?";
    }
    
    AddAssistantMessage(response);
    ShowLoading(false);
}

void AiAssistantPanel::LoadQuery(const wxString& query) {
    mode_choice_->SetSelection(1);  // Query Optimization mode
    input_text_->SetValue("Please optimize this query:\n\n" + query);
}

void AiAssistantPanel::OptimizeCurrentQuery() {
    wxString query = input_text_->GetValue();
    if (!query.IsEmpty()) {
        ProcessRequest(query, "Query Optimization");
    }
}

void AiAssistantPanel::StartSchemaDesign() {
    mode_choice_->SetSelection(2);
    input_text_->SetFocus();
}

void AiAssistantPanel::ConvertToSql(const wxString& natural_language) {
    mode_choice_->SetSelection(3);
    input_text_->SetValue(natural_language);
    ProcessRequest(natural_language, "Natural Language to SQL");
}

void AiAssistantPanel::StartMigrationAssistance() {
    mode_choice_->SetSelection(5);  // Migration Help
    input_text_->SetValue("I need help migrating from [source] to [target database]");
    input_text_->SetFocus();
}

void AiAssistantPanel::GenerateDocumentation() {
    // This would generate documentation based on current database
    AddAssistantMessage("Documentation generation is available from the Schema context menu.\n\n"
                       "Select Tables → Right-click → Generate Documentation\n\n"
                       "I can create:\n"
                       "- Markdown schema documentation\n"
                       "- API documentation\n"
                       "- Entity Relationship Diagrams\n"
                       "- Change logs");
}

void AiAssistantPanel::OnThreadEvent(wxThreadEvent& event) {
    wxString response = event.GetString();
    bool success = event.GetInt() != 0;
    
    if (success && !response.IsEmpty()) {
        AddAssistantMessage(response);
    } else {
        AddAssistantMessage("Sorry, I encountered an error processing your request. "
                           "Please check your AI settings and try again.");
    }
    ShowLoading(false);
}

} // namespace scratchrobin
