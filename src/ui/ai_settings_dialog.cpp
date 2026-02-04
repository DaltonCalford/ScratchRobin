/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#include "ai_settings_dialog.h"

#include "../core/ai_assistant.h"
#include "../core/ai_providers.h"

#include <wx/sizer.h>
#include <wx/button.h>
#include <wx/choice.h>
#include <wx/slider.h>
#include <wx/spinctrl.h>
#include <wx/checkbox.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/hyperlink.h>

namespace scratchrobin {

AiSettingsDialog::AiSettingsDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "AI Provider Settings", 
               wxDefaultPosition, wxSize(600, 550),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) {
    CreateControls();
    BindEvents();
    LoadSettings();
}

void AiSettingsDialog::CreateControls() {
    auto* main_sizer = new wxBoxSizer(wxVERTICAL);
    
    // Provider selection
    auto* provider_box = new wxStaticBoxSizer(wxVERTICAL, this, "AI Provider");
    auto* provider_row = new wxBoxSizer(wxHORIZONTAL);
    
    provider_row->Add(new wxStaticText(this, wxID_ANY, "Provider:"), 
                      0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    
    provider_choice_ = new wxChoice(this, ID_PROVIDER_CHOICE);
    provider_choice_->Append("OpenAI (GPT-4, GPT-3.5)");
    provider_choice_->Append("Anthropic (Claude)");
    provider_choice_->Append("Ollama (Local Models)");
    provider_choice_->Append("Google Gemini");
    provider_choice_->SetSelection(0);
    
    provider_row->Add(provider_choice_, 1, wxEXPAND);
    provider_box->Add(provider_row, 0, wxEXPAND | wxALL, 8);
    
    // Help link
    auto* help_link = new wxHyperlinkCtrl(this, wxID_ANY, 
                                          "How to get an API key", 
                                          "https://platform.openai.com/api-keys",
                                          wxDefaultPosition, wxDefaultSize,
                                          wxHL_DEFAULT_STYLE);
    provider_box->Add(help_link, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
    
    main_sizer->Add(provider_box, 0, wxEXPAND | wxALL, 10);
    
    // Connection settings
    auto* conn_box = new wxStaticBoxSizer(wxVERTICAL, this, "Connection Settings");
    
    // API Key
    auto* key_row = new wxBoxSizer(wxHORIZONTAL);
    key_row->Add(new wxStaticText(this, wxID_ANY, "API Key:"), 
                 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    api_key_ctrl_ = new wxTextCtrl(this, ID_API_KEY, wxEmptyString,
                                   wxDefaultPosition, wxDefaultSize,
                                   wxTE_PASSWORD);
    key_row->Add(api_key_ctrl_, 1, wxEXPAND);
    conn_box->Add(key_row, 0, wxEXPAND | wxALL, 8);
    
    // API Endpoint (optional)
    auto* endpoint_row = new wxBoxSizer(wxHORIZONTAL);
    endpoint_row->Add(new wxStaticText(this, wxID_ANY, "Endpoint:"), 
                      0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    api_endpoint_ctrl_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
    api_endpoint_ctrl_->SetToolTip("Leave empty to use default endpoint");
    endpoint_row->Add(api_endpoint_ctrl_, 1, wxEXPAND);
    conn_box->Add(endpoint_row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    
    // Model name
    auto* model_row = new wxBoxSizer(wxHORIZONTAL);
    model_row->Add(new wxStaticText(this, wxID_ANY, "Model:"), 
                   0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    model_name_ctrl_ = new wxTextCtrl(this, wxID_ANY, wxEmptyString);
    model_name_ctrl_->SetToolTip("Leave empty to use recommended default");
    model_row->Add(model_name_ctrl_, 1, wxEXPAND);
    conn_box->Add(model_row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    
    // Timeout
    auto* timeout_row = new wxBoxSizer(wxHORIZONTAL);
    timeout_row->Add(new wxStaticText(this, wxID_ANY, "Timeout (sec):"), 
                     0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    timeout_ctrl_ = new wxSpinCtrl(this, wxID_ANY, "60",
                                   wxDefaultPosition, wxDefaultSize,
                                   wxSP_ARROW_KEYS, 10, 300, 60);
    timeout_row->Add(timeout_ctrl_, 0);
    timeout_row->AddStretchSpacer(1);
    conn_box->Add(timeout_row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    
    main_sizer->Add(conn_box, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    
    // Model parameters
    auto* params_box = new wxStaticBoxSizer(wxVERTICAL, this, "Model Parameters");
    
    // Temperature
    auto* temp_row = new wxBoxSizer(wxHORIZONTAL);
    temp_row->Add(new wxStaticText(this, wxID_ANY, "Temperature:"), 
                  0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    temperature_slider_ = new wxSlider(this, ID_TEMPERATURE_SLIDER, 30, 0, 100);
    temp_row->Add(temperature_slider_, 1, wxALIGN_CENTER_VERTICAL);
    temperature_label_ = new wxStaticText(this, wxID_ANY, "0.30");
    temp_row->Add(temperature_label_, 0, wxALIGN_CENTER_VERTICAL | wxLEFT, 8);
    params_box->Add(temp_row, 0, wxEXPAND | wxALL, 8);
    
    // Max tokens
    auto* tokens_row = new wxBoxSizer(wxHORIZONTAL);
    tokens_row->Add(new wxStaticText(this, wxID_ANY, "Max Tokens:"), 
                    0, wxALIGN_CENTER_VERTICAL | wxRIGHT, 8);
    max_tokens_ctrl_ = new wxSpinCtrl(this, wxID_ANY, "4096",
                                      wxDefaultPosition, wxDefaultSize,
                                      wxSP_ARROW_KEYS, 256, 32768, 4096);
    tokens_row->Add(max_tokens_ctrl_, 0);
    tokens_row->AddStretchSpacer(1);
    params_box->Add(tokens_row, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    
    main_sizer->Add(params_box, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    
    // Feature toggles
    auto* features_box = new wxStaticBoxSizer(wxVERTICAL, this, "Enabled Features");
    
    enable_schema_design_ = new wxCheckBox(this, wxID_ANY, "Schema Design");
    enable_schema_design_->SetValue(true);
    features_box->Add(enable_schema_design_, 0, wxALL, 4);
    
    enable_query_optimization_ = new wxCheckBox(this, wxID_ANY, "Query Optimization");
    enable_query_optimization_->SetValue(true);
    features_box->Add(enable_query_optimization_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 4);
    
    enable_code_generation_ = new wxCheckBox(this, wxID_ANY, "Code Generation");
    enable_code_generation_->SetValue(true);
    features_box->Add(enable_code_generation_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 4);
    
    enable_documentation_ = new wxCheckBox(this, wxID_ANY, "Documentation Generation");
    enable_documentation_->SetValue(true);
    features_box->Add(enable_documentation_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 4);
    
    main_sizer->Add(features_box, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    
    // Status and test
    auto* status_sizer = new wxBoxSizer(wxHORIZONTAL);
    
    status_label_ = new wxStaticText(this, wxID_ANY, "Status: Not configured");
    status_sizer->Add(status_label_, 1, wxALIGN_CENTER_VERTICAL);
    
    test_button_ = new wxButton(this, ID_TEST_CONNECTION, "Test Connection");
    status_sizer->Add(test_button_, 0);
    
    main_sizer->Add(status_sizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
    
    // Buttons
    auto* button_sizer = new wxBoxSizer(wxHORIZONTAL);
    button_sizer->AddStretchSpacer(1);
    
    auto* help_btn = new wxButton(this, wxID_HELP, "Help");
    button_sizer->Add(help_btn, 0, wxRIGHT, 5);
    
    auto* cancel_btn = new wxButton(this, wxID_CANCEL, "Cancel");
    button_sizer->Add(cancel_btn, 0, wxRIGHT, 5);
    
    auto* save_btn = new wxButton(this, wxID_OK, "Save");
    save_btn->SetDefault();
    button_sizer->Add(save_btn, 0);
    
    main_sizer->Add(button_sizer, 0, wxEXPAND | wxALL, 10);
    
    SetSizer(main_sizer);
}

void AiSettingsDialog::BindEvents() {
    provider_choice_->Bind(wxEVT_CHOICE, &AiSettingsDialog::OnProviderChanged, this);
    temperature_slider_->Bind(wxEVT_SLIDER, [this](wxCommandEvent&) {
        double temp = temperature_slider_->GetValue() / 100.0;
        temperature_label_->SetLabel(wxString::Format("%.2f", temp));
    });
    test_button_->Bind(wxEVT_BUTTON, &AiSettingsDialog::OnTestConnection, this);
    Bind(wxEVT_BUTTON, &AiSettingsDialog::OnSave, this, wxID_OK);
    Bind(wxEVT_BUTTON, &AiSettingsDialog::OnHelp, this, wxID_HELP);
}

void AiSettingsDialog::LoadSettings() {
    // TODO: Load from configuration store when available
    // For now, set defaults
    
    provider_choice_->SetSelection(0);
    api_key_ctrl_->SetValue("");
    api_endpoint_ctrl_->SetValue("");
    model_name_ctrl_->SetValue("");
    temperature_slider_->SetValue(30);
    temperature_label_->SetLabel("0.30");
    max_tokens_ctrl_->SetValue(4096);
    timeout_ctrl_->SetValue(60);
    
    enable_schema_design_->SetValue(true);
    enable_query_optimization_->SetValue(true);
    enable_code_generation_->SetValue(true);
    enable_documentation_->SetValue(true);
    
    UpdateProviderUI();
    
    // Update status
    if (!api_key_ctrl_->GetValue().IsEmpty()) {
        status_label_->SetLabel("Status: API key configured");
    }
}

void AiSettingsDialog::SaveSettings() {
    // TODO: Save to configuration store when available
    
    // Get selected provider
    int selection = provider_choice_->GetSelection();
    std::string provider;
    switch (selection) {
        case 0: provider = OpenAiProvider::NAME; break;
        case 1: provider = AnthropicProvider::NAME; break;
        case 2: provider = OllamaProvider::NAME; break;
        case 3: provider = GeminiProvider::NAME; break;
        default: provider = OpenAiProvider::NAME; break;
    }
    
    // Build config
    AiProviderConfig ai_config;
    ai_config.api_key = api_key_ctrl_->GetValue().ToStdString();
    ai_config.api_endpoint = api_endpoint_ctrl_->GetValue().ToStdString();
    ai_config.model_name = model_name_ctrl_->GetValue().ToStdString();
    ai_config.temperature = static_cast<float>(temperature_slider_->GetValue()) / 100.0f;
    ai_config.max_tokens = max_tokens_ctrl_->GetValue();
    ai_config.timeout_seconds = timeout_ctrl_->GetValue();
    ai_config.enable_schema_design = enable_schema_design_->GetValue();
    ai_config.enable_query_optimization = enable_query_optimization_->GetValue();
    ai_config.enable_code_generation = enable_code_generation_->GetValue();
    ai_config.enable_documentation = enable_documentation_->GetValue();
    
    // Initialize the AI assistant with new settings
    auto& manager = AiAssistantManager::Instance();
    manager.SetActiveProvider(provider);
    manager.SetConfig(ai_config);
}

void AiSettingsDialog::UpdateProviderUI() {
    int selection = provider_choice_->GetSelection();
    
    // Update help link URL based on provider
    std::string help_url;
    std::string default_endpoint;
    std::string default_model;
    
    switch (selection) {
        case 0:  // OpenAI
            help_url = "https://platform.openai.com/api-keys";
            default_endpoint = "https://api.openai.com/v1/chat/completions";
            default_model = "gpt-4o";
            break;
        case 1:  // Anthropic
            help_url = "https://console.anthropic.com/settings/keys";
            default_endpoint = "https://api.anthropic.com/v1/messages";
            default_model = "claude-3-5-sonnet-20241022";
            break;
        case 2:  // Ollama
            help_url = "https://ollama.com/download";
            default_endpoint = "http://localhost:11434/api/chat";
            default_model = "codellama";
            break;
        case 3:  // Gemini
            help_url = "https://aistudio.google.com/app/apikey";
            default_endpoint = "https://generativelanguage.googleapis.com/v1beta/models/";
            default_model = "gemini-pro";
            break;
    }
    
    // Update tooltip for endpoint field
    if (!default_endpoint.empty()) {
        api_endpoint_ctrl_->SetToolTip("Default: " + default_endpoint);
    }
    if (!default_model.empty()) {
        model_name_ctrl_->SetToolTip("Recommended: " + default_model);
    }
}

void AiSettingsDialog::TestConnection() {
    status_label_->SetLabel("Status: Testing...");
    status_label_->SetForegroundColour(wxColour(128, 128, 0));  // Yellow/orange
    test_button_->Disable();
    
    // Get current settings
    int selection = provider_choice_->GetSelection();
    std::string provider_name;
    switch (selection) {
        case 0: provider_name = OpenAiProvider::NAME; break;
        case 1: provider_name = AnthropicProvider::NAME; break;
        case 2: provider_name = OllamaProvider::NAME; break;
        case 3: provider_name = GeminiProvider::NAME; break;
        default: provider_name = OpenAiProvider::NAME; break;
    }
    
    AiProviderConfig test_config;
    test_config.api_key = api_key_ctrl_->GetValue().ToStdString();
    test_config.api_endpoint = api_endpoint_ctrl_->GetValue().ToStdString();
    test_config.model_name = model_name_ctrl_->GetValue().ToStdString();
    test_config.timeout_seconds = timeout_ctrl_->GetValue();
    
    // Create provider and test
    std::unique_ptr<AiProvider> provider;
    if (provider_name == OpenAiProvider::NAME) {
        provider = std::make_unique<OpenAiProvider>();
    } else if (provider_name == AnthropicProvider::NAME) {
        provider = std::make_unique<AnthropicProvider>();
    } else if (provider_name == OllamaProvider::NAME) {
        provider = std::make_unique<OllamaProvider>();
    } else if (provider_name == GeminiProvider::NAME) {
        provider = std::make_unique<GeminiProvider>();
    }
    
    if (!provider || !provider->Initialize(test_config)) {
        status_label_->SetLabel("Status: Failed to initialize provider");
        status_label_->SetForegroundColour(wxColour(255, 0, 0));  // Red
        test_button_->Enable();
        return;
    }
    
    // Send a simple test request
    AiRequest request;
    request.id = "test";
    request.prompt = "Say 'Connection successful' and nothing else.";
    request.max_tokens = 50;
    
    auto response = provider->SendRequest(request);
    
    test_button_->Enable();
    
    if (response.success) {
        status_label_->SetLabel("Status: Connection successful!");
        status_label_->SetForegroundColour(wxColour(0, 128, 0));  // Green
    } else {
        wxString msg = "Status: Failed - " + response.error_message;
        status_label_->SetLabel(msg);
        status_label_->SetForegroundColour(wxColour(255, 0, 0));  // Red
    }
}

void AiSettingsDialog::OnProviderChanged(wxCommandEvent& /*event*/) {
    UpdateProviderUI();
}

void AiSettingsDialog::OnTestConnection(wxCommandEvent& /*event*/) {
    TestConnection();
}

void AiSettingsDialog::OnSave(wxCommandEvent& /*event*/) {
    SaveSettings();
    EndModal(wxID_OK);
}

void AiSettingsDialog::OnHelp(wxCommandEvent& /*event*/) {
    int selection = provider_choice_->GetSelection();
    std::string help_url;
    
    switch (selection) {
        case 0: help_url = "https://platform.openai.com/api-keys"; break;
        case 1: help_url = "https://console.anthropic.com/settings/keys"; break;
        case 2: help_url = "https://ollama.com/download"; break;
        case 3: help_url = "https://aistudio.google.com/app/apikey"; break;
        default: help_url = "https://platform.openai.com/api-keys"; break;
    }
    
    wxLaunchDefaultBrowser(help_url);
}

} // namespace scratchrobin
