#ifndef USER_ERR_H__INCLUDED
#define USER_ERR_H__INCLUDED

enum Error {
    ERR_OK = 0,
    ERR_BADARG,
    ERR_NORES,
    ERR_TRUNC,
    ERR_INVID,
    ERR_BADITC,
    ERR_CORRUPT,
    ERR_UNKN,
};

const char *err2str(int err);

#endif // USER_ERR_H__INCLUDED
