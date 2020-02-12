#ifndef WPN114_OSC_H
#define WPN114_OSC_H

#include <stdint.h>
#include <stdbool.h>
#include <wpn114/types.h>
#include <alloca.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

/* A handle on an OSC message data structure */
typedef struct womsg womsg_t;

/* A handle on an OSC bundle data structure */
typedef struct wobdl wobdl_t;

const char*
wosc_strerr(int err);

/** Checks OSC method/uri for irregularities.
 * Returns zero if format is correct, >0 if incorrect */
int
wuri_check(const char* uri)
__nonnull((1));

/** Returns first segment length of method/uri,
 * counting the initial '/'.
 * e.g.: /foo/bar would return 4 (stopping at '/') */
int
wuri_seglen(const char* uri)
__nonnull((1));

/** Returns max segment length between two methods */
int
wuri_seglen_max(const char* u1, const char* u2)
__nonnull((1, 2));

/** Compares <u1> first method segment with <u2> first method segment.
 * Returns positive/negative if segments are not equal.
 * Sets len to max segment length */
int
wuri_segcmp(const char* u1, const char* u2, int* len)
__nonnull((1, 2));

/** Returns method's depth, e.g. : /foo/bar would return 2 */
int
wuri_depth(const char* uri)
__nonnull((1));

/** Tests equality for method depth with <eq> */
bool
wuri_depth_eq(const char* uri, unsigned int eq)
__nonnull((1));

/** Returns a pointer to the last segment of the method */
const char*
wuri_last(const char* uri)
__nonnull((1));

/** Copies parent method to <buf> */
int
wuri_parent(const char* uri, char* buf)
__nonnull((1, 2));

/// Allocates womsg_t from the stack
#define womsg_alloca(_ptr)                                   \
    do { *_ptr = (womsg_t*) alloca(_womsg_sizeof());         \
         memset(*_ptr, 0, _womsg_sizeof()); } while (0)

/** Allocates <dst> pointer from <allocator> */
int
womsg_walloc(struct walloc_t* allocator, womsg_t** dst)
__nonnull((1));

/* used for alloca */
int _womsg_sizeof(void);
int _wobdl_sizeof(void);

/** Prints OSC message as a raw byte array */
void
womsg_printraw(womsg_t* msg)
__nonnull((1));

/** Decodes a raw byte array osc-encoded message, ready to be
 * read through <dst> message handle */
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

/** Returns message's total length (in bytes) */
int
womsg_getlen(womsg_t* msg)
__nonnull((1));

/** Returns message's total number of arguments */
int
womsg_getargc(womsg_t* msg)
__nonnull((1));

/** Returns message's argument tagline */
const char*
womsg_gettag(womsg_t* msg)
__nonnull((1));

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

#ifdef __cplusplus
}
#endif
#endif
