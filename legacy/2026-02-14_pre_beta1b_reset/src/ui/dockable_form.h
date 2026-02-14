/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0
 */
#ifndef SCRATCHROBIN_DOCKABLE_FORM_H
#define SCRATCHROBIN_DOCKABLE_FORM_H

#include <wx/panel.h>
#include <wx/string.h>
#include <wx/menu.h>
#include <wx/toolbar.h>
#include <wx/frame.h>
#include "layout/dockable_window.h"

namespace scratchrobin {

// ============================================================================
// DockableForm - Base class for all dockable document forms
// ============================================================================
class DockableForm : public wxPanel, public IDocumentWindow {
public:
    DockableForm(wxWindow* parent, const wxString& title = "");
    
    // IDockableWindow implementation
    std::string GetWindowId() const override { return document_title_.ToStdString(); }
    std::string GetWindowTitle() const override { return document_title_.ToStdString(); }
    std::string GetWindowType() const override { return "document"; }
    wxWindow* GetWindow() override { return this; }
    const wxWindow* GetWindow() const override { return this; }
    
    // IDocumentWindow implementation
    std::string GetDocumentType() const override = 0;
    
    // Document identity  
    virtual wxString GetDocumentTitleWx() const { return document_title_; }
    std::string GetDocumentPath() const override { return document_path_.ToStdString(); }
    
    // State
    virtual bool IsModified() const override { return is_modified_; }
    virtual void SetModified(bool modified) { is_modified_ = modified; }
    virtual bool Save() override { return true; }
    virtual bool Load(const std::string& path) override { return true; }
    virtual bool Reload() override { return true; }
    
    // Menu/Toolbar
    virtual wxMenuBar* GetMenuBar() override { return nullptr; }
    virtual wxToolBar* GetToolBar() override { return nullptr; }
    
    // Activation - IDockableWindow interface
    void OnActivate() override;
    void OnDeactivate() override;
    bool IsActive() const { return is_active_; }
    
    // Close handling
    virtual bool CanClose() const override { return true; }
    virtual bool OnCloseRequest() override { return CanClose(); }
    virtual void OnClosing() {}
    
    // Docking state
    bool IsDocked() const { return is_docked_; }
    void SetDocked(bool docked) { is_docked_ = docked; }
    
    bool IsFloating() const { return is_floating_; }
    void SetFloating(bool floating) { is_floating_ = floating; }
    
protected:
    wxString document_title_;
    wxString document_path_;
    bool is_modified_ = false;
    bool is_active_ = false;
    bool is_docked_ = true;
    bool is_floating_ = false;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_DOCKABLE_FORM_H
