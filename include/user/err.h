#pragma once

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

const char *err2str(int err);
}
