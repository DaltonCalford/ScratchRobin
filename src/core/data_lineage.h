/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_DATA_LINEAGE_H
#define SCRATCHROBIN_DATA_LINEAGE_H

#include <ctime>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace scratchrobin {

// ============================================================================
// Lineage Node Types
// ============================================================================
enum class LineageNodeType {
    TABLE,
    VIEW,
    COLUMN,
    PROCEDURE,
    FUNCTION,
    TRIGGER,
    EXTERNAL_SOURCE,
    EXTERNAL_SINK,
    TRANSFORMATION,
    JOIN,
    AGGREGATION,
    FILTER
};

std::string LineageNodeTypeToString(LineageNodeType type);

// ============================================================================
// Lineage Node
// ============================================================================
struct LineageNode {
    std::string id;
    std::string name;
    LineageNodeType type;
    std::string schema;
    std::string database;
    
    // For columns
    std::string table_name;
    std::string data_type;
    
    // For transformations
    std::string transformation_logic;  // SQL expression, formula, etc.
    std::string transformation_type;   // "CAST", "CONCAT", "CALCULATION", etc.
    
    // Metadata
    std::string description;
    std::map<std::string, std::string> tags;
    std::time_t discovered_at;
    
    LineageNode();
    LineageNode(const std::string& id, const std::string& name, LineageNodeType type);
    
    std::string GetFullName() const;
    bool IsSource() const;
    bool IsSink() const;
    bool IsTransformation() const;
};

// ============================================================================
// Lineage Edge (Data Flow)
// ============================================================================
struct LineageEdge {
    std::string id;
    std::string from_node_id;
    std::string to_node_id;
    
    // Edge properties
    std::string edge_type;  // "direct", "transformed", "aggregated", "filtered"
    std::string relationship;  // "populates", "derives", "joins", "filters"
    
    // For data flow analysis
    int confidence_score = 100;  // 0-100, how certain is this lineage
    std::string detection_method;  // "sql_parser", "manual", "runtime", "static_analysis"
    
    // Transformation details (if applicable)
    std::string transformation_description;
    
    std::time_t discovered_at;
    
    LineageEdge();
    LineageEdge(const std::string& from, const std::string& to);
};

// ============================================================================
// Impact Analysis Result
// ============================================================================
struct ImpactAnalysisResult {
    // Direct dependencies (read from this object)
    std::vector<std::string> upstream_nodes;
    
    // Dependents (write to this object)
    std::vector<std::string> downstream_nodes;
    
    // Full transitive closure
    std::vector<std::string> all_upstream;
    std::vector<std::string> all_downstream;
    
    // Risk assessment
    struct Risk {
        std::string node_id;
        std::string risk_type;  // "data_loss", "breakage", "performance"
        std::string description;
        int severity;  // 1-10
    };
    std::vector<Risk> risks;
    
    // Estimated impact
    int total_objects_affected = 0;
    int tables_affected = 0;
    int views_affected = 0;
    int procedures_affected = 0;
};

// ============================================================================
// Data Lineage Graph
// ============================================================================
class LineageGraph {
public:
    std::map<std::string, std::unique_ptr<LineageNode>> nodes;
    std::map<std::string, std::unique_ptr<LineageEdge>> edges;
    
    // Node/Edge management
    void AddNode(std::unique_ptr<LineageNode> node);
    void RemoveNode(const std::string& id);
    LineageNode* FindNode(const std::string& id);
    
    void AddEdge(std::unique_ptr<LineageEdge> edge);
    void RemoveEdge(const std::string& id);
    LineageEdge* FindEdge(const std::string& id);
    
    // Query methods
    std::vector<LineageNode*> GetUpstreamNodes(const std::string& node_id, int depth = -1);
    std::vector<LineageNode*> GetDownstreamNodes(const std::string& node_id, int depth = -1);
    std::vector<LineageEdge*> GetIncomingEdges(const std::string& node_id);
    std::vector<LineageEdge*> GetOutgoingEdges(const std::string& node_id);
    
    // Path finding
    std::vector<std::vector<std::string>> FindAllPaths(const std::string& from_id, 
                                                        const std::string& to_id);
    
    // Impact analysis
    ImpactAnalysisResult AnalyzeImpact(const std::string& node_id);
    
    // Graph operations
    void Clear();
    bool IsEmpty() const { return nodes.empty(); }
    
    // Export
    void ExportAsDot(std::ostream& out) const;  // GraphViz format
    void ExportAsJson(std::ostream& out) const;
    
    // Serialization
    void SaveToFile(const std::string& path) const;
    static std::unique_ptr<LineageGraph> LoadFromFile(const std::string& path);
};

// ============================================================================
// SQL Lineage Parser
// ============================================================================
class SqlLineageParser {
public:
    struct ParseResult {
        std::vector<std::unique_ptr<LineageNode>> nodes;
        std::vector<std::unique_ptr<LineageEdge>> edges;
        std::vector<std::string> errors;
        bool success = false;
    };
    
    // Parse SQL to extract lineage
    ParseResult Parse(const std::string& sql, const std::string& default_schema = "");
    
    // Parse specific SQL types
    ParseResult ParseSelect(const std::string& sql, const std::string& default_schema = "");
    ParseResult ParseInsert(const std::string& sql, const std::string& default_schema = "");
    ParseResult ParseUpdate(const std::string& sql, const std::string& default_schema = "");
    ParseResult ParseCreateView(const std::string& sql, const std::string& default_schema = "");
    ParseResult ParseCreateProcedure(const std::string& sql, const std::string& default_schema = "");
    
private:
    // Helper methods for parsing
    void ExtractTableReferences(const std::string& sql, ParseResult& result);
    void ExtractColumnReferences(const std::string& sql, ParseResult& result);
    void ExtractJoins(const std::string& sql, ParseResult& result);
    void ExtractAggregations(const std::string& sql, ParseResult& result);
};

// ============================================================================
// Runtime Lineage Collector
// ============================================================================
class RuntimeLineageCollector {
public:
    struct QueryEvent {
        std::string query_id;
        std::string sql;
        std::string session_id;
        std::string user_id;
        std::time_t timestamp;
        
        // Source tables (read from)
        std::vector<std::string> source_tables;
        
        // Target tables (written to)
        std::vector<std::string> target_tables;
        
        // Columns accessed
        std::vector<std::string> columns_read;
        std::vector<std::string> columns_written;
        
        // Performance metrics
        int64_t rows_read = 0;
        int64_t rows_written = 0;
        int64_t execution_time_ms = 0;
    };
    
    // Event recording
    void RecordQuery(const QueryEvent& event);
    void RecordDataMovement(const std::string& from_table, const std::string& to_table,
                            const std::vector<std::string>& columns);
    
    // Analysis
    std::vector<QueryEvent> GetQueryHistory(const std::string& table_name,
                                             std::time_t from, std::time_t to);
    
    // Generate lineage from collected data
    std::unique_ptr<LineageGraph> GenerateLineageGraph();
    
    // Configuration
    void SetRetentionPeriod(int days);
    void EnableCollection(bool enable);
    bool IsCollectionEnabled() const { return enabled_; }
    
private:
    void EnforceRetentionPolicy();
    
    std::vector<QueryEvent> events_;
    bool enabled_ = false;
    int retention_days_ = 30;
};

// ============================================================================
// Lineage Visualizer Options
// ============================================================================
struct LineageVisualOptions {
    bool show_columns = true;
    bool show_transformations = true;
    bool group_by_schema = false;
    int max_depth = -1;
    std::string highlight_node_id;
};

// ============================================================================
// Lineage Visualizer
// ============================================================================
class LineageVisualizer {
public:
    using VisualOptions = LineageVisualOptions;
    
    // Generate visual representation
    std::string GenerateGraphvizDot(const LineageGraph& graph, 
                                     const VisualOptions& options = VisualOptions());
    
    // Export formats
    void ExportAsSvg(const LineageGraph& graph, const std::string& path,
                     const VisualOptions& options = VisualOptions());
    void ExportAsPng(const LineageGraph& graph, const std::string& path,
                     const VisualOptions& options = VisualOptions());
    
    // Interactive data for UI
    struct NodePosition {
        std::string node_id;
        double x, y;
        double width, height;
    };
    
    std::vector<NodePosition> CalculateLayout(const LineageGraph& graph,
                                               int canvas_width, int canvas_height);
};

// ============================================================================
// Lineage Manager (Main API)
// ============================================================================
class LineageManager {
public:
    static LineageManager& Instance();
    
    // Initialize from database
    void InitializeFromDatabase(const std::string& connection_string);
    
    // Build lineage from SQL scripts
    void BuildFromSqlScripts(const std::vector<std::string>& script_paths);
    
    // Build lineage from runtime queries
    void EnableRuntimeCollection(bool enable);
    
    // Get current lineage graph
    LineageGraph* GetGraph() { return graph_.get(); }
    
    // Query methods
    ImpactAnalysisResult AnalyzeImpact(const std::string& object_name);
    std::vector<std::string> GetDataSources(const std::string& object_name);
    std::vector<std::string> GetDataSinks(const std::string& object_name);
    
    // Manual lineage entry
    void AddManualLineage(const std::string& from_object, const std::string& to_object,
                          const std::string& relationship);
    
    // Refresh
    void RefreshFromDatabase();
    
    // Export
    void ExportLineage(const std::string& path, const std::string& format);
    
private:
    LineageManager() = default;
    ~LineageManager() = default;
    
    std::unique_ptr<LineageGraph> graph_;
    std::unique_ptr<RuntimeLineageCollector> collector_;
    std::unique_ptr<SqlLineageParser> parser_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_DATA_LINEAGE_H
