/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0
 */
#include "form_drag_drop.h"
#include "form_container.h"

#include <wx/window.h>

namespace scratchrobin {
namespace ui {

//=============================================================================
// FormDragData Implementation
//=============================================================================

const wxDataFormat& FormDragData::GetFormat() {
    static wxDataFormat format("application/x-scratchrobin-form");
    return format;
}

FormDragData::FormDragData(const std::string& formId, const std::string& sourceContainerId)
    : wxDataObjectSimple(GetFormat()),
      formId_(formId),
      sourceContainerId_(sourceContainerId) {
}

size_t FormDragData::GetDataSize() const {
    // Format: "formId|sourceContainerId\0"
    return formId_.length() + 1 + sourceContainerId_.length() + 1;
}

bool FormDragData::GetDataHere(void* buf) const {
    char* ptr = static_cast<char*>(buf);
    
    // Copy formId
    std::copy(formId_.begin(), formId_.end(), ptr);
    ptr += formId_.length();
    *ptr++ = '|';
    
    // Copy sourceContainerId
    std::copy(sourceContainerId_.begin(), sourceContainerId_.end(), ptr);
    ptr += sourceContainerId_.length();
    *ptr = '\0';
    
    return true;
}

bool FormDragData::SetData(size_t len, const void* buf) {
    const char* data = static_cast<const char*>(buf);
    std::string str(data, len > 0 ? len - 1 : 0);
    
    // Parse "formId|sourceContainerId"
    size_t sep = str.find('|');
    if (sep == std::string::npos) {
        formId_ = str;
        sourceContainerId_.clear();
    } else {
        formId_ = str.substr(0, sep);
        sourceContainerId_ = str.substr(sep + 1);
    }
    
    return true;
}

//=============================================================================
// FormDropTarget Implementation
//=============================================================================

FormDropTarget::FormDropTarget(FormContainer* container)
    : wxDropTarget(&formData_),
      container_(container),
      formData_() {
}

wxDragResult FormDropTarget::OnEnter(wxCoord x, wxCoord y, wxDragResult defResult) {
    if (!container_) {
        return wxDragNone;
    }
    
    SetHighlight(true);
    return defResult;
}

wxDragResult FormDropTarget::OnDragOver(wxCoord x, wxCoord y, wxDragResult defResult) {
    if (!container_) {
        return wxDragNone;
    }
    
    // Could check if the form can be accepted here
    // For now, accept all valid drags
    return defResult;
}

void FormDropTarget::OnLeave() {
    SetHighlight(false);
}

wxDragResult FormDropTarget::OnData(wxCoord x, wxCoord y, wxDragResult defResult) {
    SetHighlight(false);
    
    if (!container_) {
        return wxDragNone;
    }
    
    if (!GetData()) {
        return wxDragNone;
    }
    
    std::string formId = formData_.GetFormId();
    std::string sourceId = formData_.GetSourceContainerId();
    
    // Don't accept drops from self
    if (sourceId == container_->GetConfig().containerId) {
        return wxDragNone;
    }
    
    // Invoke the container's drop callback if set
    auto callback = container_->GetDropCallback();
    if (callback) {
        // Find the form in the source container
        // This requires FormContainerManager to be involved
        // For now, just signal that a drop occurred
        // callback(container_, formId, sourceId);
    }
    
    return defResult;
}

void FormDropTarget::SetHighlight(bool highlight) {
    if (isHighlighted_ == highlight) {
        return;
    }
    
    isHighlighted_ = highlight;
    
    if (container_) {
        // Visual feedback - could change background color or draw border
        wxWindow* window = container_->GetWindow();
        if (window) {
            if (highlight) {
                window->SetBackgroundColour(wxColour(200, 220, 255));  // Light blue
            } else {
                window->SetBackgroundColour(wxNullColour);  // Reset
            }
            window->Refresh();
        }
    }
}

//=============================================================================
// FormDragSource Implementation
//=============================================================================

FormDragSource::FormDragSource(wxWindow* sourceWindow, IFormWindow* form, 
                                FormContainer* sourceContainer)
    : wxDropSource(sourceWindow),
      data_(form ? form->GetFormId() : "", 
            sourceContainer ? sourceContainer->GetConfig().containerId : "") {
    SetData(data_);
}

bool FormDragSource::DoDragDrop() {
    wxDragResult result = wxDropSource::DoDragDrop(wxDrag_CopyOnly);
    return (result == wxDragCopy || result == wxDragMove);
}

} // namespace ui
} // namespace scratchrobin
