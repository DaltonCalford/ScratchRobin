// Copyright (c) 2026, Dennis C. Alfonso
// Licensed under the MIT License. See LICENSE file in the project root.
#pragma once

#include <ctime>
#include <string>
#include <vector>
#include <memory>

namespace scratchrobin {

// Forward declarations
enum class DiagramType;
class Project;
class ProjectObject;

namespace diagram {

/**
 * @brief Specific diagram entity types - each appears as separate item in project tree
 */
enum class DiagramEntityType {
    ERD,           // Entity-Relationship Diagram
    DataFlow,      // Data Flow Diagram (DFD)
    UML,           // UML Class Diagram
    MindMap,       // Mind Map
    Whiteboard,    // Free-form whiteboard
    Silverston,    // Silverston data model diagram
    
    Count          // Number of types (for iteration)
};

/**
 * @brief Get display name for diagram entity type
 */
std::string GetDiagramEntityTypeName(DiagramEntityType type);

/**
 * @brief Get plural display name for diagram entity type
 */
std::string GetDiagramEntityTypePluralName(DiagramEntityType type);

/**
 * @brief Get icon index for diagram entity type
 */
int GetDiagramEntityTypeIcon(DiagramEntityType type);

/**
 * @brief Get file extension for diagram entity type
 */
std::string GetDiagramEntityTypeExtension(DiagramEntityType type);

/**
 * @brief Get project object kind string for diagram entity type
 */
std::string GetDiagramEntityTypeKind(DiagramEntityType type);

/**
 * @brief Parse diagram entity type from kind string
 */
DiagramEntityType KindToDiagramEntityType(const std::string& kind);

/**
 * @brief Check if a project object kind is a diagram type
 */
bool IsDiagramKind(const std::string& kind);

/**
 * @brief Check if a project object kind represents a specific diagram type
 */
DiagramEntityType GetDiagramTypeFromKind(const std::string& kind);

/**
 * @brief Convert to/from generic DiagramType enum
 */
DiagramEntityType DiagramTypeToEntityType(DiagramType type);
DiagramType EntityTypeToDiagramType(DiagramEntityType type);

/**
 * @brief Represents a diagram entity in the project
 * 
 * Each diagram entity is a separate project object with its own:
 * - Kind (erd_diagram, dfd_diagram, etc.)
 * - File storage
 * - Tree representation
 * - Can be dragged/dropped/linked to other diagrams
 */
struct DiagramEntity {
    std::string id;                    // Unique ID
    std::string name;                  // Display name
    DiagramEntityType type;            // Specific diagram type
    std::string file_path;             // Path to diagram file
    std::string parent_diagram_id;     // For embedded diagrams
    
    // Cross-diagram linking
    struct Link {
        std::string target_diagram_id;
        std::string source_node_id;
        std::string target_node_id;
        std::string link_type;         // "embed", "reference", "sync"
    };
    std::vector<Link> links;
    
    // Metadata
    std::string created_by;
    std::time_t created_at;
    std::string modified_by;
    std::time_t modified_at;
    
    // Serialization
    std::string ToJson() const;
    static DiagramEntity FromJson(const std::string& json);
};

/**
 * @brief Manages diagram entities within a project
 */
class DiagramEntityManager {
public:
    explicit DiagramEntityManager(Project* project);
    
    // Creation
    std::shared_ptr<ProjectObject> CreateDiagram(
        DiagramEntityType type,
        const std::string& name,
        const std::string& parent_path = "");
    
    // Retrieval
    std::vector<std::shared_ptr<ProjectObject>> GetDiagramsByType(DiagramEntityType type);
    std::vector<std::shared_ptr<ProjectObject>> GetAllDiagrams();
    std::shared_ptr<ProjectObject> GetDiagram(const std::string& id);
    
    // Cross-diagram operations
    bool LinkDiagrams(const std::string& source_id, 
                      const std::string& target_id,
                      const std::string& link_type);
    bool EmbedDiagram(const std::string& parent_id,
                      const std::string& child_id);
    bool UnlinkDiagrams(const std::string& source_id,
                        const std::string& target_id);
    
    // Drag/drop support
    bool CanDragTo(const std::string& source_id, const std::string& target_id);
    bool HandleDrop(const std::string& source_id, 
                    const std::string& target_id,
                    int drop_action);  // 0=embed, 1=link, 2=copy
    
    // Tree organization
    std::string GetTreeCategoryPath(DiagramEntityType type) const;
    
private:
    Project* project_;
};

} // namespace diagram
} // namespace scratchrobin
