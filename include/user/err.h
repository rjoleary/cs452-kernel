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

template <typename T>
class ErrorOr;

template <typename T>
const char *asErrorString(const ErrorOr<T> &errOr) {
    return errorToString(errOr.asError());
}

// Note that negative values of T are considered error.
template <typename T>
class ErrorOr {
    // Must be size 4 to fit in syscall return value.
    static_assert(sizeof(T) <= 4, "ErrorOr needs size of 4 or less");
    int data = 0;

    ErrorOr() = default;

public:
    static ErrorOr fromValue(T data) {
        ErrorOr ret;
        memcpy(&ret.data, &data, sizeof(T));
        return ret;
    }

    static ErrorOr fromError(Error err) {
        ErrorOr ret;
        ret.data = -static_cast<int>(err);
        return ret;
    }

    static ErrorOr fromInt(int value) {
        ErrorOr ret;
        ret.data = value;
        return ret;
    }

    static ErrorOr fromBoth(Error err, T value) {
        return fromError(err).replace(value);
    }

    // Return true if value is an error.
    bool isError() const {
        return data < 0;
    }

    // Return the value or assert.
    T asValue() const {
        if (isError()) {
            assert(asErrorString(*this));
        }
        T ret;
        memcpy(&ret, &data, sizeof(T));
        return ret;
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
};
static_assert(sizeof(ErrorOr<Tid>) == sizeof(Tid), "ErrorOr<Tid> != Tid");


// For functions returning void.
template <>
class ErrorOr<void> {
    Error data;
    ErrorOr() = default;

public:
    static ErrorOr fromOk() {
        ErrorOr ret;
        ret.data = Error::Ok;
        return ret;
    }

    static ErrorOr fromError(Error err) {
        ErrorOr ret;
        ret.data = err;
        return ret;
    }

    static ErrorOr fromInt(int value) {
        ErrorOr ret;
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
            assert(asErrorString(*this));
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
};
static_assert(sizeof(ErrorOr<void>) == 4, "ErrorOr<void> not size 4");

// Shortcut for asValue.
template <typename T>
T operator~(ErrorOr<T> e) {
    return e.asValue();
}

// For arbitrary data.
template <typename T>
class ErrorOr2 {
    T data;
    Error err;

    ErrorOr2() = default;

public:
    static ErrorOr2 fromValue(T data) {
        return ErrorOr2 {
            data,
            Error::Ok,
        };
    }

    static ErrorOr2 fromError(Error err) {
        return ErrorOr2 {
            {},
            err,
        };
    }

    static ErrorOr2 fromBoth(Error err, T value) {
        return ErrorOr2 {
            value,
            err,
        };
    }

    // Return true if value is an error.
    bool isError() const {
        return err;
    }

    // Return the value or assert.
    const T &asValue() const {
        return const_cast<ErrorOr2*>(this)->asValue();
    }

    // Return the value or assert.
    T &asValue() {
        if (isError()) {
            assert(errorToString(err));
        }

        T ret;
        memcpy(&ret, &data, sizeof(T));
        return ret;
    }

    // Return the error.
    Error asError() const {
        return err;
    }
};

}
