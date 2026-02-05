/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#include "data_lineage.h"

#include <algorithm>
#include <fstream>
#include <queue>
#include <set>
#include <sstream>

namespace scratchrobin {

// ============================================================================
// Lineage Node Type
// ============================================================================
std::string LineageNodeTypeToString(LineageNodeType type) {
    switch (type) {
        case LineageNodeType::TABLE: return "TABLE";
        case LineageNodeType::VIEW: return "VIEW";
        case LineageNodeType::COLUMN: return "COLUMN";
        case LineageNodeType::PROCEDURE: return "PROCEDURE";
        case LineageNodeType::FUNCTION: return "FUNCTION";
        case LineageNodeType::TRIGGER: return "TRIGGER";
        case LineageNodeType::EXTERNAL_SOURCE: return "EXTERNAL_SOURCE";
        case LineageNodeType::EXTERNAL_SINK: return "EXTERNAL_SINK";
        case LineageNodeType::TRANSFORMATION: return "TRANSFORMATION";
        case LineageNodeType::JOIN: return "JOIN";
        case LineageNodeType::AGGREGATION: return "AGGREGATION";
        case LineageNodeType::FILTER: return "FILTER";
        default: return "UNKNOWN";
    }
}

// ============================================================================
// Lineage Node Implementation
// ============================================================================
LineageNode::LineageNode() : discovered_at(std::time(nullptr)) {}

LineageNode::LineageNode(const std::string& id_, const std::string& name_, 
                          LineageNodeType type_)
    : id(id_), name(name_), type(type_), discovered_at(std::time(nullptr)) {}

std::string LineageNode::GetFullName() const {
    if (!schema.empty()) {
        return schema + "." + name;
    }
    return name;
}

bool LineageNode::IsSource() const {
    return type == LineageNodeType::EXTERNAL_SOURCE ||
           type == LineageNodeType::TABLE;
}

bool LineageNode::IsSink() const {
    return type == LineageNodeType::EXTERNAL_SINK;
}

bool LineageNode::IsTransformation() const {
    return type == LineageNodeType::TRANSFORMATION ||
           type == LineageNodeType::JOIN ||
           type == LineageNodeType::AGGREGATION ||
           type == LineageNodeType::FILTER;
}

// ============================================================================
// Lineage Edge Implementation
// ============================================================================
LineageEdge::LineageEdge() : discovered_at(std::time(nullptr)) {}

LineageEdge::LineageEdge(const std::string& from, const std::string& to)
    : from_node_id(from), to_node_id(to), discovered_at(std::time(nullptr)) {
    id = "edge_" + from + "_" + to;
}

// ============================================================================
// Lineage Graph Implementation
// ============================================================================
void LineageGraph::AddNode(std::unique_ptr<LineageNode> node) {
    if (node && !node->id.empty()) {
        nodes[node->id] = std::move(node);
    }
}

void LineageGraph::RemoveNode(const std::string& id) {
    // Remove associated edges first
    std::vector<std::string> edges_to_remove;
    for (const auto& [edge_id, edge] : edges) {
        if (edge->from_node_id == id || edge->to_node_id == id) {
            edges_to_remove.push_back(edge_id);
        }
    }
    for (const auto& edge_id : edges_to_remove) {
        edges.erase(edge_id);
    }
    
    // Remove node
    nodes.erase(id);
}

LineageNode* LineageGraph::FindNode(const std::string& id) {
    auto it = nodes.find(id);
    if (it != nodes.end()) {
        return it->second.get();
    }
    return nullptr;
}

void LineageGraph::AddEdge(std::unique_ptr<LineageEdge> edge) {
    if (edge && !edge->id.empty()) {
        // Verify nodes exist
        if (nodes.find(edge->from_node_id) != nodes.end() &&
            nodes.find(edge->to_node_id) != nodes.end()) {
            edges[edge->id] = std::move(edge);
        }
    }
}

void LineageGraph::RemoveEdge(const std::string& id) {
    edges.erase(id);
}

LineageEdge* LineageGraph::FindEdge(const std::string& id) {
    auto it = edges.find(id);
    if (it != edges.end()) {
        return it->second.get();
    }
    return nullptr;
}

std::vector<LineageNode*> LineageGraph::GetUpstreamNodes(const std::string& node_id, 
                                                          int depth) {
    std::vector<LineageNode*> result;
    std::set<std::string> visited;
    std::queue<std::pair<std::string, int>> queue;
    
    queue.push({node_id, 0});
    visited.insert(node_id);
    
    while (!queue.empty()) {
        auto [current_id, current_depth] = queue.front();
        queue.pop();
        
        if (depth >= 0 && current_depth >= depth) {
            continue;
        }
        
        // Find all edges where current node is the target
        for (const auto& [edge_id, edge] : edges) {
            if (edge->to_node_id == current_id) {
                const std::string& source_id = edge->from_node_id;
                if (visited.find(source_id) == visited.end()) {
                    visited.insert(source_id);
                    if (LineageNode* node = FindNode(source_id)) {
                        result.push_back(node);
                    }
                    queue.push({source_id, current_depth + 1});
                }
            }
        }
    }
    
    return result;
}

std::vector<LineageNode*> LineageGraph::GetDownstreamNodes(const std::string& node_id, 
                                                            int depth) {
    std::vector<LineageNode*> result;
    std::set<std::string> visited;
    std::queue<std::pair<std::string, int>> queue;
    
    queue.push({node_id, 0});
    visited.insert(node_id);
    
    while (!queue.empty()) {
        auto [current_id, current_depth] = queue.front();
        queue.pop();
        
        if (depth >= 0 && current_depth >= depth) {
            continue;
        }
        
        // Find all edges where current node is the source
        for (const auto& [edge_id, edge] : edges) {
            if (edge->from_node_id == current_id) {
                const std::string& target_id = edge->to_node_id;
                if (visited.find(target_id) == visited.end()) {
                    visited.insert(target_id);
                    if (LineageNode* node = FindNode(target_id)) {
                        result.push_back(node);
                    }
                    queue.push({target_id, current_depth + 1});
                }
            }
        }
    }
    
    return result;
}

std::vector<LineageEdge*> LineageGraph::GetIncomingEdges(const std::string& node_id) {
    std::vector<LineageEdge*> result;
    for (const auto& [edge_id, edge] : edges) {
        if (edge->to_node_id == node_id) {
            result.push_back(edge.get());
        }
    }
    return result;
}

std::vector<LineageEdge*> LineageGraph::GetOutgoingEdges(const std::string& node_id) {
    std::vector<LineageEdge*> result;
    for (const auto& [edge_id, edge] : edges) {
        if (edge->from_node_id == node_id) {
            result.push_back(edge.get());
        }
    }
    return result;
}

ImpactAnalysisResult LineageGraph::AnalyzeImpact(const std::string& node_id) {
    ImpactAnalysisResult result;
    
    LineageNode* node = FindNode(node_id);
    if (!node) return result;
    
    // Get direct dependencies
    auto upstream = GetUpstreamNodes(node_id, 1);
    for (auto* n : upstream) {
        result.upstream_nodes.push_back(n->id);
    }
    
    // Get direct dependents
    auto downstream = GetDownstreamNodes(node_id, 1);
    for (auto* n : downstream) {
        result.downstream_nodes.push_back(n->id);
    }
    
    // Get full transitive closure
    auto all_up = GetUpstreamNodes(node_id, -1);
    for (auto* n : all_up) {
        result.all_upstream.push_back(n->id);
    }
    
    auto all_down = GetDownstreamNodes(node_id, -1);
    for (auto* n : all_down) {
        result.all_downstream.push_back(n->id);
    }
    
    // Calculate impact statistics
    result.total_objects_affected = result.all_upstream.size() + result.all_downstream.size();
    
    for (auto* n : all_down) {
        switch (n->type) {
            case LineageNodeType::TABLE:
                result.tables_affected++;
                break;
            case LineageNodeType::VIEW:
                result.views_affected++;
                break;
            case LineageNodeType::PROCEDURE:
            case LineageNodeType::FUNCTION:
            case LineageNodeType::TRIGGER:
                result.procedures_affected++;
                break;
            default:
                break;
        }
    }
    
    // Assess risks
    if (!result.downstream_nodes.empty()) {
        ImpactAnalysisResult::Risk risk;
        risk.node_id = node_id;
        risk.risk_type = "breakage";
        risk.description = "Changes may break " + 
                          std::to_string(result.downstream_nodes.size()) + 
                          " dependent objects";
        risk.severity = std::min(10, (int)result.downstream_nodes.size());
        result.risks.push_back(risk);
    }
    
    return result;
}

void LineageGraph::Clear() {
    nodes.clear();
    edges.clear();
}

void LineageGraph::ExportAsDot(std::ostream& out) const {
    out << "digraph Lineage {\n";
    out << "  rankdir=TB;\n";
    out << "  node [shape=box, style=\"rounded,filled\"];\n\n";
    
    // Output nodes
    for (const auto& [id, node] : nodes) {
        out << "  \"" << id << "\" [label=\"" << node->name << "\"";
        
        // Color based on type
        switch (node->type) {
            case LineageNodeType::TABLE:
                out << ", fillcolor=\"lightblue\"";
                break;
            case LineageNodeType::VIEW:
                out << ", fillcolor=\"lightgreen\"";
                break;
            case LineageNodeType::COLUMN:
                out << ", fillcolor=\"lightyellow\"";
                break;
            case LineageNodeType::TRANSFORMATION:
                out << ", fillcolor=\"lightcoral\"";
                break;
            default:
                out << ", fillcolor=\"lightgray\"";
        }
        
        out << "];\n";
    }
    
    out << "\n";
    
    // Output edges
    for (const auto& [id, edge] : edges) {
        out << "  \"" << edge->from_node_id << "\" -> \"" 
            << edge->to_node_id << "\"";
        
        if (!edge->relationship.empty()) {
            out << " [label=\"" << edge->relationship << "\"]";
        }
        
        out << ";\n";
    }
    
    out << "}\n";
}

void LineageGraph::ExportAsJson(std::ostream& out) const {
    out << "{\n";
    out << "  \"nodes\": [\n";
    
    bool first = true;
    for (const auto& [id, node] : nodes) {
        if (!first) out << ",\n";
        first = false;
        out << "    {\"id\": \"" << node->id << "\", ";
        out << "\"name\": \"" << node->name << "\", ";
        out << "\"type\": \"" << LineageNodeTypeToString(node->type) << "\"}";
    }
    
    out << "\n  ],\n";
    out << "  \"edges\": [\n";
    
    first = true;
    for (const auto& [id, edge] : edges) {
        if (!first) out << ",\n";
        first = false;
        out << "    {\"from\": \"" << edge->from_node_id << "\", ";
        out << "\"to\": \"" << edge->to_node_id << "\"}";
    }
    
    out << "\n  ]\n";
    out << "}\n";
}

// ============================================================================
// SQL Lineage Parser Implementation
// ============================================================================
SqlLineageParser::ParseResult SqlLineageParser::Parse(const std::string& sql, 
                                                       const std::string& default_schema) {
    // Determine SQL type and delegate to specific parser
    std::string sql_upper = sql;
    std::transform(sql_upper.begin(), sql_upper.end(), sql_upper.begin(), ::toupper);
    
    if (sql_upper.find("SELECT") == 0) {
        return ParseSelect(sql, default_schema);
    } else if (sql_upper.find("INSERT") == 0) {
        return ParseInsert(sql, default_schema);
    } else if (sql_upper.find("UPDATE") == 0) {
        return ParseUpdate(sql, default_schema);
    } else if (sql_upper.find("CREATE VIEW") != std::string::npos) {
        return ParseCreateView(sql, default_schema);
    }
    
    ParseResult result;
    result.success = false;
    result.errors.push_back("Unsupported SQL type");
    return result;
}

SqlLineageParser::ParseResult SqlLineageParser::ParseSelect(const std::string& sql,
                                                             const std::string& default_schema) {
    ParseResult result;
    result.success = true;
    
    // Simplified parsing - in production would use proper SQL parser
    // Extract table references from FROM and JOIN clauses
    ExtractTableReferences(sql, result);
    ExtractColumnReferences(sql, result);
    
    return result;
}

SqlLineageParser::ParseResult SqlLineageParser::ParseInsert(const std::string& sql,
                                                             const std::string& default_schema) {
    ParseResult result;
    result.success = true;
    
    // Parse INSERT INTO table_name ... SELECT ...
    // Target table is the INSERT target
    // Source tables are from the SELECT part
    
    // Simplified implementation
    ExtractTableReferences(sql, result);
    
    return result;
}

SqlLineageParser::ParseResult SqlLineageParser::ParseUpdate(const std::string& sql,
                                                             const std::string& default_schema) {
    ParseResult result;
    result.success = true;
    
    ExtractTableReferences(sql, result);
    
    return result;
}

SqlLineageParser::ParseResult SqlLineageParser::ParseCreateView(const std::string& sql,
                                                                 const std::string& default_schema) {
    ParseResult result;
    result.success = true;
    
    // Parse CREATE VIEW view_name AS SELECT ...
    // The view is the target, tables in SELECT are sources
    
    ExtractTableReferences(sql, result);
    
    return result;
}

SqlLineageParser::ParseResult SqlLineageParser::ParseCreateProcedure(
    const std::string& sql, const std::string& default_schema) {
    ParseResult result;
    result.success = true;
    
    // Parse stored procedure body for table references
    ExtractTableReferences(sql, result);
    
    return result;
}

void SqlLineageParser::ExtractTableReferences(const std::string& sql, ParseResult& result) {
    // Simplified regex-based extraction
    // In production, would use proper SQL parser
    
    // Look for patterns like: FROM table_name, JOIN table_name, INTO table_name
    // This is a stub implementation
}

void SqlLineageParser::ExtractColumnReferences(const std::string& sql, ParseResult& result) {
    // Extract column references from SELECT list, WHERE clause, etc.
    // Stub implementation
}

void SqlLineageParser::ExtractJoins(const std::string& sql, ParseResult& result) {
    // Extract JOIN clauses and their conditions
    // Stub implementation
}

void SqlLineageParser::ExtractAggregations(const std::string& sql, ParseResult& result) {
    // Extract GROUP BY, aggregate functions
    // Stub implementation
}

// ============================================================================
// Runtime Lineage Collector Implementation
// ============================================================================
void RuntimeLineageCollector::RecordQuery(const QueryEvent& event) {
    if (!enabled_) return;
    
    events_.push_back(event);
    
    // Clean up old events based on retention policy
    EnforceRetentionPolicy();
}

void RuntimeLineageCollector::RecordDataMovement(const std::string& from_table,
                                                  const std::string& to_table,
                                                  const std::vector<std::string>& columns) {
    if (!enabled_) return;
    
    QueryEvent event;
    event.timestamp = std::time(nullptr);
    event.source_tables.push_back(from_table);
    event.target_tables.push_back(to_table);
    event.columns_read = columns;
    event.columns_written = columns;
    
    RecordQuery(event);
}

std::vector<RuntimeLineageCollector::QueryEvent> 
RuntimeLineageCollector::GetQueryHistory(const std::string& table_name,
                                          std::time_t from, std::time_t to) {
    std::vector<QueryEvent> result;
    
    for (const auto& event : events_) {
        if (event.timestamp >= from && event.timestamp <= to) {
            // Check if table is involved
            bool involves_table = false;
            for (const auto& t : event.source_tables) {
                if (t == table_name) {
                    involves_table = true;
                    break;
                }
            }
            for (const auto& t : event.target_tables) {
                if (t == table_name) {
                    involves_table = true;
                    break;
                }
            }
            
            if (involves_table) {
                result.push_back(event);
            }
        }
    }
    
    return result;
}

std::unique_ptr<LineageGraph> RuntimeLineageCollector::GenerateLineageGraph() {
    auto graph = std::make_unique<LineageGraph>();
    
    // Build graph from collected events
    for (const auto& event : events_) {
        // Add source table nodes
        for (const auto& table : event.source_tables) {
            auto node = std::make_unique<LineageNode>(
                "table_" + table, table, LineageNodeType::TABLE);
            graph->AddNode(std::move(node));
        }
        
        // Add target table nodes
        for (const auto& table : event.target_tables) {
            auto node = std::make_unique<LineageNode>(
                "table_" + table, table, LineageNodeType::TABLE);
            graph->AddNode(std::move(node));
        }
        
        // Add edges from sources to targets
        for (const auto& source : event.source_tables) {
            for (const auto& target : event.target_tables) {
                auto edge = std::make_unique<LineageEdge>(
                    "table_" + source, "table_" + target);
                edge->detection_method = "runtime";
                graph->AddEdge(std::move(edge));
            }
        }
    }
    
    return graph;
}

void RuntimeLineageCollector::SetRetentionPeriod(int days) {
    retention_days_ = days;
}

void RuntimeLineageCollector::EnableCollection(bool enable) {
    enabled_ = enable;
}

void RuntimeLineageCollector::EnforceRetentionPolicy() {
    if (retention_days_ <= 0) {
        return;  // No retention limit
    }
    
    std::time_t now = std::time(nullptr);
    std::time_t cutoff = now - (retention_days_ * 24 * 60 * 60);  // Convert days to seconds
    
    // Remove events older than the cutoff
    events_.erase(
        std::remove_if(events_.begin(), events_.end(),
            [cutoff](const QueryEvent& event) {
                return event.timestamp < cutoff;
            }),
        events_.end()
    );
}

// ============================================================================
// Lineage Manager Implementation
// ============================================================================
LineageManager& LineageManager::Instance() {
    static LineageManager instance;
    return instance;
}

void LineageManager::InitializeFromDatabase(const std::string& connection_string) {
    graph_ = std::make_unique<LineageGraph>();
    
    // Query database for views, procedures, triggers
    // Parse their SQL to extract lineage
    // Build graph
    
    // Stub implementation
}

void LineageManager::BuildFromSqlScripts(const std::vector<std::string>& script_paths) {
    if (!parser_) {
        parser_ = std::make_unique<SqlLineageParser>();
    }
    
    for (const auto& path : script_paths) {
        std::ifstream file(path);
        if (file.is_open()) {
            std::string sql((std::istreambuf_iterator<char>(file)),
                           std::istreambuf_iterator<char>());
            
            auto result = parser_->Parse(sql);
            
            // Add parsed nodes and edges to graph
            for (auto& node : result.nodes) {
                graph_->AddNode(std::move(node));
            }
            for (auto& edge : result.edges) {
                graph_->AddEdge(std::move(edge));
            }
        }
    }
}

void LineageManager::EnableRuntimeCollection(bool enable) {
    if (!collector_) {
        collector_ = std::make_unique<RuntimeLineageCollector>();
    }
    collector_->EnableCollection(enable);
}

ImpactAnalysisResult LineageManager::AnalyzeImpact(const std::string& object_name) {
    if (!graph_) {
        return ImpactAnalysisResult();
    }
    
    // Find node by name
    for (const auto& [id, node] : graph_->nodes) {
        if (node->name == object_name || node->GetFullName() == object_name) {
            return graph_->AnalyzeImpact(id);
        }
    }
    
    return ImpactAnalysisResult();
}

std::vector<std::string> LineageManager::GetDataSources(const std::string& object_name) {
    std::vector<std::string> result;
    
    if (!graph_) return result;
    
    for (const auto& [id, node] : graph_->nodes) {
        if (node->name == object_name || node->GetFullName() == object_name) {
            auto upstream = graph_->GetUpstreamNodes(id, -1);
            for (auto* n : upstream) {
                result.push_back(n->GetFullName());
            }
            break;
        }
    }
    
    return result;
}

std::vector<std::string> LineageManager::GetDataSinks(const std::string& object_name) {
    std::vector<std::string> result;
    
    if (!graph_) return result;
    
    for (const auto& [id, node] : graph_->nodes) {
        if (node->name == object_name || node->GetFullName() == object_name) {
            auto downstream = graph_->GetDownstreamNodes(id, -1);
            for (auto* n : downstream) {
                result.push_back(n->GetFullName());
            }
            break;
        }
    }
    
    return result;
}

void LineageManager::AddManualLineage(const std::string& from_object,
                                       const std::string& to_object,
                                       const std::string& relationship) {
    if (!graph_) {
        graph_ = std::make_unique<LineageGraph>();
    }
    
    // Create nodes if they don't exist
    std::string from_id = "manual_" + from_object;
    std::string to_id = "manual_" + to_object;
    
    if (!graph_->FindNode(from_id)) {
        auto node = std::make_unique<LineageNode>(from_id, from_object, 
                                                   LineageNodeType::TABLE);
        graph_->AddNode(std::move(node));
    }
    
    if (!graph_->FindNode(to_id)) {
        auto node = std::make_unique<LineageNode>(to_id, to_object,
                                                   LineageNodeType::TABLE);
        graph_->AddNode(std::move(node));
    }
    
    // Create edge
    auto edge = std::make_unique<LineageEdge>(from_id, to_id);
    edge->relationship = relationship;
    edge->detection_method = "manual";
    graph_->AddEdge(std::move(edge));
}

void LineageManager::RefreshFromDatabase() {
    // Re-scan database for changes
    // Update graph with new/removed objects
}

void LineageManager::ExportLineage(const std::string& path, const std::string& format) {
    if (!graph_) return;
    
    std::ofstream file(path);
    if (!file.is_open()) return;
    
    if (format == "dot" || format == "gv") {
        graph_->ExportAsDot(file);
    } else if (format == "json") {
        graph_->ExportAsJson(file);
    }
}

} // namespace scratchrobin
