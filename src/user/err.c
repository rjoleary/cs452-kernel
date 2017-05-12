#include <user/err.h>

const char *err2str(int err) {
    if (err < 0) {
        err = -err;
    }

    switch (err) {
        case ERR_OK:      return "ok";
        case ERR_BADARG:  return "bad argument";
        case ERR_NORES:   return "no resource available";
        case ERR_TRUNC:   return "truncated";
        case ERR_INVID:   return "invalid id";
        case ERR_BADITC:  return "could not complete inter-task communcation";
        case ERR_CORRUPT: return "corrupted volatile data";
        case ERR_UNKN:
        default:          return "unknown error";
    }
}
