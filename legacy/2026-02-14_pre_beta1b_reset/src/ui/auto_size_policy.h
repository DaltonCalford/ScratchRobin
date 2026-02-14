/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_AUTO_SIZE_POLICY_H
#define SCRATCHROBIN_AUTO_SIZE_POLICY_H

#include <wx/window.h>
#include <wx/gdicmn.h>
#include <chrono>

namespace scratchrobin {

/**
 * AutoSizePolicy - Controls automatic resizing behavior for the main form.
 *
 * This class implements different sizing strategies based on content visibility
 * and user preferences. It supports:
 * - FIXED: Manual size (user resized)
 * - COMPACT: Minimum size to show menu/iconbar only
 * - ADAPTIVE: Grow to fit content, shrink when empty (default)
 * - FULLSCREEN: Maximized state
 * - CUSTOM: Saved user preference
 */
class AutoSizePolicy {
public:
    /**
     * Sizing mode enumeration
     */
    enum class Mode {
        FIXED,           // Fixed size (user resized)
        COMPACT,         // Minimum size to show menu/iconbar
        ADAPTIVE,        // Grow to fit content, shrink when empty
        FULLSCREEN,      // Maximized
        CUSTOM           // Saved user preference
    };

    /**
     * Default constructor - initializes with ADAPTIVE mode
     */
    AutoSizePolicy() = default;

    /**
     * Set the current sizing mode
     * @param mode The new sizing mode
     */
    void SetMode(Mode mode) { mode_ = mode; }

    /**
     * Get the current sizing mode
     * @return Current mode
     */
    Mode GetMode() const { return mode_; }

    /**
     * Set custom size (used with CUSTOM mode)
     * @param size The custom window size
     */
    void SetCustomSize(const wxSize& size) { custom_size_ = size; }

    /**
     * Get the custom size
     * @return Custom size
     */
    wxSize GetCustomSize() const { return custom_size_; }

    /**
     * Set custom position (used with CUSTOM mode)
     * @param position The custom window position
     */
    void SetCustomPosition(const wxPoint& position) { custom_position_ = position; }

    /**
     * Get the custom position
     * @return Custom position
     */
    wxPoint GetCustomPosition() const { return custom_position_; }

    /**
     * Mark that the user manually resized the window
     * This temporarily disables auto-sizing
     */
    void MarkUserResized();

    /**
     * Check if user recently resized (within cooldown period)
     * @return true if within 5-second cooldown
     */
    bool IsUserResizeCooldownActive() const;

    /**
     * Calculate desired size based on content and current mode
     * @param main_form The main window to size
     * @param has_navigator Whether navigator panel is visible
     * @param has_document_manager Whether document manager/content is visible
     * @param content_size Size of the content area
     * @return Calculated desired size
     */
    wxSize CalculateSize(wxWindow* main_form,
                         bool has_navigator,
                         bool has_document_manager,
                         const wxSize& content_size);

    /**
     * Get the minimum compact size (menu + iconbar only)
     * @return Minimum size for compact mode
     */
    static wxSize GetMinimumCompactSize();

    /**
     * Get the minimum working size (with content)
     * @return Minimum size for working mode
     */
    static wxSize GetMinimumWorkingSize();

    /**
     * Get the maximum allowed size (screen dimensions)
     * @return Maximum size
     */
    static wxSize GetMaximumSize();

    /**
     * Convert mode to string representation
     * @param mode The mode to convert
     * @return String name of the mode
     */
    static const char* ModeToString(Mode mode);

    /**
     * Convert string to mode
     * @param str The string to parse
     * @return Corresponding mode (ADAPTIVE if invalid)
     */
    static Mode StringToMode(const char* str);

private:
    Mode mode_ = Mode::ADAPTIVE;
    wxSize custom_size_;
    wxPoint custom_position_;
    std::chrono::steady_clock::time_point last_user_resize_;

    // Constants
    static constexpr int kCompactWidth = 400;
    static constexpr int kCompactHeight = 100;
    static constexpr int kWorkingWidth = 800;
    static constexpr int kWorkingHeight = 600;
    static constexpr int kPadding = 20;
    static constexpr auto kUserResizeCooldown = std::chrono::seconds(5);
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_AUTO_SIZE_POLICY_H
