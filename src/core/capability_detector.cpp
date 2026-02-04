/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#include "capability_detector.h"

#include <algorithm>
#include <cctype>
#include <regex>
#include <sstream>

namespace scratchrobin {

namespace {

std::string ToLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

bool ExecuteSimpleQuery(ConnectionBackend* backend, const std::string& sql) {
    if (!backend || !backend->IsConnected()) {
        return false;
    }
    
    QueryResult result;
    std::string error;
    return backend->ExecuteQuery(sql, &result, &error);
}

std::string ExecuteScalarQuery(ConnectionBackend* backend, const std::string& sql) {
    if (!backend || !backend->IsConnected()) {
        return "";
    }
    
    QueryResult result;
    std::string error;
    if (backend->ExecuteQuery(sql, &result, &error)) {
        if (!result.rows.empty() && !result.rows[0].empty()) {
            return result.rows[0][0].text;
        }
    }
    return "";
}

} // namespace

BackendCapabilities CapabilityDetector::DetectCapabilities(ConnectionBackend* backend) {
    if (!backend || !backend->IsConnected()) {
        return GetStaticCapabilities("unknown");
    }
    
    BackendCapabilities caps = backend->Capabilities();
    std::string backendName = ToLower(backend->BackendName());
    
    // Get server version
    caps.serverVersion = DetectServerVersion(backend);
    caps.serverType = backendName;
    ParseVersion(caps.serverVersion, &caps.majorVersion, &caps.minorVersion, &caps.patchVersion);
    
    // Backend-specific capability detection
    if (backendName == "postgresql" || backendName == "postgres") {
        caps.supportsCancel = true;
        caps.supportsExplain = true;
        caps.supportsDomains = true;
        caps.supportsSequences = true;
        caps.supportsTriggers = true;
        caps.supportsProcedures = true;  // PostgreSQL 11+
        caps.supportsUserAdmin = true;
        caps.supportsRoleAdmin = true;
        caps.supportsSchemas = true;
        caps.supportsTablespaces = true;
        caps.supportsMultipleDatabases = true;
        caps.supportsSavepoints = true;
        caps.supportsConstraints = true;
        caps.supportsIndexes = true;
        caps.supportsViews = true;
        caps.supportsTempTables = true;
        caps.supportsStreaming = true;
    } else if (backendName == "mysql" || backendName == "mariadb") {
        caps.supportsCancel = true;
        caps.supportsExplain = true;
        caps.supportsSequences = false;  // MySQL 8.0+ has sequences, but rarely used
        caps.supportsTriggers = true;
        caps.supportsProcedures = true;
        caps.supportsUserAdmin = true;
        caps.supportsRoleAdmin = (backendName == "mariadb" || caps.majorVersion >= 8);
        caps.supportsSchemas = true;  // MySQL uses schemas as databases
        caps.supportsTablespaces = true;
        caps.supportsMultipleDatabases = true;
        caps.supportsSavepoints = true;
        caps.supportsConstraints = true;
        caps.supportsIndexes = true;
        caps.supportsViews = true;
        caps.supportsTempTables = true;
        caps.supportsStreaming = true;
    } else if (backendName == "firebird") {
        caps.supportsCancel = true;
        caps.supportsExplain = false;
        caps.supportsDomains = true;
        caps.supportsSequences = true;  // Generators
        caps.supportsTriggers = true;
        caps.supportsProcedures = true;
        caps.supportsUserAdmin = true;
        caps.supportsRoleAdmin = true;
        caps.supportsSchemas = false;  // No schema support
        caps.supportsTablespaces = false;
        caps.supportsMultipleDatabases = true;
        caps.supportsSavepoints = true;
        caps.supportsConstraints = true;
        caps.supportsIndexes = true;
        caps.supportsViews = true;
        caps.supportsTempTables = true;
        caps.supportsStreaming = true;
    } else if (backendName == "native" || backendName == "scratchbird") {
        // ScratchBird native - full feature set
        caps.supportsCancel = true;
        caps.supportsExplain = true;
        caps.supportsSblr = true;
        caps.supportsDomains = true;
        caps.supportsSequences = true;
        caps.supportsTriggers = true;
        caps.supportsProcedures = true;
        caps.supportsJobScheduler = true;
        caps.supportsUserAdmin = true;
        caps.supportsRoleAdmin = true;
        caps.supportsGroupAdmin = true;
        caps.supportsDdlExtract = true;
        caps.supportsDependencies = true;
        caps.supportsSchemas = true;
        caps.supportsTablespaces = true;
        caps.supportsMultipleDatabases = true;
        caps.supportsSavepoints = true;
        caps.supportsConstraints = true;
        caps.supportsIndexes = true;
        caps.supportsViews = true;
        caps.supportsTempTables = true;
        caps.supportsStreaming = true;
        caps.supportsBackup = true;
    } else if (backendName == "mock") {
        // Mock backend - supports everything for testing
        caps.supportsCancel = true;
        caps.supportsExplain = true;
        caps.supportsSblr = true;
        caps.supportsDomains = true;
        caps.supportsSequences = true;
        caps.supportsTriggers = true;
        caps.supportsProcedures = true;
        caps.supportsJobScheduler = true;
        caps.supportsUserAdmin = true;
        caps.supportsRoleAdmin = true;
        caps.supportsGroupAdmin = true;
        caps.supportsDdlExtract = true;
        caps.supportsDependencies = true;
        caps.supportsSchemas = true;
        caps.supportsTablespaces = true;
        caps.supportsMultipleDatabases = true;
        caps.supportsSavepoints = true;
        caps.supportsConstraints = true;
        caps.supportsIndexes = true;
        caps.supportsViews = true;
        caps.supportsTempTables = true;
        caps.supportsStreaming = true;
        caps.supportsBackup = true;
    }
    
    return caps;
}

BackendCapabilities CapabilityDetector::GetStaticCapabilities(const std::string& backendName) {
    BackendCapabilities caps;
    std::string name = ToLower(backendName);
    
    if (name == "postgresql" || name == "postgres") {
        caps.supportsTransactions = true;
        caps.supportsPaging = true;
        caps.supportsExplain = true;
        caps.supportsDomains = true;
        caps.supportsSequences = true;
        caps.supportsTriggers = true;
        caps.supportsProcedures = true;
        caps.supportsUserAdmin = true;
        caps.supportsRoleAdmin = true;
        caps.supportsSchemas = true;
        caps.supportsTablespaces = true;
        caps.supportsMultipleDatabases = true;
        caps.supportsSavepoints = true;
    } else if (name == "mysql" || name == "mariadb") {
        caps.supportsTransactions = true;
        caps.supportsPaging = true;
        caps.supportsExplain = true;
        caps.supportsTriggers = true;
        caps.supportsProcedures = true;
        caps.supportsUserAdmin = true;
        caps.supportsMultipleDatabases = true;
        caps.supportsSavepoints = true;
    } else if (name == "firebird") {
        caps.supportsTransactions = true;
        caps.supportsPaging = true;
        caps.supportsDomains = true;
        caps.supportsSequences = true;
        caps.supportsTriggers = true;
        caps.supportsProcedures = true;
        caps.supportsUserAdmin = true;
        caps.supportsRoleAdmin = true;
        caps.supportsMultipleDatabases = true;
        caps.supportsSavepoints = true;
    } else if (name == "native" || name == "scratchbird") {
        // All features supported
        caps.supportsCancel = true;
        caps.supportsTransactions = true;
        caps.supportsPaging = true;
        caps.supportsSavepoints = true;
        caps.supportsExplain = true;
        caps.supportsSblr = true;
        caps.supportsStreaming = true;
        caps.supportsDdlExtract = true;
        caps.supportsDependencies = true;
        caps.supportsConstraints = true;
        caps.supportsIndexes = true;
        caps.supportsUserAdmin = true;
        caps.supportsRoleAdmin = true;
        caps.supportsGroupAdmin = true;
        caps.supportsJobScheduler = true;
        caps.supportsDomains = true;
        caps.supportsSequences = true;
        caps.supportsTriggers = true;
        caps.supportsProcedures = true;
        caps.supportsViews = true;
        caps.supportsTempTables = true;
        caps.supportsMultipleDatabases = true;
        caps.supportsTablespaces = true;
        caps.supportsSchemas = true;
        caps.supportsBackup = true;
        caps.supportsImportExport = true;
    }
    
    return caps;
}

bool CapabilityDetector::ParseVersion(const std::string& version, 
                                      int* major, int* minor, int* patch) {
    if (!major || !minor || !patch) {
        return false;
    }
    
    *major = 0;
    *minor = 0;
    *patch = 0;
    
    if (version.empty()) {
        return false;
    }
    
    // Try to extract version numbers using regex
    // Matches patterns like: 13.4, 8.0.25, 3.0.7, 15.2 (Debian...)
    std::regex versionRegex(R"((\d+)\.(\d+)(?:\.(\d+))?)");
    std::smatch match;
    
    if (std::regex_search(version, match, versionRegex)) {
        try {
            *major = std::stoi(match[1].str());
            *minor = std::stoi(match[2].str());
            if (match[3].matched && !match[3].str().empty()) {
                *patch = std::stoi(match[3].str());
            }
            return true;
        } catch (...) {
            return false;
        }
    }
    
    return false;
}

std::string CapabilityDetector::DetectServerVersion(ConnectionBackend* backend) {
    if (!backend || !backend->IsConnected()) {
        return "";
    }
    
    std::string backendName = ToLower(backend->BackendName());
    std::string version;
    
    // Try backend-specific version queries
    if (backendName == "postgresql" || backendName == "postgres") {
        version = ExecuteScalarQuery(backend, "SELECT version()");
    } else if (backendName == "mysql" || backendName == "mariadb") {
        version = ExecuteScalarQuery(backend, "SELECT version()");
    } else if (backendName == "firebird") {
        version = ExecuteScalarQuery(backend, 
            "SELECT rdb$get_context('SYSTEM', 'ENGINE_VERSION') FROM rdb$database");
    }
    
    return version;
}

const char* CapabilityMatrix::GetMarkdownTable() {
    return R"(
# Backend Capability Matrix

| Feature | PostgreSQL | MySQL | Firebird | ScratchBird |
|---------|------------|-------|----------|-------------|
| Transactions | ✅ | ✅ | ✅ | ✅ |
| Savepoints | ✅ | ✅ | ✅ | ✅ |
| Cancel Query | ✅ | ✅ | ✅ | ✅ |
| EXPLAIN | ✅ | ✅ | ❌ | ✅ |
| SBLR View | ❌ | ❌ | ❌ | ✅ |
| Domains | ✅ | ❌ | ✅ | ✅ |
| Sequences | ✅ | ⚠️* | ✅ | ✅ |
| Triggers | ✅ | ✅ | ✅ | ✅ |
| Procedures | ✅ | ✅ | ✅ | ✅ |
| Job Scheduler | ❌ | ❌ | ❌ | ✅ |
| User Admin | ✅ | ✅ | ✅ | ✅ |
| Role Admin | ✅ | ⚠️** | ✅ | ✅ |
| Group Admin | ❌ | ❌ | ❌ | ✅ |
| Schemas | ✅ | ✅ | ❌ | ✅ |
| Tablespaces | ✅ | ✅ | ❌ | ✅ |
| Multiple Databases | ✅ | ✅ | ✅ | ✅ |
| DDL Extract | ⚠️ | ⚠️ | ⚠️ | ✅ |
| Dependencies | ⚠️ | ⚠️ | ⚠️ | ✅ |

*MySQL 8.0+ has sequences but they're rarely used
**MySQL 8.0+ has roles

Legend:
- ✅ Full support
- ⚠️ Partial support
- ❌ Not supported
)";
}

} // namespace scratchrobin
