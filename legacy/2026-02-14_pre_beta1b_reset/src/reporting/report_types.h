// Copyright (c) 2026, Dennis C. Alfonso
// Licensed under the MIT License. See LICENSE file in the project root.
#pragma once

#include <ctime>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace scratchrobin {
namespace reporting {

// Forward declarations
class ReportCache;
class QueryExecutor;

// ============================================================================
// Common Types
// ============================================================================

using ReportId = std::string;
using CollectionId = std::string;
using Timestamp = std::time_t;

// Parameter types for parameterized queries
enum class ParameterType {
    Text,
    Number,
    Date,
    DateTime,
    Enum,
    FieldFilter
};

struct Parameter {
    std::string id;
    std::string name;
    ParameterType type;
    std::optional<std::string> default_value;
    std::vector<std::string> enum_values;  // For Enum type
    std::optional<std::string> field_ref;  // For FieldFilter type
};

// ============================================================================
// Visualization Types
// ============================================================================

enum class VisualizationType {
    Table,
    Bar,
    Line,
    Area,
    Pie,
    Scatter,
    KPI,        // Single number display
    Funnel,
    Gauge,
    Map
};

struct Visualization {
    VisualizationType type = VisualizationType::Table;
    std::map<std::string, std::string> options;
    std::optional<std::string> x_axis_column;
    std::optional<std::string> y_axis_column;
    std::optional<std::string> series_column;
};

// ============================================================================
// Query Types
// ============================================================================

enum class QueryType {
    Builder,    // Visual query builder
    NativeSQL   // Raw SQL
};

struct Filter {
    std::string column;
    std::string operator_;  // =, !=, <, >, <=, >=, LIKE, IN, etc.
    std::variant<std::string, double, bool> value;
    std::optional<std::string> parameter_ref;  // If filter uses a parameter
};

struct Aggregation {
    std::string column;
    std::string function;  // SUM, AVG, COUNT, MIN, MAX
    std::string alias;
};

struct Breakout {
    std::string column;
    std::optional<std::string> time_granularity;  // day, week, month, year
};

struct BuilderQuery {
    std::string source;  // table:name or model:name
    std::vector<Filter> filters;
    std::vector<Aggregation> aggregations;
    std::vector<Breakout> breakouts;
    std::vector<std::string> order_by;
    int limit = 10000;
};

struct Query {
    QueryType type = QueryType::Builder;
    std::optional<BuilderQuery> builder_query;
    std::optional<std::string> native_sql;
};

// ============================================================================
// Question (Saved Query)
// ============================================================================

struct Question {
    ReportId id;
    std::string name;
    std::optional<std::string> description;
    CollectionId collection_id;
    
    // Query configuration
    bool sql_mode = false;  // true = native SQL, false = builder
    Query query;
    std::vector<Parameter> parameters;
    
    // Visualization
    Visualization visualization;
    
    // Metadata
    std::string connection_ref;
    std::optional<std::string> model_ref;  // Associated semantic model
    
    // Execution tracking
    struct LastRun {
        Timestamp at;
        int row_count;
        std::optional<std::string> error_message;
    };
    std::optional<LastRun> last_run;
    
    // Timestamps
    Timestamp created_at;
    Timestamp updated_at;
    std::string created_by;
    std::string updated_by;
    
    // Serialization
    std::string ToJson() const;
    static Question FromJson(const std::string& json);
};

// ============================================================================
// Dashboard
// ============================================================================

struct DashboardCard {
    std::string id;
    ReportId question_id;
    struct Position {
        int x = 0;
        int y = 0;
        int w = 6;
        int h = 4;
    };
    Position position;
    std::optional<Visualization> visualization_override;
};

struct DashboardFilter {
    std::string id;
    std::string type;  // date_range, enum, text, number
    std::vector<std::string> targets;  // question_id:column mappings
    bool required = false;
    std::optional<std::string> default_value;
};

struct Dashboard {
    ReportId id;
    std::string name;
    std::optional<std::string> description;
    CollectionId collection_id;
    
    std::vector<DashboardCard> cards;
    std::vector<DashboardFilter> filters;
    
    // Timestamps
    Timestamp created_at;
    Timestamp updated_at;
    std::string created_by;
    std::string updated_by;
    
    // Serialization
    std::string ToJson() const;
    static Dashboard FromJson(const std::string& json);
};

// ============================================================================
// Semantic Layer - Model
// ============================================================================

struct ModelField {
    std::string name;
    std::string type;
    bool visible = true;
    std::optional<std::string> description;
    std::optional<std::string> foreign_key_ref;  // table.column
};

struct ModelJoin {
    std::string name;
    std::string target_model;
    std::string source_column;
    std::string target_column;
    std::string join_type;  // inner, left, right
};

struct Model {
    ReportId id;
    std::string name;
    std::optional<std::string> description;
    CollectionId collection_id;
    
    std::string source;  // table:name or sql:query
    std::vector<ModelField> fields;
    std::vector<ModelJoin> joins;
    
    // Timestamps
    Timestamp created_at;
    Timestamp updated_at;
    std::string created_by;
    std::string updated_by;
    
    // Serialization
    std::string ToJson() const;
    static Model FromJson(const std::string& json);
};

// ============================================================================
// Semantic Layer - Metric
// ============================================================================

struct Metric {
    ReportId id;
    std::string name;
    std::optional<std::string> description;
    CollectionId collection_id;
    
    std::string expression;  // e.g., "SUM(orders.total)"
    std::optional<std::string> model_ref;
    std::optional<std::string> default_time_dimension;
    
    // Timestamps
    Timestamp created_at;
    Timestamp updated_at;
    std::string created_by;
    std::string updated_by;
    
    // Serialization
    std::string ToJson() const;
    static Metric FromJson(const std::string& json);
};

// ============================================================================
// Semantic Layer - Segment
// ============================================================================

struct Segment {
    ReportId id;
    std::string name;
    std::optional<std::string> description;
    CollectionId collection_id;
    
    std::string expression;  // e.g., "customers.status = 'active'"
    std::string scope;  // table:name or model:name
    
    // Timestamps
    Timestamp created_at;
    Timestamp updated_at;
    std::string created_by;
    std::string updated_by;
    
    // Serialization
    std::string ToJson() const;
    static Segment FromJson(const std::string& json);
};

// ============================================================================
// Alert
// ============================================================================

struct AlertCondition {
    std::string operator_;  // <, >, <=, >=, =, !=, "above", "below"
    double threshold;
    std::optional<std::string> compare_to;  // previous_value, rolling_avg, etc.
};

struct Alert {
    ReportId id;
    std::string name;
    ReportId question_id;
    
    AlertCondition condition;
    std::string schedule;  // hourly, daily, realtime
    std::vector<std::string> channels;  // email, slack, webhook
    bool only_on_change = true;
    bool enabled = true;
    
    // Tracking
    std::optional<Timestamp> last_triggered;
    std::optional<double> last_value;
    
    // Timestamps
    Timestamp created_at;
    Timestamp updated_at;
    std::string created_by;
    
    // Serialization
    std::string ToJson() const;
    static Alert FromJson(const std::string& json);
};

// ============================================================================
// Subscription
// ============================================================================

struct Subscription {
    ReportId id;
    std::string name;
    std::string target_type;  // "dashboard" or "question"
    ReportId target_id;
    
    std::string schedule;  // hourly, daily, weekly, cron expression
    std::map<std::string, std::string> filters;  // Filter overrides
    std::vector<std::string> channels;
    bool include_csv = false;
    bool enabled = true;
    
    // Tracking
    std::optional<Timestamp> last_run;
    int run_count = 0;
    int fail_count = 0;
    
    // Timestamps
    Timestamp created_at;
    Timestamp updated_at;
    std::string created_by;
    
    // Serialization
    std::string ToJson() const;
    static Subscription FromJson(const std::string& json);
};

// ============================================================================
// Collection (Organization)
// ============================================================================

struct Collection {
    ReportId id;
    std::string name;
    std::optional<std::string> parent_id;  // For nesting
    std::optional<std::string> description;
    
    enum class Status {
        Official,
        Community,
        Archived
    };
    Status status = Status::Official;
    
    // Permissions
    struct Permissions {
        std::vector<std::string> view;     // role:viewer, user:name, etc.
        std::vector<std::string> curate;   // Can add/edit items
        std::vector<std::string> admin;    // Can manage permissions
    };
    Permissions permissions;
    
    // Timestamps
    Timestamp created_at;
    Timestamp updated_at;
    std::string created_by;
    
    // Serialization
    std::string ToJson() const;
    static Collection FromJson(const std::string& json);
};

// ============================================================================
// Timeline (Events)
// ============================================================================

struct TimelineEvent {
    std::string id;
    std::string type;  // deployment, incident, release, etc.
    Timestamp timestamp;
    std::string title;
    std::optional<std::string> description;
    std::map<std::string, std::string> metadata;
};

struct Timeline {
    ReportId id;
    std::string name;
    std::optional<std::string> description;
    CollectionId collection_id;
    
    std::vector<TimelineEvent> events;
    
    // Timestamps
    Timestamp created_at;
    Timestamp updated_at;
    std::string created_by;
    
    // Serialization
    std::string ToJson() const;
    static Timeline FromJson(const std::string& json);
};

} // namespace reporting
} // namespace scratchrobin
