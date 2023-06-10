#ifndef _H_TIMEHELP
#define _H_TIMEHELP

#include <stddef.h>

const char * getTimeValString(struct timeval tv, const char * format, char * buf, size_t buflen);

#endif
