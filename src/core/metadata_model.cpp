#include "metadata_model.h"

#include <algorithm>

namespace scratchrobin {

void MetadataModel::AddObserver(MetadataObserver* observer) {
    if (!observer) {
        return;
    }
    if (std::find(observers_.begin(), observers_.end(), observer) != observers_.end()) {
        return;
    }
    observers_.push_back(observer);
}

void MetadataModel::RemoveObserver(MetadataObserver* observer) {
    if (!observer) {
        return;
    }
    observers_.erase(std::remove(observers_.begin(), observers_.end(), observer), observers_.end());
}

void MetadataModel::LoadStub() {
    MetadataSnapshot snapshot;
    MetadataNode root;
    root.label = "Connections";

    MetadataNode local;
    local.label = "Local ScratchBird";
    local.children.push_back(MetadataNode{"Host: 127.0.0.1:3050", {}});
    local.children.push_back(MetadataNode{"Database: /data/scratchbird/demo.sdb", {}});
    root.children.push_back(std::move(local));

    snapshot.roots.push_back(std::move(root));
    snapshot_ = std::move(snapshot);
    NotifyObservers();
}

void MetadataModel::UpdateConnections(const std::vector<ConnectionProfile>& profiles) {
    MetadataSnapshot snapshot;
    MetadataNode root;
    root.label = "Connections";

    for (const auto& profile : profiles) {
        MetadataNode entry;
        entry.label = profile.name.empty() ? profile.database : profile.name;
        if (!profile.host.empty() || profile.port != 0) {
            std::string host_label = "Host: " + (profile.host.empty() ? "localhost" : profile.host);
            if (profile.port != 0) {
                host_label += ":" + std::to_string(profile.port);
            }
            entry.children.push_back(MetadataNode{host_label, {}});
        }
        if (!profile.database.empty()) {
            entry.children.push_back(MetadataNode{"Database: " + profile.database, {}});
        }
        if (!profile.username.empty()) {
            entry.children.push_back(MetadataNode{"User: " + profile.username, {}});
        }
        root.children.push_back(std::move(entry));
    }

    snapshot.roots.push_back(std::move(root));
    snapshot_ = std::move(snapshot);
    NotifyObservers();
}

MetadataSnapshot MetadataModel::GetSnapshot() const {
    return snapshot_;
}

void MetadataModel::NotifyObservers() {
    for (auto* observer : observers_) {
        if (observer) {
            observer->OnMetadataUpdated(snapshot_);
        }
    }
}

} // namespace scratchrobin
