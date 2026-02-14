/*
 * ScratchRobin
 * Copyright (c) 2025-2026 Dalton Calford
 *
 * Licensed under the Initial Developer's Public License Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at:
 * https://www.firebirdsql.org/en/initial-developers-public-license-version-1-0/
 */
#include "credentials.h"

#include <cstdlib>
#include <cstring>
#include <mutex>
#include <unordered_map>

#ifdef SCRATCHROBIN_USE_LIBSECRET
#include <libsecret/secret.h>
#endif

namespace scratchrobin {
namespace {

enum class EnvLookupResult {
    NotHandled,
    Success,
    Error
};

EnvLookupResult ResolveFromEnv(const std::string& credentialId,
                               std::string* outPassword,
                               std::string* outError) {
    constexpr const char* kEnvPrefix = "env:";
    if (credentialId.rfind(kEnvPrefix, 0) != 0) {
        return EnvLookupResult::NotHandled;
    }

    std::string var = credentialId.substr(std::strlen(kEnvPrefix));
    if (var.empty()) {
        if (outError) {
            *outError = "Empty env credential id";
        }
        return EnvLookupResult::Error;
    }

    const char* value = std::getenv(var.c_str());
    if (!value) {
        if (outError) {
            *outError = "Environment variable not set: " + var;
        }
        return EnvLookupResult::Error;
    }

    if (outPassword) {
        *outPassword = value;
    }
    return EnvLookupResult::Success;
}

#ifdef SCRATCHROBIN_USE_LIBSECRET
const SecretSchema kScratchRobinSchema = {
    "com.scratchbird.scratchrobin",
    SECRET_SCHEMA_NONE,
    {
        { "id", SECRET_SCHEMA_ATTRIBUTE_STRING },
        { nullptr, SECRET_SCHEMA_ATTRIBUTE_STRING }
    }
};
#endif

class DefaultCredentialStore : public CredentialStore {
public:
    bool ResolvePassword(const std::string& credentialId,
                         std::string* outPassword,
                         std::string* outError) override {
        if (!outPassword) {
            if (outError) {
                *outError = "No output buffer for password";
            }
            return false;
        }

        outPassword->clear();
        if (credentialId.empty()) {
            return true;
        }

        EnvLookupResult env_result = ResolveFromEnv(credentialId, outPassword, outError);
        if (env_result == EnvLookupResult::Success) {
            return true;
        }
        if (env_result == EnvLookupResult::Error) {
            return false;
        }

#ifdef SCRATCHROBIN_USE_LIBSECRET
        GError* error = nullptr;
        gchar* password = secret_password_lookup_sync(
            &kScratchRobinSchema,
            nullptr,
            &error,
            "id",
            credentialId.c_str(),
            nullptr
        );

        if (!password) {
            if (outError) {
                if (error && error->message) {
                    *outError = error->message;
                } else {
                    *outError = "Credential not found: " + credentialId;
                }
            }
            if (error) {
                g_error_free(error);
            }
            return false;
        }

        *outPassword = password;
        secret_password_free(password);
        if (error) {
            g_error_free(error);
        }
        return true;
#else
        if (outError) {
            *outError = "No credential backend available for: " + credentialId;
        }
        return false;
#endif
    }

    bool StorePassword(const std::string& credentialId,
                       const std::string& password,
                       std::string* outError) override {
        if (credentialId.empty()) {
            if (outError) {
                *outError = "Empty credential id";
            }
            return false;
        }

        // Don't store env: credentials in keychain
        if (credentialId.rfind("env:", 0) == 0) {
            return true;  // Env credentials are resolved at lookup time
        }

#ifdef SCRATCHROBIN_USE_LIBSECRET
        GError* error = nullptr;
        gboolean result = secret_password_store_sync(
            &kScratchRobinSchema,
            SECRET_COLLECTION_DEFAULT,
            ("ScratchRobin: " + credentialId).c_str(),
            password.c_str(),
            nullptr,
            &error,
            "id",
            credentialId.c_str(),
            nullptr
        );

        if (!result) {
            if (outError) {
                if (error && error->message) {
                    *outError = error->message;
                } else {
                    *outError = "Failed to store credential: " + credentialId;
                }
            }
            if (error) {
                g_error_free(error);
            }
            return false;
        }

        if (error) {
            g_error_free(error);
        }
        return true;
#else
        if (outError) {
            *outError = "No credential backend available";
        }
        return false;
#endif
    }

    bool DeletePassword(const std::string& credentialId,
                        std::string* outError) override {
        if (credentialId.empty()) {
            return true;
        }

#ifdef SCRATCHROBIN_USE_LIBSECRET
        GError* error = nullptr;
        gboolean result = secret_password_clear_sync(
            &kScratchRobinSchema,
            nullptr,
            &error,
            "id",
            credentialId.c_str(),
            nullptr
        );

        if (!result && error) {
            if (outError) {
                *outError = error->message;
            }
            g_error_free(error);
            return false;
        }

        if (error) {
            g_error_free(error);
        }
        return true;
#else
        return true;  // Nothing to delete without backend
#endif
    }

    bool HasPassword(const std::string& credentialId) override {
        if (credentialId.empty()) {
            return false;
        }

        // Check for env: credentials
        if (credentialId.rfind("env:", 0) == 0) {
            std::string var = credentialId.substr(4);
            return std::getenv(var.c_str()) != nullptr;
        }

#ifdef SCRATCHROBIN_USE_LIBSECRET
        GError* error = nullptr;
        gchar* password = secret_password_lookup_sync(
            &kScratchRobinSchema,
            nullptr,
            &error,
            "id",
            credentialId.c_str(),
            nullptr
        );

        bool has_password = (password != nullptr);
        
        if (password) {
            secret_password_free(password);
        }
        if (error) {
            g_error_free(error);
        }
        return has_password;
#else
        return false;
#endif
    }
    
    // API Key storage using a separate schema attribute
    bool StoreApiKey(const std::string& provider,
                     const std::string& api_key) override {
        if (provider.empty() || api_key.empty()) {
            return false;
        }
        
        std::string credentialId = "ai_api_key_" + provider;
        std::string error;
        return StorePassword(credentialId, api_key, &error);
    }
    
    std::string GetApiKey(const std::string& provider) override {
        if (provider.empty()) {
            return "";
        }
        
        std::string credentialId = "ai_api_key_" + provider;
        std::string api_key;
        std::string error;
        
        if (ResolvePassword(credentialId, &api_key, &error)) {
            return api_key;
        }
        return "";
    }
    
    bool DeleteApiKey(const std::string& provider) override {
        if (provider.empty()) {
            return false;
        }
        
        std::string credentialId = "ai_api_key_" + provider;
        std::string error;
        return DeletePassword(credentialId, &error);
    }
};

class MemoryCredentialStore : public CredentialStore {
public:
    bool ResolvePassword(const std::string& credentialId,
                         std::string* outPassword,
                         std::string* outError) override {
        if (!outPassword) {
            if (outError) {
                *outError = "No output buffer for password";
            }
            return false;
        }
        if (credentialId.empty()) {
            if (outError) {
                *outError = "Empty credential id";
            }
            return false;
        }

        std::lock_guard<std::mutex> lock(mu_);
        auto it = store_.find(credentialId);
        if (it == store_.end()) {
            if (outError) {
                *outError = "Credential not found: " + credentialId;
            }
            return false;
        }
        *outPassword = it->second;
        return true;
    }

    bool StorePassword(const std::string& credentialId,
                       const std::string& password,
                       std::string* outError) override {
        if (credentialId.empty()) {
            if (outError) {
                *outError = "Empty credential id";
            }
            return false;
        }
        if (credentialId.rfind("env:", 0) == 0) {
            return true; // Env credentials are resolved at lookup time
        }
        std::lock_guard<std::mutex> lock(mu_);
        store_[credentialId] = password;
        return true;
    }

    bool DeletePassword(const std::string& credentialId,
                        std::string* /*outError*/) override {
        if (credentialId.empty()) {
            return true;
        }
        std::lock_guard<std::mutex> lock(mu_);
        store_.erase(credentialId);
        return true;
    }

    bool HasPassword(const std::string& credentialId) override {
        if (credentialId.empty()) {
            return false;
        }
        if (credentialId.rfind("env:", 0) == 0) {
            std::string var = credentialId.substr(4);
            return std::getenv(var.c_str()) != nullptr;
        }
        std::lock_guard<std::mutex> lock(mu_);
        return store_.find(credentialId) != store_.end();
    }

    bool StoreApiKey(const std::string& provider,
                     const std::string& api_key) override {
        if (provider.empty() || api_key.empty()) {
            return false;
        }
        std::string credentialId = "ai_api_key_" + provider;
        std::string error;
        return StorePassword(credentialId, api_key, &error);
    }

    std::string GetApiKey(const std::string& provider) override {
        if (provider.empty()) {
            return "";
        }
        std::string credentialId = "ai_api_key_" + provider;
        std::string api_key;
        std::string error;
        if (ResolvePassword(credentialId, &api_key, &error)) {
            return api_key;
        }
        return "";
    }

    bool DeleteApiKey(const std::string& provider) override {
        if (provider.empty()) {
            return false;
        }
        std::string credentialId = "ai_api_key_" + provider;
        std::string error;
        return DeletePassword(credentialId, &error);
    }

private:
    std::mutex mu_;
    std::unordered_map<std::string, std::string> store_;
};

} // namespace

std::unique_ptr<CredentialStore> CreateDefaultCredentialStore() {
    const char* backend = std::getenv("SCRATCHROBIN_CREDENTIALS_BACKEND");
    if (backend && std::string(backend) == "memory") {
        return std::make_unique<MemoryCredentialStore>();
    }
    return std::make_unique<DefaultCredentialStore>();
}

} // namespace scratchrobin
