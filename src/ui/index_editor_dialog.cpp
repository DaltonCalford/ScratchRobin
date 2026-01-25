#include "index_editor_dialog.h"

#include <algorithm>
#include <cctype>
#include <sstream>
#include <vector>

#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/sizer.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>

namespace scratchrobin {
namespace {

std::string Trim(std::string value) {
    auto not_space = [](unsigned char ch) { return !std::isspace(ch); };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), not_space));
    value.erase(std::find_if(value.rbegin(), value.rend(), not_space).base(), value.end());
    return value;
}

bool IsQuotedIdentifier(const std::string& value) {
    return value.size() >= 2 && value.front() == '"' && value.back() == '"';
}

bool IsSimpleIdentifier(const std::string& value) {
    if (value.empty()) {
        return false;
    }
    if (!(std::isalpha(static_cast<unsigned char>(value[0])) || value[0] == '_')) {
        return false;
    }
    for (char ch : value) {
        if (!(std::isalnum(static_cast<unsigned char>(ch)) || ch == '_')) {
            return false;
        }
    }
    return true;
}

std::string QuoteIdentifier(const std::string& value) {
    if (IsSimpleIdentifier(value) || IsQuotedIdentifier(value)) {
        return value;
    }
    std::string out = "\"";
    for (char ch : value) {
        if (ch == '"') {
            out.push_back('"');
        }
        out.push_back(ch);
    }
    out.push_back('"');
    return out;
}

std::vector<std::string> SplitLines(const std::string& value) {
    std::vector<std::string> parts;
    std::stringstream stream(value);
    std::string line;
    while (std::getline(stream, line)) {
        line = Trim(line);
        if (line.empty()) {
            continue;
        }
        if (!line.empty() && line.back() == ',') {
            line.pop_back();
            line = Trim(line);
        }
        if (!line.empty()) {
            parts.push_back(line);
        }
    }
    return parts;
}

wxChoice* BuildChoice(wxWindow* parent, const std::vector<std::string>& options) {
    auto* choice = new wxChoice(parent, wxID_ANY);
    for (const auto& option : options) {
        choice->Append(option);
    }
    if (!options.empty()) {
        choice->SetSelection(0);
    }
    return choice;
}

std::string JoinParts(const std::vector<std::string>& parts) {
    std::ostringstream out;
    for (size_t i = 0; i < parts.size(); ++i) {
        if (i > 0) {
            out << ", ";
        }
        out << parts[i];
    }
    return out.str();
}

} // namespace

IndexEditorDialog::IndexEditorDialog(wxWindow* parent, IndexEditorMode mode)
    : wxDialog(parent, wxID_ANY,
               mode == IndexEditorMode::Create ? "Create Index" : "Alter Index",
               wxDefaultPosition, wxSize(640, 720),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      mode_(mode) {
    auto* rootSizer = new wxBoxSizer(wxVERTICAL);

    auto* nameLabel = new wxStaticText(this, wxID_ANY, "Index Name");
    rootSizer->Add(nameLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    name_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    rootSizer->Add(name_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    auto* tableLabel = new wxStaticText(this, wxID_ANY, "Table Name");
    rootSizer->Add(tableLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    table_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    rootSizer->Add(table_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    if (mode_ == IndexEditorMode::Create) {
        if_not_exists_ctrl_ = new wxCheckBox(this, wxID_ANY, "IF NOT EXISTS");
        rootSizer->Add(if_not_exists_ctrl_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

        unique_ctrl_ = new wxCheckBox(this, wxID_ANY, "UNIQUE");
        rootSizer->Add(unique_ctrl_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

        auto* typeLabel = new wxStaticText(this, wxID_ANY, "Index Type");
        rootSizer->Add(typeLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        type_choice_ = BuildChoice(this, {
            "DEFAULT",
            "BTREE",
            "HASH",
            "GIN",
            "GIST",
            "BRIN",
            "RTREE",
            "SPGIST",
            "FULLTEXT"
        });
        rootSizer->Add(type_choice_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

        auto* columnsLabel = new wxStaticText(this, wxID_ANY, "Index Columns (one per line)");
        rootSizer->Add(columnsLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        columns_ctrl_ = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition,
                                       wxSize(-1, 140), wxTE_MULTILINE);
        columns_ctrl_->SetHint("email\nLOWER(username)");
        rootSizer->Add(columns_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

        auto* includeLabel = new wxStaticText(this, wxID_ANY, "Include Columns (optional)");
        rootSizer->Add(includeLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        include_ctrl_ = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition,
                                       wxSize(-1, 90), wxTE_MULTILINE);
        include_ctrl_->SetHint("created_at\nstatus");
        rootSizer->Add(include_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

        auto* whereLabel = new wxStaticText(this, wxID_ANY, "Where Clause (optional)");
        rootSizer->Add(whereLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        where_ctrl_ = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition,
                                     wxDefaultSize, wxTE_MULTILINE);
        where_ctrl_->SetHint("status = 'ACTIVE'");
        rootSizer->Add(where_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

        auto* optionsLabel = new wxStaticText(this, wxID_ANY, "Index Options (raw)");
        rootSizer->Add(optionsLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        options_ctrl_ = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition,
                                       wxSize(-1, 80), wxTE_MULTILINE);
        options_ctrl_->SetHint("TABLESPACE main_ts\nWITH (fillfactor = 90)");
        rootSizer->Add(options_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    } else {
        if (table_ctrl_) {
            table_ctrl_->Enable(false);
        }

        auto* actionLabel = new wxStaticText(this, wxID_ANY, "Action");
        rootSizer->Add(actionLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        alter_action_choice_ = BuildChoice(this, {
            "RENAME TO",
            "SET TABLESPACE",
            "SET SCHEMA",
            "SET OPTIONS",
            "REBUILD"
        });
        rootSizer->Add(alter_action_choice_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

        alter_value_label_ = new wxStaticText(this, wxID_ANY, "Value");
        rootSizer->Add(alter_value_label_, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        alter_value_ctrl_ = new wxTextCtrl(this, wxID_ANY);
        rootSizer->Add(alter_value_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

        alter_value_label_2_ = new wxStaticText(this, wxID_ANY, "New Name");
        rootSizer->Add(alter_value_label_2_, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        alter_value_ctrl_2_ = new wxTextCtrl(this, wxID_ANY);
        rootSizer->Add(alter_value_ctrl_2_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    }

    rootSizer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 12);
    SetSizerAndFit(rootSizer);
    CentreOnParent();

    if (alter_action_choice_) {
        alter_action_choice_->Bind(wxEVT_CHOICE,
            [this](wxCommandEvent&) { UpdateAlterActionFields(); });
        UpdateAlterActionFields();
    }
}

std::string IndexEditorDialog::BuildSql() const {
    return mode_ == IndexEditorMode::Create ? BuildCreateSql() : BuildAlterSql();
}

std::string IndexEditorDialog::index_name() const {
    return Trim(name_ctrl_ ? name_ctrl_->GetValue().ToStdString() : std::string());
}

void IndexEditorDialog::SetIndexName(const std::string& name) {
    if (name_ctrl_) {
        name_ctrl_->SetValue(name);
        if (mode_ == IndexEditorMode::Alter) {
            name_ctrl_->Enable(false);
        }
    }
}

void IndexEditorDialog::SetTableName(const std::string& name) {
    if (table_ctrl_) {
        table_ctrl_->SetValue(name);
        if (mode_ == IndexEditorMode::Alter) {
            table_ctrl_->Enable(false);
        }
    }
}

std::string IndexEditorDialog::BuildCreateSql() const {
    std::string name = index_name();
    std::string table = Trim(table_ctrl_ ? table_ctrl_->GetValue().ToStdString()
                                         : std::string());
    if (name.empty() || table.empty()) {
        return std::string();
    }

    auto column_lines = SplitLines(columns_ctrl_ ? columns_ctrl_->GetValue().ToStdString()
                                                 : std::string());
    if (column_lines.empty()) {
        return std::string();
    }

    std::ostringstream sql;
    sql << "CREATE ";
    if (unique_ctrl_ && unique_ctrl_->IsChecked()) {
        sql << "UNIQUE ";
    }
    sql << "INDEX ";
    if (if_not_exists_ctrl_ && if_not_exists_ctrl_->IsChecked()) {
        sql << "IF NOT EXISTS ";
    }
    sql << FormatIndexPath(name) << " ON " << FormatTablePath(table) << " ";

    std::string type = type_choice_ ? type_choice_->GetStringSelection().ToStdString()
                                    : std::string();
    if (!type.empty() && type != "DEFAULT") {
        sql << "USING " << type << " ";
    }

    sql << "(" << JoinParts(column_lines) << ")";

    auto include_lines = SplitLines(include_ctrl_ ? include_ctrl_->GetValue().ToStdString()
                                                  : std::string());
    if (!include_lines.empty()) {
        sql << " INCLUDE (" << JoinParts(include_lines) << ")";
    }

    std::string where = Trim(where_ctrl_ ? where_ctrl_->GetValue().ToStdString()
                                         : std::string());
    if (!where.empty()) {
        sql << " WHERE " << where;
    }

    std::string options = Trim(options_ctrl_ ? options_ctrl_->GetValue().ToStdString()
                                             : std::string());
    if (!options.empty()) {
        sql << " " << options;
    }

    sql << ";";
    return sql.str();
}

std::string IndexEditorDialog::BuildAlterSql() const {
    std::string name = index_name();
    if (name.empty()) {
        return std::string();
    }

    std::string action = alter_action_choice_
        ? alter_action_choice_->GetStringSelection().ToStdString()
        : std::string();
    std::string value = Trim(alter_value_ctrl_ ? alter_value_ctrl_->GetValue().ToStdString()
                                               : std::string());
    std::string value2 = Trim(alter_value_ctrl_2_ ? alter_value_ctrl_2_->GetValue().ToStdString()
                                                 : std::string());

    std::ostringstream sql;
    sql << "ALTER INDEX " << FormatIndexPath(name) << " ";

    if (action == "RENAME TO") {
        if (value.empty()) {
            return std::string();
        }
        sql << "RENAME TO " << QuoteIdentifier(value);
    } else if (action == "SET TABLESPACE") {
        if (value.empty()) {
            return std::string();
        }
        sql << "SET TABLESPACE " << value;
    } else if (action == "SET SCHEMA") {
        if (value.empty()) {
            return std::string();
        }
        sql << "SET SCHEMA " << value;
    } else if (action == "SET OPTIONS") {
        if (value.empty()) {
            return std::string();
        }
        sql << "SET " << value;
    } else if (action == "REBUILD") {
        sql << "REBUILD";
    } else {
        return std::string();
    }

    sql << ";";
    return sql.str();
}

std::string IndexEditorDialog::FormatIndexPath(const std::string& value) const {
    std::string trimmed = Trim(value);
    if (trimmed.empty()) {
        return trimmed;
    }
    if (trimmed.find('.') != std::string::npos || trimmed.find('"') != std::string::npos) {
        return trimmed;
    }
    return QuoteIdentifier(trimmed);
}

std::string IndexEditorDialog::FormatTablePath(const std::string& value) const {
    std::string trimmed = Trim(value);
    if (trimmed.empty()) {
        return trimmed;
    }
    if (trimmed.find('.') != std::string::npos || trimmed.find('"') != std::string::npos) {
        return trimmed;
    }
    return QuoteIdentifier(trimmed);
}

void IndexEditorDialog::UpdateAlterActionFields() {
    if (!alter_action_choice_ || !alter_value_label_ || !alter_value_ctrl_ ||
        !alter_value_label_2_ || !alter_value_ctrl_2_) {
        return;
    }

    std::string action = alter_action_choice_->GetStringSelection().ToStdString();
    bool show_second = false;
    bool show_value = true;
    wxString label = "Value";
    wxString hint;

    if (action == "RENAME TO") {
        label = "New Index Name";
    } else if (action == "SET TABLESPACE") {
        label = "Tablespace";
    } else if (action == "SET SCHEMA") {
        label = "Schema Path";
    } else if (action == "SET OPTIONS") {
        label = "Options";
    } else if (action == "REBUILD") {
        show_value = false;
    }

    alter_value_label_->SetLabel(label);
    alter_value_ctrl_->SetHint(hint);
    alter_value_label_2_->Show(show_second);
    alter_value_ctrl_2_->Show(show_second);
    alter_value_label_->Show(show_value);
    alter_value_ctrl_->Show(show_value);

    Layout();
}

} // namespace scratchrobin
