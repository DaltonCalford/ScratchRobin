/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_MINDMAP_UTIL_H
#define SCRATCHROBIN_MINDMAP_UTIL_H

#include <string>

#include "ui/diagram_model.h"

namespace scratchrobin {

bool MindMapHasChildren(const DiagramModel& model, const std::string& node_id);
int MindMapCountDescendants(const DiagramModel& model, const std::string& node_id);

} // namespace scratchrobin

#endif // SCRATCHROBIN_MINDMAP_UTIL_H
