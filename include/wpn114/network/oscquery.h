#ifndef WPN114_OSCQUERY_H
#define WPN114_OSCQUERY_H

#include <pthread.h>
#include <wpn114/network/osc.h>
#include <wpn114/mempool.h>

#ifdef __cplusplus
extern "C" {
#endif

extern const char*
wquery_strerr(int err);

enum wquery_err {
    WQUERY_NOERROR,
    WQUERY_BINDERR_UDP,
    WQUERY_BINDERR_TCP,
    WQUERY_URI_INVALID,
    WQUERY_TYPE_MISMATCH,
    WQUERY_STRBUF_OVERFLOW,
    WQUERY_JBUF_OVERFLOW,
    WQUERY_ATTR_UNSUPPORTED
};

enum wqflags_t {
    WQNODE_CRITICAL,
    WQNODE_NOREPEAT,
    WQNODE_READONLY,
    WQNODE_WRITEONLY,
    WQNODE_FN_SETPRE,
    WQNODE_FN_SETPOST,
};

typedef struct wqnode wqnode_t;

typedef
void (*wqnode_fn) (
      wqnode_t*,    // target-node
      wvalue_t*,    // value reference
      void*         // user-data
);

extern int
wqnode_setfl(wqnode_t* node, enum wqflags_t flg)
__nonnull((1));

extern void
wqnode_setfn(wqnode_t* node, wqnode_fn fn, void* udata)
__nonnull((1, 2));

extern int wqnode_seti(wqnode_t* node, int i) __nonnull((1));;
extern int wqnode_setf(wqnode_t* node, float f) __nonnull((1));;
extern int wqnode_setc(wqnode_t* node, char c) __nonnull((1));;
extern int wqnode_setb(wqnode_t* node, bool b) __nonnull((1));;
extern int wqnode_sets(wqnode_t* node, const char* s)  __nonnull((1, 2));;

extern int wqnode_geti(wqnode_t* node, int* i) __nonnull((1, 2));
extern int wqnode_getf(wqnode_t* node, float* f) __nonnull((1, 2));;
extern int wqnode_getc(wqnode_t* node, char* c) __nonnull((1, 2));;
extern int wqnode_getb(wqnode_t* node, bool* b) __nonnull((1, 2));;
extern int wqnode_gets(wqnode_t* node, const char** s) __nonnull((1, 2));;

typedef struct wqtree wqtree_t;

extern int
wqtree_malloc(wqtree_t** dst)
__nonnull((1));

extern int
wqtree_palloc(wqtree_t** dst, struct wmemp_t* mp)
__nonnull((1, 2));

extern int
wqtree_addnd(wqtree_t* tree, const char* uri, enum wtype_t wtp, wqnode_t** dst)
__nonnull((1, 2, 4));

extern int wqtree_addndi(wqtree_t* tree, const char* uri, wqnode_t** dst) __nonnull((1, 2, 3));
extern int wqtree_addndf(wqtree_t* tree, const char* uri, wqnode_t** dst) __nonnull((1, 2, 3));
extern int wqtree_addndb(wqtree_t* tree, const char* uri, wqnode_t** dst) __nonnull((1, 2, 3));
extern int wqtree_addndc(wqtree_t* tree, const char* uri, wqnode_t** dst) __nonnull((1, 2, 3));

// note: if strlim == 0, and in malloc mode, no limit for str in size
extern int wqtree_addnds(wqtree_t* tree, const char* uri, wqnode_t** dst,
                         int strlim) __nonnull((1, 2, 3));

extern wqnode_t*
wqtree_getnd(wqtree_t* tree, const char* uri)
__nonnull((1, 2));

typedef struct wqserver wqserver_t;

extern int
wqserver_malloc(wqserver_t** dst)
__nonnull((1));

extern int
wqserver_palloc(wqserver_t** dst, struct wmemp_t* mp)
__nonnull((1, 2));

extern void
wqserver_zro(wqserver_t* server)
__nonnull((1));

extern int
wqserver_expose(wqserver_t* server, wqtree_t* tree)
__nonnull((1, 2));

extern int
wqserver_run(wqserver_t* server, uint16_t udpport, uint16_t tcpport)
__nonnull((1));

extern int
wqserver_iterate(wqserver_t* server, int ms)
__nonnull((1));

extern int
wqserver_stop(wqserver_t* server)
__nonnull((1));

typedef struct wqclient wqclient_t;

extern int
wqclient_malloc(wqclient_t** dst)
__nonnull((1));

extern int
wqclient_palloc(wqclient_t** dst, struct wmemp_t* mp)
__nonnull((1, 2));

extern void
wqclient_zro(wqclient_t* client)
__nonnull((1));

extern int
wqclient_connect(wqclient_t* client, const char* addr, uint16_t port)
__nonnull((1));

extern int
wqclient_iterate(wqclient_t* client, int ms)
__nonnull((1));

extern int
wqclient_disconnect(wqclient_t* client)
__nonnull((1));

#ifdef __cplusplus
}
#endif
#endif
