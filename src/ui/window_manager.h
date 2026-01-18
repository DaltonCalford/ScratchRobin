#ifndef SCRATCHROBIN_WINDOW_MANAGER_H
#define SCRATCHROBIN_WINDOW_MANAGER_H

#include <cstddef>
#include <unordered_set>

#include <wx/frame.h>

namespace scratchrobin {

class WindowManager {
public:
    void RegisterWindow(wxFrame* frame);
    void UnregisterWindow(wxFrame* frame);
    size_t WindowCount() const;
    void CloseAll();

private:
    std::unordered_set<wxFrame*> windows_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_WINDOW_MANAGER_H
