/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "graphviz_layout.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <memory>
#include <sstream>
#include <stdexcept>

namespace scratchrobin {
namespace diagram {

std::vector<NodePosition> GraphvizLayoutEngine::Layout(const DiagramModel& model,
                                                        const LayoutOptions& options) {
    std::vector<NodePosition> positions;
    
    if (!IsDotAvailable()) {
        // Fallback to built-in Sugiyama layout
        auto sugiyama = LayoutEngine::Create(LayoutAlgorithm::Sugiyama);
        return sugiyama->Layout(model, options);
    }
    
    // Generate DOT format
    std::string dotInput = GenerateDot(model, options);
    
    // Execute dot
    std::string output;
    std::string error;
    if (!ExecuteDot(dotInput, output, error)) {
        // Fallback on error
        auto sugiyama = LayoutEngine::Create(LayoutAlgorithm::Sugiyama);
        return sugiyama->Layout(model, options);
    }
    
    // Parse output
    return ParseDotOutput(output);
}

std::string GraphvizLayoutEngine::GenerateDot(const DiagramModel& model,
                                               const LayoutOptions& options) {
    std::ostringstream dot;
    
    dot << "digraph G {\n";
    dot << "  graph [bgcolor=\"white\", dpi=\"96\"];\n";
    dot << "  node [shape=\"box\", style=\"filled\", fillcolor=\"lightblue\"];\n";
    dot << "  edge [color=\"gray\"];\n\n";
    
    // Add nodes
    const auto& nodes = model.nodes();
    for (const auto& node : nodes) {
        std::string label = node.name;
        // Escape quotes in label
        size_t pos = 0;
        while ((pos = label.find('"', pos)) != std::string::npos) {
            label.replace(pos, 1, "\\\"");
            pos += 2;
        }
        dot << "  \"" << node.id << "\" [label=\"" << label << "\"];\n";
    }
    
    dot << "\n";
    
    // Add edges
    for (const auto& edge : model.edges()) {
        dot << "  \"" << edge.source_id << "\" -> \"" << edge.target_id << "\";\n";
    }
    
    dot << "}\n";
    
    return dot.str();
}

std::vector<NodePosition> GraphvizLayoutEngine::ParseDotOutput(const std::string& output) {
    std::vector<NodePosition> positions;
    
    std::istringstream stream(output);
    std::string line;
    
    while (std::getline(stream, line)) {
        // Look for node position lines like:
        // nodeId [pos="x,y"];
        size_t posAttr = line.find("[pos=\"");
        if (posAttr == std::string::npos) continue;
        
        // Extract node ID
        size_t idEnd = line.find(' ');
        if (idEnd == std::string::npos) continue;
        
        std::string nodeId = line.substr(0, idEnd);
        // Remove quotes if present
        if (nodeId.front() == '"') nodeId = nodeId.substr(1);
        if (nodeId.back() == '"') nodeId = nodeId.substr(0, nodeId.size() - 1);
        
        // Extract position
        size_t posStart = posAttr + 6;
        size_t posEnd = line.find('"', posStart);
        if (posEnd == std::string::npos) continue;
        
        std::string posStr = line.substr(posStart, posEnd - posStart);
        
        // Parse x,y coordinates
        size_t comma = posStr.find(',');
        if (comma == std::string::npos) continue;
        
        try {
            double x = std::stod(posStr.substr(0, comma));
            double y = std::stod(posStr.substr(comma + 1));
            
            NodePosition pos;
            pos.node_id = nodeId;
            pos.x = x;
            pos.y = y;
            pos.width = 100;  // Default width
            pos.height = 80;  // Default height
            
            positions.push_back(pos);
        } catch (...) {
            // Skip malformed lines
        }
    }
    
    return positions;
}

bool GraphvizLayoutEngine::ExecuteDot(const std::string& input,
                                       std::string& output,
                                       std::string& error) {
    // Use popen to execute dot command
    const char* cmd = "dot -Tplain";
    
    std::array<char, 128> buffer;
    
    // Open pipe to dot process
    FILE* pipe = popen(cmd, "w+");
    if (!pipe) {
        error = "Failed to start dot process";
        return false;
    }
    
    // Write input to dot
    fprintf(pipe, "%s", input.c_str());
    
    // Read output
    output.clear();
    while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
        output += buffer.data();
    }
    
    int status = pclose(pipe);
    return status == 0;
}

bool GraphvizLayoutEngine::IsDotAvailable() {
    FILE* pipe = popen("which dot", "r");
    if (!pipe) return false;
    
    char buffer[256];
    bool found = (fgets(buffer, sizeof(buffer), pipe) != nullptr);
    pclose(pipe);
    
    return found;
}

} // namespace diagram
} // namespace scratchrobin
