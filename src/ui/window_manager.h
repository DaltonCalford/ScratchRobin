#ifndef SCRATCHROBIN_WINDOW_MANAGER_H
#define SCRATCHROBIN_WINDOW_MANAGER_H

#include <cstddef>
#include <unordered_set>
#include <vector>

#include <wx/frame.h>

namespace scratchrobin {

class WindowManager {
public:
    void RegisterWindow(wxFrame* frame);
    void UnregisterWindow(wxFrame* frame);
    size_t WindowCount() const;
    void CloseAll();
    void RegisterDiagramHost(wxFrame* frame);
    void UnregisterDiagramHost(wxFrame* frame);
    wxFrame* GetDiagramHost() const;
    std::vector<wxFrame*> GetWindows() const;

private:
    std::unordered_set<wxFrame*> windows_;
    wxFrame* diagram_host_ = nullptr;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_WINDOW_MANAGER_H
