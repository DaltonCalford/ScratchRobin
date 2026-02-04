/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_CAPABILITY_DETECTOR_H
#define SCRATCHROBIN_CAPABILITY_DETECTOR_H

#include "core/connection_backend.h"

#include <string>

namespace scratchrobin {

// Detects server capabilities by querying the database
class CapabilityDetector {
public:
    // Detect capabilities for a connected backend
    static BackendCapabilities DetectCapabilities(ConnectionBackend* backend);
    
    // Get capabilities for a backend type without connecting
    static BackendCapabilities GetStaticCapabilities(const std::string& backendName);
    
    // Parse version string
    static bool ParseVersion(const std::string& version, 
                             int* major, int* minor, int* patch);

private:
    // Detection queries for each capability
    static bool DetectCancelSupport(ConnectionBackend* backend);
    static bool DetectExplainSupport(ConnectionBackend* backend);
    static bool DetectSblrSupport(ConnectionBackend* backend);
    static bool DetectDomainsSupport(ConnectionBackend* backend);
    static bool DetectSequencesSupport(ConnectionBackend* backend);
    static bool DetectTriggersSupport(ConnectionBackend* backend);
    static bool DetectProceduresSupport(ConnectionBackend* backend);
    static bool DetectJobSchedulerSupport(ConnectionBackend* backend);
    static bool DetectTablespacesSupport(ConnectionBackend* backend);
    static bool DetectUserAdminSupport(ConnectionBackend* backend);
    static bool DetectRoleAdminSupport(ConnectionBackend* backend);
    
    // Get server version info
    static std::string DetectServerVersion(ConnectionBackend* backend);
};

// Capability matrix documentation
struct CapabilityMatrix {
    static const char* GetMarkdownTable();
};

} // namespace scratchrobin

#endif // SCRATCHROBIN_CAPABILITY_DETECTOR_H
