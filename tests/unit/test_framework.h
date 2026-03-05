// Copyright (c) 2025 Silverstone Data Systems
// robin-migrate: Unit Test Framework

#ifndef ROBIN_MIGRATE_TESTS_UNIT_TEST_FRAMEWORK_H_
#define ROBIN_MIGRATE_TESTS_UNIT_TEST_FRAMEWORK_H_

#include <string>
#include <vector>
#include <functional>
#include <sstream>
#include <chrono>

namespace scratchrobin {
namespace testing {

struct TestFailure {
  std::string message;
  std::string file;
  int line{0};
  bool passed{true};
};

struct TestCaseResult {
  std::string test_name;
  bool passed{false};
  std::vector<TestFailure> failures;
  int execution_time_ms{0};
};

using TestFunc = std::function<TestFailure(void)>;

struct TestCase {
  std::string name;
  TestFunc func;
};

class UnitTestFramework {
public:
  static void RegisterTest(const std::string& suite, const std::string& name, TestFunc func);
  static std::vector<TestCaseResult> RunAllTests();
  static void PrintResults(const std::vector<TestCaseResult>& results);
  static UnitTestFramework& GetInstance();

private:
  UnitTestFramework() = default;
  std::vector<std::pair<std::string, TestCase>> tests_;
};

#define ASSERT_TRUE(expr) \
  if (!(expr)) { return TestFailure{#expr " should be true", __FILE__, __LINE__, false}; }

#define ASSERT_EQ(expected, actual) \
  if ((expected) != (actual)) { \
    std::stringstream ss; \
    ss << "Expected: " << (expected) << ", Actual: " << (actual); \
    return TestFailure{ss.str(), __FILE__, __LINE__, false}; \
  }

}  // namespace testing
}  // namespace scratchrobin

#endif
