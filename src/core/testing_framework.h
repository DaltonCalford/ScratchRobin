/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_TESTING_FRAMEWORK_H
#define SCRATCHROBIN_TESTING_FRAMEWORK_H

#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>
#include <chrono>

namespace scratchrobin {

// Forward declarations
class TestSuite;
class TestCase;
class TestRunner;
class TestResult;

// ============================================================================
// Test Types
// ============================================================================
enum class TestType {
    UNIT,           // Database object level tests
    INTEGRATION,    // Workflow/transaction tests
    PERFORMANCE,    // Benchmark and load tests
    DATA_QUALITY,   // Data validation tests
    SECURITY,       // Security and access tests
    MIGRATION       // Migration validation tests
};

std::string TestTypeToString(TestType type);

// ============================================================================
// Test Status
// ============================================================================
enum class TestStatus {
    PENDING,        // Not yet executed
    RUNNING,        // Currently executing
    PASSED,         // Test passed
    FAILED,         // Test failed
    SKIPPED,        // Test skipped
    ERROR           // Test error (infrastructure issue)
};

std::string TestStatusToString(TestStatus status);

// ============================================================================
// Assertion Result
// ============================================================================
struct AssertionResult {
    bool passed = false;
    std::string message;
    std::string expected;
    std::string actual;
    std::string file;
    int line = 0;
    std::chrono::microseconds execution_time;
};

// ============================================================================
// Test Case - Individual test
// ============================================================================
class TestCase {
public:
    std::string id;
    std::string name;
    std::string description;
    TestType type = TestType::UNIT;
    std::vector<std::string> tags;
    
    // SQL-based test definition
    struct SqlStep {
        std::string name;
        std::string sql;
        std::string expected_result;  // "success", "fail", or result data
        std::map<std::string, std::string> parameters;
    };
    
    std::vector<SqlStep> setup_steps;
    std::vector<SqlStep> test_steps;
    std::vector<SqlStep> teardown_steps;
    
    // Performance test configuration
    struct PerformanceConfig {
        int iterations = 1;
        int concurrent_users = 1;
        std::chrono::seconds duration{0};
        double max_avg_time_ms = 1000.0;
        double max_p95_time_ms = 2000.0;
        double max_error_rate = 0.01;
    };
    PerformanceConfig perf_config;
    
    // Data quality configuration
    struct DataQualityConfig {
        std::string table;
        std::string column;
        std::string metric;  // "null_percentage", "uniqueness", "freshness", etc.
        std::string operator_;
        double threshold;
    };
    std::vector<DataQualityConfig> quality_checks;
    
    // Execution results
    TestStatus status = TestStatus::PENDING;
    std::vector<AssertionResult> assertions;
    std::chrono::microseconds execution_time;
    std::string error_message;
    std::string output_log;
    
    TestCase() = default;
    TestCase(const std::string& id_, const std::string& name_, TestType type_);
    
    // Execution
    void Execute(TestRunner& runner);
    
    // Serialization
    void ToYaml(std::ostream& out) const;
    static std::unique_ptr<TestCase> FromYaml(const std::string& yaml);
};

// ============================================================================
// Test Suite - Collection of tests
// ============================================================================
class TestSuite {
public:
    std::string name;
    std::string version;
    std::string description;
    
    std::vector<std::unique_ptr<TestCase>> tests;
    
    // Global setup/teardown
    std::string global_setup_sql;
    std::string global_teardown_sql;
    
    // Execution configuration
    struct ExecutionConfig {
        bool parallel = false;
        int max_workers = 4;
        bool fail_fast = false;
        std::chrono::seconds timeout{300};
        std::string environment;  // dev, staging, prod
        std::string connection_string;
    };
    ExecutionConfig config;
    
    TestSuite() = default;
    explicit TestSuite(const std::string& name_);
    
    // Test management
    void AddTest(std::unique_ptr<TestCase> test);
    void RemoveTest(const std::string& id);
    TestCase* GetTest(const std::string& id);
    std::vector<TestCase*> GetTestsByType(TestType type);
    std::vector<TestCase*> GetTestsByTag(const std::string& tag);
    
    // Execution
    void ExecuteAll(TestRunner& runner);
    void ExecuteByType(TestRunner& runner, TestType type);
    void ExecuteByTag(TestRunner& runner, const std::string& tag);
    
    // Statistics
    struct Stats {
        int total = 0;
        int passed = 0;
        int failed = 0;
        int skipped = 0;
        int errors = 0;
        std::chrono::microseconds total_time;
    };
    Stats GetStats() const;
    
    // Serialization
    void ToYaml(std::ostream& out) const;
    static std::unique_ptr<TestSuite> FromYaml(const std::string& yaml);
    void SaveToFile(const std::string& path) const;
    static std::unique_ptr<TestSuite> LoadFromFile(const std::string& path);
};

// ============================================================================
// Test Runner - Executes tests
// ============================================================================
class TestRunner {
public:
    struct ConnectionInfo {
        std::string name;
        std::string connection_string;
        std::string backend_type;
        bool is_read_only = false;
    };
    
    TestRunner();
    ~TestRunner();
    
    // Configuration
    void SetConnection(const ConnectionInfo& conn);
    void SetParallelExecution(bool parallel, int max_workers = 4);
    void SetFailFast(bool fail_fast);
    
    // Execution
    TestResult ExecuteTest(TestCase& test);
    std::vector<TestResult> ExecuteTests(const std::vector<TestCase*>& tests);
    
    // Assertions
    AssertionResult AssertTrue(bool condition, const std::string& message);
    AssertionResult AssertFalse(bool condition, const std::string& message);
    AssertionResult AssertEquals(const std::string& expected, const std::string& actual);
    AssertionResult AssertEquals(int expected, int actual);
    AssertionResult AssertEquals(double expected, double actual, double tolerance);
    AssertionResult AssertNull(const std::string& value);
    AssertionResult AssertNotNull(const std::string& value);
    AssertionResult AssertTableExists(const std::string& schema, const std::string& table);
    AssertionResult AssertColumnExists(const std::string& schema, const std::string& table,
                                        const std::string& column);
    AssertionResult AssertQueryResult(const std::string& sql, const std::string& expected);
    AssertionResult AssertQuerySuccess(const std::string& sql);
    AssertionResult AssertQueryFails(const std::string& sql);
    AssertionResult AssertExecutionTime(const std::string& sql, double max_ms);
    AssertionResult AssertIndexUsed(const std::string& sql);
    AssertionResult AssertNullPercentage(const std::string& table, const std::string& column,
                                          double max_percentage);
    AssertionResult AssertUniqueness(const std::string& table, const std::string& column);
    AssertionResult AssertReferentialIntegrity(const std::string& from_table,
                                                const std::string& from_column,
                                                const std::string& to_table,
                                                const std::string& to_column);
    
    // Performance measurement
    struct BenchmarkResult {
        double avg_time_ms;
        double min_time_ms;
        double max_time_ms;
        double p50_time_ms;
        double p95_time_ms;
        double p99_time_ms;
        int iterations;
        double throughput_qps;
        std::vector<double> individual_times;
    };
    BenchmarkResult BenchmarkQuery(const std::string& sql, int iterations = 100);
    
    // Progress callback
    using ProgressCallback = std::function<void(const TestCase&, TestStatus, const std::string&)>;
    void SetProgressCallback(ProgressCallback callback);
    
private:
    ConnectionInfo connection_;
    bool parallel_ = false;
    int max_workers_ = 4;
    bool fail_fast_ = false;
    ProgressCallback progress_callback_;
    
    bool ExecuteSql(const std::string& sql, std::string* error = nullptr);
    bool ExecuteSqlWithResult(const std::string& sql, std::vector<std::map<std::string, std::string>>* result,
                               std::string* error = nullptr);
};

// ============================================================================
// Test Result
// ============================================================================
class TestResult {
public:
    std::string test_id;
    std::string test_name;
    TestType type;
    TestStatus status = TestStatus::PENDING;
    
    std::vector<AssertionResult> assertions;
    std::chrono::microseconds execution_time;
    std::string error_message;
    std::string output_log;
    
    // Summary
    int assertions_passed = 0;
    int assertions_failed = 0;
    
    // Metadata
    std::string executed_by;
    std::time_t executed_at;
    std::string environment;
    std::string connection_info;
    
    TestResult() = default;
    explicit TestResult(const TestCase& test);
    
    // Reporting
    std::string ToString() const;
    void ToJson(std::ostream& out) const;
    void ToJUnitXml(std::ostream& out) const;
    
    bool DidPass() const { return status == TestStatus::PASSED; }
};

// ============================================================================
// Test Report Generator
// ============================================================================
class TestReportGenerator {
public:
    enum class Format {
        TEXT,
        JSON,
        HTML,
        JUNIT_XML,
        MARKDOWN
    };
    
    static void GenerateReport(const TestSuite& suite, const std::vector<TestResult>& results,
                                Format format, std::ostream& out);
    
    static void GenerateReport(const TestSuite& suite, const std::vector<TestResult>& results,
                                Format format, const std::string& file_path);
    
private:
    static void GenerateTextReport(const TestSuite& suite, const std::vector<TestResult>& results,
                                    std::ostream& out);
    static void GenerateJsonReport(const TestSuite& suite, const std::vector<TestResult>& results,
                                    std::ostream& out);
    static void GenerateHtmlReport(const TestSuite& suite, const std::vector<TestResult>& results,
                                    std::ostream& out);
    static void GenerateJUnitReport(const TestSuite& suite, const std::vector<TestResult>& results,
                                     std::ostream& out);
    static void GenerateMarkdownReport(const TestSuite& suite, const std::vector<TestResult>& results,
                                        std::ostream& out);
};

// ============================================================================
// Auto-Test Generator
// ============================================================================
class AutoTestGenerator {
public:
    // Generate tests from database schema
    static std::unique_ptr<TestSuite> GenerateSchemaTests(const std::string& connection_string,
                                                           const std::vector<std::string>& schemas);
    
    // Generate tests from project objects
    static std::unique_ptr<TestSuite> GenerateProjectTests(const class Project& project);
    
    // Generate data quality tests based on data profiling
    static std::unique_ptr<TestSuite> GenerateDataQualityTests(const std::string& connection_string,
                                                                const std::vector<std::string>& tables);
private:
    static void AddTableStructureTests(TestSuite& suite, const std::string& schema,
                                        const std::string& table);
    static void AddConstraintTests(TestSuite& suite, const std::string& schema,
                                    const std::string& table);
    static void AddForeignKeyTests(TestSuite& suite, const std::string& schema,
                                    const std::string& table);
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_TESTING_FRAMEWORK_H
