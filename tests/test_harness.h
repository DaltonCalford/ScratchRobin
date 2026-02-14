#pragma once

#include <cstdlib>
#include <exception>
#include <fstream>
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

inline std::string JsonEscape(const std::string& text) {
    std::string out;
    out.reserve(text.size() + 8);
    for (char c : text) {
        if (c == '\\') {
            out += "\\\\";
        } else if (c == '"') {
            out += "\\\"";
        } else if (c == '\n') {
            out += "\\n";
        } else {
            out.push_back(c);
        }
    }
    return out;
}

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
    std::vector<std::string> failed_test_ids;
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
            failed_test_ids.push_back(name);
        } catch (...) {
            r.passed = false;
            r.details = "unknown exception";
            ++failed;
            failed_test_ids.push_back(name);
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

    const std::size_t passed = tests.size() - static_cast<std::size_t>(failed);
    std::cout << "Summary: " << passed << "/" << tests.size() << " passed\n";

    std::ostringstream json;
    json << "{\"total\":" << tests.size()
         << ",\"passed\":" << passed
         << ",\"failed\":" << failed
         << ",\"failed_test_ids\":[";
    for (std::size_t i = 0; i < failed_test_ids.size(); ++i) {
        if (i > 0U) {
            json << ',';
        }
        json << "\"" << JsonEscape(failed_test_ids[i]) << "\"";
    }
    json << "]}";
    std::cout << "SummaryJson: " << json.str() << '\n';

    const char* summary_path = std::getenv("SCRATCHROBIN_TEST_SUMMARY_PATH");
    if (summary_path != nullptr && summary_path[0] != '\0') {
        std::ofstream out(summary_path, std::ios::binary | std::ios::app);
        if (out) {
            out << json.str() << '\n';
        }
    }

    return failed == 0 ? 0 : 1;
}

}  // namespace scratchrobin::tests
