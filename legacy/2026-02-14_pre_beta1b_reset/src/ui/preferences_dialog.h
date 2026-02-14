/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_PREFERENCES_DIALOG_H
#define SCRATCHROBIN_PREFERENCES_DIALOG_H

#include <string>
#include <vector>

#include <wx/dialog.h>
#include <wx/colour.h>

class wxButton;
class wxCheckBox;
class wxChoice;
class wxColourPickerCtrl;
class wxFontPickerCtrl;
class wxNotebook;
class wxSpinCtrl;
class wxTextCtrl;

namespace scratchrobin {

// Editor preferences
struct EditorPreferences {
    // Font settings
    std::string font_family = "Consolas";
    int font_size = 11;
    
    // Colors
    wxColour background_color = wxColour(255, 255, 255);
    wxColour foreground_color = wxColour(0, 0, 0);
    wxColour selection_color = wxColour(51, 153, 255);
    
    // Tab settings
    int tab_width = 4;
    bool use_spaces_for_tabs = true;
    
    // Editor behavior
    bool word_wrap = false;
    bool show_line_numbers = true;
    bool auto_indent = true;
};

// Results/Grid preferences
struct ResultsPreferences {
    int default_row_limit = 1000;
    std::string null_display = "<NULL>";
    std::string date_time_format = "%Y-%m-%d %H:%M:%S";
    bool show_grid_lines = true;
};

// SSL mode for connections
enum class SslMode {
    Disable,
    Allow,
    Prefer,
    Require,
    VerifyCA,
    VerifyFull
};

// Connection preferences
struct ConnectionPreferences {
    int connect_timeout_seconds = 30;
    int query_timeout_seconds = 0;  // 0 = no timeout
    SslMode ssl_mode = SslMode::Prefer;
    bool auto_reconnect = true;
    int keep_alive_interval_seconds = 60;
};

// CSV delimiter type
enum class CsvDelimiter {
    Comma,
    Semicolon,
    Tab
};

// Export preferences
struct ExportPreferences {
    CsvDelimiter csv_delimiter = CsvDelimiter::Comma;
    char csv_quote_char = '"';
    bool include_headers = true;
    std::string date_format = "%Y-%m-%d";
};

// Diagram notation type
enum class DiagramNotation {
    CrowsFoot,
    Bachman,
    UML,
    IDEF1X
};

// Diagram preferences
struct DiagramPreferences {
    DiagramNotation default_notation = DiagramNotation::CrowsFoot;
    int grid_size = 10;
    bool snap_to_grid = true;
    int default_paper_size = 0;  // 0=A4, 1=Letter, 2=A3, etc.
};

// Network/Proxy preferences
struct NetworkPreferences {
    std::string http_proxy_host;
    int http_proxy_port = 8080;
    std::string socks_proxy_host;
    int socks_proxy_port = 1080;
    std::vector<std::string> no_proxy_domains;
};

// Complete application preferences
struct ApplicationPreferences {
    EditorPreferences editor;
    ResultsPreferences results;
    ConnectionPreferences connection;
    ExportPreferences export_prefs;
    DiagramPreferences diagram;
    NetworkPreferences network;
};

// Preferences dialog
class PreferencesDialog : public wxDialog {
public:
    PreferencesDialog(wxWindow* parent, ApplicationPreferences& prefs);

    bool IsConfirmed() const { return confirmed_; }

    // Static helper methods for loading/saving preferences
    static bool LoadPreferences(ApplicationPreferences* out_prefs);
    static bool SavePreferences(const ApplicationPreferences& prefs);

private:
    void BuildLayout();
    void BuildEditorTab(wxNotebook* notebook);
    void BuildResultsTab(wxNotebook* notebook);
    void BuildConnectionTab(wxNotebook* notebook);
    void BuildExportTab(wxNotebook* notebook);
    void BuildDiagramTab(wxNotebook* notebook);
    void BuildNetworkTab(wxNotebook* notebook);

    void LoadValues();
    void SaveValues();

    void OnOk(wxCommandEvent& event);
    void OnApply(wxCommandEvent& event);
    void OnResetDefaults(wxCommandEvent& event);

    // Helper methods
    static std::vector<std::string> ParseNoProxyList(const std::string& text);
    static std::string JoinNoProxyList(const std::vector<std::string>& domains);

    ApplicationPreferences& prefs_;
    ApplicationPreferences original_prefs_;
    bool confirmed_ = false;

    // Editor tab controls
    wxFontPickerCtrl* font_picker_ = nullptr;
    wxColourPickerCtrl* bg_color_picker_ = nullptr;
    wxColourPickerCtrl* fg_color_picker_ = nullptr;
    wxColourPickerCtrl* selection_color_picker_ = nullptr;
    wxSpinCtrl* tab_width_spin_ = nullptr;
    wxCheckBox* use_spaces_chk_ = nullptr;
    wxCheckBox* word_wrap_chk_ = nullptr;
    wxCheckBox* line_numbers_chk_ = nullptr;
    wxCheckBox* auto_indent_chk_ = nullptr;

    // Results tab controls
    wxSpinCtrl* row_limit_spin_ = nullptr;
    wxTextCtrl* null_display_ctrl_ = nullptr;
    wxTextCtrl* date_time_format_ctrl_ = nullptr;
    wxCheckBox* grid_lines_chk_ = nullptr;

    // Connection tab controls
    wxSpinCtrl* connect_timeout_spin_ = nullptr;
    wxSpinCtrl* query_timeout_spin_ = nullptr;
    wxChoice* ssl_mode_choice_ = nullptr;
    wxCheckBox* auto_reconnect_chk_ = nullptr;
    wxSpinCtrl* keep_alive_spin_ = nullptr;

    // Export tab controls
    wxChoice* csv_delimiter_choice_ = nullptr;
    wxTextCtrl* csv_quote_ctrl_ = nullptr;
    wxCheckBox* include_headers_chk_ = nullptr;
    wxTextCtrl* export_date_format_ctrl_ = nullptr;

    // Diagram tab controls
    wxChoice* notation_choice_ = nullptr;
    wxSpinCtrl* grid_size_spin_ = nullptr;
    wxCheckBox* snap_to_grid_chk_ = nullptr;
    wxChoice* paper_size_choice_ = nullptr;

    // Network tab controls
    wxTextCtrl* http_proxy_host_ctrl_ = nullptr;
    wxSpinCtrl* http_proxy_port_spin_ = nullptr;
    wxTextCtrl* socks_proxy_host_ctrl_ = nullptr;
    wxSpinCtrl* socks_proxy_port_spin_ = nullptr;
    wxTextCtrl* no_proxy_ctrl_ = nullptr;

    wxDECLARE_EVENT_TABLE();
};

// Helper functions for enum conversions
std::string SslModeToString(SslMode mode);
SslMode SslModeFromString(const std::string& str);
std::string CsvDelimiterToString(CsvDelimiter delimiter);
CsvDelimiter CsvDelimiterFromString(const std::string& str);
std::string DiagramNotationToString(DiagramNotation notation);
DiagramNotation DiagramNotationFromString(const std::string& str);

} // namespace scratchrobin

#endif // SCRATCHROBIN_PREFERENCES_DIALOG_H
