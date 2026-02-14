/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0
 */
#ifndef SCRATCHROBIN_FORM_CONTAINER_H
#define SCRATCHROBIN_FORM_CONTAINER_H

#include <functional>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

#include <wx/aui/aui.h>
#include <wx/panel.h>
#include <wx/window.h>

namespace scratchrobin {
namespace ui {

// Forward declarations
class FormWindow;
class FormContainer;

/**
 * @brief Categories of forms that can be hosted in containers.
 */
enum class FormCategory {
    Unknown,
    Diagram,        // ERD, DFD, UML diagrams
    SqlEditor,      // SQL query editors
    TableDesigner,  // Table design forms
    Report,         // Report viewers/designers
    Monitor,        // Monitoring dashboards
    Settings,       // Configuration dialogs
    Tool            // Utility tools
};

/**
 * @brief Interface for windows that can be hosted in FormContainers.
 */
class IFormWindow {
public:
    virtual ~IFormWindow() = default;
    
    /**
     * @brief Get the form category for type-checking.
     */
    virtual FormCategory GetFormCategory() const = 0;
    
    /**
     * @brief Get a unique identifier for this form instance.
     */
    virtual std::string GetFormId() const = 0;
    
    /**
     * @brief Get the display title for tabs.
     */
    virtual wxString GetFormTitle() const = 0;
    
    /**
     * @brief Called when the form is activated (tab selected).
     */
    virtual void OnFormActivated() {}
    
    /**
     * @brief Called when the form is deactivated (tab unselected).
     */
    virtual void OnFormDeactivated() {}
    
    /**
     * @brief Check if this form can accept a child form (for containment).
     */
    virtual bool CanAcceptChild(IFormWindow* child) const { return false; }
    
    /**
     * @brief Add a child form (for diagram containment).
     */
    virtual void AddChildForm(IFormWindow* child) {}
    
    /**
     * @brief Remove a child form.
     */
    virtual void RemoveChildForm(IFormWindow* child) {}
    
    /**
     * @brief Get the wxWindow pointer for this form.
     */
    virtual wxWindow* GetWindow() = 0;
    virtual const wxWindow* GetWindow() const = 0;
};

/**
 * @brief A container that hosts multiple forms in a tabbed interface.
 * 
 * Features:
 * - Invisible when empty
 * - Single form shown without tabs
 * - Tab bar appears with 2+ forms
 * - Type-restricted form acceptance
 * - Drag-drop support for docking
 */
class FormContainer : public wxPanel {
public:
    /**
     * @brief Container configuration.
     */
    struct Config {
        std::string containerId;
        FormCategory acceptedCategory = FormCategory::Unknown;  // Unknown = accept all
        bool allowMultipleForms = true;
        bool showCloseButtons = true;
        wxString defaultTitle = "Forms";
        wxBitmap containerIcon;
    };

    FormContainer(wxWindow* parent, const Config& config);
    ~FormContainer() override;

    // Disable copy/move
    FormContainer(const FormContainer&) = delete;
    FormContainer& operator=(const FormContainer&) = delete;

    /**
     * @brief Add a form to this container.
     * @param form The form to add (must match accepted category)
     * @return true if form was added
     */
    bool AddForm(std::shared_ptr<IFormWindow> form);
    
    /**
     * @brief Add a plain wxWindow (backward compatibility).
     * @param window The window to add
     * @param title Tab title
     * @param formId Unique identifier
     * @return true if window was added
     */
    bool AddWindow(wxWindow* window, const wxString& title, const std::string& formId);

    /**
     * @brief Remove a form from this container.
     */
    void RemoveForm(const std::string& formId);

    /**
     * @brief Activate (select) a specific form.
     */
    void ActivateForm(const std::string& formId);

    /**
     * @brief Check if this container can accept a form.
     */
    bool CanAcceptForm(IFormWindow* form) const;

    /**
     * @brief Get the number of forms in this container.
     */
    size_t GetFormCount() const;

    /**
     * @brief Get all form IDs in this container.
     */
    std::vector<std::string> GetFormIds() const;

    /**
     * @brief Get the currently active form.
     */
    std::shared_ptr<IFormWindow> GetActiveForm() const;

    /**
     * @brief Check if this container is empty.
     */
    bool IsEmpty() const { return forms_.empty(); }

    /**
     * @brief Get the container configuration.
     */
    const Config& GetConfig() const { return config_; }

    /**
     * @brief Set visibility mode based on form count.
     * Empty = invisible, Single = no tabs, Multiple = show tabs
     */
    void UpdateVisibility();

    /**
     * @brief Callback when a form is dropped onto this container.
     */
    using DropCallback = std::function<void(FormContainer* target, 
                                             std::shared_ptr<IFormWindow> form)>;
    void SetDropCallback(DropCallback callback) { dropCallback_ = callback; }
    DropCallback GetDropCallback() const { return dropCallback_; }

    /**
     * @brief Enable drag-drop as a drop target.
     */
    void EnableDropTarget(bool enable);
    
    /**
     * @brief Get the wxWindow pointer for this container.
     */
    wxWindow* GetWindow() { return this; }

protected:
    void OnPageChanged(wxAuiNotebookEvent& event);
    void OnPageClose(wxAuiNotebookEvent& event);
    void OnDragOver(wxMouseEvent& event);
    void OnDrop(wxMouseEvent& event);

private:
    Config config_;
    wxAuiNotebook* notebook_ = nullptr;
    
    // Form storage: formId -> form
    std::map<std::string, std::shared_ptr<IFormWindow>> forms_;
    std::string activeFormId_;
    
    DropCallback dropCallback_;
    
    void CreateNotebook();
    void DestroyNotebook();
    wxString MakeTabTitle(const IFormWindow* form) const;
};

/**
 * @brief Special container for diagram containment with mini-view support.
 */
class DiagramContainer : public FormContainer {
public:
    DiagramContainer(wxWindow* parent, const std::string& containerId);
    
    /**
     * @brief Add a diagram as a child element (for nested diagrams).
     * Creates a mini-view representation.
     */
    bool AddDiagramAsChild(std::shared_ptr<IFormWindow> diagram, 
                           IFormWindow* parentDiagram);
};

/**
 * @brief Manager for all form containers in the application.
 */
class FormContainerManager {
public:
    static FormContainerManager& Instance();

    /**
     * @brief Create a new container with the given configuration.
     */
    FormContainer* CreateContainer(wxWindow* parent, const FormContainer::Config& config);

    /**
     * @brief Register a default container for a form category.
     */
    void RegisterDefaultContainer(FormCategory category, FormContainer* container);

    /**
     * @brief Get the default container for a form category.
     */
    FormContainer* GetDefaultContainer(FormCategory category) const;

    /**
     * @brief Find a container by ID.
     */
    FormContainer* FindContainer(const std::string& containerId) const;

    /**
     * @brief Move a form from one container to another.
     */
    bool MoveForm(const std::string& formId, FormContainer* from, FormContainer* to);

    /**
     * @brief Get all containers.
     */
    std::vector<FormContainer*> GetAllContainers() const;

    /**
     * @brief Find the best container for a form (based on category).
     */
    FormContainer* FindBestContainer(IFormWindow* form) const;

private:
    FormContainerManager() = default;
    ~FormContainerManager() = default;

    std::map<std::string, FormContainer*> containers_;
    std::map<FormCategory, FormContainer*> defaultContainers_;
};

} // namespace ui
} // namespace scratchrobin

#endif // SCRATCHROBIN_FORM_CONTAINER_H
