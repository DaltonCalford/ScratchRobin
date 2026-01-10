#ifndef SCRATCHROBIN_APP_H
#define SCRATCHROBIN_APP_H

#include <wx/app.h>

namespace scratchrobin {

class ScratchRobinApp : public wxApp {
public:
    bool OnInit() override;
    int OnExit() override;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_APP_H
