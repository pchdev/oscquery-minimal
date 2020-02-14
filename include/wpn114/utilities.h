#ifndef WPN_UTILITIES_H
#define WPN_UTILITIES_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <wpn114/wpn114.h>

#define WCOLOR_REGULAR  "\x1B[0m"
#define WCOLOR_RED      "\x1B[31m"
#define WCOLOR_GREEN    "\x1B[32m"
#define WCOLOR_YELLOW   "\x1B[33m"
#define WCOLOR_BLUE     "\x1B[34m"
#define WCOLOR_MAGENTA  "\x1B[35m"
#define WCOLOR_CYAN     "\x1B[36m"
#define WCOLOR_WHITE    "\x1B[37m"

#define wpnmin(_a, _b) (_a < _b ? _a : _b)
#define wpnmax(_a, _b) (_a > _b ? _a : _b)

#define wpnszof(_tp, _n) (sizeof(_tp)*_n)
#define wpnszof2(_tp, _n1, _n2) (wpnszof(_tp, _n1*_n2))

#define wpnout(_ltl, ...)                                                           \
    fprintf(stdout, "[wpn114] " _ltl, ##__VA_ARGS__)

#define wpnerr(_ltl, ...)                                                           \
    fprintf(stderr, "[wpn114] error\n    %s (line %u):\n    " _ltl,                 \
        __FILE__, __LINE__, ##__VA_ARGS__)

#define wpnwrap(_vlu, _lim)                                                         \
    _vlu = (_vlu > _lim ? _lim : _vlu)

#ifdef __GNUC__
    #ifndef __clang__
        #define _wpn_attr_packed  __attribute__((__packed__))
        #else
        #define _wpn_attr_packed
    #endif
#else
    #define _wpn_attr_packed
#endif

#ifdef __GNUC__
    #define WPN_UNUSED __attribute__ ((unused))
#else
    #define WPN_UNUSED
#endif

#endif
