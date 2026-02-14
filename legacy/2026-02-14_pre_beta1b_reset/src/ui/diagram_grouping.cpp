/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "diagram_grouping.h"

#include "diagram_model.h"

#include <algorithm>

namespace scratchrobin {

void NodeGroup::CalculateBounds(const DiagramModel& model) {
    if (node_ids.empty()) {
        x = y = width = height = 0;
        return;
    }
    
    const auto& nodes = model.nodes();
    
    bool first = true;
    double minX = 0, minY = 0, maxX = 0, maxY = 0;
    
    for (const auto& node_id : node_ids) {
        auto it = std::find_if(nodes.begin(), nodes.end(),
            [&node_id](const DiagramNode& n) { return n.id == node_id; });
        
        if (it != nodes.end()) {
            if (first) {
                minX = it->x;
                minY = it->y;
                maxX = it->x + it->width;
                maxY = it->y + it->height;
                first = false;
            } else {
                minX = std::min(minX, it->x);
                minY = std::min(minY, it->y);
                maxX = std::max(maxX, it->x + it->width);
                maxY = std::max(maxY, it->y + it->height);
            }
        }
    }
    
    // Add padding
    double padding = 20.0;
    x = minX - padding;
    y = minY - padding;
    width = (maxX - minX) + 2 * padding;
    height = (maxY - minY) + 2 * padding;
}

std::string DiagramGrouping::CreateGroup(const std::vector<std::string>& node_ids,
                                          const std::string& name) {
    if (node_ids.empty()) return "";
    
    NodeGroup group;
    group.id = "group_" + std::to_string(next_group_id_++);
    group.name = name;
    group.node_ids = node_ids;
    
    groups_.push_back(group);
    return group.id;
}

void DiagramGrouping::Ungroup(const std::string& group_id) {
    groups_.erase(std::remove_if(groups_.begin(), groups_.end(),
        [&group_id](const NodeGroup& g) { return g.id == group_id; }),
        groups_.end());
}

void DiagramGrouping::AddToGroup(const std::string& group_id, const std::string& node_id) {
    NodeGroup* group = GetGroup(group_id);
    if (!group) return;
    
    // Remove from other groups first
    NodeGroup* oldGroup = FindGroupContaining(node_id);
    if (oldGroup && oldGroup->id != group_id) {
        RemoveFromGroup(oldGroup->id, node_id);
    }
    
    // Add to new group if not already there
    if (std::find(group->node_ids.begin(), group->node_ids.end(), node_id) 
        == group->node_ids.end()) {
        group->node_ids.push_back(node_id);
    }
}

void DiagramGrouping::RemoveFromGroup(const std::string& group_id, const std::string& node_id) {
    NodeGroup* group = GetGroup(group_id);
    if (!group) return;
    
    group->node_ids.erase(std::remove(group->node_ids.begin(), group->node_ids.end(), node_id),
                          group->node_ids.end());
}

NodeGroup* DiagramGrouping::FindGroupContaining(const std::string& node_id) {
    for (auto& group : groups_) {
        if (std::find(group.node_ids.begin(), group.node_ids.end(), node_id) 
            != group.node_ids.end()) {
            return &group;
        }
    }
    return nullptr;
}

NodeGroup* DiagramGrouping::GetGroup(const std::string& group_id) {
    for (auto& group : groups_) {
        if (group.id == group_id) {
            return &group;
        }
    }
    return nullptr;
}

void DiagramGrouping::ExpandGroup(const std::string& group_id) {
    NodeGroup* group = GetGroup(group_id);
    if (group) {
        group->is_expanded = true;
    }
}

void DiagramGrouping::CollapseGroup(const std::string& group_id) {
    NodeGroup* group = GetGroup(group_id);
    if (group) {
        group->is_expanded = false;
    }
}

void DiagramGrouping::MoveGroup(const std::string& group_id, double dx, double dy,
                                DiagramModel& model) {
    NodeGroup* group = GetGroup(group_id);
    if (!group) return;
    
    // Move all nodes in the group
    for (const auto& node_id : group->node_ids) {
        auto& nodes = model.nodes();
        auto it = std::find_if(nodes.begin(), nodes.end(),
            [&node_id](const DiagramNode& n) { return n.id == node_id; });
        
        if (it != nodes.end()) {
            it->x += dx;
            it->y += dy;
        }
    }
    
    // Update group bounds
    group->x += dx;
    group->y += dy;
}

} // namespace scratchrobin
