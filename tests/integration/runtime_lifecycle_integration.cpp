#include "core/reject.h"
#include "runtime/runtime_services.h"
#include "tests/test_harness.h"

#include <filesystem>
#include <fstream>
#include <string>

using namespace scratchrobin;
using namespace scratchrobin::runtime;

namespace {

void WriteText(const std::filesystem::path& path, const std::string& text) {
    std::filesystem::create_directories(path.parent_path());
    std::ofstream out(path, std::ios::binary);
    out << text;
}

void ExpectReject(const std::string& code, const std::function<void()>& fn) {
    try {
        fn();
    } catch (const RejectError& ex) {
        scratchrobin::tests::AssertEq(ex.payload().code, code, "unexpected reject code");
        return;
    }
    throw std::runtime_error("expected reject was not thrown");
}

}  // namespace

int main() {
    std::vector<std::pair<std::string, scratchrobin::tests::TestFn>> tests;

    tests.push_back({"integration/runtime_startup_example_fallback", [] {
                        namespace fs = std::filesystem;
                        const fs::path temp = fs::temp_directory_path() / "scratchrobin_runtime_it";
                        fs::remove_all(temp);

                        WriteText(temp / "config/scratchrobin.toml.example",
                                  "[startup]\n"
                                  "enabled = true\n"
                                  "show_progress = true\n\n"
                                  "[network]\n"
                                  "connect_timeout_ms = 3000\n"
                                  "query_timeout_ms = 0\n"
                                  "read_timeout_ms = 1000\n"
                                  "write_timeout_ms = 1000\n\n"
                                  "[metadata]\n"
                                  "use_fixture = false\n"
                                  "fixture_path = \"\"\n\n"
                                  "[runtime]\n"
                                  "mandatory_backends = false\n");

                        WriteText(temp / "config/connections.toml.example",
                                  "[[connection]]\n"
                                  "name = \"local\"\n"
                                  "backend = \"scratchbird\"\n"
                                  "mode = \"network\"\n"
                                  "host = \"127.0.0.1\"\n"
                                  "port = 3092\n"
                                  "database = \"scratchbird\"\n"
                                  "username = \"sysdba\"\n"
                                  "credential_id = \"default\"\n");

                        ScratchRobinRuntime runtime;
                        StartupPaths paths;
                        paths.app_config_path = (temp / "user/scratchrobin.toml").string();
                        paths.app_config_example_path = (temp / "config/scratchrobin.toml.example").string();
                        paths.connections_path = (temp / "user/connections.toml").string();
                        paths.connections_example_path = (temp / "config/connections.toml.example").string();
                        paths.session_state_path = (temp / "work/session_state.json").string();

                        const auto report = runtime.Startup(paths);
                        scratchrobin::tests::AssertTrue(report.ok, "startup expected to succeed");
                        scratchrobin::tests::AssertEq(report.config_source, "example_fallback", "wrong config source");
                        scratchrobin::tests::AssertTrue(report.connection_profile_count == 1U, "expected one connection");
                        scratchrobin::tests::AssertTrue(report.main_frame_visible, "main frame should be visible");
                        scratchrobin::tests::AssertTrue(runtime.open_window_count() == 1U, "window manager not initialized");
                        scratchrobin::tests::AssertTrue(runtime.job_queue_running(), "job queue should be running");

                        runtime.Shutdown(paths);
                        scratchrobin::tests::AssertTrue(runtime.open_window_count() == 0U, "windows should be closed");
                        scratchrobin::tests::AssertTrue(!runtime.job_queue_running(), "job queue should be stopped");
                        scratchrobin::tests::AssertTrue(fs::exists(temp / "work/session_state.json"),
                                                        "session state should be persisted");

                        fs::remove_all(temp);
                    }});

    tests.push_back({"integration/runtime_mandatory_backend_reject", [] {
                        namespace fs = std::filesystem;
                        const fs::path temp = fs::temp_directory_path() / "scratchrobin_runtime_mandatory_it";
                        fs::remove_all(temp);

                        WriteText(temp / "config/scratchrobin.toml.example",
                                  "[startup]\n"
                                  "enabled = true\n"
                                  "show_progress = true\n\n"
                                  "[metadata]\n"
                                  "use_fixture = false\n"
                                  "fixture_path = \"\"\n\n"
                                  "[runtime]\n"
                                  "mandatory_backends = true\n");

                        WriteText(temp / "config/connections.toml.example",
                                  "[[connection]]\n"
                                  "name = \"pg\"\n"
                                  "backend = \"postgresql\"\n"
                                  "mode = \"network\"\n"
                                  "host = \"127.0.0.1\"\n"
                                  "port = 5432\n"
                                  "database = \"postgres\"\n"
                                  "username = \"postgres\"\n"
                                  "credential_id = \"default\"\n");

                        ScratchRobinRuntime runtime({"network"});
                        StartupPaths paths;
                        paths.app_config_path = (temp / "user/scratchrobin.toml").string();
                        paths.app_config_example_path = (temp / "config/scratchrobin.toml.example").string();
                        paths.connections_path = (temp / "user/connections.toml").string();
                        paths.connections_example_path = (temp / "config/connections.toml.example").string();
                        paths.session_state_path = (temp / "work/session_state.json").string();

                        ExpectReject("SRB1-R-9001", [&] { (void)runtime.Startup(paths); });
                        fs::remove_all(temp);
                    }});

    return scratchrobin::tests::RunTests(tests);
}

