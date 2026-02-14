/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "notation_test_dialog.h"

#include "diagram_canvas.h"
#include "diagram_model.h"

#include <wx/button.h>
#include <wx/notebook.h>
#include <wx/panel.h>
#include <wx/sizer.h>
#include <wx/stattext.h>

namespace scratchrobin {

wxBEGIN_EVENT_TABLE(NotationTestDialog, wxDialog)
wxEND_EVENT_TABLE()

NotationTestDialog::NotationTestDialog(wxWindow* parent)
    : wxDialog(parent, wxID_ANY, "ERD Notation Reference",
               wxDefaultPosition, wxSize(900, 700)) {
    BuildLayout();
}

void NotationTestDialog::BuildLayout() {
    auto* root = new wxBoxSizer(wxVERTICAL);
    
    // Title
    root->Add(new wxStaticText(this, wxID_ANY, 
        "ERD Notation Reference - Visual examples of each notation style"),
        0, wxALL, 12);
    
    // Notebook with tabs for each notation
    auto* notebook = new wxNotebook(this, wxID_ANY);
    
    CreateCrowsFootPage(notebook);
    CreateIDEF1XPage(notebook);
    CreateUMLPage(notebook);
    CreateChenPage(notebook);
    
    root->Add(notebook, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    // Close button
    auto* btnSizer = new wxBoxSizer(wxHORIZONTAL);
    btnSizer->AddStretchSpacer(1);
    btnSizer->Add(new wxButton(this, wxID_CLOSE, "Close"), 0);
    root->Add(btnSizer, 0, wxEXPAND | wxALL, 12);
    
    SetSizer(root);
}

void NotationTestDialog::CreateCrowsFootPage(wxNotebook* notebook) {
    auto* page = new wxPanel(notebook);
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    
    sizer->Add(new wxStaticText(page, wxID_ANY, 
        "Crow's Foot Notation (IE Notation):\n\n"
        "| Symbol | Meaning |\n"
        "|--------|---------| |\n"
        "| | | One and only one |\n"
        "| O | | Zero or one |\n"
        "| < | | One or many |\n"
        "| O< | | Zero or many |\n\n"
        "Example: Customer ||---O< Order (One customer has zero or many orders)"),
        1, wxALL, 12);
    
    // Add a canvas with sample diagram
    auto* canvas = new DiagramCanvas(page, DiagramType::Erd);
    canvas->SetNotation(ErdNotation::CrowsFoot);
    
    // Create sample entities
    DiagramModel model(DiagramType::Erd);
    
    DiagramNode customer;
    customer.id = "customer";
    customer.name = "Customer";
    customer.type = "TABLE";
    customer.x = 50;
    customer.y = 100;
    customer.width = 150;
    customer.height = 100;
    customer.attributes.push_back({"id", "INT", true, false, false});
    customer.attributes.push_back({"name", "VARCHAR(100)", false, false, false});
    model.AddNode(customer);
    
    DiagramNode order;
    order.id = "order";
    order.name = "Order";
    order.type = "TABLE";
    order.x = 350;
    order.y = 100;
    order.width = 150;
    order.height = 100;
    order.attributes.push_back({"id", "INT", true, false, false});
    order.attributes.push_back({"customer_id", "INT", false, true, false});
    model.AddNode(order);
    
    DiagramEdge edge;
    edge.id = "e1";
    edge.source_id = "customer";
    edge.target_id = "order";
    edge.label = "places";
    edge.target_cardinality = Cardinality::ZeroOrMany;
    model.AddEdge(edge);
    
    sizer->Add(canvas, 2, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    page->SetSizer(sizer);
    notebook->AddPage(page, "Crow's Foot");
}

void NotationTestDialog::CreateIDEF1XPage(wxNotebook* notebook) {
    auto* page = new wxPanel(notebook);
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    
    sizer->Add(new wxStaticText(page, wxID_ANY,
        "IDEF1X Notation:\n\n"
        "- Independent entities: Square corners\n"
        "- Dependent entities: Rounded corners\n"
        "- Solid line: Identifying relationship\n"
        "- Dashed line: Non-identifying relationship\n"
        "- Cardinality shown as 'P' (one), 'Z' (zero/one), 'M' (many)"),
        1, wxALL, 12);
    
    auto* canvas = new DiagramCanvas(page, DiagramType::Erd);
    canvas->SetNotation(ErdNotation::IDEF1X);
    sizer->Add(canvas, 2, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    page->SetSizer(sizer);
    notebook->AddPage(page, "IDEF1X");
}

void NotationTestDialog::CreateUMLPage(wxNotebook* notebook) {
    auto* page = new wxPanel(notebook);
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    
    sizer->Add(new wxStaticText(page, wxID_ANY,
        "UML Class Diagram Notation:\n\n"
        "- Three-compartment boxes: Name, Attributes, Operations\n"
        "- Association lines with multiplicity\n"
        "- Diamonds: Composition (filled) or Aggregation (empty)\n"
        "- Arrows: Inheritance (solid) or Dependency (dashed)"),
        1, wxALL, 12);
    
    auto* canvas = new DiagramCanvas(page, DiagramType::Erd);
    canvas->SetNotation(ErdNotation::UML);
    sizer->Add(canvas, 2, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    page->SetSizer(sizer);
    notebook->AddPage(page, "UML");
}

void NotationTestDialog::CreateChenPage(wxNotebook* notebook) {
    auto* page = new wxPanel(notebook);
    auto* sizer = new wxBoxSizer(wxVERTICAL);
    
    sizer->Add(new wxStaticText(page, wxID_ANY,
        "Chen Notation:\n\n"
        "- Entities: Rectangles\n"
        "- Relationships: Diamonds\n"
        "- Attributes: Ovals connected to entities\n"
        "- Double border: Weak entity\n"
        "- 1, M, N labels for cardinality"),
        1, wxALL, 12);
    
    auto* canvas = new DiagramCanvas(page, DiagramType::Erd);
    canvas->SetNotation(ErdNotation::Chen);
    sizer->Add(canvas, 2, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 12);
    
    page->SetSizer(sizer);
    notebook->AddPage(page, "Chen");
}

} // namespace scratchrobin
