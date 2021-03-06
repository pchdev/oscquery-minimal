#include <wpn114/network/oscquery.h>
#include <wpn114/utilities.h>
#include <dependencies/mjson/mjson.h>
#include <assert.h>

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
        return "invalid uri format, "
               "check invalid characters";
    default:
        return "unsupported error code";
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

#define WQNODE_IGNORE 0
#define WQNODE_LISTEN 1

// we want to limit this to 64 bytes
struct wqnode {
    struct wqnode* sibling;
    struct wqnode* child;
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
    return _allocator->alloc(dst,
            sizeof(struct wqnode),
           _allocator->data);
}

static int
wqnode_set_uri(wqnode_t* nd, const char* uri)
{
    int err;
    if (!(err = wuri_check(uri)))
        nd->uri = uri;
    return err;
}

void
wqnode_set_fn(wqnode_t* nd, wqnode_fn fn, void* udt)
{
    nd->fn  = fn;
    nd->udt = udt;
}

int
wqnode_set_flags(wqnode_t* nd, enum wqflags_t fl)
{
    // TODO: check flags first
    nd->flags = fl;
    return 0;
}

static inline int
wqnode_add_sibling(wqnode_t* node, wqnode_t* sibling)
{
    while (node->sibling)
        node = node->sibling;
    node->sibling = sibling;
//    wpnout(WCOLOR_RED "adding %s as a sibling to %s\n"
//           WCOLOR_REGULAR, sibling->uri, node->uri);
    return 0;
}

static inline int
wqnode_add_child(wqnode_t* parent, wqnode_t* child)
{
    if (parent->child == NULL) {
        parent->child = child;
//        wpnout(WCOLOR_RED "adding node %s to parent %s\n"
//               WCOLOR_REGULAR, child->uri, parent->uri);
    } else {
        wqnode_add_sibling(parent->child, child);
    }
    return 0;
}

bool
wqnode_is_child(wqnode_t* parent, wqnode_t* child)
{
    wqnode_t* target = parent->child;
    while (target) {
        if (target == child)
            return true;
        target = child->sibling;
    }
    return false;
}

static inline bool
wqnode_is_sibling_2(wqnode_t* nd1, wqnode_t* nd2)
{
    while (nd1) {
        if (nd1->sibling == nd2)
            return true;
        nd1 = nd1->sibling;
    }
    return false;
}

bool
wqnode_is_sibling(wqnode_t* nd1, wqnode_t* nd2)
{
    bool ans;
    if ((ans = wqnode_is_sibling_2(nd1, nd2)))
        return ans;
    else
        return wqnode_is_sibling_2(nd2, nd1);
}

static __always_inline int
wqnode_check_type(wqnode_t* nd, enum wtype_t tp)
{
    return nd->value.t == tp ? 0 : WQUERY_TYPE_MISMATCH;
}

static int
wqnode_setv(wqnode_t* nd, wvalue_t* v)
{
    int err;
    if (!(err = wqnode_check_type(nd, v->t))) {
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
    if (!(err = wqnode_check_type(nd, WOSC_TYPE_STRING))) {
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
    if (!(err = wqnode_check_type(nd, WOSC_TYPE_INT)))
        *i = nd->value.u.i;
    return err;
}

int
wqnode_getf(wqnode_t* nd, float* f)
{
    int err;
    if (!(err = wqnode_check_type(nd, WOSC_TYPE_FLOAT)))
        *f = nd->value.u.f;
    return err;
}

int
wqnode_getc(wqnode_t* nd, char* c)
{
    int err;
    if (!(err = wqnode_check_type(nd, WOSC_TYPE_CHAR)))
        *c = nd->value.u.c;
    return err;
}

int
wqnode_gets(wqnode_t* nd, const char** s)
{
    int err;
    if (!(err = wqnode_check_type(nd, WOSC_TYPE_STRING)))
        *s = nd->value.u.s->dat;
    return err;
}

const char*
wqnode_get_name(wqnode_t* nd)
{
    const char* last = wuri_last(nd->uri);
    return ++last;
}

int
wqnode_get_access(wqnode_t* nd)
{
    if (nd->flags & WQNODE_READONLY)
        return WQNODE_ACCESS_R;
    else if (nd->flags & WQNODE_WRITEONLY)
        return WQNODE_ACCESS_W;
    else
        return WQNODE_ACCESS_RW;
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
    if ((err = _allocator->alloc(_dst,
                sizeof(struct wqtree),
               _allocator->data)) >= 0) {
        (*_dst)->flags = 0;
        (*_dst)->alloc = _allocator;
        memset(&(*_dst)->root, 0, sizeof(struct wqnode));
        (*_dst)->root.uri = "/";
        (*_dst)->root.value.t = 'N';
        err = 0;
    }
    return err;
}

void
wqnode_print(struct wqnode* node)
{
    printf("node: %s (%c)\n", node->uri, node->value.t);
    if (node->child)
        wqnode_print(node->child);
    if (node->sibling)
        wqnode_print(node->sibling);
}

void
wqtree_print(struct wqtree* tree)
{
    wqnode_print(&tree->root);
}

wqnode_t*
wqtree_get_node(wqtree_t* tree, const char* uri)
{
    wqnode_t* target = &tree->root;
    const char* utarget, *uuri;
    int offset = 0, lim;
    // if query is root, no need to enter the while loop
    if (strcmp(uri, "/") == 0)
        return target;
    // else, start with root's first child
    target = target->child;
    while (target) {
        // get both uri's maximum segment length
        // and compare the two of them with it
        utarget = target->uri+offset;
        uuri = uri+offset;
        if (wuri_segcmp(utarget, uuri, &lim)) {
            // if no match
            // try with sibling
            target = target->sibling;
        }  else if (lim == strlen(uuri)) {
            // segment matches
            // if this is the last uri segment, return target
            break;
        } else if (target->child) {
            // partial match, follow up with children
            target = target->child;
            offset += lim;
        } else {
            // no more children,
            // node can't be found, return NULL
            return NULL;
        }
    }
    return target;
}

static wqnode_t*
wqnode_get_parent(wqtree_t* tree, const char* uri)
{
    wqnode_t* parent = &tree->root;
    wqnode_t* target;
    const char* utarget, *uuri;
    int offset = 0, lim;
    // if depth == 1 (e.g. /foo), parent will be root
    if (!parent->child || wuri_depth_eq(uri, 1))
        return parent;
    // else start with root's first child
    target = parent->child;
    while (target) {
        // get both uri's maximum segment length
        // and compare the two of them with it
        utarget = target->uri+offset;
        uuri = uri+offset;
        if (wuri_segcmp(utarget, uuri, &lim)) {
            // if no match
            // try with sibling
            if (target->sibling)
                target = target->sibling;
            else
                // if no siblings, return parent
                return parent;
        } else {           
            // target ends with next segment,
            if (wuri_depth_eq(uuri+lim, 1)) {
                return target;
            }
            // else, try with children
            else if (target->child) {
                parent = target;
                target = target->child;
                offset += lim;
            } else {
                return parent;
            }
        }
    }
    return target;
}

static int
wqtree_add_node(wqtree_t* tree, const char* uri,
                enum wtype_t type, wqnode_t** dst)
{
    int err;
    wqnode_t* parent, *node;
    if (wuri_check(uri))
        return WQUERY_URI_INVALID;
    if ((err = wqnode_walloc(tree->alloc, &node)) < 0)
        return err;
    parent = wqnode_get_parent(tree, uri);
    assert(parent);
    node->value.t = type;
    wqnode_set_uri(node, uri);
    wqnode_add_child(parent, node);
    *dst = node;
    return 0;
}

#define WQTREE_DECL_ADDND(_Type, _Tag) \
    int wqtree_addnd##_Tag(wqtree_t* tree, const char* uri, wqnode_t** dst) \
    { return wqtree_add_node(tree, uri, _Type, dst); }

WQTREE_DECL_ADDND(WOSC_TYPE_NIL, N);
WQTREE_DECL_ADDND(WOSC_TYPE_INT, i);
WQTREE_DECL_ADDND(WOSC_TYPE_FLOAT, f);
WQTREE_DECL_ADDND(WOSC_TYPE_BOOL, b);
WQTREE_DECL_ADDND(WOSC_TYPE_CHAR, c);

int
wqtree_addnds(wqtree_t* tree, const char* uri,
              wqnode_t** dst, int strlim)
{
    wstr_t* str;
    int err;    
    if ((err = wqtree_add_node(tree, uri, 's', dst)))
        return err;    
    if ((err = wstr_walloc(tree->alloc, &str, strlim)) >= 0) {
        err = 0;
        str->cap = strlim;
        (*dst)->value.u.s = str;
    }
    return err;
}

static int
wqnode_update(wqnode_t* nd, womsg_t* womsg)
{
    int err;
    enum wtype_t type = *womsg_gettag(womsg);
    if (!(err = wqnode_check_type(nd, type))) {
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
        wqnode_t* target;
        const char* uri = womsg_geturi(womsg);
        if ((target = wqtree_get_node(tree, uri)) == NULL)
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

struct wqconnection {
    struct mg_connection* tcp;
    int udp;
};

struct wqserver {
    struct mg_mgr mgr;
    struct wqconnection cn[WQUERY_MAX_CONNECTIONS];
    struct wqtree* tree;
    struct walloc_t* allocator;
#ifdef WPN114_MULTITHREAD
    pthread_t thread;
#endif
    bool running;
    uint16_t uport;
};

int
wqserver_walloc(struct walloc_t* _allocator, wqserver_t** dst)
{
    int err;
    if ((err = _allocator->alloc(dst,
                sizeof(struct wqserver),
               _allocator->data)) >= 0) {
        memset(*dst, 0, sizeof(struct wqserver));
        err = 0;
    }
    return err;
}

void
wqserver_set_allocator(struct wqserver* server,
                       struct walloc_t* allocator)
{
    server->allocator = allocator;
}

int
wqserver_expose(wqserver_t* server, wqtree_t* tree)
{
    server->tree = tree;
    return 0;
}

static struct wqconnection*
wqserver_get_connection(wqserver_t* server, struct mg_connection* mgc)
{
    for (int n = 0; n < WQUERY_MAX_CONNECTIONS; ++n)
         if (server->cn[n].tcp == mgc)
             return &server->cn[n];
    return NULL;
}

static inline int
wqserver_reply_json(struct mg_connection* mgc,
                    const char* json)
{
    mg_send_head(mgc, HTTP_OK, strlen(json), HTTP_MIME_JSON);
    mg_printf(mgc, "%.*s", (int)strlen(json), json);
    return 0;
}

static int
wqserver_add_connection(struct wqserver* server,
                        struct mg_connection* mgc)
{
    for (int nc = 0; nc < WQUERY_MAX_CONNECTIONS; ++nc) {
        if (server->cn[nc].tcp == NULL) {
            server->cn[nc].tcp = mgc;
            return 0;
        }
    }
    return 1;
}

static void
wqserver_cmd_listen(wqserver_t* server, char* data, int size,
                    bool status)
{
    wqnode_t* target;
    char* path;
    int n = 0;
    // we don't need to reuse data after that
    // this way, we don't need extra allocation
    path = &data[strcspn(data, "/")];
    while (path[n] != '\"')  n++;
    path[n] = '\0';
    if ((target = wqtree_get_node(server->tree, path)))
        target->status = status;
    else
        wpnerr("could not find node %s\n", path);
}

static void
wqserver_handle_ws_text(wqserver_t* server,
                        struct mg_connection* mgc,
                        struct websocket_message* wm)
{
    // parse json
    char cmd[32], *data = (char*)wm->data;
    int size = wm->size;
    mjson_get_string(data, size, "$.COMMAND", cmd, sizeof(cmd));

    if (strcmp(cmd, "LISTEN") == 0)
        wqserver_cmd_listen(server, data, size, true);
    else if (strcmp(cmd, "IGNORE") == 0)
        wqserver_cmd_listen(server, data, size, false);

    else if (strcmp(cmd, "START_OSC_STREAMING") == 0) {
        struct wqconnection* wqc;
        double port;
        mjson_get_number(data, size, "$.DATA.LOCAL_SERVER_PORT", &port);
        if ((wqc = wqserver_get_connection(server, mgc)))
             wqc->udp = port;
        else
            wpnerr("could not find wqconnection, "
                   "ignoring message: %s\n", wm->data);
    }
}

// better to omit fields that are 'false'?
static const char*
s_host_ext =
    "{ "
        "\"ACCESS\": true,"
        "\"VALUE\": true,"
        "\"CRITICAL\": true,"
        "\"LISTEN\": true,"
        "\"OSC_STREAMING\": true"
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
    "}";

#define WJSTR(_str) "\"" _str "\""

static void
wqserver_handle_request(wqserver_t* server,
                        struct mg_connection* mgc,
                        struct http_message* hm)
{
    wqnode_t* target;
    target = wqtree_get_node(server->tree, hm->uri.p);
    if (hm->query_string.len) {
        if (strspn(hm->query_string.p, "HOST_INFO") == 9) {
            // use server allocator?
            int err;
            char* buf;
            if ((err = server->allocator->alloc(&buf, 256,
                server->allocator->data))) {
                wpnerr("could not allocate temporary string storage "
                       "required for http replying, aborting..\n");
                assert(0);
            }
            err = snprintf(buf, 256, "{%s:%s, %s:%d, %s:%s, %s:%s}",
                     WJSTR("NAME"), WJSTR("wqserver"),
                     WJSTR("OSC_PORT"), server->uport,
                     WJSTR("OSC_TRANSPORT"), WJSTR("UDP"),
                     WJSTR("EXTENSIONS"), s_host_ext);
            wpnout("replying with host_info: %s\n", buf);
            server->allocator->free(&buf, 256,
            server->allocator->data);
            wqserver_reply_json(mgc, buf);
        } else {
            // query attribute, we need to allocate, in case value is a looong string for example            
            int err;
            char* buf;
    //            wqnode_attr_printj(target, hm->query_string.p, buf, 128);
            wqserver_reply_json(mgc, buf);
        }
    } else {
        // query all, including subnodes
//        char buf[WPN114_MAXJSON];
//            wqnode_printj(target, buf, WPN114_MAXJSON);
//        wqserver_reply_json(mgc, buf);
    }
}

static void
wqserver_tcp_handle(struct mg_connection* mgc, int event, void* data)
{
    wqserver_t* server = mgc->mgr->user_data;
    switch (event) {
    case MG_EV_WEBSOCKET_HANDSHAKE_DONE:
        wqserver_add_connection(server, mgc);
        break;
    case MG_EV_HTTP_REQUEST:
        wqserver_handle_request(server, mgc, data);
        break;
    case MG_EV_WEBSOCKET_FRAME: {
        struct websocket_message* wm = data;
        if (wm->flags & WEBSOCKET_OP_TEXT)
            wqserver_handle_ws_text(server, mgc, wm);
        else if (wm->flags & WEBSOCKET_OP_BINARY)
            // OSC over websocket, update tree
            wqtree_update_osc(server->tree, wm->data, wm->size);
        break;
    }
    case MG_EV_CLOSE: {
        if (mgc->flags & MG_F_IS_WEBSOCKET) {
            struct wqconnection* wqc;
            if ((wqc = wqserver_get_connection(server, mgc)))
                memset(wqc, 0, sizeof(struct wqconnection));
            else
                wpnerr("couldn't find wqconnection...\n");
        }
    }
    }
}

static void
wqserver_udp_handle(struct mg_connection* mgc, int event,
                    WPN_UNUSED void* data)
{
    wqserver_t* server = mgc->mgr->user_data;
    if (event == MG_EV_RECV)
        wqtree_update_osc(server->tree,
                          (byte_t*)mgc->recv_mbuf.buf,
                          mgc->recv_mbuf.len);
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

    mg_mgr_init(&server->mgr, server);
    if ((c_tcp = mg_bind(&server->mgr, s_tcp,
                 wqserver_tcp_handle)) == NULL) {
        return WQUERY_BINDERR_TCP;
    }
    mg_set_protocol_http_websocket(c_tcp);
    if ((c_udp = mg_bind(&server->mgr, udp_hdr,
                 wqserver_udp_handle)) == NULL) {
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
    struct walloc_t* allocator;
#ifdef WQUERY_MULTITHREAD
    pthread_t thread;    
#endif
    bool running;
    uint16_t port;
};

int
wqclient_walloc(struct walloc_t* _allocator, wqclient_t** dst)
{
    int err;
    if ((err = _allocator->alloc(dst,
                sizeof(struct wqclient),
               _allocator->data)) >= 0) {
        memset(*dst, 0, sizeof(struct wqclient));
        err = 0;
    }
    return err;
}

void
wqclient_set_allocator(struct wqclient* client,
                       struct walloc_t* allocator)
{
    client->allocator = allocator;
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
wqclient_tcp_handle(struct mg_connection* mgc, int event, void* data)
{
    wqclient_t* cli = mgc->mgr->user_data;
    switch (event) {
    case MG_EV_WEBSOCKET_HANDSHAKE_DONE: {
        // once handshake is done, request both host_info & json tree
        char addr[32], port[8], url[64];
        mg_sock_addr_to_str(&cli->cn.tcp->sa, addr, sizeof(addr), MG_SOCK_STRINGIFY_IP);
        mg_sock_addr_to_str(&cli->cn.tcp->sa, port, sizeof(port), MG_SOCK_STRINGIFY_PORT);
        sprintf(url, "ws://%s:%s/", addr, port);
        mg_connect_http(&cli->mgr, wqclient_tcp_handle, url, NULL, NULL);
        strcat(url, "?HOST_INFO");
        mg_connect_http(&cli->mgr, wqclient_tcp_handle, url, NULL, NULL);
        break;
    }
    case MG_EV_WEBSOCKET_FRAME: {
        struct websocket_message* wm = data;
        if (wm->flags & WEBSOCKET_OP_BINARY)
            wqtree_update_osc(&cli->tree, wm->data, wm->size);
        else
            wpnerr("client unsupported websocket message type\n");
        break;
    }
    case MG_EV_HTTP_REPLY: {
        struct http_message* hm = data;
        if (strspn(hm->query_string.p, "HOST_INFO") == 9) {
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
wqclient_udp_handle(struct mg_connection* mgc, int event,
                    WPN_UNUSED void* data)
{
    wqclient_t* cli = mgc->mgr->user_data;    
    if (event == MG_EV_RECV)
        wqtree_update_osc(&cli->tree,
                          (byte_t*)mgc->recv_mbuf.buf,
                          mgc->recv_mbuf.len);
}

int
wqclient_connect(wqclient_t* client, const char* addr, uint16_t port)
{
    char waddr[32];
    struct mg_connection* mgct, *mgcu;
    sprintf(waddr, "%s:%d", addr, port);

    mg_mgr_init(&client->mgr, client);
    if ((mgcu = mg_bind(&client->mgr, "udp://1234", wqclient_udp_handle)) == NULL)
        return WQUERY_BINDERR_UDP;
    if ((mgct = mg_connect_ws(&client->mgr, wqclient_tcp_handle, waddr, NULL, NULL)) == NULL)
        return WQUERY_BINDERR_TCP;

    client->cn.tcp = mgct;
    client->running = true;
#ifdef WQUERY_MULTITHREAD
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
#ifdef WQUERY_MULTITHREAD
    pthread_join(client->thread, 0);
#endif
    return 0;
}
