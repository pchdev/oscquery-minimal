#ifndef WPN_TYPES_H
#define WPN_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include <wpn114/alloc.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Used for raw data buffers, such as midi
 * we use unsigned for these */
typedef uint8_t byte_t;

enum wtype_t {
    WTYPE_NIL,      // N
    WTYPE_INT,      // i
    WTYPE_FLOAT,    // f
    WTYPE_CHAR,     // c
    WTYPE_BOOL,     // T/F
    WTYPE_STRING,   // s
    WTYPE_PULSE,    // ..
    WTYPE_VEC2F,    // [ff]
    WTYPE_VEC3F,    // [fff]
    WTYPE_VEC4F,    // [ffff]
    WTYPE_INVALID
};

typedef struct {
    uint16_t usd;
    uint16_t cap;
    char dat[];
} wstr_t;

int
wstr_walloc(struct walloc_t* alloc, wstr_t** dst, uint16_t strlim)
__nonnull((1, 2));

union wvariant_t {
    wstr_t* s;
    float f;
    char c;
    bool b;
    int i;
};

typedef struct {
    union wvariant_t u;
    enum wtype_t t;
} wvalue_t;

#ifdef __cplusplus
}
#endif
#endif
