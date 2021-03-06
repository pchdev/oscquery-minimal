#ifndef WPN_ALLOC_H
#define WPN_ALLOC_H

#include <stdlib.h>
#include <stdint.h>

typedef int (*walloc_fn) (
    void*,      // dst-ptr
    size_t,     // size
    void*       // ptr
);

struct walloc_t {
    walloc_fn alloc;
    walloc_fn free;
    void* data;
};

int
walloc_dynamic(void* dst, size_t nbytes, void*)
__nonnull((1));

int
walloc_memp(void* dst, size_t nbytes, void* memp)
__nonnull((1, 3));

int
wfree_dynamic(void* dst, size_t nbytes, void*)
__nonnull((1));

int
wfree_memp(void* dst, size_t nbytes, void* memp)
__nonnull((1, 3));

#define WPN_MALLOC walloc_dynamic
#define WPN_MEMP walloc_memp

/** Simple memory pool, pointing to a raw byte array.
 * Struct is not presented as an opaque type pointer, for convenience.
 * Do not directly set struct fields, use the proper functions/macros. */
struct wmemp_t {
    unsigned int usd;             // Number of bytes used
    const unsigned int cap;       // Capacity
    uint8_t* const dat;           // Data
};
/* Convenience macro, declares a static memory pool without the need
 * to set the actual byte array manually */
#define wpn_declstatic_mp(_nm, _sz)                     \
    static uint8_t _nm##_u8a[_sz];                      \
    static struct wmemp_t _nm = { 0, _sz, _nm##_u8a }

#define wpn_declstatic_alloc_mp(_nm, _sz)               \
    wpn_declstatic_mp(_nm##_mp, _sz);                   \
    static struct walloc_t _nm = { walloc_memp, wfree_memp, &_nm##_mp };

/** Upfront check if <nbytes> can be allocated from <mp>.
 * Returns remaining free space (negative if there's
 * not enough space). */
extern int
wmemp_chk(struct wmemp_t* mp, size_t nbytes)
__nonnull((1));

/** Requests <nbytes> from mempool <mp>, assigning area to <ptr>.
   Returns free remaning space (in bytes),
   negative if capacity is exceeded, in which case, allocation
   does not occur.*/
extern int
wmemp_req(struct wmemp_t* mp, size_t nbytes, void** ptr)
__nonnull((1, 3));

/** Same as wmemp_req, excepts it memsets allocated space to 0. */
extern int
wmemp_req0(struct wmemp_t* mp, size_t nbytes, void** ptr)
__nonnull((1, 3));

/** Expands local existing memory area, pushing back
 * overlapping areas if any. Returns negative count if failed.
   Returns remaining free space in the pool otherwise. */
extern int
wmemp_exp(struct wmemp_t* mp, void** area, size_t oldsz, size_t newsz)
__nonnull((1, 2));

extern int
wmemp_free(struct wmemp_t* mp, void* area, size_t nbytes);

/** Returns free space (in bytes) remaining in mempool <mp>. */
extern int
wmemp_rmn(struct wmemp_t* mp)
__nonnull((1));

/** Prints remaning free space (in bytes) to stdout, with a wpn header. */
extern void
wmemp_rmnprint(struct wmemp_t* mp)
__nonnull((1));

#endif
