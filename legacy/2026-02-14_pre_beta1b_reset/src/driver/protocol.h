/**
 * @file protocol.h
 * @brief Protocol definitions for driver-server communication
 * 
 * This file defines the message protocol used between the ScratchRobin
 * application and the separate driver processes. The protocol is designed
 * to be:
 *   - Language-agnostic (works with C++, Python, etc.)
 *   - Versioned for backward compatibility
 *   - Efficient for binary data transfer
 *   - Self-describing for debugging
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>
#include <optional>
#include <variant>
#include <chrono>

namespace scratchrobin {
namespace protocol {

// =============================================================================
// Protocol Version
// =============================================================================

constexpr uint16_t PROTOCOL_VERSION_MAJOR = 1;
constexpr uint16_t PROTOCOL_VERSION_MINOR = 0;
constexpr uint16_t PROTOCOL_VERSION_PATCH = 0;

// =============================================================================
// Message Types
// =============================================================================

enum class MessageType : uint16_t {
    // Connection Management
    ConnectRequest = 0x0100,
    ConnectResponse = 0x0101,
    DisconnectRequest = 0x0102,
    DisconnectResponse = 0x0103,
    
    // Query Execution
    ExecuteRequest = 0x0200,
    ExecuteResponse = 0x0201,
    QueryRequest = 0x0202,
    QueryResponse = 0x0203,
    
    // Schema Introspection
    GetSchemaRequest = 0x0300,
    GetSchemaResponse = 0x0301,
    GetCapabilitiesRequest = 0x0302,
    GetCapabilitiesResponse = 0x0303,
    
    // DDL Operations
    CreateTableRequest = 0x0400,
    CreateTableResponse = 0x0401,
    DropTableRequest = 0x0402,
    DropTableResponse = 0x0403,
    AlterTableRequest = 0x0404,
    AlterTableResponse = 0x0405,
    
    // Transaction Control
    BeginTransactionRequest = 0x0500,
    BeginTransactionResponse = 0x0501,
    CommitRequest = 0x0502,
    CommitResponse = 0x0503,
    RollbackRequest = 0x0504,
    RollbackResponse = 0x0505,
    
    // Prepared Statements
    PrepareRequest = 0x0600,
    PrepareResponse = 0x0601,
    ExecutePreparedRequest = 0x0602,
    ExecutePreparedResponse = 0x0603,
    
    // Streaming
    FetchChunkRequest = 0x0700,
    FetchChunkResponse = 0x0701,
    CancelStream = 0x0702,
    
    // Events/Notifications
    Notification = 0x0800,
    ProgressUpdate = 0x0801,
    
    // Errors
    ErrorResponse = 0x0F00,
    
    // Protocol
    Ping = 0xFF00,
    Pong = 0xFF01,
    VersionNegotiation = 0xFF02
};

// =============================================================================
// Data Value Types
// =============================================================================

enum class ValueType : uint8_t {
    Null = 0,
    Boolean,
    Int32,
    Int64,
    Float32,
    Float64,
    String,
    Binary,
    Date,
    Time,
    DateTime,
    Timestamp,
    Interval,
    UUID,
    Json,
    Array,
    Decimal  // High-precision decimal as string
};

/**
 * @brief Protocol value representation
 * 
 * Uses std::variant for type-safe value storage. Binary data is stored
 * as vector<uint8_t> to handle any encoding.
 */
struct ProtocolValue {
    ValueType type;
    std::variant<
        std::monostate,              // Null
        bool,                        // Boolean
        int32_t,                     // Int32
        int64_t,                     // Int64
        float,                       // Float32
        double,                      // Float64
        std::string,                 // String, Date, Time, DateTime, Timestamp, Json, Decimal
        std::vector<uint8_t>,        // Binary, UUID
        std::vector<ProtocolValue>   // Array
    > data;
    
    // Convenience constructors
    static ProtocolValue Null() { return {ValueType::Null, {}}; }
    static ProtocolValue Boolean(bool v) { return {ValueType::Boolean, v}; }
    static ProtocolValue Int32(int32_t v) { return {ValueType::Int32, v}; }
    static ProtocolValue Int64(int64_t v) { return {ValueType::Int64, v}; }
    static ProtocolValue Float64(double v) { return {ValueType::Float64, v}; }
    static ProtocolValue String(const std::string& v) { return {ValueType::String, v}; }
    static ProtocolValue Binary(const std::vector<uint8_t>& v) { return {ValueType::Binary, v}; }
    static ProtocolValue Date(const std::string& v) { return {ValueType::Date, v}; }
    static ProtocolValue DateTime(const std::string& v) { return {ValueType::DateTime, v}; }
    static ProtocolValue Json(const std::string& v) { return {ValueType::Json, v}; }
};

// =============================================================================
// Column Metadata
// =============================================================================

struct ColumnMetadata {
    std::string name;
    ValueType type;
    uint32_t size;           // Max size in bytes, or 0 for variable
    uint32_t precision;      // For numeric types
    uint32_t scale;          // For numeric types
    bool nullable;
    bool primaryKey;
    std::optional<std::string> defaultValue;
    std::map<std::string, std::string> attributes;  // Backend-specific
};

// =============================================================================
// Row and Result Set
// =============================================================================

using Row = std::vector<ProtocolValue>;

struct ResultSet {
    std::vector<ColumnMetadata> columns;
    std::vector<Row> rows;
    bool hasMoreRows = false;      // For streaming results
    uint64_t totalRowCount = 0;    // May be estimate for large results
};

// =============================================================================
// Error Information
// =============================================================================

struct ErrorInfo {
    uint32_t code;
    std::string message;
    std::optional<std::string> sqlState;
    std::optional<std::string> detail;
    std::optional<std::string> hint;
    std::optional<std::string> position;  // Position in SQL
    std::optional<std::string> schemaName;
    std::optional<std::string> tableName;
    std::optional<std::string> columnName;
    std::optional<std::string> constraintName;
};

// =============================================================================
// Request/Response Base
// =============================================================================

struct RequestHeader {
    uint64_t requestId;          // Unique identifier for correlation
    MessageType type;
    uint16_t protocolVersion;
    std::chrono::milliseconds timeout{30000};
    std::map<std::string, std::string> metadata;
};

struct ResponseHeader {
    uint64_t requestId;          // Matches request
    MessageType type;
    bool success;
    std::optional<ErrorInfo> error;
    std::chrono::milliseconds processingTime;
};

// =============================================================================
// Connection Messages
// =============================================================================

struct ConnectRequest {
    std::string driverId;        // "postgresql", "mysql", "firebird"
    std::map<std::string, std::string> parameters;
    std::chrono::seconds connectTimeout{30};
    std::chrono::seconds queryTimeout{0};  // 0 = no timeout
};

struct ConnectResponse {
    uint64_t connectionId;
    std::string serverVersion;
    std::map<std::string, std::string> serverInfo;
    std::vector<std::string> supportedFeatures;
};

// =============================================================================
// Query Messages
// =============================================================================

struct ExecuteRequest {
    std::string sql;
    std::vector<ProtocolValue> parameters;  // For parameterized queries
};

struct ExecuteResponse {
    uint64_t rowsAffected;
    std::optional<std::string> lastInsertId;
    std::optional<std::string> noticeMessages;
};

struct QueryRequest {
    std::string sql;
    std::vector<ProtocolValue> parameters;
    uint32_t maxRows = 0;  // 0 = no limit
    uint32_t fetchSize = 1000;
};

struct QueryResponse {
    ResultSet resultSet;
    uint64_t executionTimeMs;
};

// =============================================================================
// Schema Messages
// =============================================================================

struct TableMetadata {
    std::string schema;
    std::string name;
    std::string type;  // "table", "view", "system_table", "temporary"
    std::vector<ColumnMetadata> columns;
    std::vector<std::string> primaryKey;
    std::vector<std::map<std::string, std::string>> foreignKeys;
    std::vector<std::map<std::string, std::string>> indexes;
    std::map<std::string, std::string> attributes;
};

struct SchemaMetadata {
    std::vector<std::string> schemas;
    std::vector<TableMetadata> tables;
    std::vector<std::map<std::string, std::string>> sequences;
    std::map<std::string, std::vector<std::string>> enums;
};

struct GetSchemaRequest {
    std::optional<std::string> schemaPattern;  // Filter by schema name
    std::optional<std::string> tablePattern;   // Filter by table name
    std::vector<std::string> tableTypes;       // Empty = all types
};

struct GetSchemaResponse {
    SchemaMetadata schema;
};

// =============================================================================
// Capability Messages
// =============================================================================

struct CapabilityInfo {
    std::string driverName;
    std::string driverVersion;
    
    // Feature flags
    bool supportsTransactions;
    bool supportsSavepoints;
    bool supportsPreparedStatements;
    bool supportsStoredProcedures;
    bool supportsMultipleResultSets;
    bool supportsBatchExecution;
    bool supportsScrollableCursors;
    bool supportsHoldableCursors;
    bool supportsPositionedUpdates;
    bool supportsNamedParameters;
    bool supportsLimitOffset;
    bool supportsReturning;
    bool supportsUpsert;
    bool supportsWindowFunctions;
    bool supportsCommonTableExpressions;
    bool supportsJson;
    bool supportsArrays;
    bool supportsFullTextSearch;
    bool supportsSpatialTypes;
    
    // Limitations
    uint32_t maxConnections;
    uint32_t maxStatementLength;
    uint32_t maxIdentifierLength;
    uint32_t maxIndexKeys;
    uint32_t maxRowSize;
    
    // SQL conformance
    uint8_t sqlConformanceLevel;  // 0=none, 1=entry, 2=intermediate, 3=full
    std::vector<std::string> supportedAggregates;
    std::vector<std::string> supportedFunctions;
    std::vector<std::string> supportedDataTypes;
};

struct GetCapabilitiesResponse {
    CapabilityInfo capabilities;
};

// =============================================================================
// DDL Messages
// =============================================================================

struct ColumnDefinition {
    std::string name;
    std::string dataType;
    std::optional<uint32_t> length;
    std::optional<uint32_t> precision;
    std::optional<uint32_t> scale;
    bool nullable = true;
    std::optional<std::string> defaultValue;
    bool primaryKey = false;
    bool unique = false;
    std::optional<std::string> references;  // Foreign key reference
    std::map<std::string, std::string> attributes;
};

struct CreateTableRequest {
    std::string schema;
    std::string name;
    std::vector<ColumnDefinition> columns;
    std::vector<std::string> constraints;
    std::vector<std::string> indexes;
    bool ifNotExists = false;
    std::map<std::string, std::string> attributes;
};

struct CreateTableResponse {
    bool created;
    std::optional<std::string> notice;
};

// =============================================================================
// Streaming Messages
// =============================================================================

struct FetchChunkRequest {
    uint64_t queryId;
    uint32_t chunkSize;
    uint64_t offset;
};

struct FetchChunkResponse {
    std::vector<Row> rows;
    bool isLastChunk;
    uint64_t totalRowsFetched;
};

// =============================================================================
// Message Serialization
// =============================================================================

/**
 * @brief Serialize a message to wire format
 * 
 * @param header Request/Response header
 * @param payload Message-specific payload (must match header.type)
 * @return Serialized bytes ready for transmission
 */
std::vector<uint8_t> SerializeMessage(
    const RequestHeader& header,
    const auto& payload
);

/**
 * @brief Deserialize a message from wire format
 * 
 * @param data Raw bytes from wire
 * @param header Output: parsed header
 * @return Payload variant that must be cast based on header.type
 */
std::variant<
    ConnectRequest,
    ConnectResponse,
    ExecuteRequest,
    ExecuteResponse,
    QueryRequest,
    QueryResponse,
    GetSchemaRequest,
    GetSchemaResponse,
    GetCapabilitiesResponse,
    CreateTableRequest,
    CreateTableResponse,
    FetchChunkRequest,
    FetchChunkResponse,
    ErrorInfo
> DeserializeMessage(
    const std::vector<uint8_t>& data,
    ResponseHeader& header
);

// =============================================================================
// Wire Format
// =============================================================================

/**
 * @brief Wire format structure
 * 
 * All messages start with a fixed 8-byte header:
 *   - 4 bytes: Magic number "SRDB" (0x53 0x52 0x44 0x42)
 *   - 4 bytes: Message length (including header, little-endian)
 *   - N bytes: Message payload (JSON or binary depending on version)
 * 
 * Protocol v1.0 uses JSON encoding for human readability and
 * debugging. Future versions may add binary encoding for performance.
 */

constexpr uint32_t WIRE_MAGIC = 0x44425253;  // "SRDB" little-endian
constexpr uint32_t WIRE_HEADER_SIZE = 8;

/**
 * @brief Encode message to wire format
 */
std::vector<uint8_t> EncodeToWire(const std::vector<uint8_t>& payload);

/**
 * @brief Decode message from wire format
 * @return Payload bytes (without header), empty if invalid
 */
std::optional<std::vector<uint8_t>> DecodeFromWire(
    const std::vector<uint8_t>& wireData,
    uint32_t& outMessageLength
);

} // namespace protocol
} // namespace scratchrobin
