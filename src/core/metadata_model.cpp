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
    root.kind = "root";

    MetadataNode local;
    local.label = "Local ScratchBird";
    local.kind = "connection";
    local.children.push_back(MetadataNode{"Host: 127.0.0.1:3050", "host", "", {}, {}});
    local.children.push_back(MetadataNode{"Database: /data/scratchbird/demo.sdb", "database", "", {}, {}});

    MetadataNode schema;
    schema.label = "Schema: public";
    schema.kind = "schema";

    MetadataNode table;
    table.label = "Table: demo";
    table.kind = "table";
    table.ddl = "CREATE TABLE public.demo (\n"
                "    id BIGINT PRIMARY KEY,\n"
                "    name VARCHAR(64) NOT NULL,\n"
                "    created_at TIMESTAMPTZ DEFAULT now()\n"
                ");";
    table.dependencies = {
        "Index: demo_pkey",
        "Sequence: demo_id_seq"
    };
    schema.children.push_back(std::move(table));
    local.children.push_back(std::move(schema));
    root.children.push_back(std::move(local));

    snapshot.roots.push_back(std::move(root));
    snapshot_ = std::move(snapshot);
    NotifyObservers();
}

void MetadataModel::UpdateConnections(const std::vector<ConnectionProfile>& profiles) {
    MetadataSnapshot snapshot;
    MetadataNode root;
    root.label = "Connections";
    root.kind = "root";

    for (const auto& profile : profiles) {
        MetadataNode entry;
        entry.label = profile.name.empty() ? profile.database : profile.name;
        entry.kind = "connection";
        if (!profile.host.empty() || profile.port != 0) {
            std::string host_label = "Host: " + (profile.host.empty() ? "localhost" : profile.host);
            if (profile.port != 0) {
                host_label += ":" + std::to_string(profile.port);
            }
            entry.children.push_back(MetadataNode{host_label, "host", "", {}, {}});
        }
        if (!profile.database.empty()) {
            entry.children.push_back(MetadataNode{"Database: " + profile.database, "database", "", {}, {}});
        }
        if (!profile.username.empty()) {
            entry.children.push_back(MetadataNode{"User: " + profile.username, "user", "", {}, {}});
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
