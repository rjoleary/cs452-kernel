#include <user/err.h>

static const char *strs[] = {
    // ERR_OK
    "ok",

    // ERR_BADARG
    "bad argument",

    // ERR_NORES
    "no resource available",

    // ERR_TRUNC
    "truncated",

    // ERR_INVID
    "invalid id",

    // ERR_BADITC
    "could not complete inter-task communcation",

    // ERR_CORRUPT
    "corrupted volatile data",
};

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

const char *err2str(int err) {
    if (err < 0) {
        err = -err;
    }

    if (err >= ERR_UNKN || err >= ARRAY_SIZE(strs)) {
        return "unknown error";
    }

    return strs[err];
}
