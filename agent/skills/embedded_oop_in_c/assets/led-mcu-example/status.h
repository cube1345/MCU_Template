#ifndef STATUS_H
#define STATUS_H

enum status {
    STATUS_OK = 0,
    STATUS_ERR_INVALID_ARG = -1,
    STATUS_ERR_INVALID_STATE = -2,
    STATUS_ERR_UNSUPPORTED = -3,
    STATUS_ERR_IO = -4,
};

#endif
