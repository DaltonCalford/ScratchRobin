/**
 * @file backend_test_harness.h
 * @brief Test harness for backend adapter integration testing
 * 
 * This harness provides a framework for testing backend adapters against
 * real database instances. Tests are gated via environment variables.
 * 
 * Environment Variables:
 *   - SCRATCHROBIN_TEST_PG_DSN: PostgreSQL test connection string
 *   - SCRATCHROBIN_TEST_MYSQL_DSN: MySQL test connection string  
 *   - SCRATCHROBIN_TEST_FB_DSN: Firebird test connection string
 *   - SCRATCHROBIN_TEST_TIMEOUT: Query timeout in seconds (default: 30)
 * 
 * Example usage:
 *   SCRATCHROBIN_TEST_PG_DSN="host=localhost dbname=test" ./test_runner
 */

#pragma once

#include <memory>
#include <string>
#include <functional>
#include <vector>
#include <optional>
#include <chrono>

// Forward declarations
class Connection;
struct ConnectionParameters;
struct SchemaModel;
struct CapabilityInfo;

namespace scratchrobin {
namespace testing {

/**
 * @brief Test result container
 */
struct TestResult {
    std::string testName;
    bool passed = false;
    std::optional<std::string> errorMessage;
    std::chrono::milliseconds duration{0};
    
    // Backend metadata captured during test
    std::optional<std::string> backendVersion;
    std::optional<std::string> serverInfo;
    std::optional<std::vector<std::string>> supportedFeatures;
};

/**
 * @brief Database backend type
 */
enum class BackendType {
    PostgreSQL,
    MySQL,
    Firebird,
    Unknown
};

/**
 * @brief Test configuration from environment
 */
struct TestConfiguration {
    std::optional<std::string> postgresDSN;
    std::optional<std::string> mysqlDSN;
    std::optional<std::string> firebirdDSN;
    std::chrono::seconds timeout{30};
    bool verboseOutput = false;
    bool captureDiagnostics = true;
    
    /**
     * @brief Load configuration from environment variables
     */
    static TestConfiguration FromEnvironment();
    
    /**
     * @brief Check if any backend is configured for testing
     */
    bool HasAnyBackend() const {
        return postgresDSN.has_value() || 
               mysqlDSN.has_value() || 
               firebirdDSN.has_value();
    }
    
    /**
     * @brief Check if specific backend is configured
     */
    bool HasBackend(BackendType type) const;
};

/**
 * @brief Test fixture for backend integration tests
 * 
 * Provides RAII-managed test database connections with automatic
 * cleanup and diagnostic capture.
 */
class BackendTestFixture {
public:
    /**
     * @brief Create fixture for specific backend type
     * @throws std::runtime_error if backend not configured
     */
    explicit BackendTestFixture(BackendType type);
    ~BackendTestFixture();
    
    // Non-copyable, movable
    BackendTestFixture(const BackendTestFixture&) = delete;
    BackendTestFixture& operator=(const BackendTestFixture&) = delete;
    BackendTestFixture(BackendTestFixture&&) noexcept;
    BackendTestFixture& operator=(BackendTestFixture&&) noexcept;
    
    /**
     * @brief Get the backend type
     */
    BackendType GetType() const { return type_; }
    
    /**
     * @brief Check if connection is active
     */
    bool IsConnected() const;
    
    /**
     * @brief Get the underlying connection
     */
    Connection& GetConnection();
    
    /**
     * @brief Get connection parameters used
     */
    const ConnectionParameters& GetParameters() const { return *params_; }
    
    /**
     * @brief Execute SQL and return success/failure
     */
    bool ExecuteSQL(const std::string& sql);
    
    /**
     * @brief Execute SQL query and return result
     */
    std::optional<std::string> QuerySingle(const std::string& sql);
    
    /**
     * @brief Get backend capabilities
     */
    CapabilityInfo GetCapabilities();
    
    /**
     * @brief Load schema from current database
     */
    std::optional<SchemaModel> LoadSchema();
    
    /**
     * @brief Begin transaction (for test isolation)
     */
    void BeginTransaction();
    
    /**
     * @brief Rollback transaction
     */
    void RollbackTransaction();
    
    /**
     * @brief Get diagnostic log from this session
     */
    std::vector<std::string> GetDiagnostics() const { return diagnostics_; }
    
private:
    void Connect();
    void Disconnect();
    void LogDiagnostic(const std::string& message);
    
    BackendType type_;
    std::unique_ptr<ConnectionParameters> params_;
    std::unique_ptr<Connection> connection_;
    std::vector<std::string> diagnostics_;
    bool inTransaction_ = false;
};

/**
 * @brief Test case definition
 */
using TestFunction = std::function<TestResult(BackendTestFixture&)>;

/**
 * @brief Test suite runner
 */
class BackendTestRunner {
public:
    explicit BackendTestRunner(const TestConfiguration& config);
    
    /**
     * @brief Register a test case
     */
    void RegisterTest(const std::string& name, BackendType backend, TestFunction test);
    
    /**
     * @brief Run all registered tests
     */
    std::vector<TestResult> RunAll();
    
    /**
     * @brief Run tests for specific backend only
     */
    std::vector<TestResult> RunBackend(BackendType backend);
    
    /**
     * @brief Get test statistics
     */
    struct Statistics {
        size_t totalTests = 0;
        size_t passedTests = 0;
        size_t failedTests = 0;
        size_t skippedTests = 0;
        std::chrono::milliseconds totalDuration{0};
    };
    Statistics GetStatistics() const;
    
private:
    TestConfiguration config_;
    struct TestCase {
        std::string name;
        BackendType backend;
        TestFunction function;
    };
    std::vector<TestCase> tests_;
    Statistics stats_;
};

/**
 * @brief Pre-defined test cases
 */
namespace standard_tests {
    
    /**
     * @brief Test basic connection lifecycle
     */
    TestResult TestConnectionLifecycle(BackendTestFixture& fixture);
    
    /**
     * @brief Test capability detection
     */
    TestResult TestCapabilityDetection(BackendTestFixture& fixture);
    
    /**
     * @brief Test basic query execution
     */
    TestResult TestBasicQueryExecution(BackendTestFixture& fixture);
    
    /**
     * @brief Test schema introspection
     */
    TestResult TestSchemaIntrospection(BackendTestFixture& fixture);
    
    /**
     * @brief Test table creation and DDL
     */
    TestResult TestTableCreationDDL(BackendTestFixture& fixture);
    
    /**
     * @brief Test data type mapping
     */
    TestResult TestDataTypeMapping(BackendTestFixture& fixture);
    
    /**
     * @brief Test transaction support
     */
    TestResult TestTransactionSupport(BackendTestFixture& fixture);
    
    /**
     * @brief Test error handling
     */
    TestResult TestErrorHandling(BackendTestFixture& fixture);
    
    /**
     * @brief Test prepared statement support
     */
    TestResult TestPreparedStatements(BackendTestFixture& fixture);
    
    /**
     * @brief Test large result set handling
     */
    TestResult TestLargeResultSet(BackendTestFixture& fixture);
    
} // namespace standard_tests

/**
 * @brief Helper macros for test assertions
 */
#define SR_TEST_ASSERT(condition, message) \
    if (!(condition)) { \
        return TestResult{testName, false, (message), elapsed}; \
    }

#define SR_TEST_ASSERT_NO_THROW(expression, message) \
    try { \
        expression; \
    } catch (const std::exception& e) { \
        return TestResult{testName, false, std::string(message) + ": " + e.what(), elapsed}; \
    }

} // namespace testing
} // namespace scratchrobin
