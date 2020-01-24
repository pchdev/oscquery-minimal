#ifndef WPN_ZEROCONF_H
#define WPN_ZEROCONF_H

#include <wpn114/alloc.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct wzservice wzservice_t;

extern int
wzservice_malloc(wzservice_t** dst)
__nonnull((1));

extern int
wzservice_palloc(struct wmemp_t* mp, wzservice_t** dst)
__nonnull((1,2));

extern int
wzservice_settype(wzservice_t* srv, const char* type)
__nonnull((1, 2));

extern int
wzservice_publish(wzservice_t* srv)
__nonnull((1));

extern int
wzservice_iter(wzservice_t* srv, int ms)
__nonnull((1));

extern int
wzservice_stop(wzservice_t* srv)
__nonnull((1));

typedef struct wzbrowser wzbrowser_t;

extern int
wzbrowser_malloc(wzbrowser_t** dst)
__nonnull((1));

extern int
wzbrowser_palloc(struct wmemp_t* mp, wzbrowser_t** dst)
__nonnull((1, 2));

extern int
wzbrowser_settype(wzbrowser_t* bws, const char* type)
__nonnull((1, 2));

extern int
wzbrowser_addtarget(wzbrowser_t* bws, const char* target)
__nonnull((1, 2));

extern int
wzbrowser_run(wzbrowser_t* bws)
__nonnull((1));

extern int
wzbrowser_iter(wzbrowser_t* bws, int ms)
__nonnull((1));

#ifdef __cplusplus
}
#endif
#endif
