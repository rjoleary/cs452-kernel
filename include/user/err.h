#ifndef USER_ERR_H__INCLUDED
#define USER_ERR_H__INCLUDED

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

#endif // USER_ERR_H__INCLUDED
