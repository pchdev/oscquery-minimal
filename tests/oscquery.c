#include <wpn114/network/oscquery.h>
#include <wpn114/utilities.h>
#include <signal.h>
#include <unistd.h>

static sig_atomic_t
s_sig = 0;

static void
signal_hdl(int sgno)
{
    signal(sgno, signal_hdl);
    s_sig = sgno;
}

void
tree_callback(wqtree_t* tree, const char* uri, wqvalue_t* v, void* udt)
{        
    long spn;
    if ((spn = strspn(uri, "/foo/bar")) > 1) {
        uri += spn;
        if (strcmp(uri, "int") == 0) {
            // after returning from this function, /foo/bar/int value won't be set
            wpnout("/foo/bar/int value: %d\n", v->u.i);
        } else if (strcmp(uri, "float") == 0) {
            // limit float value to 50
            if (v->u.f > 50.f)
                v->u.f = 50.f;
            else if (v->u.f < 0.f)
                v->u.f = 0.f;
        } else if (strcmp(uri, "bool") == 0) {
            wpnout("/foo/bar/bool value %d\n", v->u.b);
        }
    }
}

wpn_declstatic_mp(wqnmp, 512);

int
wquery_unittest_01(void)
{
    int err;
    wqtree_t* tree;
    wqtree_palloc(&tree, &wqnmp);
    // we don't want intermediate nodes to be created as not to use too much memory
    wqnode_t *ind, *fnd, *bnd, *cnd, *snd;
    if ((err = wqtree_addndi(tree, "/foo/bar/int", &ind)) ||
        (err = wqtree_addndf(tree, "/foo/bar/float", &fnd)) ||
        (err = wqtree_addndc(tree, "/foo/bar/char", &cnd)) ||
        (err = wqtree_addnds(tree, "/foo/bar/string", &snd)) ||
        (err = wqtree_addndb(tree, "/foo/bar/bool", &bnd))) {
        wpnerr("%s\n", wquery_strerr(err));
        return 1;
    }
    wmemp_rmnprint(&wqnmp);

    wqnode_setflags(ind, WQNODE_READONLY | WQNODE_CALLBACK_SETNO);
    wqnode_setflags(bnd, WQNODE_CRITICAL | WQNODE_CALLBACK_SETPOST);
    wqnode_setflags(fnd, WQNODE_NOREPEAT | WQNODE_CALLBACK_SETPRE);
    wqnode_setflags(cnd, WQNODE_CRITICAL | WQNODE_CALLBACK_SETPOST);
    wqnode_setflags(snd, WQNODE_CRITICAL | WQNODE_CALLBACK_SETPOST);

    wqtree_setcallback(tree, tree_callback, NULL);

    wqnode_seti(ind, 43);
    wqnode_setf(fnd, 47.31);
    wqnode_setb(bnd, true);
    wqnode_setc(cnd, 'A');
    wqnode_sets(snd, "hello world!");

    wqserver_t* qserver;
    wqserver_palloc(&qserver, &wqnmp);
    wqserver_zro(qserver);
    wqserver_expose(qserver, tree);
    wqserver_run(qserver, 4731, 4389);

    while (s_sig == 0)
        sleep(1);

    return err;
}
