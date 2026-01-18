#include "credentials.h"

#include <cstdlib>
#include <cstring>

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
};

} // namespace

std::unique_ptr<CredentialStore> CreateDefaultCredentialStore() {
    return std::make_unique<DefaultCredentialStore>();
}

} // namespace scratchrobin
