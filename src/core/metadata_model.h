#ifndef SCRATCHROBIN_METADATA_MODEL_H
#define SCRATCHROBIN_METADATA_MODEL_H

#include <string>
#include <vector>

#include "connection_manager.h"

namespace scratchrobin {

struct MetadataNode {
    std::string label;
    std::string kind;
    std::string ddl;
    std::vector<std::string> dependencies;
    std::vector<MetadataNode> children;
};

struct MetadataSnapshot {
    std::vector<MetadataNode> roots;
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
    MetadataSnapshot GetSnapshot() const;

private:
    void NotifyObservers();

    MetadataSnapshot snapshot_;
    std::vector<MetadataObserver*> observers_;
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_METADATA_MODEL_H
