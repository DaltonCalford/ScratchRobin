/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "preferences_dialog.h"

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/clrpicker.h>
#include <wx/config.h>
#include <wx/fdrepdlg.h>
#include <wx/fontpicker.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/tokenzr.h>

#include <algorithm>
#include <sstream>

namespace scratchrobin {

namespace {

// Configuration keys
constexpr char kConfigAppName[] = "ScratchRobin";
constexpr char kConfigVendorName[] = "DaltonCalford";

// Editor section
constexpr char kKeyEditorFontFamily[] = "/Editor/FontFamily";
constexpr char kKeyEditorFontSize[] = "/Editor/FontSize";
constexpr char kKeyEditorBgColor[] = "/Editor/BackgroundColor";
constexpr char kKeyEditorFgColor[] = "/Editor/ForegroundColor";
constexpr char kKeyEditorSelectionColor[] = "/Editor/SelectionColor";
constexpr char kKeyEditorTabWidth[] = "/Editor/TabWidth";
constexpr char kKeyEditorUseSpaces[] = "/Editor/UseSpacesForTabs";
constexpr char kKeyEditorWordWrap[] = "/Editor/WordWrap";
constexpr char kKeyEditorLineNumbers[] = "/Editor/ShowLineNumbers";
constexpr char kKeyEditorAutoIndent[] = "/Editor/AutoIndent";

// Results section
constexpr char kKeyResultsRowLimit[] = "/Results/DefaultRowLimit";
constexpr char kKeyResultsNullDisplay[] = "/Results/NullDisplay";
constexpr char kKeyResultsDateTimeFormat[] = "/Results/DateTimeFormat";
constexpr char kKeyResultsGridLines[] = "/Results/ShowGridLines";

// Connection section
constexpr char kKeyConnTimeout[] = "/Connection/ConnectTimeout";
constexpr char kKeyQueryTimeout[] = "/Connection/QueryTimeout";
constexpr char kKeySslMode[] = "/Connection/SslMode";
constexpr char kKeyAutoReconnect[] = "/Connection/AutoReconnect";
constexpr char kKeyKeepAlive[] = "/Connection/KeepAliveInterval";

// Export section
constexpr char kKeyExportCsvDelimiter[] = "/Export/CsvDelimiter";
constexpr char kKeyExportCsvQuote[] = "/Export/CsvQuoteChar";
constexpr char kKeyExportIncludeHeaders[] = "/Export/IncludeHeaders";
constexpr char kKeyExportDateFormat[] = "/Export/DateFormat";

// Diagram section
constexpr char kKeyDiagramNotation[] = "/Diagram/DefaultNotation";
constexpr char kKeyDiagramGridSize[] = "/Diagram/GridSize";
constexpr char kKeyDiagramSnapToGrid[] = "/Diagram/SnapToGrid";
constexpr char kKeyDiagramPaperSize[] = "/Diagram/DefaultPaperSize";

// Network section
constexpr char kKeyNetworkHttpProxyHost[] = "/Network/HttpProxyHost";
constexpr char kKeyNetworkHttpProxyPort[] = "/Network/HttpProxyPort";
constexpr char kKeyNetworkSocksProxyHost[] = "/Network/SocksProxyHost";
constexpr char kKeyNetworkSocksProxyPort[] = "/Network/SocksProxyPort";
constexpr char kKeyNetworkNoProxy[] = "/Network/NoProxyDomains";

// Color serialization helpers
wxString ColorToString(const wxColour& color) {
    return wxString::Format("%d,%d,%d", color.Red(), color.Green(), color.Blue());
}

wxColour StringToColor(const wxString& str) {
    wxColour color;
    if (color.Set(str)) {
        return color;
    }
    // Try parsing "R,G,B" format
    wxStringTokenizer tokenizer(str, ",");
    if (tokenizer.CountTokens() == 3) {
        long r, g, b;
        if (tokenizer.GetNextToken().ToLong(&r) &&
            tokenizer.GetNextToken().ToLong(&g) &&
            tokenizer.GetNextToken().ToLong(&b)) {
            return wxColour(static_cast<unsigned char>(r),
                           static_cast<unsigned char>(g),
                           static_cast<unsigned char>(b));
        }
    }
    return wxColour(255, 255, 255);  // Default to white
}

} // namespace

// =============================================================================
// Enum Conversion Functions
// =============================================================================

std::string SslModeToString(SslMode mode) {
    switch (mode) {
        case SslMode::Disable: return "disable";
        case SslMode::Allow: return "allow";
        case SslMode::Prefer: return "prefer";
        case SslMode::Require: return "require";
        case SslMode::VerifyCA: return "verify-ca";
        case SslMode::VerifyFull: return "verify-full";
        default: return "prefer";
    }
}

SslMode SslModeFromString(const std::string& str) {
    if (str == "disable") return SslMode::Disable;
    if (str == "allow") return SslMode::Allow;
    if (str == "prefer") return SslMode::Prefer;
    if (str == "require") return SslMode::Require;
    if (str == "verify-ca") return SslMode::VerifyCA;
    if (str == "verify-full") return SslMode::VerifyFull;
    return SslMode::Prefer;
}

std::string CsvDelimiterToString(CsvDelimiter delimiter) {
    switch (delimiter) {
        case CsvDelimiter::Comma: return "comma";
        case CsvDelimiter::Semicolon: return "semicolon";
        case CsvDelimiter::Tab: return "tab";
        default: return "comma";
    }
}

CsvDelimiter CsvDelimiterFromString(const std::string& str) {
    if (str == "semicolon") return CsvDelimiter::Semicolon;
    if (str == "tab") return CsvDelimiter::Tab;
    return CsvDelimiter::Comma;
}

std::string DiagramNotationToString(DiagramNotation notation) {
    switch (notation) {
        case DiagramNotation::CrowsFoot: return "crowsfoot";
        case DiagramNotation::Bachman: return "bachman";
        case DiagramNotation::UML: return "uml";
        case DiagramNotation::IDEF1X: return "idef1x";
        default: return "crowsfoot";
    }
}

DiagramNotation DiagramNotationFromString(const std::string& str) {
    if (str == "bachman") return DiagramNotation::Bachman;
    if (str == "uml") return DiagramNotation::UML;
    if (str == "idef1x") return DiagramNotation::IDEF1X;
    return DiagramNotation::CrowsFoot;
}

// =============================================================================
// Preferences Load/Save
// =============================================================================

bool PreferencesDialog::LoadPreferences(ApplicationPreferences* out_prefs) {
    if (!out_prefs) return false;

    wxConfig config(kConfigAppName, kConfigVendorName);

    // Editor preferences
    wxString fontFamilyStr;
    config.Read(kKeyEditorFontFamily, &fontFamilyStr, "Consolas");
    out_prefs->editor.font_family = fontFamilyStr.ToStdString();
    config.Read(kKeyEditorFontSize, &out_prefs->editor.font_size, 11);
    
    wxString bgColorStr, fgColorStr, selColorStr;
    config.Read(kKeyEditorBgColor, &bgColorStr, "255,255,255");
    config.Read(kKeyEditorFgColor, &fgColorStr, "0,0,0");
    config.Read(kKeyEditorSelectionColor, &selColorStr, "51,153,255");
    out_prefs->editor.background_color = StringToColor(bgColorStr);
    out_prefs->editor.foreground_color = StringToColor(fgColorStr);
    out_prefs->editor.selection_color = StringToColor(selColorStr);
    
    config.Read(kKeyEditorTabWidth, &out_prefs->editor.tab_width, 4);
    config.Read(kKeyEditorUseSpaces, &out_prefs->editor.use_spaces_for_tabs, true);
    config.Read(kKeyEditorWordWrap, &out_prefs->editor.word_wrap, false);
    config.Read(kKeyEditorLineNumbers, &out_prefs->editor.show_line_numbers, true);
    config.Read(kKeyEditorAutoIndent, &out_prefs->editor.auto_indent, true);

    // Results preferences
    wxString nullDisplayStr, dateTimeFormatStr;
    config.Read(kKeyResultsRowLimit, &out_prefs->results.default_row_limit, 1000);
    config.Read(kKeyResultsNullDisplay, &nullDisplayStr, "<NULL>");
    out_prefs->results.null_display = nullDisplayStr.ToStdString();
    config.Read(kKeyResultsDateTimeFormat, &dateTimeFormatStr, "%Y-%m-%d %H:%M:%S");
    out_prefs->results.date_time_format = dateTimeFormatStr.ToStdString();
    config.Read(kKeyResultsGridLines, &out_prefs->results.show_grid_lines, true);

    // Connection preferences
    config.Read(kKeyConnTimeout, &out_prefs->connection.connect_timeout_seconds, 30);
    config.Read(kKeyQueryTimeout, &out_prefs->connection.query_timeout_seconds, 0);
    
    wxString sslModeStr;
    config.Read(kKeySslMode, &sslModeStr, "prefer");
    out_prefs->connection.ssl_mode = SslModeFromString(sslModeStr.ToStdString());
    
    config.Read(kKeyAutoReconnect, &out_prefs->connection.auto_reconnect, true);
    config.Read(kKeyKeepAlive, &out_prefs->connection.keep_alive_interval_seconds, 60);

    // Export preferences
    wxString delimiterStr;
    config.Read(kKeyExportCsvDelimiter, &delimiterStr, "comma");
    out_prefs->export_prefs.csv_delimiter = CsvDelimiterFromString(delimiterStr.ToStdString());
    
    wxString quoteStr;
    config.Read(kKeyExportCsvQuote, &quoteStr, "\"");
    out_prefs->export_prefs.csv_quote_char = quoteStr.IsEmpty() ? '"' : quoteStr[0];
    
    config.Read(kKeyExportIncludeHeaders, &out_prefs->export_prefs.include_headers, true);
    wxString dateFormatStr;
    config.Read(kKeyExportDateFormat, &dateFormatStr, "%Y-%m-%d");
    out_prefs->export_prefs.date_format = dateFormatStr.ToStdString();

    // Diagram preferences
    wxString notationStr;
    config.Read(kKeyDiagramNotation, &notationStr, "crowsfoot");
    out_prefs->diagram.default_notation = DiagramNotationFromString(notationStr.ToStdString());
    
    config.Read(kKeyDiagramGridSize, &out_prefs->diagram.grid_size, 10);
    config.Read(kKeyDiagramSnapToGrid, &out_prefs->diagram.snap_to_grid, true);
    config.Read(kKeyDiagramPaperSize, &out_prefs->diagram.default_paper_size, 0);

    // Network preferences
    wxString httpProxyHost, socksProxyHost;
    config.Read(kKeyNetworkHttpProxyHost, &httpProxyHost, "");
    out_prefs->network.http_proxy_host = httpProxyHost.ToStdString();
    config.Read(kKeyNetworkHttpProxyPort, &out_prefs->network.http_proxy_port, 8080);
    config.Read(kKeyNetworkSocksProxyHost, &socksProxyHost, "");
    out_prefs->network.socks_proxy_host = socksProxyHost.ToStdString();
    config.Read(kKeyNetworkSocksProxyPort, &out_prefs->network.socks_proxy_port, 1080);
    
    wxString noProxyStr;
    config.Read(kKeyNetworkNoProxy, &noProxyStr, "");
    out_prefs->network.no_proxy_domains = ParseNoProxyList(noProxyStr.ToStdString());

    return true;
}

bool PreferencesDialog::SavePreferences(const ApplicationPreferences& prefs) {
    wxConfig config(kConfigAppName, kConfigVendorName);

    // Editor preferences
    config.Write(kKeyEditorFontFamily, wxString(prefs.editor.font_family));
    config.Write(kKeyEditorFontSize, prefs.editor.font_size);
    config.Write(kKeyEditorBgColor, wxString(ColorToString(prefs.editor.background_color)));
    config.Write(kKeyEditorFgColor, wxString(ColorToString(prefs.editor.foreground_color)));
    config.Write(kKeyEditorSelectionColor, wxString(ColorToString(prefs.editor.selection_color)));
    config.Write(kKeyEditorTabWidth, prefs.editor.tab_width);
    config.Write(kKeyEditorUseSpaces, prefs.editor.use_spaces_for_tabs);
    config.Write(kKeyEditorWordWrap, prefs.editor.word_wrap);
    config.Write(kKeyEditorLineNumbers, prefs.editor.show_line_numbers);
    config.Write(kKeyEditorAutoIndent, prefs.editor.auto_indent);

    // Results preferences
    config.Write(kKeyResultsRowLimit, prefs.results.default_row_limit);
    config.Write(kKeyResultsNullDisplay, wxString(prefs.results.null_display));
    config.Write(kKeyResultsDateTimeFormat, wxString(prefs.results.date_time_format));
    config.Write(kKeyResultsGridLines, prefs.results.show_grid_lines);

    // Connection preferences
    config.Write(kKeyConnTimeout, prefs.connection.connect_timeout_seconds);
    config.Write(kKeyQueryTimeout, prefs.connection.query_timeout_seconds);
    config.Write(kKeySslMode, wxString(SslModeToString(prefs.connection.ssl_mode)));
    config.Write(kKeyAutoReconnect, prefs.connection.auto_reconnect);
    config.Write(kKeyKeepAlive, prefs.connection.keep_alive_interval_seconds);

    // Export preferences
    config.Write(kKeyExportCsvDelimiter, wxString(CsvDelimiterToString(prefs.export_prefs.csv_delimiter)));
    config.Write(kKeyExportCsvQuote, wxString(prefs.export_prefs.csv_quote_char));
    config.Write(kKeyExportIncludeHeaders, prefs.export_prefs.include_headers);
    config.Write(kKeyExportDateFormat, wxString(prefs.export_prefs.date_format));

    // Diagram preferences
    config.Write(kKeyDiagramNotation, wxString(DiagramNotationToString(prefs.diagram.default_notation)));
    config.Write(kKeyDiagramGridSize, prefs.diagram.grid_size);
    config.Write(kKeyDiagramSnapToGrid, prefs.diagram.snap_to_grid);
    config.Write(kKeyDiagramPaperSize, prefs.diagram.default_paper_size);

    // Network preferences
    config.Write(kKeyNetworkHttpProxyHost, wxString(prefs.network.http_proxy_host));
    config.Write(kKeyNetworkHttpProxyPort, prefs.network.http_proxy_port);
    config.Write(kKeyNetworkSocksProxyHost, wxString(prefs.network.socks_proxy_host));
    config.Write(kKeyNetworkSocksProxyPort, prefs.network.socks_proxy_port);
    config.Write(kKeyNetworkNoProxy, wxString(JoinNoProxyList(prefs.network.no_proxy_domains)));

    config.Flush();
    return true;
}

std::vector<std::string> PreferencesDialog::ParseNoProxyList(const std::string& text) {
    std::vector<std::string> result;
    std::stringstream ss(text);
    std::string item;
    while (std::getline(ss, item, ',')) {
        // Trim whitespace
        size_t start = item.find_first_not_of(" \t");
        size_t end = item.find_last_not_of(" \t");
        if (start != std::string::npos && end != std::string::npos) {
            result.push_back(item.substr(start, end - start + 1));
        }
    }
    return result;
}

std::string PreferencesDialog::JoinNoProxyList(const std::vector<std::string>& domains) {
    std::string result;
    for (size_t i = 0; i < domains.size(); ++i) {
        if (i > 0) result += ", ";
        result += domains[i];
    }
    return result;
}

// =============================================================================
// PreferencesDialog Implementation
// =============================================================================

wxBEGIN_EVENT_TABLE(PreferencesDialog, wxDialog)
    EVT_BUTTON(wxID_OK, PreferencesDialog::OnOk)
    EVT_BUTTON(wxID_APPLY, PreferencesDialog::OnApply)
wxEND_EVENT_TABLE()

PreferencesDialog::PreferencesDialog(wxWindow* parent, ApplicationPreferences& prefs)
    : wxDialog(parent, wxID_ANY, "Preferences",
               wxDefaultPosition, wxSize(650, 550)),
      prefs_(prefs),
      original_prefs_(prefs) {
    BuildLayout();
    LoadValues();
    
    // Center on parent
    CentreOnParent();
}

void PreferencesDialog::BuildLayout() {
    auto* rootSizer = new wxBoxSizer(wxVERTICAL);

    // Notebook for tabs
    auto* notebook = new wxNotebook(this, wxID_ANY);

    BuildEditorTab(notebook);
    BuildResultsTab(notebook);
    BuildConnectionTab(notebook);
    BuildExportTab(notebook);
    BuildDiagramTab(notebook);
    BuildNetworkTab(notebook);

    rootSizer->Add(notebook, 1, wxEXPAND | wxALL, 10);

    // Button row
    auto* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
    
    // Reset defaults button (left side)
    buttonSizer->Add(new wxButton(this, wxID_RESET, "Reset to Defaults"), 0, wxRIGHT, 20);
    
    buttonSizer->AddStretchSpacer();
    
    // Standard buttons
    buttonSizer->Add(new wxButton(this, wxID_CANCEL, "Cancel"), 0, wxRIGHT, 5);
    buttonSizer->Add(new wxButton(this, wxID_APPLY, "Apply"), 0, wxRIGHT, 5);
    buttonSizer->Add(new wxButton(this, wxID_OK, "OK"), 0);
    
    rootSizer->Add(buttonSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

    SetSizer(rootSizer);
    
    // Set OK as default
    SetAffirmativeId(wxID_OK);
}

void PreferencesDialog::BuildEditorTab(wxNotebook* notebook) {
    auto* panel = new wxPanel(notebook, wxID_ANY);
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Font section
    auto* fontBox = new wxStaticBoxSizer(wxVERTICAL, panel, "Font");
    
    font_picker_ = new wxFontPickerCtrl(panel, wxID_ANY);
    fontBox->Add(font_picker_, 0, wxEXPAND | wxALL, 8);
    
    mainSizer->Add(fontBox, 0, wxEXPAND | wxALL, 8);

    // Colors section
    auto* colorBox = new wxStaticBoxSizer(wxVERTICAL, panel, "Colors");
    auto* colorSizer = new wxFlexGridSizer(2, 8, 8);
    colorSizer->AddGrowableCol(1);
    
    colorSizer->Add(new wxStaticText(panel, wxID_ANY, "Background:"), 0, wxALIGN_CENTER_VERTICAL);
    bg_color_picker_ = new wxColourPickerCtrl(panel, wxID_ANY);
    colorSizer->Add(bg_color_picker_, 0, wxEXPAND);
    
    colorSizer->Add(new wxStaticText(panel, wxID_ANY, "Foreground:"), 0, wxALIGN_CENTER_VERTICAL);
    fg_color_picker_ = new wxColourPickerCtrl(panel, wxID_ANY);
    colorSizer->Add(fg_color_picker_, 0, wxEXPAND);
    
    colorSizer->Add(new wxStaticText(panel, wxID_ANY, "Selection:"), 0, wxALIGN_CENTER_VERTICAL);
    selection_color_picker_ = new wxColourPickerCtrl(panel, wxID_ANY);
    colorSizer->Add(selection_color_picker_, 0, wxEXPAND);
    
    colorBox->Add(colorSizer, 0, wxEXPAND | wxALL, 8);
    mainSizer->Add(colorBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    // Tab settings section
    auto* tabBox = new wxStaticBoxSizer(wxVERTICAL, panel, "Tab Settings");
    auto* tabSizer = new wxFlexGridSizer(2, 8, 8);
    tabSizer->AddGrowableCol(1);
    
    tabSizer->Add(new wxStaticText(panel, wxID_ANY, "Tab Width:"), 0, wxALIGN_CENTER_VERTICAL);
    tab_width_spin_ = new wxSpinCtrl(panel, wxID_ANY, wxEmptyString,
                                     wxDefaultPosition, wxSize(80, -1));
    tab_width_spin_->SetRange(1, 16);
    tabSizer->Add(tab_width_spin_, 0);
    
    tabSizer->AddSpacer(0);
    use_spaces_chk_ = new wxCheckBox(panel, wxID_ANY, "Use spaces instead of tabs");
    tabSizer->Add(use_spaces_chk_, 0);
    
    tabBox->Add(tabSizer, 0, wxEXPAND | wxALL, 8);
    mainSizer->Add(tabBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    // Editor behavior section
    auto* behaviorBox = new wxStaticBoxSizer(wxVERTICAL, panel, "Editor Behavior");
    
    word_wrap_chk_ = new wxCheckBox(panel, wxID_ANY, "Enable word wrap");
    behaviorBox->Add(word_wrap_chk_, 0, wxALL, 4);
    
    line_numbers_chk_ = new wxCheckBox(panel, wxID_ANY, "Show line numbers");
    behaviorBox->Add(line_numbers_chk_, 0, wxALL, 4);
    
    auto_indent_chk_ = new wxCheckBox(panel, wxID_ANY, "Auto-indent");
    behaviorBox->Add(auto_indent_chk_, 0, wxALL, 4);
    
    mainSizer->Add(behaviorBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    mainSizer->AddStretchSpacer();

    panel->SetSizer(mainSizer);
    notebook->AddPage(panel, "Editor");
}

void PreferencesDialog::BuildResultsTab(wxNotebook* notebook) {
    auto* panel = new wxPanel(notebook, wxID_ANY);
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Grid display section
    auto* gridBox = new wxStaticBoxSizer(wxVERTICAL, panel, "Grid Display");
    auto* gridSizer = new wxFlexGridSizer(2, 8, 8);
    gridSizer->AddGrowableCol(1);
    
    gridSizer->Add(new wxStaticText(panel, wxID_ANY, "Default Row Limit:"), 0, wxALIGN_CENTER_VERTICAL);
    row_limit_spin_ = new wxSpinCtrl(panel, wxID_ANY, wxEmptyString,
                                     wxDefaultPosition, wxSize(100, -1));
    row_limit_spin_->SetRange(10, 100000);
    gridSizer->Add(row_limit_spin_, 0);
    
    gridSizer->Add(new wxStaticText(panel, wxID_ANY, "NULL Display:"), 0, wxALIGN_CENTER_VERTICAL);
    null_display_ctrl_ = new wxTextCtrl(panel, wxID_ANY);
    null_display_ctrl_->SetHint("Text to display for NULL values");
    gridSizer->Add(null_display_ctrl_, 0, wxEXPAND);
    
    gridSizer->Add(new wxStaticText(panel, wxID_ANY, "Date/Time Format:"), 0, wxALIGN_CENTER_VERTICAL);
    date_time_format_ctrl_ = new wxTextCtrl(panel, wxID_ANY);
    date_time_format_ctrl_->SetHint("e.g., %Y-%m-%d %H:%M:%S");
    gridSizer->Add(date_time_format_ctrl_, 0, wxEXPAND);
    
    gridBox->Add(gridSizer, 0, wxEXPAND | wxALL, 8);
    
    // Add format help text
    auto* formatHelp = new wxStaticText(panel, wxID_ANY,
        "Format codes: %Y=year, %m=month, %d=day, %H=hour, %M=minute, %S=second");
    gridBox->Add(formatHelp, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
    
    mainSizer->Add(gridBox, 0, wxEXPAND | wxALL, 8);

    // Appearance section
    auto* appearanceBox = new wxStaticBoxSizer(wxVERTICAL, panel, "Appearance");
    
    grid_lines_chk_ = new wxCheckBox(panel, wxID_ANY, "Show grid lines");
    appearanceBox->Add(grid_lines_chk_, 0, wxALL, 8);
    
    mainSizer->Add(appearanceBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    mainSizer->AddStretchSpacer();

    panel->SetSizer(mainSizer);
    notebook->AddPage(panel, "Results");
}

void PreferencesDialog::BuildConnectionTab(wxNotebook* notebook) {
    auto* panel = new wxPanel(notebook, wxID_ANY);
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Timeouts section
    auto* timeoutBox = new wxStaticBoxSizer(wxVERTICAL, panel, "Timeouts");
    auto* timeoutSizer = new wxFlexGridSizer(2, 8, 8);
    timeoutSizer->AddGrowableCol(1);
    
    timeoutSizer->Add(new wxStaticText(panel, wxID_ANY, "Connect Timeout (seconds):"), 
                      0, wxALIGN_CENTER_VERTICAL);
    connect_timeout_spin_ = new wxSpinCtrl(panel, wxID_ANY, wxEmptyString,
                                           wxDefaultPosition, wxSize(80, -1));
    connect_timeout_spin_->SetRange(1, 600);
    timeoutSizer->Add(connect_timeout_spin_, 0);
    
    timeoutSizer->Add(new wxStaticText(panel, wxID_ANY, "Query Timeout (seconds):"), 
                      0, wxALIGN_CENTER_VERTICAL);
    query_timeout_spin_ = new wxSpinCtrl(panel, wxID_ANY, wxEmptyString,
                                         wxDefaultPosition, wxSize(80, -1));
    query_timeout_spin_->SetRange(0, 3600);
    timeoutSizer->Add(query_timeout_spin_, 0);
    
    auto* timeoutHelp = new wxStaticText(panel, wxID_ANY, "(0 = no timeout)");
    timeoutSizer->AddSpacer(0);
    timeoutSizer->Add(timeoutHelp, 0);
    
    timeoutBox->Add(timeoutSizer, 0, wxEXPAND | wxALL, 8);
    mainSizer->Add(timeoutBox, 0, wxEXPAND | wxALL, 8);

    // SSL section
    auto* sslBox = new wxStaticBoxSizer(wxVERTICAL, panel, "SSL/TLS");
    auto* sslSizer = new wxFlexGridSizer(2, 8, 8);
    sslSizer->AddGrowableCol(1);
    
    sslSizer->Add(new wxStaticText(panel, wxID_ANY, "SSL Mode:"), 0, wxALIGN_CENTER_VERTICAL);
    ssl_mode_choice_ = new wxChoice(panel, wxID_ANY);
    ssl_mode_choice_->Append("Disable - Never use SSL");
    ssl_mode_choice_->Append("Allow - Use SSL if server requires it");
    ssl_mode_choice_->Append("Prefer - Use SSL if available (default)");
    ssl_mode_choice_->Append("Require - Always use SSL");
    ssl_mode_choice_->Append("Verify CA - Verify server certificate");
    ssl_mode_choice_->Append("Verify Full - Verify CA and hostname");
    sslSizer->Add(ssl_mode_choice_, 0, wxEXPAND);
    
    sslBox->Add(sslSizer, 0, wxEXPAND | wxALL, 8);
    mainSizer->Add(sslBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    // Connection behavior section
    auto* behaviorBox = new wxStaticBoxSizer(wxVERTICAL, panel, "Connection Behavior");
    auto* behaviorSizer = new wxFlexGridSizer(2, 8, 8);
    behaviorSizer->AddGrowableCol(1);
    
    auto_reconnect_chk_ = new wxCheckBox(panel, wxID_ANY, "Auto-reconnect on connection loss");
    behaviorBox->Add(auto_reconnect_chk_, 0, wxALL, 8);
    
    behaviorSizer->Add(new wxStaticText(panel, wxID_ANY, "Keep-Alive Interval (seconds):"), 
                       0, wxALIGN_CENTER_VERTICAL);
    keep_alive_spin_ = new wxSpinCtrl(panel, wxID_ANY, wxEmptyString,
                                      wxDefaultPosition, wxSize(80, -1));
    keep_alive_spin_->SetRange(0, 3600);
    behaviorSizer->Add(keep_alive_spin_, 0);
    
    behaviorBox->Add(behaviorSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    mainSizer->Add(behaviorBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    mainSizer->AddStretchSpacer();

    panel->SetSizer(mainSizer);
    notebook->AddPage(panel, "Connection");
}

void PreferencesDialog::BuildExportTab(wxNotebook* notebook) {
    auto* panel = new wxPanel(notebook, wxID_ANY);
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);

    // CSV section
    auto* csvBox = new wxStaticBoxSizer(wxVERTICAL, panel, "CSV Export Options");
    auto* csvSizer = new wxFlexGridSizer(2, 8, 8);
    csvSizer->AddGrowableCol(1);
    
    csvSizer->Add(new wxStaticText(panel, wxID_ANY, "Delimiter:"), 0, wxALIGN_CENTER_VERTICAL);
    csv_delimiter_choice_ = new wxChoice(panel, wxID_ANY);
    csv_delimiter_choice_->Append("Comma (, )");
    csv_delimiter_choice_->Append("Semicolon (;)");
    csv_delimiter_choice_->Append("Tab");
    csvSizer->Add(csv_delimiter_choice_, 0);
    
    csvSizer->Add(new wxStaticText(panel, wxID_ANY, "Quote Character:"), 0, wxALIGN_CENTER_VERTICAL);
    csv_quote_ctrl_ = new wxTextCtrl(panel, wxID_ANY, wxEmptyString,
                                     wxDefaultPosition, wxSize(50, -1));
    csvSizer->Add(csv_quote_ctrl_, 0);
    
    csvBox->Add(csvSizer, 0, wxEXPAND | wxALL, 8);
    
    include_headers_chk_ = new wxCheckBox(panel, wxID_ANY, "Include column headers in export");
    csvBox->Add(include_headers_chk_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
    
    mainSizer->Add(csvBox, 0, wxEXPAND | wxALL, 8);

    // Date format section
    auto* dateBox = new wxStaticBoxSizer(wxVERTICAL, panel, "Date/Time Format");
    auto* dateSizer = new wxFlexGridSizer(2, 8, 8);
    dateSizer->AddGrowableCol(1);
    
    dateSizer->Add(new wxStaticText(panel, wxID_ANY, "Export Date Format:"), 0, wxALIGN_CENTER_VERTICAL);
    export_date_format_ctrl_ = new wxTextCtrl(panel, wxID_ANY);
    export_date_format_ctrl_->SetHint("e.g., %Y-%m-%d");
    dateSizer->Add(export_date_format_ctrl_, 0, wxEXPAND);
    
    dateBox->Add(dateSizer, 0, wxEXPAND | wxALL, 8);
    
    auto* dateHelp = new wxStaticText(panel, wxID_ANY,
        "Format codes: %Y=year, %m=month, %d=day, %H=hour, %M=minute, %S=second");
    dateBox->Add(dateHelp, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
    
    mainSizer->Add(dateBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    mainSizer->AddStretchSpacer();

    panel->SetSizer(mainSizer);
    notebook->AddPage(panel, "Export");
}

void PreferencesDialog::BuildDiagramTab(wxNotebook* notebook) {
    auto* panel = new wxPanel(notebook, wxID_ANY);
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);

    // Notation section
    auto* notationBox = new wxStaticBoxSizer(wxVERTICAL, panel, "Notation");
    auto* notationSizer = new wxFlexGridSizer(2, 8, 8);
    notationSizer->AddGrowableCol(1);
    
    notationSizer->Add(new wxStaticText(panel, wxID_ANY, "Default Notation:"), 0, wxALIGN_CENTER_VERTICAL);
    notation_choice_ = new wxChoice(panel, wxID_ANY);
    notation_choice_->Append("Crow's Foot (IE notation)");
    notation_choice_->Append("Bachman");
    notation_choice_->Append("UML Class Diagram");
    notation_choice_->Append("IDEF1X");
    notationSizer->Add(notation_choice_, 0, wxEXPAND);
    
    notationBox->Add(notationSizer, 0, wxEXPAND | wxALL, 8);
    mainSizer->Add(notationBox, 0, wxEXPAND | wxALL, 8);

    // Grid section
    auto* gridBox = new wxStaticBoxSizer(wxVERTICAL, panel, "Grid");
    auto* gridSizer = new wxFlexGridSizer(2, 8, 8);
    gridSizer->AddGrowableCol(1);
    
    gridSizer->Add(new wxStaticText(panel, wxID_ANY, "Grid Size (pixels):"), 0, wxALIGN_CENTER_VERTICAL);
    grid_size_spin_ = new wxSpinCtrl(panel, wxID_ANY, wxEmptyString,
                                     wxDefaultPosition, wxSize(80, -1));
    grid_size_spin_->SetRange(1, 100);
    gridSizer->Add(grid_size_spin_, 0);
    
    gridBox->Add(gridSizer, 0, wxEXPAND | wxALL, 8);
    
    snap_to_grid_chk_ = new wxCheckBox(panel, wxID_ANY, "Snap to grid");
    gridBox->Add(snap_to_grid_chk_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
    
    mainSizer->Add(gridBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    // Page section
    auto* pageBox = new wxStaticBoxSizer(wxVERTICAL, panel, "Page Setup");
    auto* pageSizer = new wxFlexGridSizer(2, 8, 8);
    pageSizer->AddGrowableCol(1);
    
    pageSizer->Add(new wxStaticText(panel, wxID_ANY, "Default Paper Size:"), 0, wxALIGN_CENTER_VERTICAL);
    paper_size_choice_ = new wxChoice(panel, wxID_ANY);
    paper_size_choice_->Append("A4");
    paper_size_choice_->Append("Letter");
    paper_size_choice_->Append("A3");
    paper_size_choice_->Append("A2");
    paper_size_choice_->Append("A1");
    pageSizer->Add(paper_size_choice_, 0, wxEXPAND);
    
    pageBox->Add(pageSizer, 0, wxEXPAND | wxALL, 8);
    mainSizer->Add(pageBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    mainSizer->AddStretchSpacer();

    panel->SetSizer(mainSizer);
    notebook->AddPage(panel, "Diagram");
}

void PreferencesDialog::BuildNetworkTab(wxNotebook* notebook) {
    auto* panel = new wxPanel(notebook, wxID_ANY);
    auto* mainSizer = new wxBoxSizer(wxVERTICAL);

    // HTTP Proxy section
    auto* httpBox = new wxStaticBoxSizer(wxVERTICAL, panel, "HTTP Proxy");
    auto* httpSizer = new wxFlexGridSizer(2, 8, 8);
    httpSizer->AddGrowableCol(1);
    
    httpSizer->Add(new wxStaticText(panel, wxID_ANY, "Host:"), 0, wxALIGN_CENTER_VERTICAL);
    http_proxy_host_ctrl_ = new wxTextCtrl(panel, wxID_ANY);
    http_proxy_host_ctrl_->SetHint("proxy.example.com (leave empty for no proxy)");
    httpSizer->Add(http_proxy_host_ctrl_, 0, wxEXPAND);
    
    httpSizer->Add(new wxStaticText(panel, wxID_ANY, "Port:"), 0, wxALIGN_CENTER_VERTICAL);
    http_proxy_port_spin_ = new wxSpinCtrl(panel, wxID_ANY, wxEmptyString,
                                           wxDefaultPosition, wxSize(80, -1));
    http_proxy_port_spin_->SetRange(1, 65535);
    httpSizer->Add(http_proxy_port_spin_, 0);
    
    httpBox->Add(httpSizer, 0, wxEXPAND | wxALL, 8);
    mainSizer->Add(httpBox, 0, wxEXPAND | wxALL, 8);

    // SOCKS Proxy section
    auto* socksBox = new wxStaticBoxSizer(wxVERTICAL, panel, "SOCKS Proxy");
    auto* socksSizer = new wxFlexGridSizer(2, 8, 8);
    socksSizer->AddGrowableCol(1);
    
    socksSizer->Add(new wxStaticText(panel, wxID_ANY, "Host:"), 0, wxALIGN_CENTER_VERTICAL);
    socks_proxy_host_ctrl_ = new wxTextCtrl(panel, wxID_ANY);
    socks_proxy_host_ctrl_->SetHint("socks.example.com (leave empty for no proxy)");
    socksSizer->Add(socks_proxy_host_ctrl_, 0, wxEXPAND);
    
    socksSizer->Add(new wxStaticText(panel, wxID_ANY, "Port:"), 0, wxALIGN_CENTER_VERTICAL);
    socks_proxy_port_spin_ = new wxSpinCtrl(panel, wxID_ANY, wxEmptyString,
                                            wxDefaultPosition, wxSize(80, -1));
    socks_proxy_port_spin_->SetRange(1, 65535);
    socksSizer->Add(socks_proxy_port_spin_, 0);
    
    socksBox->Add(socksSizer, 0, wxEXPAND | wxALL, 8);
    mainSizer->Add(socksBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

    // No Proxy section
    auto* noProxyBox = new wxStaticBoxSizer(wxVERTICAL, panel, "No Proxy Domains");
    
    no_proxy_ctrl_ = new wxTextCtrl(panel, wxID_ANY);
    no_proxy_ctrl_->SetHint("example.com, localhost, 192.168.1.0/24 (comma-separated)");
    noProxyBox->Add(no_proxy_ctrl_, 0, wxEXPAND | wxALL, 8);
    
    auto* noProxyHelp = new wxStaticText(panel, wxID_ANY,
        "Enter domains, hostnames, or IP ranges that should bypass the proxy. Separate with commas.");
    noProxyBox->Add(noProxyHelp, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);
    
    mainSizer->Add(noProxyBox, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);
    mainSizer->AddStretchSpacer();

    panel->SetSizer(mainSizer);
    notebook->AddPage(panel, "Network");
}

void PreferencesDialog::LoadValues() {
    // Editor tab
    wxFont font(wxSize(0, prefs_.editor.font_size), wxFONTFAMILY_DEFAULT,
                wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, 
                wxString::FromUTF8(prefs_.editor.font_family));
    if (font_picker_) font_picker_->SetSelectedFont(font);
    if (bg_color_picker_) bg_color_picker_->SetColour(prefs_.editor.background_color);
    if (fg_color_picker_) fg_color_picker_->SetColour(prefs_.editor.foreground_color);
    if (selection_color_picker_) selection_color_picker_->SetColour(prefs_.editor.selection_color);
    if (tab_width_spin_) tab_width_spin_->SetValue(prefs_.editor.tab_width);
    if (use_spaces_chk_) use_spaces_chk_->SetValue(prefs_.editor.use_spaces_for_tabs);
    if (word_wrap_chk_) word_wrap_chk_->SetValue(prefs_.editor.word_wrap);
    if (line_numbers_chk_) line_numbers_chk_->SetValue(prefs_.editor.show_line_numbers);
    if (auto_indent_chk_) auto_indent_chk_->SetValue(prefs_.editor.auto_indent);

    // Results tab
    if (row_limit_spin_) row_limit_spin_->SetValue(prefs_.results.default_row_limit);
    if (null_display_ctrl_) null_display_ctrl_->SetValue(wxString::FromUTF8(prefs_.results.null_display));
    if (date_time_format_ctrl_) date_time_format_ctrl_->SetValue(wxString::FromUTF8(prefs_.results.date_time_format));
    if (grid_lines_chk_) grid_lines_chk_->SetValue(prefs_.results.show_grid_lines);

    // Connection tab
    if (connect_timeout_spin_) connect_timeout_spin_->SetValue(prefs_.connection.connect_timeout_seconds);
    if (query_timeout_spin_) query_timeout_spin_->SetValue(prefs_.connection.query_timeout_seconds);
    if (ssl_mode_choice_) {
        int sslIndex = 0;
        switch (prefs_.connection.ssl_mode) {
            case SslMode::Disable: sslIndex = 0; break;
            case SslMode::Allow: sslIndex = 1; break;
            case SslMode::Prefer: sslIndex = 2; break;
            case SslMode::Require: sslIndex = 3; break;
            case SslMode::VerifyCA: sslIndex = 4; break;
            case SslMode::VerifyFull: sslIndex = 5; break;
        }
        ssl_mode_choice_->SetSelection(sslIndex);
    }
    if (auto_reconnect_chk_) auto_reconnect_chk_->SetValue(prefs_.connection.auto_reconnect);
    if (keep_alive_spin_) keep_alive_spin_->SetValue(prefs_.connection.keep_alive_interval_seconds);

    // Export tab
    if (csv_delimiter_choice_) {
        int delimIndex = 0;
        switch (prefs_.export_prefs.csv_delimiter) {
            case CsvDelimiter::Comma: delimIndex = 0; break;
            case CsvDelimiter::Semicolon: delimIndex = 1; break;
            case CsvDelimiter::Tab: delimIndex = 2; break;
        }
        csv_delimiter_choice_->SetSelection(delimIndex);
    }
    if (csv_quote_ctrl_) {
        wxString quoteStr;
        quoteStr.Append(prefs_.export_prefs.csv_quote_char);
        csv_quote_ctrl_->SetValue(quoteStr);
    }
    if (include_headers_chk_) include_headers_chk_->SetValue(prefs_.export_prefs.include_headers);
    if (export_date_format_ctrl_) export_date_format_ctrl_->SetValue(wxString::FromUTF8(prefs_.export_prefs.date_format));

    // Diagram tab
    if (notation_choice_) {
        int notationIndex = 0;
        switch (prefs_.diagram.default_notation) {
            case DiagramNotation::CrowsFoot: notationIndex = 0; break;
            case DiagramNotation::Bachman: notationIndex = 1; break;
            case DiagramNotation::UML: notationIndex = 2; break;
            case DiagramNotation::IDEF1X: notationIndex = 3; break;
        }
        notation_choice_->SetSelection(notationIndex);
    }
    if (grid_size_spin_) grid_size_spin_->SetValue(prefs_.diagram.grid_size);
    if (snap_to_grid_chk_) snap_to_grid_chk_->SetValue(prefs_.diagram.snap_to_grid);
    if (paper_size_choice_) paper_size_choice_->SetSelection(prefs_.diagram.default_paper_size);

    // Network tab
    if (http_proxy_host_ctrl_) http_proxy_host_ctrl_->SetValue(wxString::FromUTF8(prefs_.network.http_proxy_host));
    if (http_proxy_port_spin_) http_proxy_port_spin_->SetValue(prefs_.network.http_proxy_port);
    if (socks_proxy_host_ctrl_) socks_proxy_host_ctrl_->SetValue(wxString::FromUTF8(prefs_.network.socks_proxy_host));
    if (socks_proxy_port_spin_) socks_proxy_port_spin_->SetValue(prefs_.network.socks_proxy_port);
    if (no_proxy_ctrl_) no_proxy_ctrl_->SetValue(JoinNoProxyList(prefs_.network.no_proxy_domains));
}

void PreferencesDialog::SaveValues() {
    // Editor tab
    if (font_picker_) {
        wxFont font = font_picker_->GetSelectedFont();
        prefs_.editor.font_family = font.GetFaceName().ToUTF8();
        prefs_.editor.font_size = font.GetPointSize();
    }
    if (bg_color_picker_) prefs_.editor.background_color = bg_color_picker_->GetColour();
    if (fg_color_picker_) prefs_.editor.foreground_color = fg_color_picker_->GetColour();
    if (selection_color_picker_) prefs_.editor.selection_color = selection_color_picker_->GetColour();
    if (tab_width_spin_) prefs_.editor.tab_width = tab_width_spin_->GetValue();
    if (use_spaces_chk_) prefs_.editor.use_spaces_for_tabs = use_spaces_chk_->GetValue();
    if (word_wrap_chk_) prefs_.editor.word_wrap = word_wrap_chk_->GetValue();
    if (line_numbers_chk_) prefs_.editor.show_line_numbers = line_numbers_chk_->GetValue();
    if (auto_indent_chk_) prefs_.editor.auto_indent = auto_indent_chk_->GetValue();

    // Results tab
    if (row_limit_spin_) prefs_.results.default_row_limit = row_limit_spin_->GetValue();
    if (null_display_ctrl_) prefs_.results.null_display = null_display_ctrl_->GetValue().ToUTF8();
    if (date_time_format_ctrl_) prefs_.results.date_time_format = date_time_format_ctrl_->GetValue().ToUTF8();
    if (grid_lines_chk_) prefs_.results.show_grid_lines = grid_lines_chk_->GetValue();

    // Connection tab
    if (connect_timeout_spin_) prefs_.connection.connect_timeout_seconds = connect_timeout_spin_->GetValue();
    if (query_timeout_spin_) prefs_.connection.query_timeout_seconds = query_timeout_spin_->GetValue();
    if (ssl_mode_choice_) {
        switch (ssl_mode_choice_->GetSelection()) {
            case 0: prefs_.connection.ssl_mode = SslMode::Disable; break;
            case 1: prefs_.connection.ssl_mode = SslMode::Allow; break;
            case 2: prefs_.connection.ssl_mode = SslMode::Prefer; break;
            case 3: prefs_.connection.ssl_mode = SslMode::Require; break;
            case 4: prefs_.connection.ssl_mode = SslMode::VerifyCA; break;
            case 5: prefs_.connection.ssl_mode = SslMode::VerifyFull; break;
        }
    }
    if (auto_reconnect_chk_) prefs_.connection.auto_reconnect = auto_reconnect_chk_->GetValue();
    if (keep_alive_spin_) prefs_.connection.keep_alive_interval_seconds = keep_alive_spin_->GetValue();

    // Export tab
    if (csv_delimiter_choice_) {
        switch (csv_delimiter_choice_->GetSelection()) {
            case 0: prefs_.export_prefs.csv_delimiter = CsvDelimiter::Comma; break;
            case 1: prefs_.export_prefs.csv_delimiter = CsvDelimiter::Semicolon; break;
            case 2: prefs_.export_prefs.csv_delimiter = CsvDelimiter::Tab; break;
        }
    }
    if (csv_quote_ctrl_) {
        wxString quoteStr = csv_quote_ctrl_->GetValue();
        prefs_.export_prefs.csv_quote_char = quoteStr.IsEmpty() ? '"' : quoteStr[0];
    }
    if (include_headers_chk_) prefs_.export_prefs.include_headers = include_headers_chk_->GetValue();
    if (export_date_format_ctrl_) prefs_.export_prefs.date_format = export_date_format_ctrl_->GetValue().ToUTF8();

    // Diagram tab
    if (notation_choice_) {
        switch (notation_choice_->GetSelection()) {
            case 0: prefs_.diagram.default_notation = DiagramNotation::CrowsFoot; break;
            case 1: prefs_.diagram.default_notation = DiagramNotation::Bachman; break;
            case 2: prefs_.diagram.default_notation = DiagramNotation::UML; break;
            case 3: prefs_.diagram.default_notation = DiagramNotation::IDEF1X; break;
        }
    }
    if (grid_size_spin_) prefs_.diagram.grid_size = grid_size_spin_->GetValue();
    if (snap_to_grid_chk_) prefs_.diagram.snap_to_grid = snap_to_grid_chk_->GetValue();
    if (paper_size_choice_) prefs_.diagram.default_paper_size = paper_size_choice_->GetSelection();

    // Network tab
    if (http_proxy_host_ctrl_) prefs_.network.http_proxy_host = http_proxy_host_ctrl_->GetValue().ToUTF8();
    if (http_proxy_port_spin_) prefs_.network.http_proxy_port = http_proxy_port_spin_->GetValue();
    if (socks_proxy_host_ctrl_) prefs_.network.socks_proxy_host = socks_proxy_host_ctrl_->GetValue().ToUTF8();
    if (socks_proxy_port_spin_) prefs_.network.socks_proxy_port = socks_proxy_port_spin_->GetValue();
    if (no_proxy_ctrl_) {
        prefs_.network.no_proxy_domains = ParseNoProxyList(no_proxy_ctrl_->GetValue().ToStdString());
    }
}

void PreferencesDialog::OnOk(wxCommandEvent& event) {
    SaveValues();
    SavePreferences(prefs_);
    confirmed_ = true;
    event.Skip();
}

void PreferencesDialog::OnApply(wxCommandEvent& event) {
    SaveValues();
    SavePreferences(prefs_);
    // Don't close dialog
}

void PreferencesDialog::OnResetDefaults(wxCommandEvent& event) {
    // Reset to defaults
    prefs_ = ApplicationPreferences();
    LoadValues();
}

} // namespace scratchrobin
