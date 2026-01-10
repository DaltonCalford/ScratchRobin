#include "sql_editor_frame.h"

#include <wx/sizer.h>
#include <wx/textctrl.h>

namespace scratchrobin {

SqlEditorFrame::SqlEditorFrame()
    : wxFrame(nullptr, wxID_ANY, "SQL Editor", wxDefaultPosition, wxSize(800, 600)) {
    auto* rootSizer = new wxBoxSizer(wxVERTICAL);
    auto* editor = new wxTextCtrl(this, wxID_ANY, "", wxDefaultPosition, wxDefaultSize,
                                  wxTE_MULTILINE | wxTE_RICH2);
    rootSizer->Add(editor, 1, wxEXPAND | wxALL, 8);
    SetSizer(rootSizer);
}

} // namespace scratchrobin
