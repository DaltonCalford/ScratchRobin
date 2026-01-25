#ifndef SCRATCHROBIN_INDEX_EDITOR_DIALOG_H
#define SCRATCHROBIN_INDEX_EDITOR_DIALOG_H

#include <string>

#include <wx/dialog.h>

class wxCheckBox;
class wxChoice;
class wxStaticText;
class wxTextCtrl;

namespace scratchrobin {

enum class IndexEditorMode {
    Create,
    Alter
};

class IndexEditorDialog : public wxDialog {
public:
    IndexEditorDialog(wxWindow* parent, IndexEditorMode mode);

    std::string BuildSql() const;

    std::string index_name() const;
    void SetIndexName(const std::string& name);
    void SetTableName(const std::string& name);

private:
    std::string BuildCreateSql() const;
    std::string BuildAlterSql() const;
    std::string FormatIndexPath(const std::string& value) const;
    std::string FormatTablePath(const std::string& value) const;
    void UpdateAlterActionFields();

    IndexEditorMode mode_;

    wxTextCtrl* name_ctrl_ = nullptr;
    wxTextCtrl* table_ctrl_ = nullptr;
    wxCheckBox* if_not_exists_ctrl_ = nullptr;
    wxCheckBox* unique_ctrl_ = nullptr;
    wxChoice* type_choice_ = nullptr;
    wxTextCtrl* columns_ctrl_ = nullptr;
    wxTextCtrl* include_ctrl_ = nullptr;
    wxTextCtrl* where_ctrl_ = nullptr;
    wxTextCtrl* options_ctrl_ = nullptr;

    wxChoice* alter_action_choice_ = nullptr;
    wxStaticText* alter_value_label_ = nullptr;
    wxTextCtrl* alter_value_ctrl_ = nullptr;
    wxStaticText* alter_value_label_2_ = nullptr;
    wxTextCtrl* alter_value_ctrl_2_ = nullptr;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_INDEX_EDITOR_DIALOG_H
