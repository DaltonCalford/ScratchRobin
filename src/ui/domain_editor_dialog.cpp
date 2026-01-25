#include "domain_editor_dialog.h"

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

bool EqualsNoCase(const std::string& left, const std::string& right) {
    if (left.size() != right.size()) {
        return false;
    }
    for (size_t i = 0; i < left.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(left[i])) !=
            std::tolower(static_cast<unsigned char>(right[i]))) {
            return false;
        }
    }
    return true;
}

bool StartsWithNoCase(const std::string& value, const std::string& prefix) {
    if (value.size() < prefix.size()) {
        return false;
    }
    return EqualsNoCase(value.substr(0, prefix.size()), prefix);
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

bool IsQuotedIdentifier(const std::string& value) {
    return value.size() >= 2 && value.front() == '"' && value.back() == '"';
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

} // namespace

DomainEditorDialog::DomainEditorDialog(wxWindow* parent, DomainEditorMode mode)
    : wxDialog(parent, wxID_ANY, mode == DomainEditorMode::Create ? "Create Domain" : "Alter Domain",
               wxDefaultPosition, wxSize(640, 760),
               wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER),
      mode_(mode) {
    auto* rootSizer = new wxBoxSizer(wxVERTICAL);

    auto* nameLabel = new wxStaticText(this, wxID_ANY, "Domain Name (global)");
    rootSizer->Add(nameLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
    name_ctrl_ = new wxTextCtrl(this, wxID_ANY);
    rootSizer->Add(name_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

    if (mode_ == DomainEditorMode::Create) {
        if_not_exists_ctrl_ = new wxCheckBox(this, wxID_ANY, "IF NOT EXISTS");
        rootSizer->Add(if_not_exists_ctrl_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 12);

        auto* kindLabel = new wxStaticText(this, wxID_ANY, "Domain Kind");
        rootSizer->Add(kindLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        kind_choice_ = BuildChoice(this, {"BASIC", "RECORD", "ENUM", "SET", "VARIANT"});
        rootSizer->Add(kind_choice_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

        auto* baseLabel = new wxStaticText(this, wxID_ANY, "Base Type (BASIC)");
        rootSizer->Add(baseLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        base_type_ctrl_ = new wxTextCtrl(this, wxID_ANY);
        base_type_ctrl_->SetHint("VARCHAR(50), INT, TIMESTAMP...");
        rootSizer->Add(base_type_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

        auto* recordLabel = new wxStaticText(this, wxID_ANY, "Record Fields (one per line)");
        rootSizer->Add(recordLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        record_fields_ctrl_ = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition,
                                             wxSize(-1, 110), wxTE_MULTILINE);
        record_fields_ctrl_->SetHint("field_name TYPE [NOT NULL] [DEFAULT expr]");
        rootSizer->Add(record_fields_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

        auto* enumLabel = new wxStaticText(this, wxID_ANY, "Enum Values (one per line)");
        rootSizer->Add(enumLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        enum_values_ctrl_ = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition,
                                           wxSize(-1, 100), wxTE_MULTILINE);
        enum_values_ctrl_->SetHint("'LOW' = 1");
        rootSizer->Add(enum_values_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

        auto* setLabel = new wxStaticText(this, wxID_ANY, "Set Element Type (SET)");
        rootSizer->Add(setLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        set_element_ctrl_ = new wxTextCtrl(this, wxID_ANY);
        set_element_ctrl_->SetHint("UUID, INT, domain_name");
        rootSizer->Add(set_element_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

        auto* variantLabel = new wxStaticText(this, wxID_ANY, "Variant Types (one per line)");
        rootSizer->Add(variantLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        variant_types_ctrl_ = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition,
                                             wxSize(-1, 100), wxTE_MULTILINE);
        variant_types_ctrl_->SetHint("INT\nVARCHAR(80)\ncustom_domain");
        rootSizer->Add(variant_types_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

        auto* inheritsLabel = new wxStaticText(this, wxID_ANY, "Inherits");
        rootSizer->Add(inheritsLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        inherits_ctrl_ = new wxTextCtrl(this, wxID_ANY);
        inherits_ctrl_->SetHint("parent_domain (global)");
        rootSizer->Add(inherits_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

        auto* collateLabel = new wxStaticText(this, wxID_ANY, "Collation");
        rootSizer->Add(collateLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        collation_ctrl_ = new wxTextCtrl(this, wxID_ANY);
        rootSizer->Add(collation_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

        not_null_ctrl_ = new wxCheckBox(this, wxID_ANY, "NOT NULL");
        rootSizer->Add(not_null_ctrl_, 0, wxLEFT | wxRIGHT | wxBOTTOM, 8);

        auto* defaultLabel = new wxStaticText(this, wxID_ANY, "Default Expression");
        rootSizer->Add(defaultLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        default_ctrl_ = new wxTextCtrl(this, wxID_ANY);
        rootSizer->Add(default_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

        auto* checkLabel = new wxStaticText(this, wxID_ANY, "Check Expression");
        rootSizer->Add(checkLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        check_ctrl_ = new wxTextCtrl(this, wxID_ANY);
        rootSizer->Add(check_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

        auto* extraLabel = new wxStaticText(this, wxID_ANY, "Additional Constraints (one per line)");
        rootSizer->Add(extraLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        extra_constraints_ctrl_ = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition,
                                                 wxSize(-1, 90), wxTE_MULTILINE);
        extra_constraints_ctrl_->SetHint("CONSTRAINT name CHECK (expr)");
        rootSizer->Add(extra_constraints_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);

        auto* withLabel = new wxStaticText(this, wxID_ANY, "WITH Clause (raw)");
        rootSizer->Add(withLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        with_clause_ctrl_ = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition,
                                           wxSize(-1, 90), wxTE_MULTILINE);
        with_clause_ctrl_->SetHint("WITH SECURITY (...)\nWITH OPTIONS (...)");
        rootSizer->Add(with_clause_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    } else {
        auto* actionLabel = new wxStaticText(this, wxID_ANY, "Action");
        rootSizer->Add(actionLabel, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        alter_action_choice_ = BuildChoice(this, {
            "SET DEFAULT",
            "DROP DEFAULT",
            "ADD CHECK",
            "DROP CONSTRAINT",
            "SET COMPAT",
            "DROP COMPAT",
            "RENAME"
        });
        rootSizer->Add(alter_action_choice_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 8);

        alter_value_label_ = new wxStaticText(this, wxID_ANY, "Value");
        rootSizer->Add(alter_value_label_, 0, wxLEFT | wxRIGHT | wxTOP, 12);
        alter_value_ctrl_ = new wxTextCtrl(this, wxID_ANY);
        rootSizer->Add(alter_value_ctrl_, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    }

    rootSizer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL), 0, wxEXPAND | wxALL, 12);
    SetSizerAndFit(rootSizer);
    CentreOnParent();

    if (kind_choice_) {
        kind_choice_->Bind(wxEVT_CHOICE,
            [this](wxCommandEvent&) { UpdateKindFields(); });
        UpdateKindFields();
    }

    if (alter_action_choice_) {
        alter_action_choice_->Bind(wxEVT_CHOICE,
            [this](wxCommandEvent&) { UpdateAlterActionFields(); });
        UpdateAlterActionFields();
    }
}

std::string DomainEditorDialog::BuildSql() const {
    return mode_ == DomainEditorMode::Create ? BuildCreateSql() : BuildAlterSql();
}

std::string DomainEditorDialog::domain_name() const {
    return Trim(name_ctrl_ ? name_ctrl_->GetValue().ToStdString() : std::string());
}

void DomainEditorDialog::SetDomainName(const std::string& name) {
    if (name_ctrl_) {
        name_ctrl_->SetValue(name);
        if (mode_ == DomainEditorMode::Alter) {
            name_ctrl_->Enable(false);
        }
    }
}

std::string DomainEditorDialog::BuildCreateSql() const {
    std::string name = domain_name();
    if (name.empty()) {
        return std::string();
    }

    std::ostringstream sql;
    sql << "CREATE DOMAIN ";
    if (if_not_exists_ctrl_ && if_not_exists_ctrl_->IsChecked()) {
        sql << "IF NOT EXISTS ";
    }
    sql << FormatDomainPath(name) << "\n";

    std::string kind = kind_choice_ ? kind_choice_->GetStringSelection().ToStdString() : "BASIC";
    if (kind == "RECORD") {
        auto fields = SplitLines(record_fields_ctrl_ ? record_fields_ctrl_->GetValue().ToStdString()
                                                     : std::string());
        if (fields.empty()) {
            return std::string();
        }
        sql << "  AS RECORD (" << "\n    ";
        for (size_t i = 0; i < fields.size(); ++i) {
            if (i > 0) {
                sql << ",\n    ";
            }
            sql << fields[i];
        }
        sql << "\n  )\n";
    } else if (kind == "ENUM") {
        auto values = SplitLines(enum_values_ctrl_ ? enum_values_ctrl_->GetValue().ToStdString()
                                                   : std::string());
        if (values.empty()) {
            return std::string();
        }
        sql << "  AS ENUM (" << "\n    ";
        for (size_t i = 0; i < values.size(); ++i) {
            if (i > 0) {
                sql << ",\n    ";
            }
            sql << values[i];
        }
        sql << "\n  )\n";
    } else if (kind == "SET") {
        std::string element_type = Trim(set_element_ctrl_ ? set_element_ctrl_->GetValue().ToStdString()
                                                          : std::string());
        if (element_type.empty()) {
            return std::string();
        }
        sql << "  AS SET OF " << element_type << "\n";
    } else if (kind == "VARIANT") {
        auto types = SplitLines(variant_types_ctrl_ ? variant_types_ctrl_->GetValue().ToStdString()
                                                    : std::string());
        if (types.empty()) {
            return std::string();
        }
        sql << "  AS VARIANT (" << "\n    ";
        for (size_t i = 0; i < types.size(); ++i) {
            if (i > 0) {
                sql << ",\n    ";
            }
            sql << types[i];
        }
        sql << "\n  )\n";
    } else {
        std::string base_type = Trim(base_type_ctrl_ ? base_type_ctrl_->GetValue().ToStdString()
                                                     : std::string());
        if (base_type.empty()) {
            return std::string();
        }
        sql << "  AS " << base_type << "\n";
    }

    std::string constraints = BuildConstraintsClause();
    if (!constraints.empty()) {
        sql << constraints << "\n";
    }

    std::string inherits = Trim(inherits_ctrl_ ? inherits_ctrl_->GetValue().ToStdString()
                                               : std::string());
    if (!inherits.empty()) {
        sql << "  INHERITS (" << FormatDomainPath(inherits) << ")\n";
    }

    std::string collate = Trim(collation_ctrl_ ? collation_ctrl_->GetValue().ToStdString()
                                               : std::string());
    if (!collate.empty()) {
        if (collate.find('.') != std::string::npos || collate.find('"') != std::string::npos) {
            sql << "  COLLATE " << collate << "\n";
        } else {
            sql << "  COLLATE " << QuoteIdentifier(collate) << "\n";
        }
    }

    std::string with_clause = BuildWithClause();
    if (!with_clause.empty()) {
        sql << "  " << with_clause << "\n";
    }

    sql << ";";
    return sql.str();
}

std::string DomainEditorDialog::BuildAlterSql() const {
    std::string name = domain_name();
    if (name.empty()) {
        return std::string();
    }

    if (!alter_action_choice_ || !alter_value_ctrl_ || !alter_value_label_) {
        return std::string();
    }

    std::string action = alter_action_choice_->GetStringSelection().ToStdString();
    std::string value = Trim(alter_value_ctrl_->GetValue().ToStdString());

    std::ostringstream sql;
    sql << "ALTER DOMAIN " << FormatDomainPath(name) << " ";

    if (action == "SET DEFAULT") {
        if (value.empty()) {
            return std::string();
        }
        sql << "SET DEFAULT " << value;
    } else if (action == "DROP DEFAULT") {
        sql << "DROP DEFAULT";
    } else if (action == "ADD CHECK") {
        if (value.empty()) {
            return std::string();
        }
        std::string expr = value;
        expr = Trim(expr);
        if (!expr.empty() && expr.front() != '(') {
            expr = "(" + expr + ")";
        }
        sql << "ADD CHECK " << expr;
    } else if (action == "DROP CONSTRAINT") {
        if (value.empty()) {
            return std::string();
        }
        sql << "DROP CONSTRAINT " << QuoteIdentifier(value);
    } else if (action == "SET COMPAT") {
        if (value.empty()) {
            return std::string();
        }
        sql << "SET COMPAT " << value;
    } else if (action == "DROP COMPAT") {
        sql << "DROP COMPAT";
    } else if (action == "RENAME") {
        if (value.empty()) {
            return std::string();
        }
        sql << "RENAME TO " << QuoteIdentifier(value);
    } else {
        return std::string();
    }

    sql << ";";
    return sql.str();
}

std::string DomainEditorDialog::BuildConstraintsClause() const {
    std::vector<std::string> clauses;

    if (not_null_ctrl_ && not_null_ctrl_->IsChecked()) {
        clauses.push_back("NOT NULL");
    }

    std::string default_expr = Trim(default_ctrl_ ? default_ctrl_->GetValue().ToStdString()
                                                  : std::string());
    if (!default_expr.empty()) {
        clauses.push_back("DEFAULT " + default_expr);
    }

    std::string check_expr = Trim(check_ctrl_ ? check_ctrl_->GetValue().ToStdString()
                                              : std::string());
    if (!check_expr.empty()) {
        if (check_expr.front() != '(') {
            check_expr = "(" + check_expr + ")";
        }
        clauses.push_back("CHECK " + check_expr);
    }

    auto extras = SplitLines(extra_constraints_ctrl_ ? extra_constraints_ctrl_->GetValue().ToStdString()
                                                     : std::string());
    for (const auto& clause : extras) {
        clauses.push_back(clause);
    }

    if (clauses.empty()) {
        return std::string();
    }

    std::ostringstream out;
    for (const auto& clause : clauses) {
        out << "  " << clause << "\n";
    }
    std::string text = out.str();
    if (!text.empty() && text.back() == '\n') {
        text.pop_back();
    }
    return text;
}

std::string DomainEditorDialog::BuildWithClause() const {
    std::string raw = Trim(with_clause_ctrl_ ? with_clause_ctrl_->GetValue().ToStdString()
                                             : std::string());
    if (raw.empty()) {
        return std::string();
    }
    if (StartsWithNoCase(raw, "WITH")) {
        return raw;
    }
    return "WITH " + raw;
}

std::string DomainEditorDialog::FormatDomainPath(const std::string& value) const {
    std::string trimmed = Trim(value);
    if (trimmed.empty()) {
        return trimmed;
    }
    if (trimmed.find('.') != std::string::npos) {
        auto pos = trimmed.find_last_of('.');
        if (pos != std::string::npos && pos + 1 < trimmed.size()) {
            trimmed = trimmed.substr(pos + 1);
        }
    }
    if (trimmed.find('"') != std::string::npos) {
        return trimmed;
    }
    return QuoteIdentifier(trimmed);
}

void DomainEditorDialog::UpdateKindFields() {
    if (!kind_choice_) {
        return;
    }
    std::string kind = kind_choice_->GetStringSelection().ToStdString();
    bool basic = kind == "BASIC";
    bool record = kind == "RECORD";
    bool enumerated = kind == "ENUM";
    bool set = kind == "SET";
    bool variant = kind == "VARIANT";

    if (base_type_ctrl_) base_type_ctrl_->Enable(basic);
    if (record_fields_ctrl_) record_fields_ctrl_->Enable(record);
    if (enum_values_ctrl_) enum_values_ctrl_->Enable(enumerated);
    if (set_element_ctrl_) set_element_ctrl_->Enable(set);
    if (variant_types_ctrl_) variant_types_ctrl_->Enable(variant);
}

void DomainEditorDialog::UpdateAlterActionFields() {
    if (!alter_action_choice_ || !alter_value_label_ || !alter_value_ctrl_) {
        return;
    }
    std::string action = alter_action_choice_->GetStringSelection().ToStdString();
    bool needs_value = true;
    wxString label = "Value";
    wxString hint;

    if (action == "SET DEFAULT") {
        label = "Default Expression";
    } else if (action == "ADD CHECK") {
        label = "Check Expression";
    } else if (action == "DROP CONSTRAINT") {
        label = "Constraint Name";
    } else if (action == "SET COMPAT") {
        label = "Compat Name";
    } else if (action == "RENAME") {
        label = "New Domain Name";
    } else {
        needs_value = false;
    }

    alter_value_label_->SetLabel(label);
    alter_value_ctrl_->Enable(needs_value);
    if (!needs_value) {
        alter_value_ctrl_->Clear();
    }
}

} // namespace scratchrobin
