#include "core/reject.h"

#include <regex>
#include <utility>

namespace scratchrobin {

namespace {

std::string ComposeMessage(const RejectPayload& payload) {
    return payload.code + ": " + payload.message;
}

int ParseCodeNumber(std::string_view code) {
    if (code.size() != 11) {
        return -1;
    }
    int value = 0;
    for (size_t i = 7; i < code.size(); ++i) {
        char c = code[i];
        if (c < '0' || c > '9') {
            return -1;
        }
        value = value * 10 + (c - '0');
    }
    return value;
}

}  // namespace

RejectError::RejectError(RejectPayload payload)
    : std::runtime_error(ComposeMessage(payload)), payload_(std::move(payload)) {}

bool IsValidRejectCodeFormat(std::string_view code) {
    static const std::regex kPattern(R"(^SRB1-R-[0-9]{4}$)");
    return std::regex_match(code.begin(), code.end(), kPattern);
}

std::string RejectCategoryForCode(std::string_view code) {
    int n = ParseCodeNumber(code);
    if (n < 0) {
        return "conformance";
    }
    if (n >= 3001 && n <= 3202) {
        return "serialization";
    }
    if (n >= 4001 && n <= 4206) {
        return "connectivity";
    }
    if (n >= 5101 && n <= 5507) {
        return "validation";
    }
    if (n >= 6101 && n <= 6303) {
        return "state";
    }
    if (n >= 7001 && n <= 7306) {
        return "capability";
    }
    if (n >= 8201 && n <= 8301) {
        return "authorization";
    }
    if (n >= 9001 && n <= 9003) {
        return "config";
    }
    return "conformance";
}

RejectError MakeReject(std::string code,
                       std::string message,
                       std::string surface,
                       std::string operation,
                       bool retryable,
                       std::string details) {
    RejectPayload payload;
    payload.code = std::move(code);
    payload.category = RejectCategoryForCode(payload.code);
    payload.message = std::move(message);
    payload.surface = std::move(surface);
    payload.operation = std::move(operation);
    payload.retryable = retryable;
    payload.details = std::move(details);
    return RejectError(std::move(payload));
}

}  // namespace scratchrobin
