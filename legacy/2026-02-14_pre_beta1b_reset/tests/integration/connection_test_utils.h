/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_TESTS_INTEGRATION_CONNECTION_TEST_UTILS_H
#define SCRATCHROBIN_TESTS_INTEGRATION_CONNECTION_TEST_UTILS_H

#include <string>
#include <vector>

#include "core/connection_backend.h"

namespace scratchrobin {
namespace test_utils {

inline std::string Trim(std::string value) {
    auto not_space = [](unsigned char ch) { return ch != ' ' && ch != '\t' && ch != '\n' && ch != '\r'; };
    while (!value.empty() && !not_space(value.front())) {
        value.erase(value.begin());
    }
    while (!value.empty() && !not_space(value.back())) {
        value.pop_back();
    }
    return value;
}

inline std::vector<std::string> SplitDsnTokens(const std::string& dsn) {
    std::vector<std::string> tokens;
    std::string current;
    char quote = '\0';
    for (char c : dsn) {
        if (quote) {
            if (c == quote) {
                quote = '\0';
                current.push_back(c);
            } else {
                current.push_back(c);
            }
            continue;
        }
        if (c == '\'' || c == '"') {
            quote = c;
            current.push_back(c);
            continue;
        }
        if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
            if (!current.empty()) {
                tokens.push_back(current);
                current.clear();
            }
            continue;
        }
        current.push_back(c);
    }
    if (!current.empty()) {
        tokens.push_back(current);
    }
    return tokens;
}

inline std::string Unquote(std::string value) {
    value = Trim(value);
    if (value.size() >= 2) {
        char first = value.front();
        char last = value.back();
        if ((first == '\'' && last == '\'') || (first == '"' && last == '"')) {
            return value.substr(1, value.size() - 2);
        }
    }
    return value;
}

inline BackendConfig ParseBackendConfigFromDsn(const std::string& dsn) {
    BackendConfig config;
    for (const auto& token : SplitDsnTokens(dsn)) {
        auto pos = token.find('=');
        if (pos == std::string::npos) {
            continue;
        }
        std::string key = Trim(token.substr(0, pos));
        std::string value = Unquote(token.substr(pos + 1));
        if (key == "host") {
            config.host = value;
        } else if (key == "port") {
            try {
                config.port = std::stoi(value);
            } catch (...) {
            }
        } else if (key == "dbname" || key == "database") {
            config.database = value;
        } else if (key == "user" || key == "username") {
            config.username = value;
        } else if (key == "password" || key == "pass") {
            config.password = value;
        } else if (key == "sslmode") {
            config.sslMode = value;
        } else if (key == "sslrootcert") {
            config.sslRootCert = value;
        } else if (key == "sslcert") {
            config.sslCert = value;
        } else if (key == "sslkey") {
            config.sslKey = value;
        } else if (key == "sslpassword") {
            config.sslPassword = value;
        } else if (key == "options") {
            config.options = value;
        } else if (key == "application_name") {
            config.applicationName = value;
        } else if (key == "role") {
            config.role = value;
        }
    }
    return config;
}

} // namespace test_utils
} // namespace scratchrobin

#endif // SCRATCHROBIN_TESTS_INTEGRATION_CONNECTION_TEST_UTILS_H
