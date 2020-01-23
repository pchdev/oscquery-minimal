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

wpn_declstatic_mp(wqnmp, 512);

// simple int node test
int
wquery_unittest_01(void)
{
    wqtree_t* tree;
    wqnode_t* ndi;
    const char* ndi_name;
    int err, ndi_v;
    wpnout("allocating wqtree..\n");
    if ((err = wqtree_palloc(&tree, &wqnmp)) < 0) {
        return err;
    }
    wmemp_rmnprint(&wqnmp);
    wpnout("allocating wqnode..\n");
    if ((err = wqtree_addndi(tree, "/foo/bar/int", &ndi))) {
        return err;
    }
    wmemp_rmnprint(&wqnmp);
    assert(wmemp_rmn(&wqnmp) == 368);
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
int
wquery_unittest_02(void)
{
    return 0;
}

int
wquery_unittest_03(void)
{
    int err;
    wqtree_t* tree;
    wqtree_palloc(&tree, &wqnmp);

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
    wmemp_rmnprint(&wqnmp);

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
    wqserver_palloc(&qserver, &wqnmp);
    wmemp_rmnprint(&wqnmp);
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
    return 0;
}
