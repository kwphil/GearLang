#ifndef ERROR_H
#define ERROR_H

enum Error {
    ERROR_NONE,
    ERROR_INV_ARGS,
    ERROR_INV_FILE,
};

void error(Error);
void error_cond(bool, Error);

#endif
