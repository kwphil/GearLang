#include <iostream>

#include "error.h"

void error(Error err) {
    switch(err) {
    case(ERROR_NONE):
        return;
    case(ERROR_INV_ARGS):
        std::cerr << "Invalid argument";
        break;
    case(ERROR_INV_FILE):
        std::cerr << "Invalid file provided";
        break;
    default:
        std::cerr << "Unknown error occured";
        break;
    }

    exit(err);
}

void error_cond(bool cond, Error err) {
    error(cond ? err : ERROR_NONE);
}
