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

// typetag/ctype        | description
// ---------------------|------------------------------------------------------------------------------------------------
// i: [int32_t]         | 32-bit big-endian two's complement integer.
// t: []                | 64-bit big-endian fixed-point time tag, semantics defined below.
// f: [float]           | 32-bit big-endian IEEE 754 floating point number.
// s: [const char*]     | A sequence of non-null ASCII characters followed by a null, followed by
//                        0-3 additional null characters to make the total number of bits a multiple of 32.
// b: []                | OSC-blob: An int32 size count, followed by that many 8-bit bytes of arbitrary binary
//                        data, followed by 0-3 additional zero bytes to make the total number of bits a multiple of 32.
// c: [char]            | An ASCII character, sent as 32-bits.
// r: []                | 32-bit RGBA color.
// m: [char[[4]]        | 4-bytes MIDI message. Bytes from MSB to LSB are: port id, status, data1, data2
// T: [bool]            | True. No bytes are allocated in the argument data.
// F: [bool]            | False. No bytes are allocated in the argument data.
// N: []                | Nil. No bytes are allocated in the argument data.
// I: []                | Infinitum. No bytes are allocated in the argument data.
// [                    | Indicates the beginning of an array. The tags followed are for data in the Array until
//                        a close brace tag is reached.
// ]                    | Indicates the end on an array.

enum wtype_t {
    WOSC_TYPE_INVALID       = '0',
    WOSC_TYPE_FALSE         = 'F',
    WOSC_TYPE_INFINITUM     = 'I',
    WOSC_TYPE_NIL           = 'N',
    WOSC_TYPE_TRUE          = 'T',
    WOSC_TYPE_BLOB          = 'b',
    WOSC_TYPE_CHAR          = 'c',
    WOSC_TYPE_FLOAT         = 'f',
    WOSC_TYPE_INT           = 'i',
    WOSC_TYPE_MIDI          = 'm',
    WOSC_TYPE_STRING        = 's',
    WOSC_TYPE_TIMETAG       = 't',
    WOSC_TYPE_BOOL,     // T/F
    WOSC_TYPE_VEC2F,    // [ff]
    WOSC_TYPE_VEC3F,    // [fff]
    WOSC_TYPE_VEC4F,    // [ffff]
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
