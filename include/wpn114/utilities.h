#ifndef WPN_UTILITIES_H
#define WPN_UTILITIES_H

#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <wpn114/wpn114.h>

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

/* This is one of the things that is quite arbitrary
   this is based on alsa alloca system, which basically implies
   the definition of a struct##_sizeof function
   and a typedef struct_t */
#define _wpn_alloca(ptr,type)                                                       \
    do { *ptr = (type##_t *) alloca(type##_sizeof());                               \
         memset(*ptr, 0, type##_sizeof()); } while (0)

#ifdef __GNUC__
    #ifndef __clang__
        #define _wpn_attr_packed  __attribute__((__packed__))
        #else
        #define _wpn_attr_packed
    #endif
#else
    #define _wpn_attr_packed
#endif

inline void
wpn_help(void)
{
wpnout(
"usage: [OPTION]...\n"
"-h, --help                 | help\n"
"-v, --version              | prints current software version\n"
"-f, --file                 | specify *.wpn configuration file\n"
"-i, --input                | selects alsa capture device\n"
"-o, --output               | select alsa playback device\n"
"-s, --samplerate           | specify global (capture/playback) sample rate\n"
"-b, --buffer-size          | specify global (capture/playback) io buffer size\n"
"-ic, --input-channels      | specify number of channels for input/capture\n"
"-oc, --output-channels     | specify number of channels for output/playback\n");
}

inline void
wpn_version(void)
{
    wpnout("version: "WPN114_VSTR"\n");
}

inline bool
wprompt_yn(bool df)
{
    char in[4];
    fgets(in, 4, stdin);
    if (in[0] == '\n')
        return df;
    else if (in[0] == 'y' ||
             in[0] == 'Y')
        return true;
    else return false;
}

#define wcolor_wgreen       "\033[0;32m"
#define wcolor_wgreenb      "\033[1;32m"
#define wcolor_rst          "\033[0m"

inline void
wprompt_color_green(void)
{
    printf("\033[0;32m");
}

inline void
wprompt_color_green_bold(void)
{
    printf("\033[1;32m");
}

inline void
wprompt_color_reset(void)
{
    printf("\033[0m");
}

#endif
