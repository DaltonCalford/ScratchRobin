#pragma once

#include <exception>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace scratchrobin::tests {

struct TestResult {
    std::string name;
    bool passed = false;
    std::string details;
};

using TestFn = std::function<void()>;

inline void AssertTrue(bool cond, const std::string& message) {
    if (!cond) {
        throw std::runtime_error(message);
    }
}

inline void AssertEq(const std::string& a, const std::string& b, const std::string& message) {
    if (a != b) {
        throw std::runtime_error(message + " expected='" + b + "' actual='" + a + "'");
    }
}

inline int RunTests(const std::vector<std::pair<std::string, TestFn>>& tests) {
    std::vector<TestResult> results;
    results.reserve(tests.size());

    int failed = 0;
    for (const auto& [name, fn] : tests) {
        TestResult r;
        r.name = name;
        try {
            fn();
            r.passed = true;
        } catch (const std::exception& ex) {
            r.passed = false;
            r.details = ex.what();
            ++failed;
        } catch (...) {
            r.passed = false;
            r.details = "unknown exception";
            ++failed;
        }
        results.push_back(r);
    }

    for (const auto& r : results) {
        std::cout << (r.passed ? "[PASS] " : "[FAIL] ") << r.name;
        if (!r.passed && !r.details.empty()) {
            std::cout << " :: " << r.details;
        }
        std::cout << '\n';
    }

    std::cout << "Summary: " << (tests.size() - static_cast<std::size_t>(failed)) << "/" << tests.size() << " passed\n";
    return failed == 0 ? 0 : 1;
}

}  // namespace scratchrobin::tests
