#include <wpn114/network/oscquery.h>
#include <wpn114/utilities.h>

#define WQNODE_IGNORE 0
#define WQNODE_LISTEN 1

const char*
wquery_strerr(int err)
{
    switch (err) {
    case WQUERY_NOERROR:
        return "no error";
    case WQUERY_BINDERR_UDP:
        return "could not bind udp socket (port might already be in use)";
    case WQUERY_BINDERR_TCP:
        return "could not bind tcp socket (port might already be in use)";
    case WQUERY_URI_INVALID:
        return "invalid uri";
    default:
        return "unknown error code";
    }
}

// ------------------------------------------------------------------------------------------------
// NODE/TREE
// ------------------------------------------------------------------------------------------------

// we want to limit this to 64 bytes
struct wqnode {
    struct wqnode* next;
    const char* uri;
    wqvalue_t value;
    enum wqflags_t flags;
    int status;
};

struct wqtree {
    struct wqnode root;
    const char* name;
    wqtree_callback vcb;
    void* udt;
    void* ptr;
    int flags;    
};

static int
wqnode_malloc(wqnode_t** dst)
{
    if ((*dst = malloc(sizeof(struct wqnode))) == NULL)
        return 1;
    else return 0;
}

static int
wqnode_palloc(wqnode_t** dst, struct wmemp_t* mp)
{
    return wmemp_req(mp, sizeof(struct wqnode),
                     (void**)dst);
}

static int
wqnode_seturi(wqtree_t* tree, wqnode_t* nd, const char* uri)
{
    return 0;
}

static int
wqnode_addnxt(wqnode_t* prnt, wqnode_t* nxt)
{

}

#define WQTREE_F_MALLOC     1
#define WQTREE_F_MEMP       2

int
wqtree_palloc(wqtree_t** dst, struct wmemp_t* mp)
{
    int nbytes;
    if ((nbytes = wmemp_req0(mp, sizeof(struct wqtree),
                  (void**) dst))) {
        (*dst)->ptr = mp;
        (*dst)->flags = WQTREE_F_MEMP;
    }
    return nbytes;
}

void
wqtree_setcallback(wqtree_t* tree, wqtree_callback cb, void* udt)
{
    tree->vcb = cb;
    tree->udt = udt;
}

static wqnode_t*
wqtree_getprnt(wqtree_t* tree, const char* uri)
{

    return 0;
}

int
wqtree_addnd(wqtree_t* tree, const char* uri,
             enum wqtype_t wqtype, wqnode_t** dst)
{
    int err = 0;
    wqnode_t* prnt, *nwnd;
    if (!wosc_checkuri(uri))
        return WQUERY_URI_INVALID;
    if ((err = wqnode_palloc(&nwnd, tree->ptr)) < 0)
        return err;
    prnt = wqtree_getprnt(tree, uri);
    nwnd->value.t = wqtype;
    wqnode_seturi(tree, nwnd, uri);
    wqnode_addnxt(prnt, nwnd);
    *dst = nwnd;
    return err;
}

int
wqtree_addndi(wqtree_t* tree, const char* uri, wqnode_t** dst)
{
    return wqtree_addnd(tree, uri, WQTYPE_INT, dst);
}

int
wqtree_addndf(wqtree_t* tree, const char* uri, wqnode_t** dst)
{
    return wqtree_addnd(tree, uri, WQTYPE_FLOAT, dst);
}

int
wqtree_addndb(wqtree_t* tree, const char* uri, wqnode_t** dst)
{
    return wqtree_addnd(tree, uri, WQTYPE_BOOL, dst);
}

int
wqtree_addndc(wqtree_t* tree, const char* uri, wqnode_t** dst)
{
    return wqtree_addnd(tree, uri, WQTYPE_CHAR, dst);
}

int
wqtree_addnds(wqtree_t* tree, const char* uri, wqnode_t** dst)
{
    return wqtree_addnd(tree, uri, WQTYPE_STRING, dst);
}

wqnode_t*
wqtree_getnd(wqtree_t* tree, const char* uri)
{
    if (!wosc_checkuri(uri)) {
        return NULL;
    }
    if (strlen(uri) == 1)
        return &tree->root;
    return 0;
}

static int
wqnode_update(wqnode_t* nd, womsg_t* womsg)
{
    return 1;
}

static int
wqtree_update(wqtree_t* tree, womsg_t* womsg)
{
    const char* uri;
    wqnode_t* target;
    uri = womsg_geturi(womsg);
    if ((target = wqtree_getnd(tree, uri)) == NULL)
        return WQUERY_URI_INVALID;
    return wqnode_update(target, womsg);
}

// ------------------------------------------------------------------------------------------------
// NETWORK
// ------------------------------------------------------------------------------------------------

#include <dependencies/mongoose/mongoose.h>
#include <dependencies/mjson/mjson.h>

#define HTTP_OK             200
#define HTTP_MIME_JSON      "Content-Type: application/json"

#ifndef WPN114_MAXCN
    #define WPN114_MAXCN    1
#endif

#ifndef WPN114_MAXPATH
    #define WPN114_MAXPATH  256
#endif

#ifndef WPN114_MAXJSON
    #define WPN114_MAXJSON  1024
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
wqserver_malloc(wqserver_t** dst)
{
    if ((*dst = malloc(sizeof(struct wqserver))) == NULL)
        return 1;
    else
        return 0;
}

int
wqserver_palloc(wqserver_t** dst, struct wmemp_t* mp)
{
    return wmemp_req(mp, sizeof(struct wqserver),
                     (void**)dst);
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

static void*
wqserver_pthread_run(void* v)
{
    wqserver_t* server = v;
    while (server->running) {
        mg_mgr_poll(&server->mgr, 200);
    }
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

static const char* s_host_ext =
        "{ "
        "\"ACCESS\": true,"
        "\"VALUE\": true,"
        "\"DESCRIPTION\": false,"
        "\"TAGS\": false,"
        "\"EXTENDED_TYPE\": false,"
        "\"UNIT\": false,"
        "\"CRITICAL\": true,"
        "\"CLIPMODE\": false,"
        "\"LISTEN\": true,"
        "\"PATH_CHANGED\": false,"
        "\"PATH_REMOVED\": false,"
        "\"PATH_ADDED\": false,"
        "\"PATH_RENAMED\": false,"
        "\"OSC_STREAMING\": true,"
        "\"HTML\": false,"
        "\"ECHO\": false "
        "}"
        ;

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
            mjson_get_string((char*)wm->data, wm->size, "$.COMMAND", cmd, sizeof(cmd));

            if (strcmp(cmd, "LISTEN") == 0) {
                char path[WPN114_MAXPATH];
                wqnode_t* target;
                mjson_get_string((char*)wm->data, wm->size, "$.DATA", path, sizeof(path));
                if ((target = wqtree_getnd(server->tree, path))) {
                    target->status = WQNODE_LISTEN;
                }
            } else if (strcmp(cmd, "IGNORE") == 0) {
                char path[WPN114_MAXPATH];
                wqnode_t* target;
                mjson_get_string((char*)wm->data, wm->size, "$.DATA", path, sizeof(path));
                if ((target = wqtree_getnd(server->tree, path))) {
                    target->status = WQNODE_IGNORE;
                }
            } else if (strcmp(cmd, "START_OSC_STREAMING") == 0) {
                struct wqconnection* wqc;
                double port;
                mjson_get_number((char*)wm->data, wm->size, "$.DATA.LOCAL_SERVER_PORT", &port);
                if ((wqc = wqserver_getcn(server, mgc)))
                    wqc->udp = port;
            }
        } else if (wm->flags & WEBSOCKET_OP_BINARY) {
            // parse OSC
            womsg_t* womsg;
            womsg_alloca(&womsg);
            womsg_decode(womsg, wm->data, wm->size);
            wqtree_update(server->tree, womsg);
        }

        break;
    }
    case MG_EV_HTTP_REQUEST: {
        struct http_message* hm = data;
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
        wqnode_t* target;
        if ((target = wqtree_getnd(server->tree, hm->uri.p))) {
            if (hm->query_string.len) {
                // query attribute
                // we don't expect it to be too large, maybe 128 bytes would be enough
                char buf[128];
                // ... TODO
//                wqserver_reply_json(mgc, buf);
            } else {
                // query all, including subnodes
                char buf[WPN114_MAXJSON];
                // ... TODO
//                wqserver_reply_json(mgc, buf);
            }
        }
        break;
    }
    case MG_EV_CLOSE: {
        struct wqconnection* wqc;
        if ((wqc = wqserver_getcn(server, mgc)))
            memset(wqc, 0, sizeof(struct wqconnection));
        else
            wpnerr("couldn't find wqconnection...\n");
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
        womsg_t* womsg;
        womsg_alloca(&womsg);
        womsg_decode(womsg, (byte_t*)mgc->recv_mbuf.buf, mgc->recv_mbuf.len);
        wqtree_update(server->tree, womsg);
    }
    }
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
wqclient_malloc(wqclient_t** dst)
{
    if ((*dst = malloc(sizeof(struct wqclient))) == NULL)
        return 1;
    else
        return 0;
}

int
wqclient_palloc(wqclient_t** dst, struct wmemp_t* mp)
{
    return wmemp_req(mp, sizeof(struct wqclient), (void**) dst);
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
    case MG_EV_CONNECT: {
        break;
    }
    case MG_EV_POLL: {
        break;
    }
    case MG_EV_WEBSOCKET_HANDSHAKE_DONE: {
        char cmd[128];
        int err;
        err = mjson_printf(&mjson_print_fixed_buf, cmd,
                           "{%Q:%s, %Q:{%Q:%d, %Q:%d}}",
                           "COMMAND", "START_OSC_STREAMING",
                           "DATA",
                           "LOCAL_SERVER_PORT", 1234,
                           "LOCAL_SENDER_PORT", 0);
        mg_send_websocket_frame(mgc, WEBSOCKET_OP_TEXT,
                                cmd, strlen(cmd));
        break;
    }
    case MG_EV_WEBSOCKET_FRAME: {
        struct websocket_message* wm = data;
        if (wm->flags & WEBSOCKET_OP_TEXT) {
            // there should not be any text reply here..
        } else if (wm->flags & WEBSOCKET_OP_BINARY) {
            womsg_t* womsg;
            womsg_alloca(&womsg);
            womsg_decode(womsg, (byte_t*)mgc->recv_mbuf.buf, mgc->recv_mbuf.len);
            wqtree_update(&cli->tree, womsg);
        }
        break;
    }
    case MG_EV_HTTP_REPLY: {
        struct http_message* hm = data;
        if (strcmp(hm->query_string.p, "HOST_INFO") == 0) {
            double uport;
            mjson_get_number(hm->body.p, hm->body.len, "$.OSC_PORT", &uport);
            cli->cn.udp = uport;
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
        womsg_t* womsg;
        womsg_alloca(&womsg);
        womsg_decode(womsg, (byte_t*)mgc->recv_mbuf.buf, mgc->recv_mbuf.len);
        wqtree_update(&cli->tree, womsg);
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
