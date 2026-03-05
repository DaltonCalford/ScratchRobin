// Copyright (c) 2025 Silverstone Data Systems
// robin-migrate: Unit Test Framework Implementation

#include "test_framework.h"

#include <iostream>
#include <iomanip>

namespace scratchrobin {
namespace testing {

UnitTestFramework& UnitTestFramework::GetInstance() {
  static UnitTestFramework instance;
  return instance;
}

void UnitTestFramework::RegisterTest(const std::string& suite, 
                                      const std::string& name, 
                                      TestFunc func) {
  GetInstance().tests_.push_back({suite, {name, func}});
}

std::vector<TestCaseResult> UnitTestFramework::RunAllTests() {
  std::vector<TestCaseResult> results;
  
  for (const auto& [suite, test_case] : GetInstance().tests_) {
    TestCaseResult result;
    result.test_name = suite + "::" + test_case.name;
    
    auto start = std::chrono::steady_clock::now();
    TestFailure failure = test_case.func();
    auto end = std::chrono::steady_clock::now();
    
    result.execution_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        end - start).count();
    
    if (failure.passed) {
      result.passed = true;
    } else {
      result.passed = false;
      result.failures.push_back(failure);
    }
    
    results.push_back(result);
  }
  
  return results;
}

void UnitTestFramework::PrintResults(const std::vector<TestCaseResult>& results) {
  int passed = 0;
  int failed = 0;
  int total_time = 0;
  
  std::cout << "\n========== UNIT TEST RESULTS ==========\n\n";
  
  for (const auto& result : results) {
    std::cout << std::left << std::setw(50) << result.test_name;
    
    if (result.passed) {
      std::cout << " [PASS] (" << result.execution_time_ms << "ms)\n";
      passed++;
    } else {
      std::cout << " [FAIL] (" << result.execution_time_ms << "ms)\n";
      for (const auto& failure : result.failures) {
        std::cout << "  - " << failure.message << "\n";
        std::cout << "    at " << failure.file << ":" << failure.line << "\n";
      }
      failed++;
    }
    
    total_time += result.execution_time_ms;
  }
  
  std::cout << "\n========================================\n";
  std::cout << "Total: " << (passed + failed) << " | ";
  std::cout << "Passed: " << passed << " | ";
  std::cout << "Failed: " << failed << " | ";
  std::cout << "Time: " << total_time << "ms\n";
  std::cout << "========================================\n\n";
}

}  // namespace testing
}  // namespace scratchrobin
