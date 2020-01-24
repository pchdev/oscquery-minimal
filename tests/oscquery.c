#include <wpn114/network/oscquery.h>
#include <wpn114/utilities.h>
#include <signal.h>
#include <unistd.h>
#include <assert.h>

static sig_atomic_t
s_sig = 0;

static void
signal_hdl(int sgno)
{
    signal(sgno, signal_hdl);
    s_sig = sgno;
}

void
ndi_fn(wqnode_t* nd, wvalue_t* v, void* udt)
{
    wpnout("/foo/bar/int value: %d\n", v->u.i);
}

void
ndf_fn(wqnode_t* nd, wvalue_t* v, void* udt)
{
    wpnout("/foo/bar/float value: %f\n", v->u.f);
}

void
ndb_fn(wqnode_t* nd, wvalue_t* v, void* udt)
{
    wpnout("/foo/bar/bool value: %d\n", v->u.b);
}

void
ndc_fn(wqnode_t* nd, wvalue_t* v, void* udt)
{
    wpnout("/foo/bar/char value: %c\n", v->u.c);
}

void
nds_fn(wqnode_t* nd, wvalue_t* v, void* udt)
{
    wpnout("/foo/bar/float value: %s\n", v->u.s->dat);
}

wpn_declstatic_alloc_mp(wqmp_01, 256);

// simple int node test
int
wquery_unittest_01(void)
{
    wqtree_t* tree;
    wqnode_t* ndi;
    const char* ndi_name;
    int err, ndi_v;
    wpnout("allocating wqtree..\n");
    if ((err = wqtree_walloc(&wqmp_01, &tree)) < 0) {
        return err;
    }
    wmemp_rmnprint(wqmp_01.data);
    wpnout("allocating wqnode..\n");
    if ((err = wqtree_addndi(tree, "/foo/bar/int", &ndi))) {
        return err;
    }
    wmemp_rmnprint(wqmp_01.data);
    ndi_name = wqnode_getname(ndi);
    assert(strcmp(ndi_name, "int") == 0);
    assert(wqnode_getaccess(ndi) == WQNODE_ACCESS_RW);
    assert(wqnode_geti(ndi, &ndi_v) == 0);
    assert(ndi_v == 0);
    assert(wqnode_seti(ndi, 47) == 0);
    wqnode_geti(ndi, &ndi_v);
    assert(ndi_v == 47);
    return err;
}

// test with string node, todo
wpn_declstatic_alloc_mp(wqmp_02, 256);

int
wquery_unittest_02(void)
{
    wqtree_t* tree;
    wqnode_t* nds;
    const char* nds_v;
    int err;
    wqtree_walloc(&wqmp_02, &tree);
    wmemp_rmnprint(wqmp_02.data);
    if ((err = wqtree_addnds(tree, "/foo/bar/string", &nds, 64))) {
        return err;
    }
    wmemp_rmnprint(wqmp_02.data);
    wqnode_sets(nds, "owls are not what they seem");
    wqnode_gets(nds, &nds_v);
    assert(strcmp(nds_v, "owls are not what they seem") == 0);
    wpnout("string is: %s\n", nds_v);
    return 0;
}

wpn_declstatic_alloc_mp(wqmp_03, 512);

int
wquery_unittest_03(void)
{
    wqserver_t* server;
    wqtree_t* tree;
    wqnode_t* ndf;
    int err;
    if ((err = wqserver_walloc(&wqmp_03, &server)) < 0) {
        wpnerr("could not allocate server\n");
        assert(false);
    }
    wqserver_zro(server);
    wqtree_walloc(&wqmp_03, &tree);
    wqtree_addndf(tree, "/float", &ndf);
    wqnode_setf(ndf, 27.31);
    wqserver_expose(server, tree);
    wqnode_setfn(ndf, ndf_fn, NULL);
    if ((err = wqserver_run(server, 1234, 5678))) {
        wpnerr("%s (%d)\n", wquery_strerr(err), err);
    }
    wpnout("running oscquery server on port 5678\n");
    while (s_sig == 0)
        sleep(10);
    return 0;
}

int
wquery_unittest_04(void)
{
    int err;
    wqtree_t* tree;
    wqtree_walloc(&wqmp_03, &tree);

    // we don't want intermediate nodes to be created as not to use too much memory
    wqnode_t *ind, *fnd, *bnd, *cnd, *snd;
    if ((err = wqtree_addndi(tree, "/foo/bar/int", &ind)) ||
        (err = wqtree_addndf(tree, "/foo/bar/float", &fnd)) ||
        (err = wqtree_addndc(tree, "/foo/bar/char", &cnd)) ||
        (err = wqtree_addnds(tree, "/foo/bar/string", &snd, 32)) ||
        (err = wqtree_addndb(tree, "/foo/bar/bool", &bnd))) {
        wpnerr("%s, %d\n", wquery_strerr(err), err);
        return 1;
    }
    wmemp_rmnprint(wqmp_03.data);

    wqnode_setfn(ind, ndi_fn, tree);
    wqnode_setfn(fnd, ndf_fn, NULL);

    wqnode_setfl(ind, WQNODE_CRITICAL | WQNODE_FN_SETPRE);
    wqnode_setfl(fnd, WQNODE_NOREPEAT | WQNODE_FN_SETPRE);
    wqnode_setfl(bnd, WQNODE_CRITICAL);
    wqnode_setfl(cnd, WQNODE_CRITICAL);
    wqnode_setfl(snd, WQNODE_CRITICAL);

    wqnode_seti(ind, 43);
    wqnode_setf(fnd, 47.31);
    wqnode_setb(bnd, true);
    wqnode_setc(cnd, 'A');
    wqnode_sets(snd, "hello world!");

    wqserver_t* qserver;
    wqserver_walloc(&wqmp_03, &qserver);
    wmemp_rmnprint(wqmp_03.data);
    wqserver_zro(qserver);
    wqserver_expose(qserver, tree);
    wqserver_run(qserver, 4731, 4389);

    while (s_sig == 0) {
#ifdef WPN114_MULTITHREAD
        sleep(1);
#else
//        wqserver_iterate(qserver, 200);
        sleep(1);
#endif
    }

    return err;
}

int
main(void)
{
    wquery_unittest_01();
    wquery_unittest_02();
    wquery_unittest_03();
    return 0;
}
