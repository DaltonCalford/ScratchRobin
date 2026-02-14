/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#include "mindmap_util.h"

#include <algorithm>

namespace scratchrobin {

bool MindMapHasChildren(const DiagramModel& model, const std::string& node_id) {
    for (const auto& node : model.nodes()) {
        if (node.parent_id == node_id) {
            return true;
        }
    }
    return false;
}

int MindMapCountDescendants(const DiagramModel& model, const std::string& node_id) {
    int count = 0;
    for (const auto& node : model.nodes()) {
        std::string current = node.parent_id;
        while (!current.empty()) {
            if (current == node_id) {
                count++;
                break;
            }
            auto parent_it = std::find_if(model.nodes().begin(), model.nodes().end(),
                                          [&](const DiagramNode& candidate) {
                                              return candidate.id == current;
                                          });
            if (parent_it == model.nodes().end()) {
                break;
            }
            current = parent_it->parent_id;
        }
    }
    return count;
}

} // namespace scratchrobin
