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
};

std::unique_ptr<CredentialStore> CreateDefaultCredentialStore();

} // namespace scratchrobin

#endif // SCRATCHROBIN_CREDENTIALS_H
