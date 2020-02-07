#include <wpn114/network/oscquery.h>
#include <wpn114/utilities.h>
#include <dependencies/mjson/mjson.h>

#define WQNODE_IGNORE 0
#define WQNODE_LISTEN 1

#ifndef WPN114_MAXPATH
    #define WPN114_MAXPATH  256
#endif

#ifndef WPN114_MAXJSON
    #define WPN114_MAXJSON  1024
#endif

const char*
wquery_strerr(int err)
{
    switch (err) {
    case WQUERY_NOERROR:
        return "no error";
    case WQUERY_BINDERR_UDP:
        return "could not bind udp socket "
               "(port might already be in use)";
    case WQUERY_BINDERR_TCP:
        return "could not bind tcp socket "
               "(port might already be in use)";
    case WQUERY_URI_INVALID:
        return "invalid uri";
    default:
        return "unknown error code";
    }
}

int
wstr_walloc(struct walloc_t* _allocator, wstr_t** dst, uint16_t strlim)
{
    int err;
    if ((err = _allocator->alloc(dst,
                sizeof(wstr_t)+strlim,
                _allocator->data)) >= 0) {
        memset(*dst, 0, sizeof(wstr_t)+strlim);
        (*dst)->cap = strlim;
    }
    return err;
}

// ------------------------------------------------------------------------------------------------
// NODE/TREE
// ------------------------------------------------------------------------------------------------

// we want to limit this to 64 bytes
struct wqnode {
    struct wqnode* sib;
    struct wqnode* chd;
    const char* uri;
    wqnode_fn fn;
    void* udt;
    wvalue_t value;
    enum wqflags_t flags;
    int status;
};

static inline int
wqnode_walloc(struct walloc_t* _allocator, wqnode_t** dst)
{
    return _allocator->alloc((void**)dst,
                       sizeof(struct wqnode),
                       _allocator->data);
}

static int
wqnode_seturi(wqnode_t* nd, const char* uri)
{
    int err;
    if (!(err = wosc_checkuri(uri)))
        nd->uri = uri;
    return err;
}

void
wqnode_setfn(wqnode_t* nd, wqnode_fn fn, void* udt)
{
    nd->fn = fn;
    nd->udt = udt;
}

int
wqnode_setfl(wqnode_t* nd, enum wqflags_t fl)
{
    // TODO: check flags first
    nd->flags = fl;
    return 0;
}

static inline int
wqnode_addsib(wqnode_t* nd, wqnode_t* sib)
{
    while (nd->sib)
        nd = nd->sib;
    nd->sib = sib;
    return 0;
}

static inline int
wqnode_addchd(wqnode_t* nd, wqnode_t* chd)
{
    if (nd->chd == NULL) {
        nd->chd = chd;
    } else {
        wqnode_addsib(nd->chd, chd);
    }
    return 0;
}

static __always_inline int
wqnode_checktp(wqnode_t* nd, enum wtype_t tp)
{
    return nd->value.t == tp ? 0 : WQUERY_TYPE_MISMATCH;
}

static int
wqnode_setv(wqnode_t* nd, wvalue_t* v)
{
    int err;
    if (!(err = wqnode_checktp(nd, v->t))) {
        if (nd->fn) {
            if (nd->flags & WQNODE_FN_SETPRE) {
                nd->fn(nd, v, nd->udt);
                nd->value = *v;
            } else {
                nd->value = *v;
                nd->fn(nd, v, nd->udt);
            }
        } else {
            nd->value = *v;
        }
    }
    return err;
}

int
wqnode_seti(wqnode_t* nd, int i)
{
    wvalue_t v = { .t = WOSC_TYPE_INT, .u.i = i };
    return wqnode_setv(nd, &v);
}

int
wqnode_setf(wqnode_t* nd, float f)
{
    wvalue_t v = { .t = WOSC_TYPE_FLOAT, .u.f = f };
    return wqnode_setv(nd, &v);
}

int
wqnode_setc(wqnode_t* nd, char c)
{    
    wvalue_t v = { .t = WOSC_TYPE_CHAR, .u.c = c };
    return wqnode_setv(nd, &v);
}

int
wqnode_setb(wqnode_t* nd, bool b)
{
    wvalue_t v = { .t = WOSC_TYPE_BOOL, .u.b = b };
    return wqnode_setv(nd, &v);
}

int
wqnode_sets(wqnode_t* nd, const char* s)
{
    int err;
    if (!(err = wqnode_checktp(nd, WOSC_TYPE_STRING))) {
        if (strlen(s) > nd->value.u.s->cap)
            return WQUERY_STRBUF_OVERFLOW;
        memset(nd->value.u.s->dat, 0, nd->value.u.s->usd);
        strcpy(nd->value.u.s->dat, s);
        // we have to store it somewhere..
        // so we can't really have a SETPRE call
        if (nd->fn)
            nd->fn(nd, &nd->value, nd->udt);
    }
    return err;
}

int
wqnode_geti(wqnode_t* nd, int* i)
{
    int err;
    if (!(err = wqnode_checktp(nd, WOSC_TYPE_INT)))
        *i = nd->value.u.i;
    return err;
}

int
wqnode_getf(wqnode_t* nd, float* f)
{
    int err;
    if (!(err = wqnode_checktp(nd, WOSC_TYPE_FLOAT)))
        *f = nd->value.u.f;
    return err;
}

int
wqnode_getc(wqnode_t* nd, char* c)
{
    int err;
    if (!(err = wqnode_checktp(nd, WOSC_TYPE_CHAR)))
        *c = nd->value.u.c;
    return err;
}

int
wqnode_gets(wqnode_t* nd, const char** s)
{
    int err;
    if (!(err = wqnode_checktp(nd, WOSC_TYPE_STRING)))
        *s = nd->value.u.s->dat;
    return err;
}

const char*
wqnode_getname(wqnode_t* nd)
{
    int len;
    const char* name;
    len = strlen(nd->uri);
    name = &nd->uri[len-1];
    while (*--name != '/')
        ;
    return ++name;
}

int
wqnode_getaccess(wqnode_t* nd)
{
    if (nd->flags & WQNODE_READONLY)
        return WQNODE_READONLY;
    else if (nd->flags & WQNODE_WRITEONLY)
        return WQNODE_WRITEONLY;
    else
        return 3;
}

static int
wqnode_contents_printj(wqnode_t* nd, char* buf, int len);

static int
wqnode_attr_printj(wqnode_t* nd, const char* attr,
                   char* buf, int len)
{
    if (strcmp(attr, "VALUE") == 0) {
        switch (nd->value.t) {
        // TOOO: check and return proper error if buffer overflow
        case WOSC_TYPE_INT:     return sprintf(buf, "\"%s\": %d", attr, nd->value.u.i);
        case WOSC_TYPE_FLOAT:   return sprintf(buf, "\"%s\": %f", attr, nd->value.u.f);
        case WOSC_TYPE_CHAR:    return sprintf(buf, "\"%s\": %c", attr, nd->value.u.c);
        case WOSC_TYPE_STRING:  return sprintf(buf, "\"%s\": \"%s\"", attr, nd->value.u.s->dat);
        case WOSC_TYPE_BOOL:    return sprintf(buf, "\"%s\": %s", attr, nd->value.u.b ? "true":"false");
        default:            return WQUERY_TYPE_MISMATCH;
        }
    } else if (strcmp(attr, "TYPE") == 0) {
        return sprintf(buf, "\"%s\": %c", attr, nd->value.t);
    } else if (strcmp(attr, "ACCESS") == 0) {
        return sprintf(buf, "\"%s\": %d", attr, wqnode_getaccess(nd));
    } else if (strcmp(attr, "CRITICAL") == 0) {
        if (nd->flags & WQNODE_CRITICAL)
            return sprintf(buf, "\"%s\": true", attr);
        else
            return 0;
    } else if (strcmp(attr, "REPETITION_FILTER") == 0) {
        if (nd->flags & WQNODE_NOREPEAT)
            return sprintf(buf, "\"%s\": true", attr);
        else
            return 0;
    }
    return WQUERY_ATTR_UNSUPPORTED;
}

static int
wqnode_printj(wqnode_t* nd, char* buf, int len)
{
    const char* name;
    int lenp;
    name = wqnode_getname(nd);
    lenp = strlen(name)
         + strlen(nd->uri)
         + 25;
    if (lenp >= len)
        return WQUERY_JBUF_OVERFLOW;
    else
        sprintf(buf, "{ \"%s\": { \"FULL_PATH\": %s", name, nd->uri);
    if (nd->value.t != WOSC_TYPE_NIL) {
        if ((lenp += wqnode_attr_printj(nd, "TYPE", buf+lenp, len)) >= len ||
            (lenp += wqnode_attr_printj(nd, "VALUE", buf+lenp, len) >= len) ||
            (lenp += wqnode_attr_printj(nd, "ACCESS", buf+lenp, len) >= len) ||
            (lenp += wqnode_attr_printj(nd, "CRITICAL", buf+lenp, len) >= len) ||
            (lenp += wqnode_attr_printj(nd, "REPETITION_FILTER", buf+lenp, len) >= len)) {
            return WQUERY_JBUF_OVERFLOW;
        }
    }
    if ((lenp += wqnode_contents_printj(nd, buf+lenp, len)) >= len)
        return WQUERY_JBUF_OVERFLOW;
    strcat(buf, " }");
    return 0;
}

static int
wqnode_contents_printj(wqnode_t* nd, char* buf, int len)
{
    int err;
    if (!nd->chd)
        return 0;
    // todo:
    // if there are any intermediate containers, print them as well..
    strcat(buf, "\"CONTENTS\": ");
    wqnode_t* child;
    child = nd->chd;
    err = wqnode_printj(nd, buf, len);
    while (child->sib) {
        err = wqnode_printj(child->sib, buf, len);
        child = child->sib;
    }
    return err;
}

struct wqtree {
    struct wqnode root;
    struct walloc_t* alloc;
    int flags;
};

int
wqtree_walloc(struct walloc_t* _allocator, wqtree_t** _dst)
{
    int err;
    if ((err = _allocator->alloc(_dst, sizeof(struct wqtree),
               _allocator->data)) >= 0) {
        (*_dst)->alloc = _allocator;
        err = 0;
    }
    return err;
}

static inline void
_uricatnext(const char* uri, char* str)
{
    *str++ = '/';
    uri++;
    while (*uri != '/')
        *str++ = *uri++;
}

static inline wqnode_t*
wqnode_getsib(wqnode_t* target, const char* sib)
{
    while (target->sib && strcmp(target->sib->uri, sib))
           target = target->sib;
    return target;
}

static inline wqnode_t*
wqnode_getchd(wqnode_t* target, const char* chd)
{
    if (target->chd == NULL)
        return target->chd;
    if (strcmp(target->chd->uri, chd))
        return wqnode_getsib(target->chd, chd);
    else
        return target->chd;
}

static wqnode_t*
wqtree_getparent(wqtree_t* tree, const char* uri)
{
    wqnode_t* target;
    wqnode_t* child;
    wstr_t* str;
    int len, err;
    target = &tree->root;
    len = strlen(uri);
    // we're looking for /foo/bar/int parent
    // look for /foo first
    // if /foo exists, set target to /foo and look for /foo/bar
    // if /foo doesn't exist, set it to root
    // we have to allocate a string here, of the same size as uri
    if ((err = wstr_walloc(tree->alloc, &str, len)) < 0) {
        wpnerr("could not allocate temporary string storage, aborting...\n");
        return NULL;
    }
    while ((child = wqnode_getchd(target, str->dat))) {
           _uricatnext(uri, str->dat);
           target = child;
    }    
    tree->alloc->free((void**)str, sizeof(wstr_t)+str->cap,
                      tree->alloc->data);
    return target;
}

int
wqtree_addnd(wqtree_t* tree, const char* uri,
             enum wtype_t type, wqnode_t** dst)
{
    int err = 0;
    wqnode_t* parent, *nd;
    if (wosc_checkuri(uri))
        return WQUERY_URI_INVALID;
    if ((err = wqnode_walloc(tree->alloc, &nd)) < 0) {
        wpnerr("walloc failed");
        return err;
    }
    if ((parent = wqtree_getparent(tree, uri)) == NULL) {
        wpnerr("getparent failed");
        return 43; // TODO: add proper error code: not enough memory space
    }
    nd->value.t = type;
    wqnode_seturi(nd, uri);
    wqnode_addchd(parent, nd);
    *dst = nd;
    return 0;
}

int
wqtree_addndi(wqtree_t* tree, const char* uri, wqnode_t** dst)
{
    return wqtree_addnd(tree, uri, WOSC_TYPE_INT, dst);
}

int
wqtree_addndf(wqtree_t* tree, const char* uri, wqnode_t** dst)
{
    return wqtree_addnd(tree, uri, WOSC_TYPE_FLOAT, dst);
}

int
wqtree_addndb(wqtree_t* tree, const char* uri, wqnode_t** dst)
{
    return wqtree_addnd(tree, uri, WOSC_TYPE_BOOL, dst);
}

int
wqtree_addndc(wqtree_t* tree, const char* uri, wqnode_t** dst)
{
    return wqtree_addnd(tree, uri, WOSC_TYPE_CHAR, dst);
}

int
wqtree_addnds(wqtree_t* tree, const char* uri,
              wqnode_t** dst, int strlim)
{
    int err;
    wstr_t* str;
    if ((err = wqtree_addnd(tree, uri, 's', dst)))
        return err;    
    if ((err = wstr_walloc(tree->alloc, &str, strlim)) >= 0) {
        str->cap = strlim;
        (*dst)->value.u.s = str;
        err = 0;
    }
    return err;
}

static wqnode_t*
wqtree_getnd_rec(wqnode_t* target, const char* uri, int len, int thresh)
{
    int spn;
    if ((spn = strspn(target->uri, uri)) == len) {
        // target found
        return target;
    } else if (spn > thresh) {
        // we have a partial identification (intermediate node)
        // do the same with children
        target = wqtree_getnd_rec(target->chd, uri, len, spn);
    } else {
        // no identification, try with siblings
        while (target)
            target = wqtree_getnd_rec(target->sib, uri, len, spn);
    }
    return target;
}

wqnode_t*
wqtree_getnd(wqtree_t* tree, const char* uri)
{
    if (wosc_checkuri(uri)) {
        wpnerr("uri: %s, incorrect format\n", uri);
        return NULL;
    }    
    wqnode_t* target;
    int len;
    target = &tree->root;
    len = strlen(uri);
    if (len > 1) {
        int len;
        target = target->chd;
        len = strlen(uri);
        // intermediate nodes are not necessarily created
        // e.g.: /foo/bar/float will be a child of root (/)
        // if we follow the test example
        // that means that 'foo' and 'bar' nodes are omitted in that case
        target = wqtree_getnd_rec(target, uri, len, 1);
    }
    return target;
}

static int
wqnode_update(wqnode_t* nd, womsg_t* womsg)
{
    int err;
    enum wtype_t type;
    type = *womsg_gettag(womsg);
    if (!(err = wqnode_checktp(nd, type))) {
        if (type == WOSC_TYPE_STRING) {
            // todo: errcheck
            womsg_readv(womsg, &nd->value);
            if (nd->fn)
                nd->fn(nd, &nd->value, nd->udt);
        } else {
            wvalue_t v;
            womsg_readv(womsg, &v);
            wqnode_setv(nd, &v);
        }
    }
    return err;
}

static int
wqtree_update_osc(wqtree_t* tree, byte_t* data, int len)
{
    int err;
    womsg_t* womsg;
    womsg_alloca(&womsg);
    if ((err = womsg_decode(womsg, data, len))) {
        wpnerr("decoding incoming OSC message (%s)\n",
               wosc_strerr(err));
        return err;
    } else {
        const char* uri;
        wqnode_t* target;
        uri = womsg_geturi(womsg);
        if ((target = wqtree_getnd(tree, uri)) == NULL)
            return WQUERY_URI_INVALID;
        return wqnode_update(target, womsg);
    }
}

// ------------------------------------------------------------------------------------------------
// NETWORK
// ------------------------------------------------------------------------------------------------

#include <dependencies/mongoose/mongoose.h>

#define HTTP_OK             200
#define HTTP_NO_CONTENT     204
#define HTTP_BAD_REQUEST    400
#define HTTP_FORBIDDEN      403
#define HTTP_NOT_FOUND      404
#define HTTP_REQ_TIME_OUT   408

#define HTTP_MIME           "Content-Type: "
#define HTTP_MIME_JSON      HTTP_MIME "application/json"

#ifndef WPN114_MAXCN
    #define WPN114_MAXCN    1
#endif

struct wqconnection {
    struct mg_connection* tcp;
    int udp;
};

// ------------------------------------------------------------------------------------------------
// SERVER
// ------------------------------------------------------------------------------------------------

struct wqserver {
    struct mg_mgr mgr;
    struct wqconnection cn[WPN114_MAXCN];
    struct wqtree* tree;
#ifdef WPN114_MULTITHREAD
    pthread_t thread;
#endif
    bool running;
    uint16_t uport;
};

int
wqserver_walloc(struct walloc_t* _allocator, wqserver_t** dst)
{
    return _allocator->alloc((void**)dst,
           sizeof(struct wqserver),
           _allocator->data);
}

void
wqserver_zro(wqserver_t* server)
{
    memset(server, 0, sizeof(struct wqserver));
    mg_mgr_init(&server->mgr, server);
}

int
wqserver_expose(wqserver_t* server, wqtree_t* tree)
{
    server->tree = tree;
    return 0;
}

static struct wqconnection*
wqserver_getcn(wqserver_t* server, struct mg_connection* mgc)
{
    int n;
    for (n = 0; n < WPN114_MAXCN; ++n)
         if (server->cn[n].tcp == mgc)
             return &server->cn[n];
    return NULL;
}

// better to omit fields that are 'false'?
static const char*
s_host_ext =
    "{ "
        "\"ACCESS\": true,"
        "\"VALUE\": true,"
        "\"CRITICAL\": true,"
        "\"LISTEN\": true,"
//        "\"DESCRIPTION\": false,"
//        "\"TAGS\": false,"
//        "\"EXTENDED_TYPE\": false,"
//        "\"UNIT\": false,"
//        "\"CLIPMODE\": false,"
//        "\"PATH_CHANGED\": false,"
//        "\"PATH_REMOVED\": false,"
//        "\"PATH_ADDED\": false,"
//        "\"PATH_RENAMED\": false,"
//        "\"HTML\": false,"
//        "\"ECHO\": false, "
        "\"OSC_STREAMING\": true"
    "}";

static inline int
wqserver_reply_json(struct mg_connection* mgc,
                    const char* json)
{
    mg_send_head(mgc, HTTP_OK, strlen(json), HTTP_MIME_JSON);
    mg_printf(mgc, "%.*s", (int)strlen(json), json);
    return 0;
}

static void
wqserver_tcp_hdl(struct mg_connection* mgc, int event, void* data)
{
    wqserver_t* server = mgc->mgr->user_data;
    switch (event) {
    case MG_EV_RECV: {
        break;
    }
    case MG_EV_WEBSOCKET_HANDSHAKE_DONE: {
        int nc;
        for (nc = 0; nc < WPN114_MAXCN; ++nc) {
            if (server->cn[nc].tcp == NULL) {
                server->cn[nc].tcp = mgc;
                break;
            }
        }
        break;
    }
    case MG_EV_WEBSOCKET_FRAME: {
        struct websocket_message* wm = data;
        if (wm->flags & WEBSOCKET_OP_TEXT) {
            // parse json
            char cmd[32];
            mjson_get_string((char*)wm->data, wm->size,
                             "$.COMMAND", cmd, sizeof(cmd));

            if (strcmp(cmd, "LISTEN") == 0) {
                char path[WPN114_MAXPATH];
                wqnode_t* target;
                mjson_get_string((char*)wm->data, wm->size,
                                 "$.DATA", path, sizeof(path));
                if ((target = wqtree_getnd(server->tree, path))) {
                    target->status = WQNODE_LISTEN;
                }
            } else if (strcmp(cmd, "IGNORE") == 0) {
                char path[WPN114_MAXPATH];
                wqnode_t* target;
                mjson_get_string((char*)wm->data, wm->size,
                                 "$.DATA", path, sizeof(path));
                if ((target = wqtree_getnd(server->tree, path))) {
                    target->status = WQNODE_IGNORE;
                }
            } else if (strcmp(cmd, "START_OSC_STREAMING") == 0) {
                struct wqconnection* wqc;
                double port;
                mjson_get_number((char*)wm->data, wm->size,
                                 "$.DATA.LOCAL_SERVER_PORT", &port);
                if ((wqc = wqserver_getcn(server, mgc)))
                    wqc->udp = port;
            }
        } else if (wm->flags & WEBSOCKET_OP_BINARY) {
            // parse OSC
            wqtree_update_osc(server->tree, wm->data, wm->size);
        }
        break;
    }
    case MG_EV_HTTP_REQUEST: {
        struct http_message* hm = data;
        wqnode_t* target;
        if (hm->uri.len == 1)
            target = &server->tree->root;
        else {
            char* uri;
            int err;
            if ((err = server->tree->alloc->alloc((void**)&uri,
                       WPN114_MAXPATH,
                       server->tree->alloc->data))) {
                wpnerr("could not allocate temporary string storage required"
                       "for http replying\n");
                assert(false);
            }
            memcpy(uri, hm->uri.p, hm->uri.len);
            uri[hm->uri.len] = 0;
            target = wqtree_getnd(server->tree, uri);
        }
        if (hm->query_string.len) {
            if (strcmp(hm->query_string.p, "HOST_INFO") == 0) {
                // we should use a static memory pool here
                char buf[WPN114_MAXJSON];
                int err;
                err = mjson_printf(&mjson_print_fixed_buf, buf,
                                   "{%Q:%s, %Q:%d, %Q:%s, %Q:%s}",
                      "NAME", "wqserver",
                      "OSC_PORT", server->uport,
                      "OSC_TRANSPORT", "UDP",
                      "EXTENSIONS", s_host_ext);
                wqserver_reply_json(mgc, buf);
                break;
            }
            // query attribute
            // we don't expect it to be too large, maybe 128 bytes would be enough
            char buf[128];
            wqnode_attr_printj(target, hm->query_string.p, buf, 128);
            wqserver_reply_json(mgc, buf);
        } else {
            // query all, including subnodes
            char buf[WPN114_MAXJSON];
            wqnode_printj(target, buf, WPN114_MAXJSON);
            wqserver_reply_json(mgc, buf);
        }
        break;
    }
    case MG_EV_CLOSE: {
        if (mgc->flags & MG_F_IS_WEBSOCKET) {
            struct wqconnection* wqc;
            if ((wqc = wqserver_getcn(server, mgc)))
                memset(wqc, 0, sizeof(struct wqconnection));
            else
                wpnerr("couldn't find wqconnection...\n");
        }
    }
    }
}

static void
wqserver_udp_hdl(struct mg_connection* mgc, int event, void* data)
{
    wqserver_t* server = mgc->mgr->user_data;
    (void) data;
    switch (event) {
    case MG_EV_RECV: {
        wqtree_update_osc(server->tree,
            (byte_t*) mgc->recv_mbuf.buf,
            mgc->recv_mbuf.len);
    }
    }
}

static void*
wqserver_pthread_run(void* v)
{
    wqserver_t* server = v;
    while (server->running) {
        mg_mgr_poll(&server->mgr, 200);
    }
    return 0;
}

int
wqserver_run(wqserver_t* server, uint16_t udpport, uint16_t wsport)
{
    char s_tcp[8], s_udp[8];
    char udp_hdr[16] = "udp://";
    struct mg_connection* c_tcp, *c_udp;
    server->uport = udpport;
    sprintf(s_tcp, "%d", wsport);
    sprintf(s_udp, "%d", udpport);
    strcat(udp_hdr, s_udp);
    if ((c_tcp = mg_bind(&server->mgr, s_tcp,
                 wqserver_tcp_hdl)) == NULL) {
        return WQUERY_BINDERR_TCP;
    }
    mg_set_protocol_http_websocket(c_tcp);
    if ((c_udp = mg_bind(&server->mgr, udp_hdr,
                 wqserver_udp_hdl)) == NULL) {
        return WQUERY_BINDERR_UDP;
    }
    server->running = true;
#ifdef WPN114_MULTITHREAD
    pthread_create(&server->thread, 0, wqserver_pthread_run, server);
#endif
    return 0;
}

int
wqserver_iterate(wqserver_t* server, int ms)
{
    return mg_mgr_poll(&server->mgr, ms);
}

int
wqserver_stop(wqserver_t* server)
{
    server->running = false;
#ifdef WPN114_MULTITHREAD
    pthread_join(server->thread, 0);
#endif
    return 0;
}

// ------------------------------------------------------------------------------------------------
// CLIENT
// ------------------------------------------------------------------------------------------------

struct wqclient {
    struct mg_mgr mgr;
    struct wqconnection cn;
    struct wqtree tree;
    pthread_t thread;    
    bool running;
};

int
wqclient_walloc(struct walloc_t* _allocator, wqclient_t** dst)
{
    return _allocator->alloc((void**)dst,
            sizeof(struct wqclient),
            _allocator->data);
}

void
wqclient_zro(wqclient_t* client)
{
    memset(client, 0, sizeof(struct wqclient));
    mg_mgr_init(&client->mgr, client);
}

static void*
wqclient_pthread_run(void* v)
{
    wqclient_t* client = v;
    while (client->running) {
        mg_mgr_poll(&client->mgr, 200);
    }
    return 0;
}

static void
wqclient_tcp_hdl(struct mg_connection* mgc, int event, void* data)
{
    wqclient_t* cli = mgc->mgr->user_data;
    switch (event) {
    case MG_EV_CONNECT:
    case MG_EV_POLL: break;
    case MG_EV_WEBSOCKET_HANDSHAKE_DONE: {
        // once handshake is done, request both host_info & json tree
        char addr[32], port[8], url[64];
        mg_sock_addr_to_str(&cli->cn.tcp->sa, addr, sizeof(addr), MG_SOCK_STRINGIFY_IP);
        mg_sock_addr_to_str(&cli->cn.tcp->sa, port, sizeof(port), MG_SOCK_STRINGIFY_PORT);
        sprintf(url, "ws://%s:%s/", addr, port);
        mg_connect_http(&cli->mgr, wqclient_tcp_hdl, url, NULL, NULL);
        strcat(url, "?HOST_INFO");
        mg_connect_http(&cli->mgr, wqclient_tcp_hdl, url, NULL, NULL);
        break;
    }
    case MG_EV_WEBSOCKET_FRAME: {
        struct websocket_message* wm = data;
        if (wm->flags & WEBSOCKET_OP_TEXT) {
            // we shouldn't receive anything here
            wpnerr("dave, this is highly irregular..");
        } else if (wm->flags & WEBSOCKET_OP_BINARY) {
            wqtree_update_osc(&cli->tree, wm->data, wm->size);
        }
        break;
    }
    case MG_EV_HTTP_REPLY: {
        struct http_message* hm = data;
        if (strcmp(hm->query_string.p, "HOST_INFO") == 0) {
            char cmd[128];
            double uport;
            int err;
            mjson_get_number(hm->body.p, hm->body.len, "$.OSC_PORT", &uport);
            cli->cn.udp = uport;
            // send back confirmation message, with our own udp port
            err = mjson_printf(&mjson_print_fixed_buf, cmd,
                               "{%Q:%s, %Q:{%Q:%d, %Q:%d}}",
                               "COMMAND", "START_OSC_STREAMING",
                               "DATA",
                               "LOCAL_SERVER_PORT", 1234,
                               "LOCAL_SENDER_PORT", 0);
            mg_send_websocket_frame(mgc, WEBSOCKET_OP_TEXT,
                                    cmd, strlen(cmd));
        }
        break;
    }
    case MG_EV_CLOSE: {
        break;
    }
    }

}

static void
wqclient_udp_hdl(struct mg_connection* mgc, int event, void* data)
{
    wqclient_t* cli = mgc->mgr->user_data;
    switch (event) {
    case MG_EV_RECV: {
        wqtree_update_osc(&cli->tree,
            (byte_t*) mgc->recv_mbuf.buf, mgc->recv_mbuf.len);
        break;
    }
    }
}

int
wqclient_connect(wqclient_t* client, const char* addr, uint16_t port)
{
    char waddr[32];
    struct mg_connection* mgct, *mgcu;
    sprintf(waddr, "%s:%d", addr, port);
    if ((mgcu = mg_bind(&client->mgr, "udp://1234", wqclient_udp_hdl)) == NULL)
        return WQUERY_BINDERR_UDP;
    if ((mgct = mg_connect_ws(&client->mgr, wqclient_tcp_hdl, waddr, NULL, NULL)) == NULL)
        return WQUERY_BINDERR_TCP;
    client->cn.tcp = mgct;
    client->running = true;
#ifdef WPN114_MULTITHREAD
    pthread_create(&client->thread, 0, wqclient_pthread_run, client);
#endif
    return 0;
}

int
wqclient_iterate(wqclient_t* client, int ms)
{
    return mg_mgr_poll(&client->mgr, ms);
}

int
wqclient_disconnect(wqclient_t* client)
{
    client->running = false;
#ifdef WPN114_MULTITHREAD
    pthread_join(client->thread, 0);
#endif
    return 0;
}
