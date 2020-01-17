#ifndef WPN_TYPES_H
#define WPN_TYPES_H

#include <stdint.h>
#include <wpn114/mempool.h>

/* Used for raw data buffers, such as midi
 * we use unsigned for these */
typedef uint8_t byte_t;

enum wtype_t {
    WTYPE_NIL,
    WTYPE_INT,
    WTYPE_FLOAT,
    WTYPE_CHAR,
    WTYPE_BOOL,
    WTYPE_STRING,
    WTYPE_PULSE,
    WTYPE_VEC2F,
    WTYPE_VEC3F,
    WTYPE_VEC4F,
    WTYPE_INVALID
};

typedef struct {
    uint16_t usd;
    uint16_t cap;
    char dat[];
} wstr_t;

int
wstr_malloc(uint16_t cap, wstr_t** dst)
__nonnull((2));

int
wstr_palloc(struct wmemp_t* mp, uint16_t cap, wstr_t** dst)
__nonnull((1, 3));

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

#endif
