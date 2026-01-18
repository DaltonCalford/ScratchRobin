#include "core/connection_manager.h"
#include "core/job_queue.h"
#include "core/mock_backend.h"
#include "core/result_exporter.h"
#include "core/value_formatter.h"

#include <chrono>
#include <condition_variable>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>

namespace {

struct TestResult {
    int passed = 0;
    int failed = 0;
};

#define CHECK(cond) do { \
    if (!(cond)) { \
        std::ostringstream oss; \
        oss << "CHECK failed: " << #cond << " at " << __FILE__ << ":" << __LINE__; \
        error = oss.str(); \
        return false; \
    } \
} while (0)

bool TestJobQueueExecutes(std::string& error) {
    scratchrobin::JobQueue queue;
    std::mutex mutex;
    std::condition_variable cv;
    bool ran = false;

    queue.Submit([&](scratchrobin::JobHandle&) {
        std::lock_guard<std::mutex> lock(mutex);
        ran = true;
        cv.notify_one();
    });

    std::unique_lock<std::mutex> lock(mutex);
    bool signaled = cv.wait_for(lock, std::chrono::milliseconds(250), [&]() { return ran; });
    CHECK(signaled);
    return true;
}

bool TestJobQueueCancelCallback(std::string& error) {
    scratchrobin::JobQueue queue;
    std::atomic<bool> canceled{false};

    auto handle = queue.Submit([&](scratchrobin::JobHandle& job) {
        while (!job.IsCanceled()) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });
    handle.SetCancelCallback([&]() { canceled.store(true); });
    handle.Cancel();

    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    CHECK(handle.IsCanceled());
    CHECK(canceled.load());
    return true;
}

std::string FixturePath() {
    return std::string(SCRATCHROBIN_TEST_SOURCE_DIR) + "/tests/fixtures/mock_basic.json";
}

bool TestMockBackendExactMatch(std::string& error) {
    auto backend = scratchrobin::CreateMockBackend();
    scratchrobin::BackendConfig config;
    config.fixturePath = FixturePath();
    std::string backend_error;
    CHECK(backend->Connect(config, &backend_error));

    scratchrobin::QueryResult result;
    CHECK(backend->ExecuteQuery("select 1", &result, &backend_error));
    CHECK(result.columns.size() == 1);
    CHECK(result.rows.size() == 1);
    CHECK(result.rows[0].size() == 1);
    CHECK(result.rows[0][0].text == "1");
    CHECK(result.commandTag == "SELECT 1");
    return true;
}

bool TestMockBackendRegexMatch(std::string& error) {
    auto backend = scratchrobin::CreateMockBackend();
    scratchrobin::BackendConfig config;
    config.fixturePath = FixturePath();
    std::string backend_error;
    CHECK(backend->Connect(config, &backend_error));

    scratchrobin::QueryResult result;
    CHECK(backend->ExecuteQuery("SELECT id, name FROM demo", &result, &backend_error));
    CHECK(result.columns.size() == 2);
    CHECK(result.rows.size() == 2);
    CHECK(result.rows[1][1].text == "beta");
    return true;
}

bool TestMockBackendNoMatch(std::string& error) {
    auto backend = scratchrobin::CreateMockBackend();
    scratchrobin::BackendConfig config;
    config.fixturePath = FixturePath();
    std::string backend_error;
    CHECK(backend->Connect(config, &backend_error));

    scratchrobin::QueryResult result;
    bool ok = backend->ExecuteQuery("select nope", &result, &backend_error);
    CHECK(!ok);
    CHECK(!backend_error.empty());
    return true;
}

bool TestConnectionManagerMockSelection(std::string& error) {
    scratchrobin::ConnectionManager manager;
    scratchrobin::ConnectionProfile profile;
    profile.name = "Mock";
    profile.backend = "mock";
    profile.fixturePath = FixturePath();

    CHECK(manager.Connect(profile));

    auto caps = manager.Capabilities();
    CHECK(caps.supportsCancel);
    CHECK(caps.supportsTransactions);

    scratchrobin::QueryResult result;
    CHECK(manager.ExecuteQuery("select 1", &result));
    CHECK(result.rows.size() == 1);
    return true;
}

bool TestValueFormatterUuid(std::string& error) {
    scratchrobin::QueryValue value;
    value.isNull = false;
    value.raw = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77,
                 0x88, 0x99, 0xaa, 0xbb, 0xcc, 0xdd, 0xee, 0xff};

    std::string formatted = scratchrobin::FormatValueForDisplay(value, "UUID");
    CHECK(formatted == "00112233-4455-6677-8899-aabbccddeeff");
    return true;
}

bool TestValueFormatterBinary(std::string& error) {
    scratchrobin::QueryValue value;
    value.isNull = false;
    value.raw = {0x01, 0x02};

    std::string formatted = scratchrobin::FormatValueForDisplay(value, "BYTEA");
    CHECK(formatted == "0x0102 (2 bytes)");
    return true;
}

bool TestResultExporterCsv(std::string& error) {
    scratchrobin::QueryResult result;
    result.columns = {{"id", "INT32"}, {"payload", "JSON"}};
    scratchrobin::QueryValue v1;
    v1.isNull = false;
    v1.text = "1";
    scratchrobin::QueryValue v2;
    v2.isNull = false;
    v2.text = "{\"a\":1}";
    result.rows = {{v1, v2}};

    std::string path = "/tmp/scratchrobin_export_test.csv";
    scratchrobin::ExportOptions options;
    options.includeHeaders = true;
    options.maxBinaryBytes = 0;
    options.includeBinarySize = false;

    CHECK(scratchrobin::ExportResultToCsv(result, path, &error, options));

    std::ifstream in(path);
    CHECK(in.is_open());
    std::ostringstream buffer;
    buffer << in.rdbuf();
    std::string contents = buffer.str();
    CHECK(contents.find("id,payload") != std::string::npos);
    CHECK(contents.find("1") != std::string::npos);
    return true;
}

bool TestResultExporterJson(std::string& error) {
    scratchrobin::QueryResult result;
    result.columns = {{"id", "INT32"}};
    scratchrobin::QueryValue v1;
    v1.isNull = false;
    v1.text = "1";
    result.rows = {{v1}};

    std::string path = "/tmp/scratchrobin_export_test.json";
    scratchrobin::ExportOptions options;
    options.includeHeaders = true;
    options.maxBinaryBytes = 0;
    options.includeBinarySize = false;

    CHECK(scratchrobin::ExportResultToJson(result, path, &error, options));

    std::ifstream in(path);
    CHECK(in.is_open());
    std::ostringstream buffer;
    buffer << in.rdbuf();
    std::string contents = buffer.str();
    CHECK(contents.find("\"columns\"") != std::string::npos);
    CHECK(contents.find("\"rows\"") != std::string::npos);
    return true;
}

struct TestCase {
    const char* name;
    bool (*fn)(std::string&);
};

} // namespace

int main() {
    TestCase tests[] = {
        {"JobQueue executes", TestJobQueueExecutes},
        {"JobQueue cancel callback", TestJobQueueCancelCallback},
        {"MockBackend exact match", TestMockBackendExactMatch},
        {"MockBackend regex match", TestMockBackendRegexMatch},
        {"MockBackend no match", TestMockBackendNoMatch},
        {"ConnectionManager mock selection", TestConnectionManagerMockSelection},
        {"Value formatter UUID", TestValueFormatterUuid},
        {"Value formatter binary", TestValueFormatterBinary},
        {"Result exporter CSV", TestResultExporterCsv},
        {"Result exporter JSON", TestResultExporterJson}
    };

    TestResult result;

    for (const auto& test : tests) {
        std::string error;
        bool ok = test.fn(error);
        if (ok) {
            ++result.passed;
            std::cout << "[PASS] " << test.name << "\n";
        } else {
            ++result.failed;
            std::cout << "[FAIL] " << test.name << "\n";
            if (!error.empty()) {
                std::cout << "       " << error << "\n";
            }
        }
    }

    std::cout << "\n" << result.passed << " passed, " << result.failed << " failed.\n";

    return result.failed == 0 ? 0 : 1;
}
