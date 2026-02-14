/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#include "document_manager.h"

#include <wx/menu.h>
#include <wx/sizer.h>
#include <wx/clipbrd.h>
#include <wx/utils.h>
#include <wx/msgdlg.h>
#include <wx/filename.h>

namespace scratchrobin {

// Define custom events
wxDEFINE_EVENT(DOCUMENT_ACTIVATED, wxCommandEvent);
wxDEFINE_EVENT(DOCUMENT_CLOSED, wxCommandEvent);
wxDEFINE_EVENT(DOCUMENT_MODIFIED_CHANGED, wxCommandEvent);

// ============================================================================
// DocumentManager Event Table
// ============================================================================
wxBEGIN_EVENT_TABLE(DocumentManager, wxPanel)
    EVT_AUINOTEBOOK_PAGE_CHANGED(wxID_ANY, DocumentManager::OnTabChanged)
    EVT_AUINOTEBOOK_PAGE_CLOSE(wxID_ANY, DocumentManager::OnTabClose)
    EVT_AUINOTEBOOK_TAB_RIGHT_DOWN(wxID_ANY, DocumentManager::OnTabContextMenu)
    EVT_AUINOTEBOOK_TAB_MIDDLE_DOWN(wxID_ANY, DocumentManager::OnTabMiddleClick)
    EVT_AUINOTEBOOK_DRAG_DONE(wxID_ANY, DocumentManager::OnTabDragDone)
wxEND_EVENT_TABLE()

// ============================================================================
// DocumentManager Implementation
// ============================================================================
DocumentManager::DocumentManager(wxWindow* parent)
    : wxPanel(parent, wxID_ANY) {
    BuildLayout();
}

DocumentManager::~DocumentManager() = default;

void DocumentManager::BuildLayout() {
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    
    // Create AUI notebook with modern tab styling
    long notebook_style = wxAUI_NB_DEFAULT_STYLE |
                          wxAUI_NB_TAB_MOVE |
                          wxAUI_NB_TAB_EXTERNAL_MOVE |
                          wxAUI_NB_SCROLL_BUTTONS |
                          wxAUI_NB_WINDOWLIST_BUTTON |
                          wxAUI_NB_CLOSE_ON_ACTIVE_TAB |
                          wxAUI_NB_CLOSE_ON_ALL_TABS |
                          wxAUI_NB_MIDDLE_CLICK_CLOSE;
    
    notebook_ = new wxAuiNotebook(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, notebook_style);
    
    sizer->Add(notebook_, 1, wxEXPAND);
    SetSizer(sizer);
    
    // Initially hidden until documents are added
    Hide();
}

void DocumentManager::AddDocument(IDocumentWindow* doc) {
    if (!doc) {
        return;
    }
    
    wxFrame* frame = doc->GetFrame();
    if (!frame) {
        return;
    }
    
    // Build tab label
    wxString label = BuildTabLabel(doc);
    
    // Add to notebook - use the frame's content
    // Note: In a full implementation, we'd reparent the frame's content
    // For now, we add the frame directly (this is a simplified approach)
    int page_index = notebook_->AddPage(frame, label, true);
    
    // Track the document
    documents_.push_back(doc);
    
    // Show the manager if it was hidden
    ShowManager();
    
    // Activate the new document
    ActivateDocument(doc);
    
    // Send event
    wxCommandEvent evt(DOCUMENT_ACTIVATED);
    evt.SetClientData(doc);
    ProcessEvent(evt);
    
    Layout();
}

void DocumentManager::RemoveDocument(IDocumentWindow* doc) {
    if (!doc) {
        return;
    }
    
    int index = FindDocumentIndex(doc);
    if (index >= 0) {
        RemoveDocument(index);
    }
}

void DocumentManager::RemoveDocument(int index) {
    if (index < 0 || index >= static_cast<int>(documents_.size())) {
        return;
    }
    
    IDocumentWindow* doc = documents_[index];
    
    // Check if can close
    if (!doc->OnCloseRequest()) {
        return;
    }
    
    // Remove from notebook
    notebook_->RemovePage(index);
    
    // Remove from tracking
    documents_.erase(documents_.begin() + index);
    
    // Send event
    wxCommandEvent evt(DOCUMENT_CLOSED);
    evt.SetClientData(doc);
    ProcessEvent(evt);
    
    // Hide if no more documents
    if (documents_.empty()) {
        HideManager();
    }
    
    Layout();
}

void DocumentManager::ActivateDocument(IDocumentWindow* doc) {
    int index = FindDocumentIndex(doc);
    if (index >= 0) {
        ActivateDocument(index);
    }
}

void DocumentManager::ActivateDocument(int index) {
    if (index < 0 || index >= static_cast<int>(documents_.size())) {
        return;
    }
    
    IDocumentWindow* old_doc = GetActiveDocument();
    
    notebook_->SetSelection(index);
    
    IDocumentWindow* new_doc = documents_[index];
    NotifyActivationChanged(old_doc, new_doc);
}

IDocumentWindow* DocumentManager::GetActiveDocument() const {
    int sel = notebook_->GetSelection();
    if (sel >= 0 && sel < static_cast<int>(documents_.size())) {
        return documents_[sel];
    }
    return nullptr;
}

IDocumentWindow* DocumentManager::GetDocument(int index) const {
    if (index >= 0 && index < static_cast<int>(documents_.size())) {
        return documents_[index];
    }
    return nullptr;
}

int DocumentManager::GetDocumentIndex(IDocumentWindow* doc) const {
    return FindDocumentIndex(doc);
}

std::vector<IDocumentWindow*> DocumentManager::GetDocuments() const {
    return documents_;
}

void DocumentManager::CloseTab(int index) {
    RemoveDocument(index);
}

void DocumentManager::CloseAllTabs() {
    // Close from end to beginning to avoid index shifting
    for (int i = static_cast<int>(documents_.size()) - 1; i >= 0; --i) {
        IDocumentWindow* doc = documents_[i];
        
        if (doc->IsModified()) {
            // Activate the tab to show user which one we're asking about
            ActivateDocument(i);
            
            if (!doc->OnCloseRequest()) {
                // User cancelled, stop closing
                return;
            }
        }
    }
    
    // Now close all
    while (!documents_.empty()) {
        RemoveDocument(static_cast<int>(documents_.size()) - 1);
    }
}

void DocumentManager::CloseOtherTabs(int index) {
    if (index < 0 || index >= static_cast<int>(documents_.size())) {
        return;
    }
    
    // Close tabs before the specified index (from right to left)
    for (int i = index - 1; i >= 0; --i) {
        IDocumentWindow* doc = documents_[i];
        
        if (doc->IsModified()) {
            ActivateDocument(i);
            if (!doc->OnCloseRequest()) {
                return;
            }
        }
        RemoveDocument(i);
    }
    
    // Close tabs after the specified index (index shifts as we remove)
    while (static_cast<int>(documents_.size()) > 1) {
        // After removing earlier tabs, our target is now at index 0
        // So we always remove index 1
        if (static_cast<int>(documents_.size()) > 1) {
            IDocumentWindow* doc = documents_[1];
            
            if (doc->IsModified()) {
                ActivateDocument(1);
                if (!doc->OnCloseRequest()) {
                    return;
                }
            }
            RemoveDocument(1);
        }
    }
}

void DocumentManager::MoveTab(int from, int to) {
    if (from < 0 || from >= static_cast<int>(documents_.size()) ||
        to < 0 || to >= static_cast<int>(documents_.size())) {
        return;
    }
    
    // Move in our tracking vector
    IDocumentWindow* doc = documents_[from];
    documents_.erase(documents_.begin() + from);
    documents_.insert(documents_.begin() + to, doc);
    
    // Note: wxAuiNotebook doesn't have MovePage, would need to remove/re-add
    // For now, just update the tracking - the UI will be correct on next refresh
    // TODO: Implement proper tab reordering
}

void DocumentManager::UpdateDocumentTitle(IDocumentWindow* doc) {
    if (!doc) {
        return;
    }
    
    int index = FindDocumentIndex(doc);
    if (index >= 0) {
        wxString label = BuildTabLabel(doc);
        notebook_->SetPageText(index, label);
        
        // Send modified changed event
        wxCommandEvent evt(DOCUMENT_MODIFIED_CHANGED);
        evt.SetClientData(doc);
        ProcessEvent(evt);
    }
}

void DocumentManager::UpdateAllTabLabels() {
    for (size_t i = 0; i < documents_.size(); ++i) {
        wxString label = BuildTabLabel(documents_[i]);
        notebook_->SetPageText(static_cast<int>(i), label);
    }
}

void DocumentManager::ShowManager() {
    if (!IsShown()) {
        Show();
        GetParent()->Layout();
    }
}

void DocumentManager::HideManager() {
    if (IsShown()) {
        Hide();
        GetParent()->Layout();
    }
}

void DocumentManager::OnNextTab() {
    int count = notebook_->GetPageCount();
    if (count <= 1) {
        return;
    }
    
    int current = notebook_->GetSelection();
    int next = (current + 1) % count;
    ActivateDocument(next);
}

void DocumentManager::OnPreviousTab() {
    int count = notebook_->GetPageCount();
    if (count <= 1) {
        return;
    }
    
    int current = notebook_->GetSelection();
    int prev = (current - 1 + count) % count;
    ActivateDocument(prev);
}

// ============================================================================
// Event Handlers
// ============================================================================
void DocumentManager::OnTabChanged(wxAuiNotebookEvent& event) {
    event.Skip();
    
    if (is_updating_) {
        return;
    }
    
    int old_sel = event.GetOldSelection();
    int new_sel = event.GetSelection();
    
    IDocumentWindow* old_doc = nullptr;
    IDocumentWindow* new_doc = nullptr;
    
    if (old_sel >= 0 && old_sel < static_cast<int>(documents_.size())) {
        old_doc = documents_[old_sel];
    }
    
    if (new_sel >= 0 && new_sel < static_cast<int>(documents_.size())) {
        new_doc = documents_[new_sel];
    }
    
    NotifyActivationChanged(old_doc, new_doc);
}

void DocumentManager::OnTabClose(wxAuiNotebookEvent& event) {
    int index = event.GetSelection();
    
    if (index >= 0 && index < static_cast<int>(documents_.size())) {
        IDocumentWindow* doc = documents_[index];
        
        if (!doc->OnCloseRequest()) {
            // Cancel the close
            event.Veto();
            return;
        }
    }
    
    // Let the notebook handle the close, we'll clean up
    event.Skip();
    RemoveDocument(index);
}

void DocumentManager::OnTabContextMenu(wxAuiNotebookEvent& event) {
    int tab_index = event.GetSelection();
    if (tab_index < 0) {
        return;
    }
    
    wxMenu menu;
    menu.Append(ID_TAB_CLOSE, "&Close");
    menu.Append(ID_TAB_CLOSE_ALL, "Close &All");
    menu.Append(ID_TAB_CLOSE_OTHERS, "Close &Others");
    menu.AppendSeparator();
    
    IDocumentWindow* doc = documents_[tab_index];
    wxString file_path = doc->GetDocumentPath();
    
    if (!file_path.IsEmpty()) {
        menu.Append(ID_TAB_COPY_PATH, "Copy Full &Path");
        menu.Append(ID_TAB_REVEAL, "&Reveal in File Manager");
        menu.AppendSeparator();
    }
    
    menu.Append(wxID_SAVE, "&Save");
    menu.Append(wxID_SAVEAS, "Save &As...");
    
    int result = GetPopupMenuSelectionFromUser(menu);
    
    if (result == ID_TAB_CLOSE) {
        CloseTab(tab_index);
    } else if (result == ID_TAB_CLOSE_ALL) {
        CloseAllTabs();
    } else if (result == ID_TAB_CLOSE_OTHERS) {
        CloseOtherTabs(tab_index);
    } else if (result == ID_TAB_COPY_PATH && !file_path.IsEmpty()) {
        if (wxTheClipboard->Open()) {
            wxTheClipboard->SetData(new wxTextDataObject(file_path));
            wxTheClipboard->Close();
        }
    } else if (result == ID_TAB_REVEAL && !file_path.IsEmpty()) {
        wxFileName fn(file_path);
        wxString path = fn.GetPath();
        if (!path.IsEmpty() && wxDirExists(path)) {
            wxLaunchDefaultBrowser("file://" + path);
        }
    } else if (result == wxID_SAVE) {
        if (doc) {
            doc->Save();
        }
    } else if (result == wxID_SAVEAS) {
        if (doc) {
            doc->SaveAs();
        }
    }
}

void DocumentManager::OnTabMiddleClick(wxAuiNotebookEvent& event) {
    int index = event.GetSelection();
    if (index >= 0) {
        CloseTab(index);
    }
}

void DocumentManager::OnTabDragDone(wxAuiNotebookEvent& event) {
    // Tab has been reordered via drag-drop
    // Rebuild our document tracking to match new order
    std::vector<IDocumentWindow*> new_order;
    int count = notebook_->GetPageCount();
    
    for (int i = 0; i < count; ++i) {
        wxWindow* page = notebook_->GetPage(i);
        // Find the corresponding IDocumentWindow
        for (auto* doc : documents_) {
            if (doc->GetWindow() == page || doc->GetContent() == page) {
                new_order.push_back(doc);
                break;
            }
        }
    }
    
    documents_ = new_order;
}

// ============================================================================
// Helper Methods
// ============================================================================
wxString DocumentManager::BuildTabLabel(IDocumentWindow* doc) const {
    if (!doc) {
        return wxString();
    }
    
    wxString label = doc->GetWindowTitle();
    
    // Add modified indicator
    if (doc->IsModified()) {
        label = "*" + label;
    }
    
    return label;
}

int DocumentManager::FindDocumentIndex(IDocumentWindow* doc) const {
    for (size_t i = 0; i < documents_.size(); ++i) {
        if (documents_[i] == doc) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

void DocumentManager::NotifyActivationChanged(IDocumentWindow* old_doc, IDocumentWindow* new_doc) {
    if (old_doc == new_doc) {
        return;
    }
    
    is_updating_ = true;
    
    if (old_doc) {
        old_doc->OnDeactivate();
    }
    
    if (new_doc) {
        new_doc->OnActivate();
    }
    
    // Send activation event
    wxCommandEvent evt(DOCUMENT_ACTIVATED);
    evt.SetClientData(new_doc);
    ProcessEvent(evt);
    
    is_updating_ = false;
}

} // namespace scratchrobin
