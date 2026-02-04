/**
 * @file cluster_model.h
 * @brief Data models for cluster management (Beta placeholder)
 * 
 * This file defines the data structures for cluster topology,
 * node management, and high availability configuration.
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
 * @brief Node role in a cluster
 */
enum class NodeRole {
    Primary,      // Read-write master node
    Secondary,    // Read replica
    Witness,      // Quorum witness (no data)
    Standby,      // Hot standby/failover target
    Arbiter       // Consensus arbiter
};

/**
 * @brief Node health status
 */
enum class NodeHealth {
    Healthy,      // Fully operational
    Degraded,     // Operating with reduced capacity
    Unhealthy,    // Failed or unreachable
    Recovering,   // In recovery/sync
    Maintenance,  // Under maintenance
    Unknown       // Status unavailable
};

/**
 * @brief Cluster topology type
 */
enum class ClusterTopology {
    SinglePrimary,    // One primary, multiple secondaries
    MultiPrimary,     // Multiple writable primaries
    Ring,             // Circular replication
    Shard,            // Sharded/partitioned
    Distributed       // Distributed consensus (Raft/Paxos)
};

/**
 * @brief Cluster node information
 */
struct ClusterNode {
    std::string nodeId;                    // Unique node identifier
    std::string name;                      // Human-readable name
    std::string host;                      // Hostname or IP
    uint16_t port = 0;                     // Port number
    NodeRole role = NodeRole::Secondary;   // Current role
    NodeHealth health = NodeHealth::Unknown; // Current health
    
    // Replication info
    std::optional<std::string> primaryNodeId; // For replicas: primary's ID
    std::vector<std::string> replicaNodeIds;  // For primaries: list of replicas
    
    // Performance metrics
    std::optional<double> replicationLagMs;   // Replication lag in milliseconds
    std::optional<double> cpuPercent;         // CPU utilization
    std::optional<double> memoryPercent;      // Memory utilization
    std::optional<uint64_t> connections;      // Active connections
    
    // Timestamps
    std::chrono::system_clock::time_point lastSeen;
    std::optional<std::chrono::system_clock::time_point> lastError;
    
    // Version info
    std::string serverVersion;
    std::string clusterProtocolVersion;
    
    // Tags/labels for organization
    std::map<std::string, std::string> tags;
};

/**
 * @brief Cluster configuration
 */
struct ClusterConfig {
    std::string clusterId;
    std::string clusterName;
    ClusterTopology topology = ClusterTopology::SinglePrimary;
    
    // Failover settings
    bool autoFailoverEnabled = false;
    uint32_t failoverTimeoutSeconds = 30;
    uint32_t failoverCooldownSeconds = 300;
    
    // Quorum settings
    uint32_t minNodesForQuorum = 2;
    bool requireWitness = false;
    
    // Replication settings
    std::string replicationMode;  // "async", "sync", "semi-sync"
    uint32_t replicationFactor = 1;
    
    // Load balancing
    bool loadBalanceReads = false;
    std::string loadBalancePolicy;  // "round-robin", "least-connections", "random"
};

/**
 * @brief Cluster health summary
 */
struct ClusterHealth {
    std::string clusterId;
    NodeHealth overallHealth = NodeHealth::Unknown;
    size_t healthyNodes = 0;
    size_t degradedNodes = 0;
    size_t unhealthyNodes = 0;
    size_t unknownNodes = 0;
    bool hasQuorum = false;
    std::optional<std::string> primaryNodeId;
    std::vector<std::string> alerts;
};

/**
 * @brief Failover history entry
 */
struct FailoverEvent {
    std::string eventId;
    std::chrono::system_clock::time_point timestamp;
    std::string oldPrimaryNodeId;
    std::string newPrimaryNodeId;
    std::string reason;  // "manual", "automatic", "failure"
    bool successful = false;
    std::optional<std::string> errorMessage;
    std::chrono::milliseconds duration{0};
};

/**
 * @brief Complete cluster state
 */
struct ClusterState {
    ClusterConfig config;
    std::vector<ClusterNode> nodes;
    ClusterHealth health;
    std::vector<FailoverEvent> recentFailovers;
    std::chrono::system_clock::time_point lastUpdated;
};

/**
 * @brief Cluster operation types
 */
enum class ClusterOperation {
    InitCluster,
    JoinNode,
    RemoveNode,
    PromoteNode,
    DemoteNode,
    Rebalance,
    Failover,
    EnableMaintenance,
    DisableMaintenance,
    UpdateConfig
};

/**
 * @brief Cluster operation request
 */
struct ClusterOperationRequest {
    ClusterOperation operation;
    std::string targetNodeId;
    std::map<std::string, std::string> parameters;
    std::string requestedBy;  // User ID
    std::chrono::system_clock::time_point requestedAt;
};

} // namespace scratchrobin
