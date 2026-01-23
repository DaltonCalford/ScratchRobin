#ifndef SCRATCHROBIN_STARTUP_FRAME_H
#define SCRATCHROBIN_STARTUP_FRAME_H

#include <wx/frame.h>

#include "core/config.h"

class wxGauge;
class wxStaticText;
class wxStaticBitmap;

namespace scratchrobin {

class StartupFrame : public wxFrame {
public:
    explicit StartupFrame(const StartupConfig& config);
    void SetStatusText(const wxString& text);
    void SetProgress(int value);

private:
    void OnClick(wxMouseEvent& event);

    StartupConfig config_;
    wxGauge* progress_ = nullptr;
    wxStaticText* status_ = nullptr;
    wxStaticBitmap* logo_ = nullptr;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_STARTUP_FRAME_H
