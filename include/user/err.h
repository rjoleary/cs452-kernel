#pragma once
#include "std.h"
#include "types.h"

namespace ctl {
enum class Error {
    Ok = 0,
    BadArg,
    NoRes,
    Trunc,
    InvId,
    BadItc,
    Corrupt,
    Unkn,
};


// Convert error to string.
inline const char *errorToString(Error err) {
    switch (err) {
        case Error::Ok:      return "ok";
        case Error::BadArg:  return "bad argument";
        case Error::NoRes:   return "no resource available";
        case Error::Trunc:   return "truncated";
        case Error::InvId:   return "invalid id";
        case Error::BadItc:  return "could not complete inter-task communcation";
        case Error::Corrupt: return "corrupted volatile data";
        case Error::Unkn:
        default:             return "unknown error";
    }
}

// Note that negative values of T are considered error.
template <typename T>
class ErrorOr {
    // Must be size 4 to fit in syscall return value.
    static_assert(sizeof(T) == 4);
    int data;

    ErrorOr() {}

public:
    static const ErrorOr<T> fromValue(T data) {
        ErrorOr<T> ret;
        ret.data = *reinterpret_cast<int*>(&data);
        return ret;
    }

    static const ErrorOr<T> fromError(Error err) {
        ErrorOr<T> ret;
        ret.data = -static_cast<int>(err);
        return ret;
    }

    static const ErrorOr<T> fromInt(int value) {
        ErrorOr<T> ret;
        ret.data = value;
        return ret;
    }

    static const ErrorOr<T> fromBoth(Error err, T value) {
        return fromError(err).replace(value);
    }

    // Return true if value is an error.
    bool isError() const {
        return data < 0;
    }

    // Return the value or assert.
    T asValue() const {
        if (isError()) {
            assert(asErrorString());
        }
        return *reinterpret_cast<const T*>(&data);
    }

    // Return the error.
    Error asError() const {
        return isError() ? static_cast<Error>(-data) : Error::Ok;
    }

    // Replace the value if not an error.
    template <typename U>
    ErrorOr<U> replace(U value) const {
        if (isError()) {
            return ErrorOr<U>::fromInt(data);
        }
        return ErrorOr<U>::fromValue(value);
    }

    // Convert error to string.
    const char *asErrorString() const {
        return errorToString(asError());
    }
};
static_assert(sizeof(ErrorOr<Tid>) == sizeof(Tid));

// For functions returning void.
template <>
class ErrorOr<void> {
    Error data;
    ErrorOr() {}

public:
    static const ErrorOr<void> fromOk() {
        ErrorOr<void> ret;
        ret.data = Error::Ok;
        return ret;
    }

    static const ErrorOr<void> fromError(Error err) {
        ErrorOr<void> ret;
        ret.data = err;
        return ret;
    }

    static const ErrorOr<void> fromInt(int value) {
        ErrorOr<void> ret;
        ret.data = static_cast<Error>(value < 0 ? -value : value);
        return ret;
    }

    // Return true if value is an error.
    bool isError() const {
        return data != Error::Ok;
    }

    // Return the value or assert.
    void asValue() const {
        if (isError()) {
            assert(asErrorString());
        }
    }

    // Return the error.
    Error asError() const {
        return data;
    }

    // Replace the value if not an error.
    template <typename T>
    ErrorOr<T> replace(T value) const {
        return ErrorOr<T>::fromError(data).replace(value);
    }

    // Convert error to string.
    const char *asErrorString() const {
        return errorToString(asError());
    }
};
static_assert(sizeof(ErrorOr<void>) == 4);

// Shortcut for asValue.
template <typename T>
T operator~(ErrorOr<T> e) {
    return e.asValue();
}
}
