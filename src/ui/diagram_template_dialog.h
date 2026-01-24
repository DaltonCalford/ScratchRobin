#ifndef SCRATCHROBIN_DIAGRAM_TEMPLATE_DIALOG_H
#define SCRATCHROBIN_DIAGRAM_TEMPLATE_DIALOG_H

#include <string>

#include <wx/dialog.h>

class wxChoice;
class wxSpinCtrl;

namespace scratchrobin {

class DiagramTemplateDialog : public wxDialog {
public:
    DiagramTemplateDialog(wxWindow* parent,
                          int grid_size,
                          const std::string& icon_set,
                          int border_width,
                          bool border_dashed);

    int grid_size() const;
    std::string icon_set() const;
    int border_width() const;
    bool border_dashed() const;

private:
    wxChoice* grid_choice_ = nullptr;
    wxChoice* icon_choice_ = nullptr;
    wxSpinCtrl* border_width_ = nullptr;
    wxChoice* border_style_ = nullptr;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_DIAGRAM_TEMPLATE_DIALOG_H
