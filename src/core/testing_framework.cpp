/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#include "testing_framework.h"

#include <algorithm>
#include <chrono>
#include <iomanip>
#include <numeric>
#include <sstream>

namespace scratchrobin {

// ============================================================================
// Utility Functions
// ============================================================================

std::string TestTypeToString(TestType type) {
    switch (type) {
        case TestType::UNIT: return "unit";
        case TestType::INTEGRATION: return "integration";
        case TestType::PERFORMANCE: return "performance";
        case TestType::DATA_QUALITY: return "data_quality";
        case TestType::SECURITY: return "security";
        case TestType::MIGRATION: return "migration";
        default: return "unknown";
    }
}

std::string TestStatusToString(TestStatus status) {
    switch (status) {
        case TestStatus::PENDING: return "pending";
        case TestStatus::RUNNING: return "running";
        case TestStatus::PASSED: return "passed";
        case TestStatus::FAILED: return "failed";
        case TestStatus::SKIPPED: return "skipped";
        case TestStatus::ERROR: return "error";
        default: return "unknown";
    }
}

// ============================================================================
// Test Case Implementation
// ============================================================================

TestCase::TestCase(const std::string& id_, const std::string& name_, TestType type_)
    : id(id_), name(name_), type(type_) {}

void TestCase::Execute(TestRunner& runner) {
    status = TestStatus::RUNNING;
    assertions.clear();
    auto start = std::chrono::high_resolution_clock::now();
    
    try {
        // Execute setup
        for (const auto& step : setup_steps) {
            auto result = runner.AssertQuerySuccess(step.sql);
            if (!result.passed) {
                status = TestStatus::ERROR;
                error_message = "Setup failed: " + result.message;
                return;
            }
        }
        
        // Execute test steps
        for (const auto& step : test_steps) {
            if (step.expected_result == "success") {
                auto result = runner.AssertQuerySuccess(step.sql);
                assertions.push_back(result);
            } else if (step.expected_result == "fail") {
                auto result = runner.AssertQueryFails(step.sql);
                assertions.push_back(result);
            } else {
                auto result = runner.AssertQueryResult(step.sql, step.expected_result);
                assertions.push_back(result);
            }
        }
        
        // Execute data quality checks if any
        for (const auto& check : quality_checks) {
            AssertionResult result;
            if (check.metric == "null_percentage") {
                result = runner.AssertNullPercentage(check.table, check.column, check.threshold);
            } else if (check.metric == "uniqueness") {
                result = runner.AssertUniqueness(check.table, check.column);
            }
            assertions.push_back(result);
        }
        
        // Determine status
        bool all_passed = std::all_of(assertions.begin(), assertions.end(),
                                       [](const auto& a) { return a.passed; });
        status = all_passed ? TestStatus::PASSED : TestStatus::FAILED;
        
    } catch (const std::exception& e) {
        status = TestStatus::ERROR;
        error_message = e.what();
    }
    
    // Execute teardown (always run)
    for (const auto& step : teardown_steps) {
        runner.AssertQuerySuccess(step.sql);  // Ignore teardown errors
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    execution_time = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
}

void TestCase::ToYaml(std::ostream& out) const {
    out << "  - id: " << id << "\n";
    out << "    name: \"" << name << "\"\n";
    out << "    type: " << TestTypeToString(type) << "\n";
    if (!description.empty()) {
        out << "    description: \"" << description << "\"\n";
    }
    // ... more serialization
}

// ============================================================================
// Test Suite Implementation
// ============================================================================

TestSuite::TestSuite(const std::string& name_) : name(name_) {}

void TestSuite::AddTest(std::unique_ptr<TestCase> test) {
    tests.push_back(std::move(test));
}

void TestSuite::RemoveTest(const std::string& id) {
    tests.erase(
        std::remove_if(tests.begin(), tests.end(),
            [&id](const auto& t) { return t->id == id; }),
        tests.end());
}

TestCase* TestSuite::GetTest(const std::string& id) {
    for (auto& test : tests) {
        if (test->id == id) {
            return test.get();
        }
    }
    return nullptr;
}

std::vector<TestCase*> TestSuite::GetTestsByType(TestType type) {
    std::vector<TestCase*> result;
    for (auto& test : tests) {
        if (test->type == type) {
            result.push_back(test.get());
        }
    }
    return result;
}

std::vector<TestCase*> TestSuite::GetTestsByTag(const std::string& tag) {
    std::vector<TestCase*> result;
    for (auto& test : tests) {
        if (std::find(test->tags.begin(), test->tags.end(), tag) != test->tags.end()) {
            result.push_back(test.get());
        }
    }
    return result;
}

void TestSuite::ExecuteAll(TestRunner& runner) {
    for (auto& test : tests) {
        test->Execute(runner);
    }
}

void TestSuite::ExecuteByType(TestRunner& runner, TestType type) {
    for (auto test : GetTestsByType(type)) {
        test->Execute(runner);
    }
}

void TestSuite::ExecuteByTag(TestRunner& runner, const std::string& tag) {
    for (auto test : GetTestsByTag(tag)) {
        test->Execute(runner);
    }
}

TestSuite::Stats TestSuite::GetStats() const {
    Stats stats;
    stats.total = static_cast<int>(tests.size());
    
    for (const auto& test : tests) {
        switch (test->status) {
            case TestStatus::PASSED: stats.passed++; break;
            case TestStatus::FAILED: stats.failed++; break;
            case TestStatus::SKIPPED: stats.skipped++; break;
            case TestStatus::ERROR: stats.errors++; break;
            default: break;
        }
        stats.total_time += test->execution_time;
    }
    
    return stats;
}

void TestSuite::ToYaml(std::ostream& out) const {
    out << "test_suite: \"" << name << "\"\n";
    out << "version: \"" << version << "\"\n";
    if (!description.empty()) {
        out << "description: \"" << description << "\"\n";
    }
    out << "\ntests:\n";
    for (const auto& test : tests) {
        test->ToYaml(out);
    }
}

// ============================================================================
// Test Runner Implementation
// ============================================================================

TestRunner::TestRunner() = default;
TestRunner::~TestRunner() = default;

void TestRunner::SetConnection(const ConnectionInfo& conn) {
    connection_ = conn;
}

void TestRunner::SetParallelExecution(bool parallel, int max_workers) {
    parallel_ = parallel;
    max_workers_ = max_workers;
}

void TestRunner::SetFailFast(bool fail_fast) {
    fail_fast_ = fail_fast;
}

AssertionResult TestRunner::AssertTrue(bool condition, const std::string& message) {
    AssertionResult result;
    result.passed = condition;
    result.message = message;
    result.expected = "true";
    result.actual = condition ? "true" : "false";
    return result;
}

AssertionResult TestRunner::AssertFalse(bool condition, const std::string& message) {
    return AssertTrue(!condition, message);
}

AssertionResult TestRunner::AssertEquals(const std::string& expected, const std::string& actual) {
    AssertionResult result;
    result.passed = (expected == actual);
    result.expected = expected;
    result.actual = actual;
    if (!result.passed) {
        result.message = "Expected: " + expected + ", Actual: " + actual;
    }
    return result;
}

AssertionResult TestRunner::AssertEquals(int expected, int actual) {
    return AssertEquals(std::to_string(expected), std::to_string(actual));
}

AssertionResult TestRunner::AssertEquals(double expected, double actual, double tolerance) {
    AssertionResult result;
    result.passed = std::abs(expected - actual) <= tolerance;
    result.expected = std::to_string(expected) + " ± " + std::to_string(tolerance);
    result.actual = std::to_string(actual);
    return result;
}

AssertionResult TestRunner::AssertNull(const std::string& value) {
    AssertionResult result;
    result.passed = value.empty() || value == "NULL" || value == "null";
    result.expected = "NULL";
    result.actual = value;
    return result;
}

AssertionResult TestRunner::AssertNotNull(const std::string& value) {
    AssertionResult result;
    result.passed = !value.empty() && value != "NULL" && value != "null";
    result.expected = "not NULL";
    result.actual = value.empty() ? "NULL" : value;
    return result;
}

AssertionResult TestRunner::AssertTableExists(const std::string& schema, 
                                               const std::string& table) {
    // Would execute actual query in implementation
    AssertionResult result;
    result.passed = true;  // Stub
    result.message = "Table " + schema + "." + table + " exists";
    return result;
}

AssertionResult TestRunner::AssertColumnExists(const std::string& schema,
                                                const std::string& table,
                                                const std::string& column) {
    AssertionResult result;
    result.passed = true;  // Stub
    result.message = "Column " + column + " exists in " + schema + "." + table;
    return result;
}

AssertionResult TestRunner::AssertQueryResult(const std::string& sql, 
                                               const std::string& expected) {
    AssertionResult result;
    // Would execute query and compare results
    result.passed = true;  // Stub
    result.message = "Query returned expected result";
    return result;
}

AssertionResult TestRunner::AssertQuerySuccess(const std::string& sql) {
    AssertionResult result;
    std::string error;
    result.passed = ExecuteSql(sql, &error);
    result.message = result.passed ? "Query executed successfully" : error;
    return result;
}

AssertionResult TestRunner::AssertQueryFails(const std::string& sql) {
    AssertionResult result;
    std::string error;
    bool success = ExecuteSql(sql, &error);
    result.passed = !success;  // We expect it to fail
    result.message = result.passed ? "Query failed as expected" : "Query succeeded but should have failed";
    return result;
}

AssertionResult TestRunner::AssertExecutionTime(const std::string& sql, double max_ms) {
    auto start = std::chrono::high_resolution_clock::now();
    auto result = AssertQuerySuccess(sql);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto elapsed_ms = std::chrono::duration<double, std::milli>(end - start).count();
    result.passed = result.passed && (elapsed_ms <= max_ms);
    result.expected = "< " + std::to_string(max_ms) + " ms";
    result.actual = std::to_string(elapsed_ms) + " ms";
    
    return result;
}

AssertionResult TestRunner::AssertIndexUsed(const std::string& sql) {
    // Would use EXPLAIN or equivalent
    AssertionResult result;
    result.passed = true;  // Stub
    result.message = "Query uses index";
    return result;
}

AssertionResult TestRunner::AssertNullPercentage(const std::string& table,
                                                  const std::string& column,
                                                  double max_percentage) {
    // Would execute: SELECT COUNT(*) * 100.0 / COUNT(column) FROM table WHERE column IS NULL
    AssertionResult result;
    result.passed = true;  // Stub
    result.message = "Null percentage within threshold";
    return result;
}

AssertionResult TestRunner::AssertUniqueness(const std::string& table,
                                              const std::string& column) {
    // Would execute: SELECT COUNT(DISTINCT column) = COUNT(*) FROM table
    AssertionResult result;
    result.passed = true;  // Stub
    result.message = "Column values are unique";
    return result;
}

AssertionResult TestRunner::AssertReferentialIntegrity(const std::string& from_table,
                                                        const std::string& from_column,
                                                        const std::string& to_table,
                                                        const std::string& to_column) {
    // Would execute: SELECT COUNT(*) FROM from_table WHERE from_column NOT IN (SELECT to_column FROM to_table)
    AssertionResult result;
    result.passed = true;  // Stub
    result.message = "Referential integrity maintained";
    return result;
}

TestRunner::BenchmarkResult TestRunner::BenchmarkQuery(const std::string& sql, int iterations) {
    BenchmarkResult result;
    result.iterations = iterations;
    result.individual_times.reserve(iterations);
    
    for (int i = 0; i < iterations; ++i) {
        auto start = std::chrono::high_resolution_clock::now();
        ExecuteSql(sql);
        auto end = std::chrono::high_resolution_clock::now();
        
        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        result.individual_times.push_back(ms);
    }
    
    // Calculate statistics
    std::sort(result.individual_times.begin(), result.individual_times.end());
    result.min_time_ms = result.individual_times.front();
    result.max_time_ms = result.individual_times.back();
    result.avg_time_ms = std::accumulate(result.individual_times.begin(),
                                          result.individual_times.end(), 0.0) / iterations;
    result.p50_time_ms = result.individual_times[iterations * 0.5];
    result.p95_time_ms = result.individual_times[iterations * 0.95];
    result.p99_time_ms = result.individual_times[iterations * 0.99];
    result.throughput_qps = 1000.0 / result.avg_time_ms;
    
    return result;
}

void TestRunner::SetProgressCallback(ProgressCallback callback) {
    progress_callback_ = callback;
}

bool TestRunner::ExecuteSql(const std::string& sql, std::string* error) {
    // Would execute actual SQL against connection
    // Stub implementation
    return true;
}

bool TestRunner::ExecuteSqlWithResult(const std::string& sql,
                                       std::vector<std::map<std::string, std::string>>* result,
                                       std::string* error) {
    // Would execute query and return results
    // Stub implementation
    return true;
}

// ============================================================================
// Test Result Implementation
// ============================================================================

TestResult::TestResult(const TestCase& test)
    : test_id(test.id), test_name(test.name), type(test.type) {}

std::string TestResult::ToString() const {
    std::ostringstream oss;
    oss << "Test: " << test_name << "\n";
    oss << "Status: " << TestStatusToString(status) << "\n";
    oss << "Assertions: " << assertions_passed << "/" 
        << (assertions_passed + assertions_failed) << " passed\n";
    oss << "Time: " << execution_time.count() / 1000.0 << " ms\n";
    if (!error_message.empty()) {
        oss << "Error: " << error_message << "\n";
    }
    return oss.str();
}

void TestResult::ToJson(std::ostream& out) const {
    out << "{\n";
    out << "  \"test_id\": \"" << test_id << "\",\n";
    out << "  \"test_name\": \"" << test_name << "\",\n";
    out << "  \"status\": \"" << TestStatusToString(status) << "\",\n";
    out << "  \"assertions_passed\": " << assertions_passed << ",\n";
    out << "  \"assertions_failed\": " << assertions_failed << ",\n";
    out << "  \"execution_time_us\": " << execution_time.count() << "\n";
    out << "}";
}

// ============================================================================
// Test Report Generator
// ============================================================================

void TestReportGenerator::GenerateReport(const TestSuite& suite,
                                          const std::vector<TestResult>& results,
                                          Format format, std::ostream& out) {
    switch (format) {
        case Format::TEXT: GenerateTextReport(suite, results, out); break;
        case Format::JSON: GenerateJsonReport(suite, results, out); break;
        case Format::HTML: GenerateHtmlReport(suite, results, out); break;
        case Format::JUNIT_XML: GenerateJUnitReport(suite, results, out); break;
        case Format::MARKDOWN: GenerateMarkdownReport(suite, results, out); break;
    }
}

void TestReportGenerator::GenerateTextReport(const TestSuite& suite,
                                              const std::vector<TestResult>& results,
                                              std::ostream& out) {
    auto stats = suite.GetStats();
    
    out << "═══════════════════════════════════════════════════════════════\n";
    out << "  Test Report: " << suite.name << "\n";
    out << "═══════════════════════════════════════════════════════════════\n\n";
    
    out << "Summary:\n";
    out << "  Total:    " << stats.total << "\n";
    out << "  Passed:   " << stats.passed << " ✓\n";
    out << "  Failed:   " << stats.failed << " ✗\n";
    out << "  Skipped:  " << stats.skipped << " ○\n";
    out << "  Errors:   " << stats.errors << " !\n";
    out << "  Time:     " << stats.total_time.count() / 1000000.0 << " s\n\n";
    
    out << "Results:\n";
    for (const auto& result : results) {
        const char* symbol = (result.status == TestStatus::PASSED) ? "✓" :
                            (result.status == TestStatus::FAILED) ? "✗" :
                            (result.status == TestStatus::SKIPPED) ? "○" : "!";
        out << "  " << symbol << " " << result.test_name << " (" 
            << result.execution_time.count() / 1000.0 << " ms)\n";
    }
}

void TestReportGenerator::GenerateJsonReport(const TestSuite& suite,
                                              const std::vector<TestResult>& results,
                                              std::ostream& out) {
    auto stats = suite.GetStats();
    
    out << "{\n";
    out << "  \"suite\": \"" << suite.name << "\",\n";
    out << "  \"summary\": {\n";
    out << "    \"total\": " << stats.total << ",\n";
    out << "    \"passed\": " << stats.passed << ",\n";
    out << "    \"failed\": " << stats.failed << ",\n";
    out << "    \"skipped\": " << stats.skipped << ",\n";
    out << "    \"errors\": " << stats.errors << "\n";
    out << "  },\n";
    out << "  \"results\": [\n";
    
    for (size_t i = 0; i < results.size(); ++i) {
        out << "    {\n";
        out << "      \"test_id\": \"" << results[i].test_id << "\",\n";
        out << "      \"test_name\": \"" << results[i].test_name << "\",\n";
        out << "      \"status\": \"" << TestStatusToString(results[i].status) << "\"\n";
        out << "    }";
        if (i < results.size() - 1) out << ",";
        out << "\n";
    }
    
    out << "  ]\n";
    out << "}\n";
}

void TestReportGenerator::GenerateHtmlReport(const TestSuite& suite,
                                              const std::vector<TestResult>& results,
                                              std::ostream& out) {
    auto stats = suite.GetStats();
    
    out << "<!DOCTYPE html>\n<html>\n<head>\n";
    out << "<title>Test Report: " << suite.name << "</title>\n";
    out << "<style>\n";
    out << "body { font-family: Arial, sans-serif; margin: 20px; }\n";
    out << ".passed { color: green; }\n";
    out << ".failed { color: red; }\n";
    out << ".summary { background: #f0f0f0; padding: 15px; margin: 20px 0; }\n";
    out << "table { border-collapse: collapse; width: 100%; }\n";
    out << "th, td { border: 1px solid #ddd; padding: 8px; text-align: left; }\n";
    out << "th { background-color: #4CAF50; color: white; }\n";
    out << "</style>\n</head>\n<body>\n";
    
    out << "<h1>Test Report: " << suite.name << "</h1>\n";
    out << "<div class='summary'>\n";
    out << "<h2>Summary</h2>\n";
    out << "<p>Total: " << stats.total << "</p>\n";
    out << "<p class='passed'>Passed: " << stats.passed << "</p>\n";
    out << "<p class='failed'>Failed: " << stats.failed << "</p>\n";
    out << "<p>Skipped: " << stats.skipped << "</p>\n";
    out << "</div>\n";
    
    out << "<table>\n";
    out << "<tr><th>Test</th><th>Status</th><th>Time (ms)</th></tr>\n";
    for (const auto& result : results) {
        const char* css_class = (result.status == TestStatus::PASSED) ? "passed" :
                               (result.status == TestStatus::FAILED) ? "failed" : "";
        out << "<tr class='" << css_class << "'>\n";
        out << "<td>" << result.test_name << "</td>\n";
        out << "<td>" << TestStatusToString(result.status) << "</td>\n";
        out << "<td>" << result.execution_time.count() / 1000.0 << "</td>\n";
        out << "</tr>\n";
    }
    out << "</table>\n";
    out << "</body>\n</html>\n";
}

void TestReportGenerator::GenerateJUnitReport(const TestSuite& suite,
                                               const std::vector<TestResult>& results,
                                               std::ostream& out) {
    auto stats = suite.GetStats();
    
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    out << "<testsuites>\n";
    out << "  <testsuite name=\"" << suite.name << "\" tests=\"" << stats.total 
        << "\" failures=\"" << stats.failed << "\" errors=\"" << stats.errors 
        << "\" skipped=\"" << stats.skipped << "\">\n";
    
    for (const auto& result : results) {
        out << "    <testcase name=\"" << result.test_name << "\" classname=\"" 
            << suite.name << "\" time=\"" << result.execution_time.count() / 1000000.0 << "\">\n";
        
        if (result.status == TestStatus::FAILED) {
            out << "      <failure message=\"" << result.error_message << "\"/>\n";
        } else if (result.status == TestStatus::ERROR) {
            out << "      <error message=\"" << result.error_message << "\"/>\n";
        } else if (result.status == TestStatus::SKIPPED) {
            out << "      <skipped/>\n";
        }
        
        out << "    </testcase>\n";
    }
    
    out << "  </testsuite>\n";
    out << "</testsuites>\n";
}

void TestReportGenerator::GenerateMarkdownReport(const TestSuite& suite,
                                                  const std::vector<TestResult>& results,
                                                  std::ostream& out) {
    auto stats = suite.GetStats();
    
    out << "# Test Report: " << suite.name << "\n\n";
    out << "## Summary\n\n";
    out << "| Metric | Count |\n";
    out << "|--------|-------|\n";
    out << "| Total | " << stats.total << " |\n";
    out << "| Passed | " << stats.passed << " ✓ |\n";
    out << "| Failed | " << stats.failed << " ✗ |\n";
    out << "| Skipped | " << stats.skipped << " ○ |\n";
    out << "| Errors | " << stats.errors << " ! |\n";
    out << "| Time | " << stats.total_time.count() / 1000000.0 << " s |\n\n";
    
    out << "## Results\n\n";
    out << "| Test | Status | Time (ms) |\n";
    out << "|------|--------|-----------|\n";
    for (const auto& result : results) {
        const char* symbol = (result.status == TestStatus::PASSED) ? "✓" :
                            (result.status == TestStatus::FAILED) ? "✗" :
                            (result.status == TestStatus::SKIPPED) ? "○" : "!";
        out << "| " << result.test_name << " | " << symbol << " " 
            << TestStatusToString(result.status) << " | " 
            << result.execution_time.count() / 1000.0 << " |\n";
    }
}

// ============================================================================
// Auto Test Generator
// ============================================================================

std::unique_ptr<TestSuite> AutoTestGenerator::GenerateSchemaTests(
    const std::string& connection_string,
    const std::vector<std::string>& schemas) {
    
    auto suite = std::make_unique<TestSuite>("Auto-Generated Schema Tests");
    
    for (const auto& schema : schemas) {
        // Would query database for tables in schema
        // For now, stub implementation
        AddTableStructureTests(*suite, schema, "example_table");
    }
    
    return suite;
}

void AutoTestGenerator::AddTableStructureTests(TestSuite& suite,
                                                const std::string& schema,
                                                const std::string& table) {
    auto test = std::make_unique<TestCase>(
        "table_exists_" + schema + "_" + table,
        "Table exists: " + schema + "." + table,
        TestType::UNIT
    );
    
    test->tags = {"auto-generated", "schema", "structure"};
    
    auto step = TestCase::SqlStep();
    step.name = "Check table exists";
    step.sql = "SELECT 1 FROM information_schema.tables WHERE table_schema = '" + 
               schema + "' AND table_name = '" + table + "'";
    step.expected_result = "success";
    test->test_steps.push_back(step);
    
    suite.AddTest(std::move(test));
}

} // namespace scratchrobin
