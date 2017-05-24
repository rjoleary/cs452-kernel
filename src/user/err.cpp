#include <err.h>

namespace ctl {
const char *err2str(int err) {
    if (err < 0) {
        err = -err;
    }
    switch (static_cast<Error>(err)) {
        case Error::Ok:      return "ok";
        case Error::BadArg:  return "bad argument";
        case Error::NoRes:   return "no resource available";
        case Error::Trunc:   return "truncated";
        case Error::InvId:   return "invalid id";
        case Error::BadItc:  return "could not complete inter-task communcation";
        case Error::Corrupt: return "corrupted volatile data";
        case Error::Unkn:
        default:      return "unknown error";
    }
}
}
