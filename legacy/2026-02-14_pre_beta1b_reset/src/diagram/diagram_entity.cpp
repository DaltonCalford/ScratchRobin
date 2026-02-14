// Copyright (c) 2026, Dennis C. Alfonso
// Licensed under the MIT License. See LICENSE file in the project root.
#include "diagram_entity.h"
#include "../core/project.h"
#include "../ui/diagram_model.h"

#include <map>
#include <sstream>

namespace scratchrobin {
namespace diagram {

// Static type information
struct DiagramTypeInfo {
    std::string name;
    std::string plural;
    std::string kind;
    std::string extension;
    int icon_index;
};

static const std::map<DiagramEntityType, DiagramTypeInfo> kTypeInfo = {
    {DiagramEntityType::ERD, {
        "ER Diagram",
        "ER Diagrams",
        "erd_diagram",
        ".erd.json",
        20  // Icon index for ERD
    }},
    {DiagramEntityType::DataFlow, {
        "Data Flow Diagram",
        "Data Flow Diagrams",
        "dfd_diagram",
        ".dfd.json",
        21  // Icon index for DFD
    }},
    {DiagramEntityType::UML, {
        "UML Diagram",
        "UML Diagrams",
        "uml_diagram",
        ".uml.json",
        22  // Icon index for UML
    }},
    {DiagramEntityType::MindMap, {
        "Mind Map",
        "Mind Maps",
        "mindmap",
        ".mindmap.json",
        23  // Icon index for Mind Map
    }},
    {DiagramEntityType::Whiteboard, {
        "Whiteboard",
        "Whiteboards",
        "whiteboard",
        ".wb.json",
        24  // Icon index for Whiteboard
    }},
    {DiagramEntityType::Silverston, {
        "Silverston Model",
        "Silverston Models",
        "silverston",
        ".silver.json",
        25  // Icon index for Silverston
    }}
};

std::string GetDiagramEntityTypeName(DiagramEntityType type) {
    auto it = kTypeInfo.find(type);
    return (it != kTypeInfo.end()) ? it->second.name : "Unknown";
}

std::string GetDiagramEntityTypePluralName(DiagramEntityType type) {
    auto it = kTypeInfo.find(type);
    return (it != kTypeInfo.end()) ? it->second.plural : "Unknown";
}

int GetDiagramEntityTypeIcon(DiagramEntityType type) {
    auto it = kTypeInfo.find(type);
    return (it != kTypeInfo.end()) ? it->second.icon_index : 0;
}

std::string GetDiagramEntityTypeExtension(DiagramEntityType type) {
    auto it = kTypeInfo.find(type);
    return (it != kTypeInfo.end()) ? it->second.extension : ".diagram.json";
}

std::string GetDiagramEntityTypeKind(DiagramEntityType type) {
    auto it = kTypeInfo.find(type);
    return (it != kTypeInfo.end()) ? it->second.kind : "diagram";
}

DiagramEntityType KindToDiagramEntityType(const std::string& kind) {
    for (const auto& [type, info] : kTypeInfo) {
        if (info.kind == kind) {
            return type;
        }
    }
    return DiagramEntityType::ERD;  // Default
}

bool IsDiagramKind(const std::string& kind) {
    for (const auto& [type, info] : kTypeInfo) {
        if (info.kind == kind) {
            return true;
        }
    }
    return kind == "diagram";  // Legacy generic diagram kind
}

DiagramEntityType GetDiagramTypeFromKind(const std::string& kind) {
    return KindToDiagramEntityType(kind);
}

DiagramType EntityTypeToDiagramType(DiagramEntityType type) {
    switch (type) {
        case DiagramEntityType::ERD:
            return DiagramType::Erd;
        case DiagramEntityType::DataFlow:
            return DiagramType::DataFlow;
        case DiagramEntityType::UML:
            return DiagramType::Erd;  // UML uses similar rendering
        case DiagramEntityType::MindMap:
            return DiagramType::MindMap;
        case DiagramEntityType::Whiteboard:
            return DiagramType::Whiteboard;
        case DiagramEntityType::Silverston:
            return DiagramType::Silverston;
        default:
            return DiagramType::Erd;
    }
}

DiagramEntityType DiagramTypeToEntityType(DiagramType type) {
    switch (type) {
        case DiagramType::Erd:
            return DiagramEntityType::ERD;
        case DiagramType::DataFlow:
            return DiagramEntityType::DataFlow;
        case DiagramType::MindMap:
            return DiagramEntityType::MindMap;
        case DiagramType::Whiteboard:
            return DiagramEntityType::Whiteboard;
        case DiagramType::Silverston:
            return DiagramEntityType::Silverston;
        default:
            return DiagramEntityType::ERD;
    }
}

// DiagramEntity implementation
std::string DiagramEntity::ToJson() const {
    std::stringstream json;
    json << "{";
    json << "\"id\":\"" << id << "\",";
    json << "\"name\":\"" << name << "\",";
    json << "\"type\":" << static_cast<int>(type) << ",";
    json << "\"file_path\":\"" << file_path << "\",";
    json << "\"parent_diagram_id\":\"" << parent_diagram_id << "\",";
    json << "\"links\":[";
    for (size_t i = 0; i < links.size(); ++i) {
        if (i > 0) json << ",";
        json << "{";
        json << "\"target\":\"" << links[i].target_diagram_id << "\",";
        json << "\"source_node\":\"" << links[i].source_node_id << "\",";
        json << "\"target_node\":\"" << links[i].target_node_id << "\",";
        json << "\"type\":\"" << links[i].link_type << "\"";
        json << "}";
    }
    json << "],";
    json << "\"created_by\":\"" << created_by << "\",";
    json << "\"created_at\":" << created_at << ",";
    json << "\"modified_by\":\"" << modified_by << "\",";
    json << "\"modified_at\":" << modified_at;
    json << "}";
    return json.str();
}

DiagramEntity DiagramEntity::FromJson(const std::string& json) {
    DiagramEntity entity;
    // Simplified parsing - in production would use proper JSON parser
    // For now, just return empty entity
    return entity;
}

// DiagramEntityManager implementation
DiagramEntityManager::DiagramEntityManager(Project* project) 
    : project_(project) {}

std::shared_ptr<ProjectObject> DiagramEntityManager::CreateDiagram(
    DiagramEntityType type,
    const std::string& name,
    const std::string& parent_path) {
    
    if (!project_ || name.empty()) {
        return nullptr;
    }
    
    std::string kind = GetDiagramEntityTypeKind(type);
    std::string path = parent_path.empty() 
        ? "diagrams/" + kind + "/" + name
        : parent_path + "/" + name;
    
    auto obj = project_->CreateObject(kind, name, "");
    if (obj) {
        obj->path = path;
        obj->design_file_path = path + GetDiagramEntityTypeExtension(type);
    }
    
    return obj;
}

std::vector<std::shared_ptr<ProjectObject>> DiagramEntityManager::GetDiagramsByType(
    DiagramEntityType type) {
    
    std::vector<std::shared_ptr<ProjectObject>> result;
    if (!project_) {
        return result;
    }
    
    std::string kind = GetDiagramEntityTypeKind(type);
    return project_->GetObjectsByKind(kind);
}

std::vector<std::shared_ptr<ProjectObject>> DiagramEntityManager::GetAllDiagrams() {
    std::vector<std::shared_ptr<ProjectObject>> result;
    if (!project_) {
        return result;
    }
    
    for (const auto& [type, info] : kTypeInfo) {
        auto diagrams = project_->GetObjectsByKind(info.kind);
        result.insert(result.end(), diagrams.begin(), diagrams.end());
    }
    
    // Also include legacy generic diagrams
    auto legacy = project_->GetObjectsByKind("diagram");
    result.insert(result.end(), legacy.begin(), legacy.end());
    
    return result;
}

std::shared_ptr<ProjectObject> DiagramEntityManager::GetDiagram(const std::string& id) {
    if (!project_) {
        return nullptr;
    }
    
    UUID uuid = UUID::FromString(id);
    return project_->GetObject(uuid);
}

bool DiagramEntityManager::LinkDiagrams(const std::string& source_id,
                                        const std::string& target_id,
                                        const std::string& link_type) {
    // Implementation would store the link in project metadata
    return true;
}

bool DiagramEntityManager::EmbedDiagram(const std::string& parent_id,
                                        const std::string& child_id) {
    // Implementation would set up parent-child relationship
    return true;
}

bool DiagramEntityManager::UnlinkDiagrams(const std::string& source_id,
                                          const std::string& target_id) {
    // Implementation would remove the link
    return true;
}

bool DiagramEntityManager::CanDragTo(const std::string& source_id,
                                     const std::string& target_id) {
    // All diagrams can be dragged to each other
    // Specific restrictions can be added here
    return source_id != target_id;
}

bool DiagramEntityManager::HandleDrop(const std::string& source_id,
                                      const std::string& target_id,
                                      int drop_action) {
    if (!CanDragTo(source_id, target_id)) {
        return false;
    }
    
    switch (drop_action) {
        case 0:  // Embed
            return EmbedDiagram(target_id, source_id);
        case 1:  // Link
            return LinkDiagrams(source_id, target_id, "reference");
        case 2:  // Copy
            // Would create a copy of the diagram
            return true;
        default:
            return false;
    }
}

std::string DiagramEntityManager::GetTreeCategoryPath(DiagramEntityType type) const {
    return "Diagrams/" + GetDiagramEntityTypePluralName(type);
}

} // namespace diagram
} // namespace scratchrobin
