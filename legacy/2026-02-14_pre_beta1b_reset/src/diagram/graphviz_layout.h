/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_GRAPHVIZ_LAYOUT_H
#define SCRATCHROBIN_GRAPHVIZ_LAYOUT_H

#include "layout_engine.h"

#include <string>
#include <vector>

namespace scratchrobin {
namespace diagram {

// Graphviz (dot) layout engine integration
class GraphvizLayoutEngine : public LayoutEngine {
public:
    std::vector<NodePosition> Layout(const DiagramModel& model,
                                      const LayoutOptions& options) override;

private:
    // Generate DOT format from diagram model
    std::string GenerateDot(const DiagramModel& model, const LayoutOptions& options);
    
    // Parse dot output to get node positions
    std::vector<NodePosition> ParseDotOutput(const std::string& output);
    
    // Execute dot command via subprocess
    bool ExecuteDot(const std::string& input, std::string& output, std::string& error);
    
    // Check if dot is available
    bool IsDotAvailable();
};

// Settings for Graphviz integration
struct GraphvizSettings {
    bool enabled = false;
    std::string dot_path = "dot";  // Path to dot executable
    std::string layout_engine = "dot";  // dot, neato, fdp, sfdp, circo, twopi
    bool use_splines = true;
    int dpi = 96;
};

} // namespace diagram
} // namespace scratchrobin

#endif // SCRATCHROBIN_GRAPHVIZ_LAYOUT_H
