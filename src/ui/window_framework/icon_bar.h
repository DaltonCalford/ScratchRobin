/*
 * ScratchBird
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */

#pragma once

#include <vector>
#include <functional>
#include <wx/bitmap.h>
#include <wx/toolbar.h>
#include "ui/window_framework/dockable_window.h"

namespace scratchrobin::ui {

// Icon bar/toolbar that can be docked at top/bottom/left/right
class IconBar : public DockableWindow {
public:
    IconBar(wxWindow* parent, const std::string& instance_id = "");
    
    // Icon definition
    struct IconItem {
        int id;
        wxBitmap bitmap;
        wxString label;
        wxString tooltip;
        std::function<void()> callback;
        bool is_toggle{false};
        bool is_checked{false};
        bool is_separator{false};
        bool is_enabled{true};
    };
    
    // Add icons
    void addIcon(const IconItem& item);
    void addSeparator();
    void addStretch();  // Flexible space
    
    // Update icon
    void setIconEnabled(int id, bool enabled);
    void setIconChecked(int id, bool checked);
    void setIconBitmap(int id, const wxBitmap& bitmap);
    void setIconTooltip(int id, const wxString& tooltip);
    
    // Remove icons
    void removeIcon(int id);
    void clearIcons();
    
    // Orientation based on dock position
    void setOrientation(wxOrientation orient);
    wxOrientation getOrientation() const { return orientation_; }
    
    // Override from DockableWindow
    std::vector<MenuContribution> getMenuContributions() override;

protected:
    void onWindowCreated() override;
    wxSize getDefaultSize() const override;

private:
    wxToolBar* toolbar_{nullptr};
    std::vector<IconItem> items_;
    wxOrientation orientation_{wxHORIZONTAL};
    
    void rebuildToolbar();
    void onToolClicked(wxCommandEvent& event);
    
    DECLARE_EVENT_TABLE()
};

// Factory function
IconBar* createMainToolbar(wxWindow* parent);
IconBar* createViewToolbar(wxWindow* parent);
IconBar* createQueryToolbar(wxWindow* parent);

}  // namespace scratchrobin::ui
