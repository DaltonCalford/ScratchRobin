/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_DOCUMENT_MANAGER_H
#define SCRATCHROBIN_DOCUMENT_MANAGER_H

#include <vector>
#include <memory>

#include <wx/panel.h>
#include <wx/aui/auibook.h>
#include <wx/event.h>

#include "layout/dockable_window.h"

namespace scratchrobin {

class IDocumentWindow;

// ============================================================================
// Document Manager - Tabbed interface for hosting multiple documents
// ============================================================================
class DocumentManager : public wxPanel {
public:
    explicit DocumentManager(wxWindow* parent);
    ~DocumentManager() override;
    
    // Document management
    void AddDocument(IDocumentWindow* doc);
    void RemoveDocument(IDocumentWindow* doc);
    void RemoveDocument(int index);
    void ActivateDocument(IDocumentWindow* doc);
    void ActivateDocument(int index);
    
    // Query
    IDocumentWindow* GetActiveDocument() const;
    IDocumentWindow* GetDocument(int index) const;
    int GetDocumentIndex(IDocumentWindow* doc) const;
    int GetDocumentCount() const { return static_cast<int>(documents_.size()); }
    std::vector<IDocumentWindow*> GetDocuments() const;
    bool HasDocuments() const { return !documents_.empty(); }
    
    // Tab operations
    void CloseTab(int index);
    void CloseAllTabs();
    void CloseOtherTabs(int index);
    void MoveTab(int from, int to);
    
    // Update UI
    void UpdateDocumentTitle(IDocumentWindow* doc);
    void UpdateAllTabLabels();
    
    // Visibility control
    bool IsManagerVisible() const { return IsShown(); }
    void ShowManager();
    void HideManager();
    
    // Keyboard shortcuts
    void OnNextTab();
    void OnPreviousTab();
    
    // Events
    wxDECLARE_EVENT_TABLE();
    
protected:
    void OnTabChanged(wxAuiNotebookEvent& event);
    void OnTabClose(wxAuiNotebookEvent& event);
    void OnTabContextMenu(wxAuiNotebookEvent& event);
    void OnTabMiddleClick(wxAuiNotebookEvent& event);
    void OnTabDragDone(wxAuiNotebookEvent& event);
    
private:
    void BuildLayout();
    wxString BuildTabLabel(IDocumentWindow* doc) const;
    int FindDocumentIndex(IDocumentWindow* doc) const;
    void NotifyActivationChanged(IDocumentWindow* old_doc, IDocumentWindow* new_doc);
    
    wxAuiNotebook* notebook_ = nullptr;
    std::vector<IDocumentWindow*> documents_;
    bool is_updating_ = false;
    
    // Context menu IDs
    static const int ID_TAB_CLOSE = wxID_HIGHEST + 2000;
    static const int ID_TAB_CLOSE_ALL = wxID_HIGHEST + 2001;
    static const int ID_TAB_CLOSE_OTHERS = wxID_HIGHEST + 2002;
    static const int ID_TAB_COPY_PATH = wxID_HIGHEST + 2003;
    static const int ID_TAB_REVEAL = wxID_HIGHEST + 2004;
};

// ============================================================================
// Document Manager Events
// ============================================================================
wxDECLARE_EVENT(DOCUMENT_ACTIVATED, wxCommandEvent);
wxDECLARE_EVENT(DOCUMENT_CLOSED, wxCommandEvent);
wxDECLARE_EVENT(DOCUMENT_MODIFIED_CHANGED, wxCommandEvent);

} // namespace scratchrobin

#endif // SCRATCHROBIN_DOCUMENT_MANAGER_H
