#ifndef WPN114_OSCQUERY_H
#define WPN114_OSCQUERY_H

#include <pthread.h>
#include <wpn114/network/osc.h>

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

enum wqaccess_t {
    WQNODE_ACCESS_R = 1,
    WQNODE_ACCESS_W = 2,
    WQNODE_ACCESS_RW = 3
};

enum wqflags_t {

    WQTREE_CREATE_INTERMEDIATE,

    /// CRITICAL flag: all messages adressing this node will
    /// transit in TCP instead of UDP, in order to guarantee
    /// its delivery.
    WQNODE_CRITICAL,

    /// READONLY flag: node's values and attributes cannot be
    /// modified from outside.
    WQNODE_READONLY,

    /// WRITEONLY flag: node's values and attributes cannot be
    /// read from outside.
    WQNODE_WRITEONLY,

    /// NOREPEAT flag: when the received value is the same as
    /// the previous one, callback function won't be triggered
    WQNODE_NOREPEAT,

    /// FN_SETPRE flag: value callback function will be triggered
    /// before the value is actually set.
    WQNODE_FN_SETPRE,   
};

typedef struct wqnode wqnode_t;

/** wqnode value callback function type definition.
 * will be called each time a new value is received */
typedef
void (*wqnode_fn) (
      wqnode_t*,    // target-node
      wvalue_t*,    // value reference
      void*         // user-data
);

/** Sets <node> flags. Returns error if incorrect */
extern int
wqnode_set_flags(wqnode_t* node, enum wqflags_t flg)
__nonnull((1));

/** Sets <node> value callback function, which will be
 * called each time a new value is received */
extern void
wqnode_set_fn(wqnode_t* node, wqnode_fn fn, void* udata)
__nonnull((1, 2));

/** Returns <node> access mode (1: R, 2: W, 3: R/W) */
extern int
wqnode_get_access(wqnode_t* node)
__nonnull((1));

/** Returns <node> name */
extern const char*
wqnode_get_name(wqnode_t* nd)
__nonnull((1));

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

/** Allocates <dst> tree from <allocator> */
extern int
wqtree_walloc(struct walloc_t* allocator, wqtree_t** dst)
__nonnull((1));

/** Prints the formatted tree/node strcture to stdout */
extern void
wqtree_print(wqtree_t* tree)
__nonnull((1));

/** Sets tree general flags */
extern int
wqtree_set_flags(wqtree_t* tree, enum wqflags_t flags)
__nonnull((1));

/** Adds node to tree, with <wtp> type and <uri>,
 * sets <dst> to point to the newly created node */
extern int wqtree_addndi(wqtree_t* tree, const char* uri, wqnode_t** dst) __nonnull((1, 2, 3));
extern int wqtree_addndf(wqtree_t* tree, const char* uri, wqnode_t** dst) __nonnull((1, 2, 3));
extern int wqtree_addndb(wqtree_t* tree, const char* uri, wqnode_t** dst) __nonnull((1, 2, 3));
extern int wqtree_addndc(wqtree_t* tree, const char* uri, wqnode_t** dst) __nonnull((1, 2, 3));

// note: if strlim == 0, and in malloc mode, no limit for str in size
extern int wqtree_addnds(wqtree_t* tree, const char* uri, wqnode_t** dst,
                         int strlim) __nonnull((1, 2, 3));

/** Gets node handle from the tree, returns NULL if
 * target could not be found */
extern wqnode_t*
wqtree_get_node(wqtree_t* tree, const char* uri)
__nonnull((1, 2));

/** A handle on an oscquery server data structure */
typedef struct wqserver wqserver_t;

/** Allocates <dst> oscquery server from <allocator> */
extern int
wqserver_walloc(struct walloc_t* allocator, wqserver_t** dst)
__nonnull((1, 2));

/** Expose <tree> of wqnodes on the network */
extern int
wqserver_expose(wqserver_t* server, wqtree_t* tree)
__nonnull((1, 2));

/** Runs the oscquery <server> on <tcpport> and <udpport> */
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
wqclient_walloc(struct walloc_t* alloc, wqclient_t** dst)
__nonnull((1, 2));

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
