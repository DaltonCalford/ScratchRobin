#pragma once

#include <set>
#include <string>
#include <vector>

#include "runtime/runtime_config.h"

namespace scratchrobin::runtime {

struct StartupPaths {
    std::string app_config_path;
    std::string app_config_example_path;
    std::string connections_path;
    std::string connections_example_path;
    std::string session_state_path;
};

struct StartupReport {
    bool ok = false;
    std::string config_source;
    std::size_t connection_profile_count = 0;
    std::size_t unavailable_backend_count = 0;
    std::string metadata_mode;
    bool splash_started = false;
    bool splash_hidden = false;
    bool main_frame_visible = false;
    std::vector<std::string> warnings;
};

class WindowManager {
public:
    void OpenWindow(const std::string& window_id);
    void CloseAll();
    std::size_t OpenWindowCount() const;

private:
    std::set<std::string> open_windows_;
};

class MetadataModel {
public:
    enum class LoadMode {
        Empty,
        Stub,
        Fixture,
    };

    bool Bootstrap(bool use_fixture,
                   const std::string& fixture_path,
                   std::vector<std::string>* warnings);

    LoadMode mode() const { return mode_; }
    std::size_t object_count() const { return objects_.size(); }

private:
    void LoadStub();
    bool LoadFixture(const std::string& fixture_path);

    LoadMode mode_ = LoadMode::Empty;
    std::vector<std::string> objects_;
};

class JobQueue {
public:
    void Start();
    void Stop();
    bool running() const { return running_; }

private:
    bool running_ = false;
};

class ConnectionManager {
public:
    void SetProfiles(std::vector<ConnectionProfile> profiles);
    void SetNetworkOptions(int connect_timeout_ms,
                           int query_timeout_ms,
                           int read_timeout_ms,
                           int write_timeout_ms);
    void SetAvailableBackends(std::set<std::string> available_backends);

    void EvaluateBackendAvailability(bool mandatory_backends,
                                     std::vector<std::string>* warnings) const;
    void DisconnectAll();

    std::size_t unavailable_backend_count() const;
    const std::vector<ConnectionProfile>& profiles() const { return profiles_; }

private:
    bool IsProfileBackendAvailable(const ConnectionProfile& profile, std::string* normalized_backend) const;

    std::vector<ConnectionProfile> profiles_;
    std::set<std::string> available_backends_;
    int connect_timeout_ms_ = 5000;
    int query_timeout_ms_ = 0;
    int read_timeout_ms_ = 30000;
    int write_timeout_ms_ = 30000;
};

class ScratchRobinRuntime {
public:
    explicit ScratchRobinRuntime(std::set<std::string> available_backends = {});

    StartupReport Startup(const StartupPaths& paths);
    void Shutdown(const StartupPaths& paths);

    bool started() const { return started_; }
    std::size_t open_window_count() const { return window_manager_.OpenWindowCount(); }
    bool job_queue_running() const { return job_queue_.running(); }

private:
    static std::set<std::string> DefaultAvailableBackends();
    static std::string MetadataModeString(MetadataModel::LoadMode mode);

    bool started_ = false;
    ConfigStore config_store_;
    WindowManager window_manager_;
    MetadataModel metadata_model_;
    ConnectionManager connection_manager_;
    JobQueue job_queue_;
};

}  // namespace scratchrobin::runtime

