#include "core/connection_manager.h"
#include "core/credentials.h"
#include "core/job_queue.h"
#include "core/metadata_model.h"
#include "core/mock_backend.h"
#include "core/result_exporter.h"
#include "core/value_formatter.h"

#include <chrono>
#include <condition_variable>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iostream>
#include <map>
#include <mutex>
#include <sstream>
#include <set>
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

std::string MetadataFixturePath() {
    return std::string(SCRATCHROBIN_TEST_SOURCE_DIR) + "/tests/fixtures/metadata_complex.json";
}

std::string MetadataInvalidFixturePath() {
    return std::string(SCRATCHROBIN_TEST_SOURCE_DIR) + "/tests/fixtures/metadata_invalid.json";
}

std::string MetadataMultiCatalogFixturePath() {
    return std::string(SCRATCHROBIN_TEST_SOURCE_DIR) + "/tests/fixtures/metadata_multicatalog.json";
}

bool ParseKeyValueDsn(const std::string& dsn,
                      std::map<std::string, std::string>* out,
                      std::string* error) {
    if (!out) {
        return false;
    }
    out->clear();
    size_t i = 0;
    auto skip_separators = [&](size_t& idx) {
        while (idx < dsn.size()) {
            char c = dsn[idx];
            if (std::isspace(static_cast<unsigned char>(c)) || c == ';') {
                ++idx;
            } else {
                break;
            }
        }
    };

    skip_separators(i);
    if (dsn.rfind("postgres://", 0) == 0 || dsn.rfind("postgresql://", 0) == 0) {
        if (error) {
            *error = "URI DSN not supported in tests; use key=value format";
        }
        return false;
    }

    while (i < dsn.size()) {
        skip_separators(i);
        if (i >= dsn.size()) {
            break;
        }
        size_t key_start = i;
        while (i < dsn.size() && dsn[i] != '=' && !std::isspace(static_cast<unsigned char>(dsn[i]))) {
            ++i;
        }
        if (i >= dsn.size() || dsn[i] != '=') {
            if (error) {
                *error = "Invalid DSN segment: expected key=value";
            }
            return false;
        }
        std::string key = dsn.substr(key_start, i - key_start);
        ++i; // skip '='
        if (i >= dsn.size()) {
            if (error) {
                *error = "Invalid DSN segment: missing value";
            }
            return false;
        }
        std::string value;
        if (dsn[i] == '\'' || dsn[i] == '"') {
            char quote = dsn[i++];
            while (i < dsn.size() && dsn[i] != quote) {
                if (dsn[i] == '\\' && i + 1 < dsn.size()) {
                    ++i;
                }
                value.push_back(dsn[i++]);
            }
            if (i < dsn.size() && dsn[i] == quote) {
                ++i;
            }
        } else {
            while (i < dsn.size() &&
                   !std::isspace(static_cast<unsigned char>(dsn[i])) &&
                   dsn[i] != ';') {
                value.push_back(dsn[i++]);
            }
        }
        if (!key.empty()) {
            (*out)[key] = value;
        }
        skip_separators(i);
    }
    return !out->empty();
}

bool PopulateProfileFromDsn(const std::string& backend,
                            const std::map<std::string, std::string>& dsn,
                            scratchrobin::ConnectionProfile* profile,
                            std::string* password,
                            std::string* error) {
    if (!profile) {
        if (error) {
            *error = "Missing profile";
        }
        return false;
    }
    profile->backend = backend;
    if (auto it = dsn.find("host"); it != dsn.end()) {
        profile->host = it->second;
    } else if (auto it2 = dsn.find("hostname"); it2 != dsn.end()) {
        profile->host = it2->second;
    }
    if (auto it = dsn.find("port"); it != dsn.end()) {
        profile->port = std::stoi(it->second);
    }
    if (auto it = dsn.find("dbname"); it != dsn.end()) {
        profile->database = it->second;
    } else if (auto it2 = dsn.find("database"); it2 != dsn.end()) {
        profile->database = it2->second;
    }
    if (auto it = dsn.find("user"); it != dsn.end()) {
        profile->username = it->second;
    } else if (auto it2 = dsn.find("username"); it2 != dsn.end()) {
        profile->username = it2->second;
    }
    if (auto it = dsn.find("sslmode"); it != dsn.end()) {
        profile->sslMode = it->second;
    } else if (auto it2 = dsn.find("ssl_mode"); it2 != dsn.end()) {
        profile->sslMode = it2->second;
    }

    if (auto it = dsn.find("password_env"); it != dsn.end() && !it->second.empty()) {
        profile->credentialId = "env:" + it->second;
    } else if (auto it2 = dsn.find("password"); it2 != dsn.end()) {
        if (password) {
            *password = it2->second;
        }
        profile->credentialId = "inline";
    }

    return true;
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

bool TestMetadataModelFixture(std::string& error) {
    scratchrobin::MetadataModel model;
    std::string load_error;
    CHECK(model.LoadFromFixture(FixturePath(), &load_error));

    const auto& snapshot = model.GetSnapshot();
    CHECK(!snapshot.roots.empty());

    bool found_table = false;
    bool found_ddl = false;
    bool found_deps = false;

    std::function<void(const scratchrobin::MetadataNode&)> visit;
    visit = [&](const scratchrobin::MetadataNode& node) {
        if (node.label == "demo" && node.kind == "table") {
            found_table = true;
            if (!node.ddl.empty()) {
                found_ddl = true;
            }
            if (!node.dependencies.empty()) {
                found_deps = true;
            }
        }
        for (const auto& child : node.children) {
            visit(child);
        }
    };

    for (const auto& root : snapshot.roots) {
        visit(root);
    }

    CHECK(found_table);
    CHECK(found_ddl);
    CHECK(found_deps);
    return true;
}

bool TestMetadataModelComplexFixture(std::string& error) {
    scratchrobin::MetadataModel model;
    std::string load_error;
    CHECK(model.LoadFromFixture(MetadataFixturePath(), &load_error));

    const auto& snapshot = model.GetSnapshot();
    CHECK(!snapshot.roots.empty());

    const scratchrobin::MetadataNode* schema_public = nullptr;
    const scratchrobin::MetadataNode* table_orders = nullptr;
    const scratchrobin::MetadataNode* column_order_id = nullptr;
    const scratchrobin::MetadataNode* functions_folder = nullptr;
    const scratchrobin::MetadataNode* loose_node = nullptr;

    std::function<void(const scratchrobin::MetadataNode&)> visit;
    visit = [&](const scratchrobin::MetadataNode& node) {
        if (!schema_public && node.label == "public" && node.kind == "schema") {
            schema_public = &node;
        }
        if (!table_orders && node.label == "orders" && node.kind == "table") {
            table_orders = &node;
        }
        if (!column_order_id && node.label == "order_id" && node.kind == "column") {
            column_order_id = &node;
        }
        if (!functions_folder && node.label == "functions" && node.kind == "folder") {
            functions_folder = &node;
        }
        if (!loose_node && node.label == "loose" && node.kind == "note") {
            loose_node = &node;
        }
        for (const auto& child : node.children) {
            visit(child);
        }
    };

    for (const auto& root : snapshot.roots) {
        visit(root);
    }

    CHECK(schema_public);
    CHECK(schema_public->path == "native.public");
    CHECK(table_orders);
    CHECK(!table_orders->ddl.empty());
    CHECK(!table_orders->dependencies.empty());
    CHECK(column_order_id);
    CHECK(column_order_id->path == "native.public.orders.order_id");
    CHECK(functions_folder);
    CHECK(loose_node);

    return true;
}

bool TestMetadataModelInvalidFixture(std::string& error) {
    scratchrobin::MetadataModel model;
    std::string load_error;
    bool ok = model.LoadFromFixture(MetadataInvalidFixturePath(), &load_error);
    CHECK(!ok);
    CHECK(!load_error.empty());
    return true;
}

bool TestMetadataModelMultiCatalogFixture(std::string& error) {
    scratchrobin::MetadataModel model;
    std::string load_error;
    CHECK(model.LoadFromFixture(MetadataMultiCatalogFixturePath(), &load_error));

    const auto& snapshot = model.GetSnapshot();
    CHECK(!snapshot.roots.empty());

    bool found_native_schema = false;
    bool found_firebird_schema = false;
    bool found_postgres_schema = false;
    bool found_postgres_catalog_schema = false;
    bool found_mysql_schema = false;
    bool found_native_table = false;
    bool found_firebird_table = false;
    bool found_postgres_table = false;
    bool found_mysql_table = false;
    std::set<std::string> catalogs;

    std::function<void(const scratchrobin::MetadataNode&)> visit;
    visit = [&](const scratchrobin::MetadataNode& node) {
        if (!node.catalog.empty()) {
            catalogs.insert(node.catalog);
        }
        if (node.kind == "schema" && node.path == "native.public") {
            found_native_schema = true;
        }
        if (node.kind == "schema" && node.path == "firebird.public") {
            found_firebird_schema = true;
        }
        if (node.kind == "schema" && node.path == "postgresql.public") {
            found_postgres_schema = true;
        }
        if (node.kind == "schema" && node.path == "postgresql.pg_catalog") {
            found_postgres_catalog_schema = true;
        }
        if (node.kind == "schema" && node.path == "mysql.system") {
            found_mysql_schema = true;
        }
        if (node.kind == "table" && node.path == "native.public.customers") {
            found_native_table = true;
        }
        if (node.kind == "table" && node.path == "firebird.public.rdb$database") {
            found_firebird_table = true;
        }
        if (node.kind == "table" && node.path == "postgresql.pg_catalog.pg_class") {
            found_postgres_table = true;
        }
        if (node.kind == "table" && node.path == "mysql.system.user") {
            found_mysql_table = true;
        }
        for (const auto& child : node.children) {
            visit(child);
        }
    };

    for (const auto& root : snapshot.roots) {
        visit(root);
    }

    CHECK(found_native_schema);
    CHECK(found_firebird_schema);
    CHECK(found_postgres_schema);
    CHECK(found_postgres_catalog_schema);
    CHECK(found_mysql_schema);
    CHECK(found_native_table);
    CHECK(found_firebird_table);
    CHECK(found_postgres_table);
    CHECK(found_mysql_table);
    CHECK(catalogs.count("native") == 1);
    CHECK(catalogs.count("firebird") == 1);
    CHECK(catalogs.count("postgresql") == 1);
    CHECK(catalogs.count("mysql") == 1);
    return true;
}

class InlineCredentialStore : public scratchrobin::CredentialStore {
public:
    explicit InlineCredentialStore(std::string password) : password_(std::move(password)) {}

    bool ResolvePassword(const std::string&,
                         std::string* outPassword,
                         std::string* outError) override {
        if (!outPassword) {
            if (outError) {
                *outError = "No output buffer for password";
            }
            return false;
        }
        *outPassword = password_;
        return true;
    }

private:
    std::string password_;
};

bool TestPostgresCancelIntegration(std::string& error) {
    const char* dsn_env = std::getenv("SCRATCHROBIN_TEST_PG_DSN");
    if (!dsn_env || std::string(dsn_env).empty()) {
        return true;
    }

    std::map<std::string, std::string> dsn;
    if (!ParseKeyValueDsn(dsn_env, &dsn, &error)) {
        return false;
    }

    scratchrobin::ConnectionProfile profile;
    std::string password;
    if (!PopulateProfileFromDsn("postgresql", dsn, &profile, &password, &error)) {
        return false;
    }

    std::unique_ptr<scratchrobin::CredentialStore> store;
    if (!password.empty()) {
        store = std::make_unique<InlineCredentialStore>(password);
    }

    scratchrobin::ConnectionManager manager(std::move(store));
    if (!manager.Connect(profile)) {
        error = manager.LastError().empty() ? "Postgres connect failed" : manager.LastError();
        return false;
    }

    std::mutex mutex;
    std::condition_variable cv;
    bool done = false;
    bool ok = false;
    std::string query_error;

    auto start = std::chrono::steady_clock::now();
    auto handle = manager.ExecuteQueryAsync("SELECT pg_sleep(30);",
        [&](bool query_ok, scratchrobin::QueryResult, const std::string& err) {
            std::lock_guard<std::mutex> lock(mutex);
            ok = query_ok;
            query_error = err;
            done = true;
            cv.notify_one();
        });

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    auto cancel_start = std::chrono::steady_clock::now();
    handle.Cancel();

    std::unique_lock<std::mutex> lock(mutex);
    bool finished = cv.wait_for(lock, std::chrono::seconds(5), [&]() { return done; });
    if (!finished) {
        error = "Cancel did not return within 5 seconds";
        return false;
    }

    auto elapsed = std::chrono::duration<double, std::milli>(
        std::chrono::steady_clock::now() - cancel_start).count();
    if (elapsed > 5000.0) {
        error = "Cancel exceeded 5 seconds";
        return false;
    }
    if (ok) {
        error = "Query completed successfully; expected cancel";
        return false;
    }
    if (query_error.empty()) {
        error = "Cancel returned without error details";
        return false;
    }

    manager.Disconnect();
    (void)start;
    return true;
}

bool TestMySqlIntegration(std::string& error) {
    const char* dsn_env = std::getenv("SCRATCHROBIN_TEST_MYSQL_DSN");
    if (!dsn_env || std::string(dsn_env).empty()) {
        return true;
    }

    std::map<std::string, std::string> dsn;
    if (!ParseKeyValueDsn(dsn_env, &dsn, &error)) {
        return false;
    }

    scratchrobin::ConnectionProfile profile;
    std::string password;
    if (!PopulateProfileFromDsn("mysql", dsn, &profile, &password, &error)) {
        return false;
    }

    std::unique_ptr<scratchrobin::CredentialStore> store;
    if (!password.empty()) {
        store = std::make_unique<InlineCredentialStore>(password);
    }

    scratchrobin::ConnectionManager manager(std::move(store));
    if (!manager.Connect(profile)) {
        error = manager.LastError().empty() ? "MySQL connect failed" : manager.LastError();
        return false;
    }

    scratchrobin::QueryResult result;
    if (!manager.ExecuteQuery("SELECT 1", &result)) {
        error = manager.LastError().empty() ? "MySQL query failed" : manager.LastError();
        return false;
    }

    CHECK(result.rows.size() == 1);
    CHECK(result.rows[0].size() == 1);
    CHECK(result.rows[0][0].text == "1");
    manager.Disconnect();
    return true;
}

bool TestFirebirdIntegration(std::string& error) {
    const char* dsn_env = std::getenv("SCRATCHROBIN_TEST_FB_DSN");
    if (!dsn_env || std::string(dsn_env).empty()) {
        return true;
    }

    std::map<std::string, std::string> dsn;
    if (!ParseKeyValueDsn(dsn_env, &dsn, &error)) {
        return false;
    }

    scratchrobin::ConnectionProfile profile;
    std::string password;
    if (!PopulateProfileFromDsn("firebird", dsn, &profile, &password, &error)) {
        return false;
    }

    std::unique_ptr<scratchrobin::CredentialStore> store;
    if (!password.empty()) {
        store = std::make_unique<InlineCredentialStore>(password);
    }

    scratchrobin::ConnectionManager manager(std::move(store));
    if (!manager.Connect(profile)) {
        error = manager.LastError().empty() ? "Firebird connect failed" : manager.LastError();
        return false;
    }

    scratchrobin::QueryResult result;
    if (!manager.ExecuteQuery("SELECT 1 FROM RDB$DATABASE", &result)) {
        error = manager.LastError().empty() ? "Firebird query failed" : manager.LastError();
        return false;
    }

    CHECK(result.rows.size() == 1);
    CHECK(result.rows[0].size() == 1);
    CHECK(result.rows[0][0].text == "1");
    manager.Disconnect();
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
        {"Metadata model fixture", TestMetadataModelFixture},
        {"Metadata model complex fixture", TestMetadataModelComplexFixture},
        {"Metadata model invalid fixture", TestMetadataModelInvalidFixture},
        {"Metadata model multi-catalog fixture", TestMetadataModelMultiCatalogFixture},
        {"Postgres cancel integration", TestPostgresCancelIntegration},
        {"MySQL integration", TestMySqlIntegration},
        {"Firebird integration", TestFirebirdIntegration},
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
