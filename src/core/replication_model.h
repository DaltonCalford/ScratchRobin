/**
 * @file replication_model.h
 * @brief Data models for replication management (Beta placeholder)
 * 
 * This file defines the data structures for replication topology,
 * replication slots, and lag monitoring.
 * 
 * Status: Beta Placeholder - UI structure only
 */

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <map>

namespace scratchrobin {

/**
 * @brief Replication mode
 */
enum class ReplicationMode {
    Async,      // Asynchronous replication
    Sync,       // Synchronous replication
    SemiSync,   // Semi-synchronous (wait for at least one replica)
    Logical,    // Logical replication (row-based)
    Physical    // Physical replication (WAL shipping)
};

/**
 * @brief Replication slot state
 */
enum class SlotState {
    Active,     // Currently replicating
    Inactive,   // Disconnected but reserved
    Stalled,    // Lag exceeds threshold
    Dropped     // Slot removed
};

/**
 * @brief Replication conflict types
 */
enum class ConflictType {
    None,
    LockTimeout,
    Deadlock,
    Sequence,
    Tablespace,
    Checksum,
    Other
};

/**
 * @brief Replication connection info
 */
struct ReplicationConnection {
    std::string connectionId;
    std::string sourceNodeId;
    std::string targetNodeId;
    std::string sourceHost;
    std::string targetHost;
    uint16_t port = 0;
    ReplicationMode mode = ReplicationMode::Async;
    bool isActive = false;
};

/**
 * @brief Replication slot for logical replication
 */
struct ReplicationSlot {
    std::string slotName;
    std::string database;
    std::string plugin;  // Logical decoding plugin
    SlotState state = SlotState::Inactive;
    
    // LSN positions (PostgreSQL-style log sequence numbers)
    std::string confirmedFlushLsn;
    std::string restartLsn;
    std::string confirmedLsn;
    
    // Activity tracking
    std::optional<std::chrono::system_clock::time_point> activeSince;
    std::optional<std::chrono::system_clock::time_point> lastConfirmed;
    
    // Flags
    bool isTemporary = false;
    bool isTwoPhase = false;
    bool isFailoverSlot = false;
};

/**
 * @brief Replication lag metrics
 */
struct ReplicationLag {
    std::string replicaNodeId;
    
    // Byte lag
    std::optional<uint64_t> lagBytes;
    
    // Time lag
    std::optional<std::chrono::milliseconds> lagTime;
    
    // Transaction lag
    std::optional<uint64_t> lagTransactions;
    
    // Apply rate
    std::optional<double> applyRateBytesPerSec;
    
    // Trend (increasing/decreasing)
    std::optional<double> lagTrend;  // +1 increasing, -1 decreasing, 0 stable
};

/**
 * @brief Replication conflict info
 */
struct ReplicationConflict {
    std::string conflictId;
    std::chrono::system_clock::time_point detectedAt;
    ConflictType type = ConflictType::None;
    std::string database;
    std::string relation;  // Table or object name
    std::string description;
    std::optional<std::string> resolution;  // How it was resolved
    bool isResolved = false;
};

/**
 * @brief Publication for logical replication (PostgreSQL-style)
 */
struct ReplicationPublication {
    std::string pubName;
    std::vector<std::string> tables;  // Empty = all tables
    std::vector<std::string> schemas; // For schema-level publications
    bool publishInsert = true;
    bool publishUpdate = true;
    bool publishDelete = true;
    bool publishTruncate = true;
    std::optional<std::string> rowFilter;  // WHERE clause filter
    std::vector<std::string> columns;  // Column list filter (empty = all)
};

/**
 * @brief Subscription for logical replication (PostgreSQL-style)
 */
struct ReplicationSubscription {
    std::string subName;
    std::string connectionString;  // Connection to publisher
    std::vector<std::string> publications;  // Publications to subscribe to
    bool enabled = true;
    bool copyData = true;  // Copy existing data on init
    bool createSlot = true;
    std::string slotName;
    
    // Sync settings
    bool synchronousCommit = false;
    bool binaryTransfer = false;
    bool streaming = false;  // Streaming in-progress transactions
};

/**
 * @brief Complete replication topology
 */
struct ReplicationTopology {
    std::string topologyId;
    std::string topologyName;
    
    // Nodes participating in replication
    std::vector<std::string> nodeIds;
    
    // Connections (source -> target mapping)
    std::vector<ReplicationConnection> connections;
    
    // Replication slots
    std::vector<ReplicationSlot> slots;
    
    // Publications (for logical replication)
    std::vector<ReplicationPublication> publications;
    
    // Subscriptions (for logical replication)
    std::vector<ReplicationSubscription> subscriptions;
    
    // Current lag metrics
    std::vector<ReplicationLag> lagMetrics;
    
    // Active conflicts
    std::vector<ReplicationConflict> activeConflicts;
    
    // Configuration
    ReplicationMode defaultMode = ReplicationMode::Async;
    std::chrono::milliseconds maxLagThreshold{10000};  // 10 seconds
    bool autoResolveConflicts = false;
};

/**
 * @brief Replication statistics
 */
struct ReplicationStats {
    std::string nodeId;
    std::chrono::system_clock::time_point timestamp;
    
    // Sent/received metrics
    uint64_t sentBytes = 0;
    uint64_t receivedBytes = 0;
    uint64_t sentTransactions = 0;
    uint64_t receivedTransactions = 0;
    
    // Replay metrics (for replicas)
    uint64_t replayedTransactions = 0;
    std::chrono::milliseconds replayLag{0};
    
    // Conflict counts
    uint32_t conflictsDetected = 0;
    uint32_t conflictsResolved = 0;
};

} // namespace scratchrobin
