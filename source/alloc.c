#include <wpn114/alloc.h>
#include <wpn114/utilities.h>


int
walloc_dynamic(void** dst, size_t nbytes, void* data)
{
    (void) data;
    return (*dst = malloc(nbytes)) == NULL;
}

int
walloc_memp(void** dst, size_t nbytes, void* data)
{
    struct wmemp_t* mp = data;
    return wmemp_req(mp, nbytes, dst);
}

int
wfree_dynamic(void** dst, size_t nbytes, void* data)
{
    (void) data;
    free(*dst);
    return 0;
}

int
wfree_memp(void** dst, size_t nbytes, void* data)
{
    int err;
    struct wmemp_t* mp = data;
    if (!(err = wmemp_free(mp, *dst, nbytes)))
        *dst = NULL;
    return err;
}

/* Upfront check if <nbytes> can be allocated from <mp>.
 * Returns remaining free space (negative if there's
 * not enough space */
int
wmemp_chk(struct wmemp_t* mp, size_t nbytes)
{
    size_t nsz = mp->usd+nbytes;
    return (int)(mp->cap-nsz);
}

/* Requests <nbytes> from mempool <mp>, assigning area to <ptr>.
   Returns free remaning space (in bytes),
   negative if capacity is exceeded, in which case, allocation
   does not occur.*/
int
wmemp_req(struct wmemp_t* mp, size_t nbytes, void** ptr)
{
    size_t nsz = mp->usd+nbytes;
    if (nsz <= mp->cap) {
       *ptr = &mp->dat[mp->usd];
        mp->usd += nbytes;
    }
    return (int)(mp->cap-nsz);
}

/* Same as wpn_mpreq, excepts it memsets allocated space to 0 */
int
wmemp_req0(struct wmemp_t* mp, size_t nbytes, void** ptr)
{
    int ret = wmemp_req(mp, nbytes, ptr);
    if (ret >= 0)
        memset(*ptr, 0, nbytes);
    return ret;
}

/* Expands local existing memory area, pushing back
 * overlapping areas if any. Returns negative count if failed.
   Returns remaining free space in the pool otherwise */
int
wmemp_exp(struct wmemp_t* mp, void** area,
          size_t osz, size_t nsz)
{
    return 1;
}

int
wmemp_free(struct wmemp_t* mp, void* area,
           size_t nbytes)
{
    memset(area, 0, nbytes);
    mp->usd -= nbytes;
    return 0;
}

/* Returns free space (in bytes) remaining in mempool <mp> */
int
wmemp_rmn(struct wmemp_t* mp)
{
    return (int)(mp->cap-mp->usd);
}

/* Prints remaning free space (in bytes) to stdout, with a wpn header */
void
wmemp_rmnprint(struct wmemp_t* mp)
{
    wpnout("wmemp [%p] used: %d bytes, remaining capacity: %d bytes\n",
           (void*) mp, mp->usd, wmemp_rmn(mp));
}
