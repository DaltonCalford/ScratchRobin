/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_DIAGRAM_LAYOUT_ENGINE_H
#define SCRATCHROBIN_DIAGRAM_LAYOUT_ENGINE_H

#include "../ui/diagram_model.h"

#include <functional>
#include <map>
#include <memory>
#include <random>
#include <vector>

namespace scratchrobin {
namespace diagram {

// Layout algorithm types
enum class LayoutAlgorithm {
    Sugiyama,       // Hierarchical layered layout
    ForceDirected,  // Spring-based layout
    Orthogonal,     // Right-angle routing
    Circular        // Circular/radial layout
};

// Layout options
struct LayoutOptions {
    LayoutAlgorithm algorithm = LayoutAlgorithm::Sugiyama;
    
    // Common options
    double node_spacing = 150.0;      // Horizontal spacing between nodes
    double level_spacing = 120.0;     // Vertical spacing between levels
    double padding = 50.0;            // Padding around the diagram
    
    // Sugiyama specific
    bool minimize_crossings = true;   // Minimize edge crossings
    int max_iterations = 10;          // Iterations for crossing reduction
    
    // Force-directed specific
    double repulsion_force = 1000.0;  // Node repulsion strength
    double attraction_force = 0.01;   // Edge attraction strength
    double damping = 0.9;             // Velocity damping factor
    int fd_iterations = 100;          // Number of simulation steps
    double min_velocity = 0.1;        // Convergence threshold
    
    // Orthogonal specific
    bool use_ports = true;            // Use connection ports
    
    // Direction (for hierarchical layouts)
    enum class Direction {
        TopDown,
        BottomUp,
        LeftRight,
        RightLeft
    } direction = Direction::TopDown;
};

// Node position result
struct NodePosition {
    std::string node_id;
    double x;
    double y;
    double width;
    double height;
};

// Layout engine base class
class LayoutEngine {
public:
    virtual ~LayoutEngine() = default;
    
    // Apply layout to model
    virtual std::vector<NodePosition> Layout(const DiagramModel& model,
                                              const LayoutOptions& options) = 0;
    
    // Factory method
    static std::unique_ptr<LayoutEngine> Create(LayoutAlgorithm algorithm);
};

// Sugiyama hierarchical layout
class SugiyamaLayout : public LayoutEngine {
public:
    std::vector<NodePosition> Layout(const DiagramModel& model,
                                      const LayoutOptions& options) override;

private:
    struct LayeredNode {
        std::string id;
        int layer = 0;
        int position = 0;
        double x = 0;
        double y = 0;
        std::vector<std::string> parents;
        std::vector<std::string> children;
    };
    
    // Build dependency graph
    std::map<std::string, LayeredNode> BuildGraph(const DiagramModel& model);
    
    // Assign nodes to layers
    void AssignLayers(std::map<std::string, LayeredNode>& graph);
    
    // Add dummy nodes for long edges
    void AddDummyNodes(std::map<std::string, LayeredNode>& graph,
                       const LayoutOptions& options);
    
    // Reduce edge crossings within layers
    void ReduceCrossings(std::map<std::string, LayeredNode>& graph,
                         const LayoutOptions& options);
    
    // Calculate final positions
    void CalculatePositions(std::map<std::string, LayeredNode>& graph,
                            const LayoutOptions& options);
};

// Force-directed layout (Fruchterman-Reingold inspired)
class ForceDirectedLayout : public LayoutEngine {
public:
    std::vector<NodePosition> Layout(const DiagramModel& model,
                                      const LayoutOptions& options) override;

private:
    struct FDNode {
        std::string id;
        double x = 0;
        double y = 0;
        double vx = 0;
        double vy = 0;
        double mass = 1.0;
        std::vector<std::string> connections;
    };
    
    void InitializePositions(std::map<std::string, FDNode>& nodes,
                             const LayoutOptions& options);
    
    void CalculateForces(std::map<std::string, FDNode>& nodes,
                         const DiagramModel& model,
                         const LayoutOptions& options);
    
    void UpdatePositions(std::map<std::string, FDNode>& nodes,
                         const LayoutOptions& options);
};

// Orthogonal layout
class OrthogonalLayout : public LayoutEngine {
public:
    std::vector<NodePosition> Layout(const DiagramModel& model,
                                      const LayoutOptions& options) override;

private:
    struct OrthoNode {
        std::string id;
        int grid_x = 0;
        int grid_y = 0;
        double x = 0;
        double y = 0;
    };
    
    void AssignGridPositions(std::map<std::string, OrthoNode>& nodes,
                             const DiagramModel& model);
    
    void CompactLayout(std::map<std::string, OrthoNode>& nodes);
};

// Utility functions
std::string LayoutAlgorithmToString(LayoutAlgorithm algo);
LayoutAlgorithm StringToLayoutAlgorithm(const std::string& str);
std::vector<std::string> GetAvailableLayoutAlgorithms();

} // namespace diagram
} // namespace scratchrobin

#endif // SCRATCHROBIN_DIAGRAM_LAYOUT_ENGINE_H
