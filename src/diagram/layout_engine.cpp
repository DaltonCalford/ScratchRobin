/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#include "layout_engine.h"

#include <algorithm>
#include <cmath>
#include <queue>
#include <unordered_set>

namespace scratchrobin {
namespace diagram {

// Factory method
std::unique_ptr<LayoutEngine> LayoutEngine::Create(LayoutAlgorithm algorithm) {
    switch (algorithm) {
        case LayoutAlgorithm::Sugiyama:
            return std::make_unique<SugiyamaLayout>();
        case LayoutAlgorithm::ForceDirected:
            return std::make_unique<ForceDirectedLayout>();
        case LayoutAlgorithm::Orthogonal:
            return std::make_unique<OrthogonalLayout>();
        default:
            return std::make_unique<SugiyamaLayout>();
    }
}

// Utility functions
std::string LayoutAlgorithmToString(LayoutAlgorithm algo) {
    switch (algo) {
        case LayoutAlgorithm::Sugiyama: return "sugiyama";
        case LayoutAlgorithm::ForceDirected: return "force";
        case LayoutAlgorithm::Orthogonal: return "orthogonal";
        case LayoutAlgorithm::Circular: return "circular";
        default: return "sugiyama";
    }
}

LayoutAlgorithm StringToLayoutAlgorithm(const std::string& str) {
    if (str == "force" || str == "forcedirected") return LayoutAlgorithm::ForceDirected;
    if (str == "orthogonal") return LayoutAlgorithm::Orthogonal;
    if (str == "circular") return LayoutAlgorithm::Circular;
    return LayoutAlgorithm::Sugiyama;
}

std::vector<std::string> GetAvailableLayoutAlgorithms() {
    return {"sugiyama", "force", "orthogonal", "circular"};
}

// ==================== Sugiyama Layout ====================

std::vector<NodePosition> SugiyamaLayout::Layout(const DiagramModel& model,
                                                  const LayoutOptions& options) {
    auto graph = BuildGraph(model);
    if (graph.empty()) {
        return {};
    }
    
    AssignLayers(graph);
    
    if (options.minimize_crossings) {
        ReduceCrossings(graph, options);
    }
    
    CalculatePositions(graph, options);
    
    std::vector<NodePosition> result;
    for (const auto& [id, node] : graph) {
        // Skip dummy nodes
        if (id.find("_dummy_") == std::string::npos) {
            result.push_back({id, node.x, node.y, 140, 80});
        }
    }
    return result;
}

std::map<std::string, SugiyamaLayout::LayeredNode> SugiyamaLayout::BuildGraph(
    const DiagramModel& model) {
    
    std::map<std::string, LayeredNode> graph;
    
    // Create nodes
    for (const auto& node : model.nodes()) {
        LayeredNode ln;
        ln.id = node.id;
        ln.x = node.x;
        ln.y = node.y;
        graph[node.id] = ln;
    }
    
    // Build parent/child relationships from edges
    for (const auto& edge : model.edges()) {
        auto it_source = graph.find(edge.source_id);
        auto it_target = graph.find(edge.target_id);
        if (it_source != graph.end() && it_target != graph.end()) {
            // Assuming source is parent, target is child
            it_source->second.children.push_back(edge.target_id);
            it_target->second.parents.push_back(edge.source_id);
        }
    }
    
    return graph;
}

void SugiyamaLayout::AssignLayers(std::map<std::string, LayeredNode>& graph) {
    // Find root nodes (no parents)
    std::queue<std::string> roots;
    for (auto& [id, node] : graph) {
        if (node.parents.empty()) {
            roots.push(id);
            node.layer = 0;
        }
    }
    
    // BFS to assign layers
    while (!roots.empty()) {
        std::string current = roots.front();
        roots.pop();
        
        auto it = graph.find(current);
        if (it == graph.end()) continue;
        
        for (const auto& child_id : it->second.children) {
            auto child_it = graph.find(child_id);
            if (child_it != graph.end()) {
                int new_layer = it->second.layer + 1;
                if (new_layer > child_it->second.layer) {
                    child_it->second.layer = new_layer;
                    roots.push(child_id);
                }
            }
        }
    }
    
    // Handle cycles - assign remaining unvisited nodes
    int max_layer = 0;
    for (const auto& [id, node] : graph) {
        max_layer = std::max(max_layer, node.layer);
    }
    
    for (auto& [id, node] : graph) {
        if (node.layer == 0 && !node.parents.empty()) {
            // Node in cycle, assign to layer based on parents
            int parent_max_layer = 0;
            for (const auto& parent_id : node.parents) {
                auto parent_it = graph.find(parent_id);
                if (parent_it != graph.end()) {
                    parent_max_layer = std::max(parent_max_layer, parent_it->second.layer);
                }
            }
            node.layer = parent_max_layer + 1;
        }
    }
}

void SugiyamaLayout::ReduceCrossings(std::map<std::string, LayeredNode>& graph,
                                      const LayoutOptions& options) {
    // Group nodes by layer
    std::map<int, std::vector<std::string>> layers;
    for (const auto& [id, node] : graph) {
        layers[node.layer].push_back(id);
    }
    
    // Iterative crossing reduction (barycenter method)
    for (int iter = 0; iter < options.max_iterations; ++iter) {
        bool improved = false;
        
        for (auto& [layer_num, node_ids] : layers) {
            if (node_ids.size() < 2) continue;
            
            // Calculate barycenter for each node
            std::vector<std::pair<std::string, double>> barycenters;
            for (const auto& id : node_ids) {
                auto it = graph.find(id);
                if (it == graph.end()) continue;
                
                double barycenter = 0;
                int count = 0;
                
                // Use parents from previous layer
                for (const auto& parent_id : it->second.parents) {
                    auto parent_it = graph.find(parent_id);
                    if (parent_it != graph.end() && parent_it->second.layer < layer_num) {
                        barycenter += std::find(layers[parent_it->second.layer].begin(),
                                                layers[parent_it->second.layer].end(),
                                                parent_id) - layers[parent_it->second.layer].begin();
                        count++;
                    }
                }
                
                if (count > 0) {
                    barycenter /= count;
                } else {
                    barycenter = std::find(node_ids.begin(), node_ids.end(), id) - node_ids.begin();
                }
                
                barycenters.push_back({id, barycenter});
            }
            
            // Sort by barycenter
            std::sort(barycenters.begin(), barycenters.end(),
                      [](const auto& a, const auto& b) { return a.second < b.second; });
            
            // Update positions
            for (size_t i = 0; i < barycenters.size(); ++i) {
                auto it = graph.find(barycenters[i].first);
                if (it != graph.end()) {
                    if (it->second.position != static_cast<int>(i)) {
                        improved = true;
                    }
                    it->second.position = i;
                }
            }
            
            // Update layer order
            node_ids.clear();
            for (const auto& [id, _] : barycenters) {
                node_ids.push_back(id);
            }
        }
        
        if (!improved) break;
    }
}

void SugiyamaLayout::CalculatePositions(std::map<std::string, LayeredNode>& graph,
                                         const LayoutOptions& options) {
    // Group by layer
    std::map<int, std::vector<std::string>> layers;
    for (const auto& [id, node] : graph) {
        layers[node.layer].push_back(id);
    }
    
    // Calculate positions
    for (const auto& [layer_num, node_ids] : layers) {
        double y = options.padding + layer_num * options.level_spacing;
        
        for (size_t i = 0; i < node_ids.size(); ++i) {
            auto it = graph.find(node_ids[i]);
            if (it != graph.end()) {
                it->second.x = options.padding + i * options.node_spacing;
                it->second.y = y;
            }
        }
    }
}

// ==================== Force-Directed Layout ====================

std::vector<NodePosition> ForceDirectedLayout::Layout(const DiagramModel& model,
                                                       const LayoutOptions& options) {
    std::map<std::string, FDNode> nodes;
    
    // Initialize nodes
    for (const auto& node : model.nodes()) {
        FDNode fn;
        fn.id = node.id;
        fn.mass = 1.0 + node.attributes.size() * 0.1;  // Heavier nodes have more attributes
        nodes[node.id] = fn;
    }
    
    // Build connections
    for (const auto& edge : model.edges()) {
        auto it_source = nodes.find(edge.source_id);
        auto it_target = nodes.find(edge.target_id);
        if (it_source != nodes.end() && it_target != nodes.end()) {
            it_source->second.connections.push_back(edge.target_id);
            it_target->second.connections.push_back(edge.source_id);
        }
    }
    
    InitializePositions(nodes, options);
    
    // Simulation loop
    for (int iter = 0; iter < options.fd_iterations; ++iter) {
        CalculateForces(nodes, model, options);
        UpdatePositions(nodes, options);
    }
    
    // Convert to result
    std::vector<NodePosition> result;
    for (const auto& [id, node] : nodes) {
        result.push_back({id, node.x, node.y, 140, 80});
    }
    return result;
}

void ForceDirectedLayout::InitializePositions(std::map<std::string, FDNode>& nodes,
                                               const LayoutOptions& options) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<> dis(0, 2 * M_PI);
    
    double radius = std::min(options.node_spacing, options.level_spacing) * std::sqrt(nodes.size());
    int i = 0;
    for (auto& [id, node] : nodes) {
        double angle = dis(gen);
        node.x = radius * std::cos(angle) + radius;
        node.y = radius * std::sin(angle) + radius;
        i++;
    }
}

void ForceDirectedLayout::CalculateForces(std::map<std::string, FDNode>& nodes,
                                           const DiagramModel& model,
                                           const LayoutOptions& options) {
    // Repulsion between all nodes
    for (auto& [id1, node1] : nodes) {
        double fx = 0, fy = 0;
        
        for (const auto& [id2, node2] : nodes) {
            if (id1 == id2) continue;
            
            double dx = node1.x - node2.x;
            double dy = node1.y - node2.y;
            double dist_sq = dx * dx + dy * dy;
            
            if (dist_sq < 0.01) continue;
            
            double dist = std::sqrt(dist_sq);
            double force = options.repulsion_force / dist_sq;
            
            fx += (dx / dist) * force;
            fy += (dy / dist) * force;
        }
        
        // Attraction along edges
        for (const auto& conn_id : node1.connections) {
            auto it = nodes.find(conn_id);
            if (it != nodes.end()) {
                double dx = it->second.x - node1.x;
                double dy = it->second.y - node1.y;
                
                fx += dx * options.attraction_force;
                fy += dy * options.attraction_force;
            }
        }
        
        node1.vx = (node1.vx + fx) * options.damping;
        node1.vy = (node1.vy + fy) * options.damping;
    }
}

void ForceDirectedLayout::UpdatePositions(std::map<std::string, FDNode>& nodes,
                                           const LayoutOptions& options) {
    for (auto& [id, node] : nodes) {
        node.x += node.vx;
        node.y += node.vy;
    }
}

// ==================== Orthogonal Layout ====================

std::vector<NodePosition> OrthogonalLayout::Layout(const DiagramModel& model,
                                                    const LayoutOptions& options) {
    std::map<std::string, OrthoNode> nodes;
    
    // Initialize
    for (const auto& node : model.nodes()) {
        OrthoNode on;
        on.id = node.id;
        nodes[node.id] = on;
    }
    
    AssignGridPositions(nodes, model);
    CompactLayout(nodes);
    
    // Convert to positions
    std::vector<NodePosition> result;
    for (const auto& [id, node] : nodes) {
        double x = options.padding + node.grid_x * options.node_spacing;
        double y = options.padding + node.grid_y * options.level_spacing;
        result.push_back({id, x, y, 140, 80});
    }
    return result;
}

void OrthogonalLayout::AssignGridPositions(std::map<std::string, OrthoNode>& nodes,
                                            const DiagramModel& model) {
    // Simple grid assignment - place nodes in a grid based on connectivity
    int grid_x = 0;
    int grid_y = 0;
    int max_cols = static_cast<int>(std::ceil(std::sqrt(nodes.size())));
    
    for (auto& [id, node] : nodes) {
        node.grid_x = grid_x;
        node.grid_y = grid_y;
        
        grid_x++;
        if (grid_x >= max_cols) {
            grid_x = 0;
            grid_y++;
        }
    }
}

void OrthogonalLayout::CompactLayout(std::map<std::string, OrthoNode>& nodes) {
    // Remove empty rows/columns
    // This is a simplified version
}

} // namespace diagram
} // namespace scratchrobin
