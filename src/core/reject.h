#pragma once

#include <stdexcept>
#include <string>
#include <string_view>

namespace scratchrobin {

struct RejectPayload {
    std::string code;
    std::string category;
    std::string message;
    std::string surface;
    std::string operation;
    bool retryable = false;
    std::string details;
};

class RejectError : public std::runtime_error {
public:
    explicit RejectError(RejectPayload payload);

    const RejectPayload& payload() const { return payload_; }

private:
    RejectPayload payload_;
};

bool IsValidRejectCodeFormat(std::string_view code);
std::string RejectCategoryForCode(std::string_view code);
RejectError MakeReject(std::string code,
                       std::string message,
                       std::string surface,
                       std::string operation,
                       bool retryable = false,
                       std::string details = "");

}  // namespace scratchrobin
