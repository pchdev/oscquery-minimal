#ifndef WPN114_OSC_H
#define WPN114_OSC_H

#include <stdint.h>
#include <stdbool.h>
#include <wpn114/types.h>
#include <alloca.h>
#include <string.h>

typedef struct womsg womsg_t;
typedef struct wobdl wobdl_t;

#define womsg_alloca(_ptr)                                   \
    do { *_ptr = (womsg_t*) alloca(_womsg_sizeof());         \
         memset(*_ptr, 0, _womsg_sizeof()); } while (0)

/* used for alloca */
int _womsg_sizeof(void);
int _wobdl_sizeof(void);

const char*
wosc_strerr(int err);

enum wtype_t
wosc_tag2tp(char tag)
__nonnull((1));

int
wosc_checkuri(const char* uri)
__nonnull((1));

int
womsg_decode(womsg_t* dst, byte_t* src, uint32_t len)
__nonnull((1, 2));

/** Only applies for writing messages. For reading,
 * use womsg_decode() directly. */
int
womsg_setbuf(womsg_t* msg, byte_t* buf, uint32_t len)
__nonnull((1, 2));

/** Returns error if the message is in read-only mode.
 * in write mode, sets the message uri, adding the
 * beginning '/' if missing, and checking its overall validity */
int
womsg_seturi(womsg_t* msg, const char* uri)
__nonnull((1));

/** Returns error if the message is in read-only mode.
 * in write mode, this will 'lock' the tag set for the message,
 * ie: it will return an error if the next written value does
 * not match its tag. */
int
womsg_settag(womsg_t* msg, const char* tag)
__nonnull((1, 2));

/** Gets message uri without copying it.
 * Be careful, for it will dangle whenever buffer goes out of scope */
const char*
womsg_geturi(womsg_t* msg)
__nonnull((1));

/** returns negative count if an error has occured. */
int
womsg_getlen(womsg_t* msg)
__nonnull((1));

int
womsg_getcnt(womsg_t* msg)
__nonnull((1));

const char*
womsg_gettag(womsg_t* msg)
__nonnull((1));

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

// we make a small modification, for convenience: b will be boolean, B will be OSC-blob...
int womsg_writei(womsg_t* msg, int32_t value) __nonnull((1));
int womsg_writef(womsg_t* msg, float value) __nonnull((1));
int womsg_writeb(womsg_t* msg, bool value) __nonnull((1));
int womsg_writec(womsg_t* msg, char value) __nonnull((1));
int womsg_writes(womsg_t* msg, const char* str) __nonnull((1, 2));

// TODO:
int womsg_writev(womsg_t* msg, wvalue_t val) __nonnull((1));
int womsg_writeh(womsg_t* msg, int64_t value) __nonnull((1));
int womsg_writet(womsg_t* msg) __nonnull((1));
int womsg_writed(womsg_t* msg, double value) __nonnull((1));
int womsg_writer(womsg_t* msg, int32_t value) __nonnull((1));
int womsg_writen(womsg_t* msg) __nonnull((1));
int womsg_writem(womsg_t* msg, char midi[]) __nonnull((1, 2));
int womsg_writeI(womsg_t* msg) __nonnull((1));

int womsg_readi(womsg_t* msg, int32_t* dst) __nonnull((1));
int womsg_readf(womsg_t* msg, float* dst) __nonnull((1));
int womsg_readb(womsg_t* msg, bool* dst) __nonnull((1));
int womsg_readc(womsg_t* msg, char* dst) __nonnull((1));
int womsg_reads(womsg_t* msg, char** dst) __nonnull((1));
int womsg_readv(womsg_t* msg, wvalue_t* dst) __nonnull((1));

#endif
