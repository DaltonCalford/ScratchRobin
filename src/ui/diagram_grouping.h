/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_DIAGRAM_GROUPING_H
#define SCRATCHROBIN_DIAGRAM_GROUPING_H

#include <string>
#include <vector>

namespace scratchrobin {

// A group of diagram nodes
struct NodeGroup {
    std::string id;
    std::string name;
    std::vector<std::string> node_ids;
    double x = 0.0;  // Group bounds
    double y = 0.0;
    double width = 0.0;
    double height = 0.0;
    bool is_expanded = true;
    
    void CalculateBounds(const class DiagramModel& model);
};

// Group management for diagrams
class DiagramGrouping {
public:
    // Create a new group from selected nodes
    std::string CreateGroup(const std::vector<std::string>& node_ids,
                            const std::string& name = "Group");
    
    // Remove a group (nodes remain)
    void Ungroup(const std::string& group_id);
    
    // Add/remove nodes from a group
    void AddToGroup(const std::string& group_id, const std::string& node_id);
    void RemoveFromGroup(const std::string& group_id, const std::string& node_id);
    
    // Get all groups
    const std::vector<NodeGroup>& groups() const { return groups_; }
    std::vector<NodeGroup>& groups() { return groups_; }
    
    // Find group containing a node
    NodeGroup* FindGroupContaining(const std::string& node_id);
    
    // Get group by ID
    NodeGroup* GetGroup(const std::string& group_id);
    
    // Expand/collapse group
    void ExpandGroup(const std::string& group_id);
    void CollapseGroup(const std::string& group_id);
    
    // Move entire group
    void MoveGroup(const std::string& group_id, double dx, double dy,
                   class DiagramModel& model);

private:
    std::vector<NodeGroup> groups_;
    int next_group_id_ = 1;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_DIAGRAM_GROUPING_H
