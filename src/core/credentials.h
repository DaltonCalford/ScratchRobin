/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developer-s-public-license-version-1-0/
 */
#ifndef SCRATCHROBIN_CREDENTIALS_H
#define SCRATCHROBIN_CREDENTIALS_H

#include <memory>
#include <string>

namespace scratchrobin {

class CredentialStore {
public:
    virtual ~CredentialStore() = default;
    virtual bool ResolvePassword(const std::string& credentialId,
                                 std::string* outPassword,
                                 std::string* outError) = 0;
    virtual bool StorePassword(const std::string& credentialId,
                               const std::string& password,
                               std::string* outError) = 0;
    virtual bool DeletePassword(const std::string& credentialId,
                                std::string* outError) = 0;
    virtual bool HasPassword(const std::string& credentialId) = 0;
    
    // API Key storage for AI providers
    virtual bool StoreApiKey(const std::string& provider,
                             const std::string& api_key) = 0;
    virtual std::string GetApiKey(const std::string& provider) = 0;
    virtual bool DeleteApiKey(const std::string& provider) = 0;
};

std::unique_ptr<CredentialStore> CreateDefaultCredentialStore();

} // namespace scratchrobin

#endif // SCRATCHROBIN_CREDENTIALS_H
