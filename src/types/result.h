#ifndef SCRATCHROBIN_RESULT_H
#define SCRATCHROBIN_RESULT_H

#include <variant>
#include <string>
#include <stdexcept>
#include <functional>
#include <memory>
#include <optional>

namespace scratchrobin {

// Error types
enum class ErrorCode {
    SUCCESS = 0,
    UNKNOWN_ERROR = 1,
    NOT_FOUND = 2,
    ALREADY_EXISTS = 3,
    PERMISSION_DENIED = 4,
    INVALID_ARGUMENT = 5,
    CONNECTION_FAILED = 6,
    TIMEOUT = 7,
    OUT_OF_MEMORY = 8,
    IO_ERROR = 9,
    PARSE_ERROR = 10,
    VALIDATION_ERROR = 11,
    NETWORK_ERROR = 12,
    DATABASE_ERROR = 13,
    CACHE_ERROR = 14,
    METADATA_ERROR = 15
};

struct Error {
    ErrorCode code;
    std::string message;
    std::optional<std::string> details;

    Error() : code(ErrorCode::UNKNOWN_ERROR), message(""), details(std::nullopt) {}
    Error(ErrorCode c, std::string msg, std::optional<std::string> det = std::nullopt)
        : code(c), message(std::move(msg)), details(std::move(det)) {}

    static Error fromException(const std::exception& e) {
        return Error(ErrorCode::UNKNOWN_ERROR, e.what());
    }
};

// Result type for handling success/error cases
template <typename T>
class Result {
public:
    // Constructors
    Result() = default;

    Result(T value)
        : data_(std::move(value)) {}

    Result(Error error)
        : data_(std::move(error)) {}

    // Success case
    static Result<T> success(T value) {
        return Result<T>(std::move(value));
    }

    // Error cases
    static Result<T> error(ErrorCode code, std::string message) {
        return Result<T>(Error(code, std::move(message)));
    }

    static Result<T> error(std::string message) {
        return Result<T>(Error(ErrorCode::UNKNOWN_ERROR, std::move(message)));
    }

    // Status checks
    bool isSuccess() const {
        return std::holds_alternative<T>(data_);
    }

    bool isError() const {
        return std::holds_alternative<Error>(data_);
    }

    // Value access
    const T& value() const {
        if (!isSuccess()) {
            throw std::runtime_error("Attempted to access value of error result");
        }
        return std::get<T>(data_);
    }

    T& value() {
        if (!isSuccess()) {
            throw std::runtime_error("Attempted to access value of error result");
        }
        return std::get<T>(data_);
    }

    // Error access
    const Error& error() const {
        if (!isError()) {
            throw std::runtime_error("Attempted to access error of success result");
        }
        return std::get<Error>(data_);
    }

    // Safe accessors
    std::optional<std::reference_wrapper<const T>> valueOrNull() const {
        if (isSuccess()) {
            return std::ref(std::get<T>(data_));
        }
        return std::nullopt;
    }

    std::optional<std::reference_wrapper<T>> valueOrNull() {
        if (isSuccess()) {
            return std::ref(std::get<T>(data_));
        }
        return std::nullopt;
    }

    const T& valueOr(const T& defaultValue) const {
        if (isSuccess()) {
            return std::get<T>(data_);
        }
        return defaultValue;
    }

    // Map and transform
    template <typename U>
    Result<U> map(std::function<U(const T&)> func) const {
        if (isSuccess()) {
            try {
                return Result<U>::success(func(std::get<T>(data_)));
            } catch (const std::exception& e) {
                return Result<U>::error(Error::fromException(e));
            }
        }
        return Result<U>(std::get<Error>(data_));
    }

    template <typename U>
    Result<U> flatMap(std::function<Result<U>(const T&)> func) const {
        if (isSuccess()) {
            try {
                return func(std::get<T>(data_));
            } catch (const std::exception& e) {
                return Result<U>::error(Error::fromException(e));
            }
        }
        return Result<U>(std::get<Error>(data_));
    }

    // Error handling
    Result<T> orElse(std::function<Result<T>()> func) const {
        if (isError()) {
            try {
                return func();
            } catch (const std::exception& e) {
                return Result<T>::error(Error::fromException(e));
            }
        }
        return *this;
    }

    void onSuccess(std::function<void(const T&)> func) const {
        if (isSuccess()) {
            func(std::get<T>(data_));
        }
    }

    void onError(std::function<void(const Error&)> func) const {
        if (isError()) {
            func(std::get<Error>(data_));
        }
    }

private:
    std::variant<T, Error> data_;
};

// Void specialization
template <>
class Result<void> {
public:
    // Constructors
    Result() : success_(true) {}

    Result(Error error)
        : success_(false), error_(std::move(error)) {}

    // Success case
    static Result<void> success() {
        return Result<void>();
    }

    // Error cases
    static Result<void> error(ErrorCode code, std::string message) {
        return Result<void>(Error(code, std::move(message)));
    }

    static Result<void> error(std::string message) {
        return Result<void>(Error(ErrorCode::UNKNOWN_ERROR, std::move(message)));
    }

    // Status checks
    bool isSuccess() const {
        return success_;
    }

    bool isError() const {
        return !success_;
    }

    // Error access
    const Error& error() const {
        if (!isError()) {
            throw std::runtime_error("Attempted to access error of success result");
        }
        return error_;
    }

    // Error handling
    Result<void> orElse(std::function<Result<void>()> func) const {
        if (isError()) {
            try {
                return func();
            } catch (const std::exception& e) {
                auto err = Error::fromException(e);
                return Result<void>::error(err.code, err.message);
            }
        }
        return *this;
    }

    void onSuccess(std::function<void()> func) const {
        if (isSuccess()) {
            func();
        }
    }

    void onError(std::function<void(const Error&)> func) const {
        if (isError()) {
            func(error_);
        }
    }

private:
    bool success_;
    Error error_;
};

// Type aliases for common use cases
using ResultVoid = Result<void>;
using ResultString = Result<std::string>;
using ResultInt = Result<int>;
using ResultBool = Result<bool>;

} // namespace scratchrobin

#endif // SCRATCHROBIN_RESULT_H
