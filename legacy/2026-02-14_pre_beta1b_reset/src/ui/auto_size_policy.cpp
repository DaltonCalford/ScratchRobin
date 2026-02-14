/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#include "auto_size_policy.h"

#include <wx/display.h>
#include <wx/menu.h>
#include <wx/toolbar.h>

namespace scratchrobin {

void AutoSizePolicy::MarkUserResized() {
    last_user_resize_ = std::chrono::steady_clock::now();
    // Switch to fixed mode when user manually resizes
    if (mode_ != Mode::FULLSCREEN) {
        mode_ = Mode::FIXED;
    }
}

bool AutoSizePolicy::IsUserResizeCooldownActive() const {
    if (last_user_resize_.time_since_epoch().count() == 0) {
        return false;
    }
    auto elapsed = std::chrono::steady_clock::now() - last_user_resize_;
    return elapsed < kUserResizeCooldown;
}

wxSize AutoSizePolicy::CalculateSize(wxWindow* main_form,
                                     bool has_navigator,
                                     bool has_document_manager,
                                     const wxSize& content_size) {
    if (!main_form) {
        return GetMinimumWorkingSize();
    }

    // Don't auto-size if user recently resized
    if (IsUserResizeCooldownActive()) {
        return main_form->GetSize();
    }

    wxSize target_size;
    wxSize minimum_size = GetMinimumWorkingSize();
    wxSize maximum_size = GetMaximumSize();

    switch (mode_) {
        case Mode::FIXED:
            // Keep current size
            target_size = main_form->GetSize();
            break;

        case Mode::COMPACT:
            // Compact mode - menu + iconbar only
            target_size = GetMinimumCompactSize();
            break;

        case Mode::ADAPTIVE: {
            // Adaptive mode - size based on content
            if (has_document_manager) {
                // Content is open - use working size or content size, whichever is larger
                target_size = content_size;
                target_size.IncBy(kPadding * 2); // Add padding
                
                // Ensure at least minimum working size
                target_size.x = std::max(target_size.x, minimum_size.x);
                target_size.y = std::max(target_size.y, minimum_size.y);
            } else if (has_navigator) {
                // Navigator only - medium size
                target_size = wxSize(
                    std::max(minimum_size.x / 2, kCompactWidth + 200),
                    minimum_size.y
                );
            } else {
                // No content - compact size
                target_size = GetMinimumCompactSize();
            }
            break;
        }

        case Mode::FULLSCREEN:
            // Maximized - use full screen size
            target_size = maximum_size;
            break;

        case Mode::CUSTOM:
            // Use saved custom size
            target_size = custom_size_;
            if (target_size.x < minimum_size.x || target_size.y < minimum_size.y) {
                target_size = minimum_size;
            }
            break;
    }

    // Clamp to maximum screen size
    target_size.x = std::min(target_size.x, maximum_size.x);
    target_size.y = std::min(target_size.y, maximum_size.y);

    // Ensure minimum size
    target_size.x = std::max(target_size.x, kCompactWidth);
    target_size.y = std::max(target_size.y, kCompactHeight);

    return target_size;
}

wxSize AutoSizePolicy::GetMinimumCompactSize() {
    return wxSize(kCompactWidth, kCompactHeight);
}

wxSize AutoSizePolicy::GetMinimumWorkingSize() {
    return wxSize(kWorkingWidth, kWorkingHeight);
}

wxSize AutoSizePolicy::GetMaximumSize() {
    // Get the primary display size
    wxDisplay display(0u);
    wxRect display_rect = display.GetClientArea();
    return display_rect.GetSize();
}

const char* AutoSizePolicy::ModeToString(Mode mode) {
    switch (mode) {
        case Mode::FIXED: return "fixed";
        case Mode::COMPACT: return "compact";
        case Mode::ADAPTIVE: return "adaptive";
        case Mode::FULLSCREEN: return "fullscreen";
        case Mode::CUSTOM: return "custom";
    }
    return "adaptive"; // Default
}

AutoSizePolicy::Mode AutoSizePolicy::StringToMode(const char* str) {
    if (!str) return Mode::ADAPTIVE;
    
    std::string s(str);
    // Convert to lowercase for comparison
    for (auto& c : s) {
        c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    }
    
    if (s == "fixed") return Mode::FIXED;
    if (s == "compact") return Mode::COMPACT;
    if (s == "adaptive") return Mode::ADAPTIVE;
    if (s == "fullscreen") return Mode::FULLSCREEN;
    if (s == "custom") return Mode::CUSTOM;
    
    return Mode::ADAPTIVE; // Default
}

} // namespace scratchrobin
