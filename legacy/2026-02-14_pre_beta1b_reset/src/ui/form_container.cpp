/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0
 */
#include "form_container.h"
#include "form_drag_drop.h"

#include <algorithm>

#include <wx/aui/auibook.h>
#include <wx/dnd.h>
#include <wx/sizer.h>

namespace scratchrobin {
namespace ui {

//=============================================================================
// FormContainer Implementation
//=============================================================================

FormContainer::FormContainer(wxWindow* parent, const Config& config)
    : wxPanel(parent, wxID_ANY), config_(config) {
    
    // Start with a simple sizer - notebook created on first form
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(sizer);
    
    // Container starts visible so it can receive layout properly
    // Individual content visibility is managed by UpdateVisibility()
}

FormContainer::~FormContainer() {
    // Forms are shared_ptr, will clean up automatically
    forms_.clear();
}

void FormContainer::CreateNotebook() {
    if (notebook_) {
        return;
    }
    
    long style = wxAUI_NB_TOP | wxAUI_NB_TAB_MOVE | wxAUI_NB_SCROLL_BUTTONS;
    
    if (config_.showCloseButtons) {
        style |= wxAUI_NB_CLOSE_ON_ACTIVE_TAB | wxAUI_NB_CLOSE_ON_ALL_TABS;
    }
    
    notebook_ = new wxAuiNotebook(this, wxID_ANY, wxDefaultPosition, 
                                   wxDefaultSize, style);
    
    // Bind events
    notebook_->Bind(wxEVT_AUINOTEBOOK_PAGE_CHANGED, &FormContainer::OnPageChanged, this);
    notebook_->Bind(wxEVT_AUINOTEBOOK_PAGE_CLOSE, &FormContainer::OnPageClose, this);
    
    GetSizer()->Add(notebook_, 1, wxEXPAND);
    Layout();
}

void FormContainer::DestroyNotebook() {
    if (!notebook_) {
        return;
    }
    
    notebook_->Unbind(wxEVT_AUINOTEBOOK_PAGE_CHANGED, &FormContainer::OnPageChanged, this);
    notebook_->Unbind(wxEVT_AUINOTEBOOK_PAGE_CLOSE, &FormContainer::OnPageClose, this);
    
    GetSizer()->Detach(notebook_);
    notebook_->Destroy();
    notebook_ = nullptr;
    
    Layout();
}

bool FormContainer::CanAcceptForm(IFormWindow* form) const {
    if (!form) {
        return false;
    }
    
    // Unknown category means accept all
    if (config_.acceptedCategory == FormCategory::Unknown) {
        return true;
    }
    
    return form->GetFormCategory() == config_.acceptedCategory;
}

bool FormContainer::AddForm(std::shared_ptr<IFormWindow> form) {
    if (!form || !CanAcceptForm(form.get())) {
        return false;
    }
    
    const std::string formId = form->GetFormId();
    
    // Check if already in this container
    if (forms_.count(formId)) {
        // Just activate it
        ActivateForm(formId);
        return true;
    }
    
    // If single-form mode and already has a form, reject
    if (!config_.allowMultipleForms && !forms_.empty()) {
        return false;
    }
    
    // Store the form
    forms_[formId] = form;
    
    // Show the container
    Show();
    
    if (forms_.size() == 1) {
        // Single form - add directly to sizer, no notebook
        wxWindow* window = form->GetWindow();
        if (window) {
            GetSizer()->Add(window, 1, wxEXPAND);
            window->Show();
            activeFormId_ = formId;
            form->OnFormActivated();
        }
    } else {
        // Multiple forms - need notebook
        if (forms_.size() == 2) {
            // Transitioning from single to multiple
            // Move first form to notebook
            auto firstForm = forms_.begin()->second;
            wxWindow* firstWindow = firstForm->GetWindow();
            
            if (firstWindow) {
                GetSizer()->Detach(firstWindow);
                firstWindow->Hide();
            }
            
            CreateNotebook();
            
            // Add first form to notebook
            if (firstWindow) {
                notebook_->AddPage(firstWindow, MakeTabTitle(firstForm.get()), 
                                   false, config_.containerIcon);
            }
        }
        
        // Add new form to notebook
        wxWindow* window = form->GetWindow();
        if (window && notebook_) {
            notebook_->AddPage(window, MakeTabTitle(form.get()), 
                              true, config_.containerIcon);
            activeFormId_ = formId;
            form->OnFormActivated();
        }
    }
    
    UpdateVisibility();
    Layout();
    Refresh();
    
    return true;
}

bool FormContainer::AddWindow(wxWindow* window, const wxString& title, 
                               const std::string& formId) {
    if (!window) {
        return false;
    }
    
    // Check if already in this container
    if (forms_.count(formId)) {
        ActivateForm(formId);
        return true;
    }
    
    // If single-form mode and already has a form, reject
    if (!config_.allowMultipleForms && !forms_.empty()) {
        return false;
    }
    
    // Create a wrapper IFormWindow for the plain wxWindow
    class WindowWrapper : public IFormWindow {
    public:
        WindowWrapper(wxWindow* w, const wxString& t, const std::string& id) 
            : window_(w), title_(t), id_(id) {}
        
        FormCategory GetFormCategory() const override { return FormCategory::Unknown; }
        std::string GetFormId() const override { return id_; }
        wxString GetFormTitle() const override { return title_; }
        wxWindow* GetWindow() override { return window_; }
        const wxWindow* GetWindow() const override { return window_; }
        
    private:
        wxWindow* window_;
        wxString title_;
        std::string id_;
    };
    
    auto wrapper = std::make_shared<WindowWrapper>(window, title, formId);
    return AddForm(wrapper);
}

void FormContainer::RemoveForm(const std::string& formId) {
    auto it = forms_.find(formId);
    if (it == forms_.end()) {
        return;
    }
    
    auto form = it->second;
    forms_.erase(it);
    
    wxWindow* window = form->GetWindow();
    
    if (forms_.empty()) {
        // Last form removed
        if (window) {
            window->Hide();
            GetSizer()->Detach(window);
        }
        DestroyNotebook();
        Hide();
    } else if (forms_.size() == 1) {
        // Transitioning to single form
        // Get remaining form
        auto remainingForm = forms_.begin()->second;
        
        // Remove from notebook
        if (window && notebook_) {
            int idx = notebook_->GetPageIndex(window);
            if (idx != wxNOT_FOUND) {
                notebook_->RemovePage(idx);
            }
        }
        
        // Destroy notebook
        DestroyNotebook();
        
        // Add remaining form directly
        wxWindow* remainingWindow = remainingForm->GetWindow();
        if (remainingWindow) {
            GetSizer()->Add(remainingWindow, 1, wxEXPAND);
            remainingWindow->Show();
            activeFormId_ = remainingForm->GetFormId();
            remainingForm->OnFormActivated();
        }
    } else {
        // Still multiple forms
        if (window && notebook_) {
            int idx = notebook_->GetPageIndex(window);
            if (idx != wxNOT_FOUND) {
                notebook_->DeletePage(idx);
            }
        }
    }
    
    if (activeFormId_ == formId) {
        activeFormId_.clear();
        if (!forms_.empty()) {
            activeFormId_ = forms_.begin()->first;
            forms_[activeFormId_]->OnFormActivated();
        }
    }
    
    form->OnFormDeactivated();
    
    UpdateVisibility();
    Layout();
    Refresh();
}

void FormContainer::ActivateForm(const std::string& formId) {
    if (activeFormId_ == formId) {
        return;
    }
    
    auto it = forms_.find(formId);
    if (it == forms_.end()) {
        return;
    }
    
    // Deactivate current
    if (!activeFormId_.empty() && forms_.count(activeFormId_)) {
        forms_[activeFormId_]->OnFormDeactivated();
    }
    
    // Activate new
    activeFormId_ = formId;
    
    if (notebook_ && forms_.size() > 1) {
        wxWindow* window = it->second->GetWindow();
        if (window) {
            int idx = notebook_->GetPageIndex(window);
            if (idx != wxNOT_FOUND) {
                notebook_->SetSelection(idx);
            }
        }
    }
    
    it->second->OnFormActivated();
}

std::shared_ptr<IFormWindow> FormContainer::GetActiveForm() const {
    if (activeFormId_.empty() || !forms_.count(activeFormId_)) {
        return nullptr;
    }
    return forms_.at(activeFormId_);
}

size_t FormContainer::GetFormCount() const {
    return forms_.size();
}

std::vector<std::string> FormContainer::GetFormIds() const {
    std::vector<std::string> ids;
    ids.reserve(forms_.size());
    for (const auto& [id, form] : forms_) {
        ids.push_back(id);
    }
    return ids;
}

wxString FormContainer::MakeTabTitle(const IFormWindow* form) const {
    if (!form) {
        return "Untitled";
    }
    
    wxString title = form->GetFormTitle();
    if (title.IsEmpty()) {
        title = "Untitled";
    }
    
    // Truncate if too long
    if (title.Length() > 30) {
        title = title.Left(27) + "...";
    }
    
    return title;
}

void FormContainer::UpdateVisibility() {
    if (forms_.empty()) {
        Hide();
    } else {
        Show();
    }
}

void FormContainer::OnPageChanged(wxAuiNotebookEvent& event) {
    int newIdx = event.GetSelection();
    int oldIdx = event.GetOldSelection();
    
    // Deactivate old page
    if (oldIdx != wxNOT_FOUND && notebook_) {
        wxWindow* oldWindow = notebook_->GetPage(oldIdx);
        if (oldWindow) {
            for (const auto& [id, form] : forms_) {
                if (form->GetWindow() == oldWindow) {
                    form->OnFormDeactivated();
                    break;
                }
            }
        }
    }
    
    // Activate new page
    if (newIdx != wxNOT_FOUND && notebook_) {
        wxWindow* newWindow = notebook_->GetPage(newIdx);
        if (newWindow) {
            for (const auto& [id, form] : forms_) {
                if (form->GetWindow() == newWindow) {
                    activeFormId_ = id;
                    form->OnFormActivated();
                    break;
                }
            }
        }
    }
    
    event.Skip();
}

void FormContainer::OnPageClose(wxAuiNotebookEvent& event) {
    int idx = event.GetSelection();
    
    if (notebook_) {
        wxWindow* window = notebook_->GetPage(idx);
        if (window) {
            for (const auto& [id, form] : forms_) {
                if (form->GetWindow() == window) {
                    RemoveForm(id);
                    break;
                }
            }
        }
    }
    
    event.Veto();  // We handle removal ourselves
}

void FormContainer::EnableDropTarget(bool enable) {
    if (enable) {
        SetDropTarget(new FormDropTarget(this));
    } else {
        SetDropTarget(nullptr);
    }
}

//=============================================================================
// DiagramContainer Implementation
//=============================================================================

DiagramContainer::DiagramContainer(wxWindow* parent, const std::string& containerId)
    : FormContainer(parent, [containerId]() {
        Config cfg;
        cfg.containerId = containerId;
        cfg.acceptedCategory = FormCategory::Diagram;
        cfg.allowMultipleForms = true;
        cfg.showCloseButtons = true;
        cfg.defaultTitle = "Diagrams";
        return cfg;
      }()) {
}

bool DiagramContainer::AddDiagramAsChild(std::shared_ptr<IFormWindow> diagram,
                                          IFormWindow* parentDiagram) {
    if (!diagram || !parentDiagram) {
        return false;
    }
    
    if (diagram->GetFormCategory() != FormCategory::Diagram) {
        return false;
    }
    
    // Check if parent can accept this child
    if (!parentDiagram->CanAcceptChild(diagram.get())) {
        return false;
    }
    
    // Add to parent's child list
    parentDiagram->AddChildForm(diagram.get());
    
    // Also add to container for tab management
    return AddForm(diagram);
}

//=============================================================================
// FormContainerManager Implementation
//=============================================================================

FormContainerManager& FormContainerManager::Instance() {
    static FormContainerManager instance;
    return instance;
}

FormContainer* FormContainerManager::CreateContainer(wxWindow* parent, 
                                                      const FormContainer::Config& config) {
    auto* container = new FormContainer(parent, config);
    containers_[config.containerId] = container;
    return container;
}

void FormContainerManager::RegisterDefaultContainer(FormCategory category, 
                                                     FormContainer* container) {
    if (container) {
        defaultContainers_[category] = container;
    }
}

FormContainer* FormContainerManager::GetDefaultContainer(FormCategory category) const {
    auto it = defaultContainers_.find(category);
    if (it != defaultContainers_.end()) {
        return it->second;
    }
    return nullptr;
}

FormContainer* FormContainerManager::FindContainer(const std::string& containerId) const {
    auto it = containers_.find(containerId);
    if (it != containers_.end()) {
        return it->second;
    }
    return nullptr;
}

bool FormContainerManager::MoveForm(const std::string& formId, 
                                     FormContainer* from, 
                                     FormContainer* to) {
    if (!from || !to) {
        return false;
    }
    
    // Find the form
    auto forms = from->GetFormIds();
    if (std::find(forms.begin(), forms.end(), formId) == forms.end()) {
        return false;
    }
    
    // Get the form
    auto activeForm = from->GetActiveForm();
    if (!activeForm || activeForm->GetFormId() != formId) {
        // Need to find it differently
        // This is a limitation - we need to store forms differently
        return false;
    }
    
    // Check if destination can accept
    if (!to->CanAcceptForm(activeForm.get())) {
        return false;
    }
    
    // Remove from source
    from->RemoveForm(formId);
    
    // Add to destination
    return to->AddForm(activeForm);
}

std::vector<FormContainer*> FormContainerManager::GetAllContainers() const {
    std::vector<FormContainer*> result;
    result.reserve(containers_.size());
    for (const auto& [id, container] : containers_) {
        result.push_back(container);
    }
    return result;
}

FormContainer* FormContainerManager::FindBestContainer(IFormWindow* form) const {
    if (!form) {
        return nullptr;
    }
    
    FormCategory category = form->GetFormCategory();
    
    // Try category-specific container first
    auto* categoryContainer = GetDefaultContainer(category);
    if (categoryContainer && categoryContainer->CanAcceptForm(form)) {
        return categoryContainer;
    }
    
    // Find any container that accepts this form
    for (const auto& [id, container] : containers_) {
        if (container->CanAcceptForm(form)) {
            return container;
        }
    }
    
    return nullptr;
}

} // namespace ui
} // namespace scratchrobin
