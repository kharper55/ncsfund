#ifndef UTIL_H
#define UTIL_H

#include <math.h>
#include <errno.h> // For Zephyr error codes

typedef int nrf_err_t;

#define ENONE    0    // No error
#define EGENERIC 2001 // sys/errno.h defines __ELASTERROR 2000 and specifies custom error codes can be defined starting at 2001. This is a generic error code for use in application where no specific error code applies.

#endif // UTIL_H