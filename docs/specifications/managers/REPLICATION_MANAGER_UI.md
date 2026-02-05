# Replication Manager UI Specification

**Status**: Beta Placeholder  
**Target Release**: Beta  
**Priority**: P2

---

## Overview

The Replication Manager provides tools for monitoring and managing database replication, including streaming replication lag tracking, logical replication slot management, and conflict resolution.

---

## Planned Features

### 1. Replication Topology View
- Visual representation of replication relationships
- Master-replica hierarchy display
- Cascading replication chains
- Multi-master ring topology support

### 2. Lag Monitoring
- Real-time replication lag metrics
- Byte lag and time lag tracking
- Historical lag trend charts
- Configurable lag thresholds with alerts
- Replication rate monitoring

### 3. Replication Slot Management
- View active/inactive replication slots
- Create and drop logical replication slots
- Monitor slot LSN positions
- Slot persistence configuration
- Failover slot management

### 4. Logical Replication (PostgreSQL)
- Publication management
  - Create/modify publications
  - Table/schema inclusion rules
  - Row-level filtering
  - Column list filtering
- Subscription management
  - Create/modify subscriptions
  - Connection string management
  - Initial data copy control
  - Two-phase commit support

### 5. Conflict Detection & Resolution
- Real-time conflict monitoring
- Conflict classification by type
- Automatic resolution policies
- Manual conflict review interface
- Conflict history tracking

### 6. Replication Statistics
- Transactions replicated per second
- Bytes transferred
- Replication apply rate
- Conflict counts
- Error logs

---

## UI Components

### Main Window Layout
```
+----------------------------------------------------------+
|  Replication Manager                                     |
+----------------------------------------------------------+
|  [Topology] [Lag Monitor] [Slots] [Pub/Sub] [Conflicts] |
+----------------------------------------------------------+
|                                                          |
|  Tab Content:                                            |
|  +--------------------------------------------------+   |
|  | Replication topology diagram or metrics view      |   |
|  +--------------------------------------------------+   |
|                                                          |
|  +--------------------------------------------------+   |
|  | Alert Summary / Recent Events                    |   |
|  +--------------------------------------------------+   |
```

### Tabs

#### Topology Tab
- Visual replication tree/graph
- Node status indicators
- Connection health
- Replication mode display (async/sync/semi-sync)

#### Lag Monitor Tab
- Real-time lag table
- Lag visualization (gauges/progress bars)
- Historical charts
- Alert status

#### Slots Tab
- Replication slot list
- LSN position tracking
- Slot state management
- Size monitoring (for lagging slots)

#### Pub/Sub Tab (Logical Replication)
- Publications list with tables
- Subscriptions list with status
- Connection health
- Sync progress

#### Conflicts Tab
- Active conflicts list
- Resolution recommendations
- Manual resolution actions
- Conflict history

---

## Data Models

See `src/core/replication_model.h`:

- `ReplicationConnection` - Connection between nodes
- `ReplicationSlot` - Slot state and LSN tracking
- `ReplicationLag` - Lag metrics per replica
- `ReplicationConflict` - Conflict information
- `ReplicationPublication` - Publication configuration
- `ReplicationSubscription` - Subscription configuration
- `ReplicationTopology` - Complete topology state
- `ReplicationStats` - Performance statistics

---

## Supported Replication Types

| Type | Status | Description |
|------|--------|-------------|
| Streaming Replication | Planned | Physical WAL streaming |
| Logical Replication | Planned | Row-level logical replication |
| Trigger-Based | Future | Legacy trigger replication |
| Synchronous | Planned | Synchronous commit replication |
| Cascading | Planned | Multi-level replication chains |

---

## Backend Support

| Backend | Streaming | Logical | Notes |
|---------|-----------|---------|-------|
| PostgreSQL | Planned | Planned | Native support |
| MySQL | Planned | N/A | Via replication protocol |
| Firebird | Future | Future | Future consideration |
| ScratchBird | Planned | Planned | Native support |

---

## Implementation Status

- [x] Data models defined (`replication_model.h`)
- [x] Stub UI implemented (`replication_manager_frame.cpp`)
- [ ] Streaming replication monitoring
- [ ] Logical replication management
- [ ] Lag metrics collection
- [ ] Conflict detection system
- [ ] Topology visualization

---

## Related Specifications

- See also: [CLUSTER_MANAGER_UI.md](CLUSTER_MANAGER_UI.md) - Cluster management often works with replication
