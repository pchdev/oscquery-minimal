#include <wpn114/network/oscquery.h>

// we want to limit this to 64 bytes
struct wqnode {
    struct wqnode* next;
    const char* uri;
    wqvalue_t value;
    enum wqflags_t flags;
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

    return 0;
}
