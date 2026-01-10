#ifndef SCRATCHROBIN_MAIN_FRAME_H
#define SCRATCHROBIN_MAIN_FRAME_H

#include <wx/frame.h>

namespace scratchrobin {

class MainFrame : public wxFrame {
public:
    MainFrame();

private:
    void BuildMenu();
    void BuildLayout();

    void OnNewSqlEditor(wxCommandEvent& event);
    void OnQuit(wxCommandEvent& event);

    wxDECLARE_EVENT_TABLE();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_MAIN_FRAME_H
