#ifndef SCRATCHROBIN_TABLE_EDITOR_DIALOG_H
#define SCRATCHROBIN_TABLE_EDITOR_DIALOG_H

#include <string>

#include <wx/dialog.h>

class wxCheckBox;
class wxChoice;
class wxStaticText;
class wxTextCtrl;

namespace scratchrobin {

enum class TableEditorMode {
    Create,
    Alter
};

class TableEditorDialog : public wxDialog {
public:
    TableEditorDialog(wxWindow* parent, TableEditorMode mode);

    std::string BuildSql() const;
    std::string table_name() const;

    void SetTableName(const std::string& name);

private:
    std::string BuildCreateSql() const;
    std::string BuildAlterSql() const;
    std::string FormatTablePath(const std::string& value) const;
    void UpdateAlterActionFields();

    TableEditorMode mode_;

    wxTextCtrl* name_ctrl_ = nullptr;
    wxCheckBox* if_not_exists_ctrl_ = nullptr;
    wxTextCtrl* columns_ctrl_ = nullptr;
    wxTextCtrl* constraints_ctrl_ = nullptr;
    wxTextCtrl* options_ctrl_ = nullptr;

    wxChoice* alter_action_choice_ = nullptr;
    wxStaticText* alter_value_label_ = nullptr;
    wxTextCtrl* alter_value_ctrl_ = nullptr;
    wxStaticText* alter_value_label_2_ = nullptr;
    wxTextCtrl* alter_value_ctrl_2_ = nullptr;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_TABLE_EDITOR_DIALOG_H
