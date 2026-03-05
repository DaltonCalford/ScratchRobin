// Copyright (c) 2025 Silverstone Data Systems
// robin-migrate: UI Integration Tests
// Purpose: End-to-end testing framework for UI components

#ifndef ROBIN_MIGRATE_TESTS_INTEGRATION_UI_INTEGRATION_TESTS_H_
#define ROBIN_MIGRATE_TESTS_INTEGRATION_UI_INTEGRATION_TESTS_H_

#include <string>
#include <vector>
#include <functional>

namespace scratchrobin {
namespace testing {

/**
 * Test result structure
 */
struct TestResult {
  std::string test_name;
  bool passed{false};
  std::string error_message;
  int execution_time_ms{0};
};

/**
 * Integration test suite for UI components
 */
class UIIntegrationTests {
public:
  /**
   * Run all integration tests
   */
  static std::vector<TestResult> RunAllTests();

  /**
   * Individual test cases
   */
  static TestResult TestMainFrameCreation();
  static TestResult TestProjectNavigatorLoading();
  static TestResult TestConnectionDialog();
  static TestResult TestSqlEditorExecution();
  static TestResult TestQueryHistory();
  static TestResult TestDataGridPagination();
  static TestResult TestAsyncQueryExecution();
  static TestResult TestQueryBuilder();
  static TestResult TestObjectMetadataDialog();
  static TestResult TestPreferencesPersistence();

private:
  static TestResult RunTest(const std::string& name,
                             std::function<bool(std::string&)> test_func);
};

}  // namespace testing
}  // namespace scratchrobin

#endif  // ROBIN_MIGRATE_TESTS_INTEGRATION_UI_INTEGRATION_TESTS_H_
