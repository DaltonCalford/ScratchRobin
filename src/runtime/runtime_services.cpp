#include "runtime/runtime_services.h"

#include <filesystem>
#include <fstream>
#include <sstream>

#include "core/beta1b_contracts.h"
#include "core/reject.h"

namespace scratchrobin::runtime {

void WindowManager::OpenWindow(const std::string& window_id) {
    if (!window_id.empty()) {
        open_windows_.insert(window_id);
    }
}

void WindowManager::CloseAll() {
    open_windows_.clear();
}

std::size_t WindowManager::OpenWindowCount() const {
    return open_windows_.size();
}

bool MetadataModel::LoadFixture(const std::string& fixture_path) {
    std::ifstream in(fixture_path, std::ios::binary);
    if (!in) {
        return false;
    }

    objects_.clear();
    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty()) {
            objects_.push_back(line);
        }
    }
    mode_ = LoadMode::Fixture;
    return true;
}

void MetadataModel::LoadStub() {
    objects_.clear();
    objects_.push_back("stub.catalog");
    objects_.push_back("stub.schema");
    mode_ = LoadMode::Stub;
}

bool MetadataModel::Bootstrap(bool use_fixture,
                              const std::string& fixture_path,
                              std::vector<std::string>* warnings) {
    if (use_fixture && !fixture_path.empty()) {
        if (LoadFixture(fixture_path)) {
            return true;
        }
        if (warnings != nullptr) {
            warnings->push_back("META-1001: metadata fixture load failed, using stub snapshot");
        }
    }
    LoadStub();
    return true;
}

void JobQueue::Start() {
    running_ = true;
}

void JobQueue::Stop() {
    running_ = false;
}

void ConnectionManager::SetProfiles(std::vector<ConnectionProfile> profiles) {
    profiles_ = std::move(profiles);
}

void ConnectionManager::SetNetworkOptions(int connect_timeout_ms,
                                          int query_timeout_ms,
                                          int read_timeout_ms,
                                          int write_timeout_ms) {
    connect_timeout_ms_ = connect_timeout_ms;
    query_timeout_ms_ = query_timeout_ms;
    read_timeout_ms_ = read_timeout_ms;
    write_timeout_ms_ = write_timeout_ms;
}

void ConnectionManager::SetAvailableBackends(std::set<std::string> available_backends) {
    available_backends_ = std::move(available_backends);
}

bool ConnectionManager::IsProfileBackendAvailable(const ConnectionProfile& profile,
                                                  std::string* normalized_backend) const {
    beta1b::ConnectionProfile contract_profile;
    contract_profile.backend = profile.backend;
    switch (profile.mode) {
        case ConnectionMode::Ipc: contract_profile.mode = beta1b::ConnectionMode::Ipc; break;
        case ConnectionMode::Embedded: contract_profile.mode = beta1b::ConnectionMode::Embedded; break;
        case ConnectionMode::Network:
        default: contract_profile.mode = beta1b::ConnectionMode::Network; break;
    }

    const std::string backend = beta1b::SelectBackend(contract_profile);
    if (normalized_backend != nullptr) {
        *normalized_backend = backend;
    }
    return available_backends_.count(backend) > 0U;
}

void ConnectionManager::EvaluateBackendAvailability(bool mandatory_backends,
                                                    std::vector<std::string>* warnings) const {
    for (const auto& profile : profiles_) {
        std::string backend;
        const bool available = IsProfileBackendAvailable(profile, &backend);
        if (available) {
            continue;
        }

        if (mandatory_backends) {
            throw MakeReject("SRB1-R-9001",
                             "required build dependency/profile mismatch",
                             "runtime",
                             "evaluate_backend_availability",
                             false,
                             profile.name + ":" + backend);
        }
        if (warnings != nullptr) {
            warnings->push_back("BACKEND-1001: backend unavailable for profile " + profile.name + " (" + backend + ")");
        }
    }
}

void ConnectionManager::DisconnectAll() {
    (void)connect_timeout_ms_;
    (void)query_timeout_ms_;
    (void)read_timeout_ms_;
    (void)write_timeout_ms_;
}

std::size_t ConnectionManager::unavailable_backend_count() const {
    std::size_t missing = 0;
    for (const auto& profile : profiles_) {
        std::string backend;
        if (!IsProfileBackendAvailable(profile, &backend)) {
            ++missing;
        }
    }
    return missing;
}

std::set<std::string> ScratchRobinRuntime::DefaultAvailableBackends() {
    return {"network", "postgresql", "mysql", "firebird", "embedded", "ipc", "mock"};
}

std::string ScratchRobinRuntime::MetadataModeString(MetadataModel::LoadMode mode) {
    switch (mode) {
        case MetadataModel::LoadMode::Fixture: return "fixture";
        case MetadataModel::LoadMode::Stub: return "stub";
        case MetadataModel::LoadMode::Empty:
        default: return "empty";
    }
}

ScratchRobinRuntime::ScratchRobinRuntime(std::set<std::string> available_backends) {
    if (available_backends.empty()) {
        available_backends = DefaultAvailableBackends();
    }
    connection_manager_.SetAvailableBackends(std::move(available_backends));
}

StartupReport ScratchRobinRuntime::Startup(const StartupPaths& paths) {
    if (started_) {
        throw MakeReject("SRB1-R-5101", "runtime already started", "runtime", "startup");
    }

    StartupReport report;
    job_queue_.Start();

    const ConfigBundle config = config_store_.LoadWithFallback(paths.app_config_path,
                                                               paths.app_config_example_path,
                                                               paths.connections_path,
                                                               paths.connections_example_path);

    std::vector<std::string> warnings;
    warnings.reserve(config.warnings.size() + 4);
    for (const auto& warning : config.warnings) {
        warnings.push_back(warning.code + ": " + warning.message);
    }

    connection_manager_.SetNetworkOptions(config.app.connect_timeout_ms,
                                          config.app.query_timeout_ms,
                                          config.app.read_timeout_ms,
                                          config.app.write_timeout_ms);
    connection_manager_.SetProfiles(config.connections);
    connection_manager_.EvaluateBackendAvailability(config.app.mandatory_backends, &warnings);

    metadata_model_.Bootstrap(config.app.metadata_use_fixture, config.app.metadata_fixture_path, &warnings);

    if (config.app.startup_enabled) {
        report.splash_started = true;
    }

    window_manager_.OpenWindow("MainFrame");
    report.main_frame_visible = window_manager_.OpenWindowCount() > 0U;
    if (report.splash_started) {
        report.splash_hidden = true;
    }

    report.ok = true;
    report.config_source = ToString(config.source);
    report.connection_profile_count = config.connections.size();
    report.unavailable_backend_count = connection_manager_.unavailable_backend_count();
    report.metadata_mode = MetadataModeString(metadata_model_.mode());
    report.warnings = std::move(warnings);

    started_ = true;
    return report;
}

void ScratchRobinRuntime::Shutdown(const StartupPaths& paths) {
    if (!started_) {
        return;
    }

    window_manager_.CloseAll();

    if (!paths.session_state_path.empty()) {
        const std::filesystem::path session_path(paths.session_state_path);
        std::error_code ec;
        std::filesystem::create_directories(session_path.parent_path(), ec);
        std::ofstream out(session_path, std::ios::binary);
        if (out) {
            out << "{\"session\":\"closed\",\"open_windows\":0}\n";
        }
    }

    job_queue_.Stop();
    connection_manager_.DisconnectAll();
    started_ = false;
}

}  // namespace scratchrobin::runtime

