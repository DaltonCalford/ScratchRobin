/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_METADATA_MODEL_H
#define SCRATCHROBIN_METADATA_MODEL_H

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "connection_manager.h"

namespace scratchrobin {

enum class MetadataType {
    Schema,
    Table,
    View,
    Procedure,
    Function,
    Trigger,
    Index,
    Column,
    Constraint
};

struct MetadataNode {
    // Original fields (for backward compatibility)
    std::string label;
    std::string kind;
    std::string catalog;
    std::string path;
    std::string ddl;
    std::vector<std::string> dependencies;
    std::vector<MetadataNode> children;
    
    // New fields (for test compatibility)
    int id = 0;
    MetadataType type;
    std::string name;   // Alias for label
    std::string schema;
    int parent_id = 0;
    int64_t row_count = 0;
    
    bool HasDependency(const std::string& dep) const {
        for (const auto& d : dependencies) {
            if (d == dep) return true;
        }
        return false;
    }
    
    bool operator==(const MetadataNode& other) const {
        return id == other.id;
    }
    
    bool operator!=(const MetadataNode& other) const {
        return !(*this == other);
    }
};

struct MetadataSnapshot {
    std::vector<MetadataNode> nodes;
    std::vector<MetadataNode> roots;  // Alias for top-level nodes
    std::chrono::system_clock::time_point timestamp;
};

class MetadataObserver {
public:
    virtual ~MetadataObserver() = default;
    virtual void OnMetadataUpdated(const MetadataSnapshot& snapshot) = 0;
};

class MetadataModel {
public:
    void AddObserver(MetadataObserver* observer);
    void RemoveObserver(MetadataObserver* observer);

    void LoadStub();
    void UpdateConnections(const std::vector<ConnectionProfile>& profiles);
    void UpdateNode(const MetadataNode& node);
    void RemoveNode(int id);
    std::optional<MetadataNode> FindNodeByPath(const std::string& path) const;
    std::vector<MetadataNode> FindNodesByType(MetadataType type) const;
    void Clear();
    const MetadataSnapshot& GetSnapshot() const;
    void SetFixturePath(const std::string& path);
    bool LoadFromFixture(const std::string& path, std::string* error);
    void Refresh();
    const std::string& LastError() const { return last_error_; }

private:
    void NotifyObservers();
    void LoadFallback(const std::string& message);
    bool LoadFromConnections(std::string* error);

    MetadataSnapshot snapshot_;
    std::vector<MetadataObserver*> observers_;
    std::vector<ConnectionProfile> profiles_;
    std::string fixture_path_;
    std::string last_error_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_METADATA_MODEL_H
