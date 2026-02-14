/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0
 */
#ifndef SCRATCHROBIN_FORM_DRAG_DROP_H
#define SCRATCHROBIN_FORM_DRAG_DROP_H

#include <wx/dnd.h>
#include <wx/dataobj.h>

#include <string>

namespace scratchrobin {
namespace ui {

class FormContainer;
class IFormWindow;

/**
 * @brief Custom data format for dragging forms between containers.
 */
class FormDragData : public wxDataObjectSimple {
public:
    static const wxDataFormat& GetFormat();
    
    FormDragData(const std::string& formId = "", const std::string& sourceContainerId = "");
    
    // wxDataObjectSimple overrides
    size_t GetDataSize() const override;
    bool GetDataHere(void* buf) const override;
    bool SetData(size_t len, const void* buf) override;
    
    // Accessors
    std::string GetFormId() const { return formId_; }
    std::string GetSourceContainerId() const { return sourceContainerId_; }
    void SetFormId(const std::string& id) { formId_ = id; }
    void SetSourceContainerId(const std::string& id) { sourceContainerId_ = id; }
    
private:
    std::string formId_;
    std::string sourceContainerId_;
};

/**
 * @brief Drop target for FormContainer that accepts dragged forms.
 */
class FormDropTarget : public wxDropTarget {
public:
    explicit FormDropTarget(FormContainer* container);
    
    // wxDropTarget overrides
    wxDragResult OnEnter(wxCoord x, wxCoord y, wxDragResult defResult) override;
    wxDragResult OnDragOver(wxCoord x, wxCoord y, wxDragResult defResult) override;
    void OnLeave() override;
    wxDragResult OnData(wxCoord x, wxCoord y, wxDragResult defResult) override;
    
    void SetHighlight(bool highlight);
    
private:
    FormContainer* container_;
    FormDragData formData_;
    bool isHighlighted_ = false;
};

/**
 * @brief Drag source for forms that initiates drag operations.
 */
class FormDragSource : public wxDropSource {
public:
    FormDragSource(wxWindow* sourceWindow, IFormWindow* form, FormContainer* sourceContainer);
    
    bool DoDragDrop();
    
private:
    FormDragData data_;
};

} // namespace ui
} // namespace scratchrobin

#endif // SCRATCHROBIN_FORM_DRAG_DROP_H
