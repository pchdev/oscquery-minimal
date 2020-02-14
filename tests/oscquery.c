#include <wpn114/network/oscquery.h>
#include <wpn114/utilities.h>
#include <signal.h>
#include <unistd.h>
#include <assert.h>
#include "tests.h"

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

// simple int node test
wpn_declstatic_alloc_mp(wqmp_01, 256);
wtest(query_01)
{
    wtest_begin(query_01);
    wqtree_t* tree;
    wqnode_t* ndi;
    int ndi_v;
    wtest_fassert_soft(wqtree_walloc(&wqmp_01, &tree));
    wtest_fassert_soft(wqtree_addndi(tree, "/foo/bar/int", &ndi));
    wtest_fassert_soft(strcmp(wqnode_get_name(ndi), "int"));
    wtest_assert_soft(wqnode_get_access(ndi) == WQNODE_ACCESS_RW);
    wtest_fassert_soft(wqnode_geti(ndi, &ndi_v));
    wtest_fassert_soft(ndi_v);
    wtest_fassert_soft(wqnode_seti(ndi, 47));
    wqnode_geti(ndi, &ndi_v);
    wtest_assert_soft(ndi_v == 47);
    wqtree_print(tree);
    wmemp_rmnprint(&wqmp_01_mp);
    wtest_end;
}

// test with string node
wpn_declstatic_alloc_mp(wqmp_02, 256);
wtest(query_02)
{
    wtest_begin(query_02);
    wqtree_t* tree;
    wqnode_t* nds;
    const char* nds_v;
    wtest_fassert_soft(wqtree_walloc(&wqmp_02, &tree));
    wtest_fassert_soft(wqtree_addnds(tree, "/foo/bar/string", &nds, 64));
    wtest_fassert_soft(wqnode_sets(nds, "owls are not what they seem"));
    wtest_fassert_soft(wqnode_gets(nds, &nds_v));
    wtest_fassert_soft(strcmp(nds_v, "owls are not what they seem"));
    wqtree_print(tree);
    wmemp_rmnprint(&wqmp_02_mp);
    wtest_end;
}

// test simple server run
wpn_declstatic_alloc_mp(wqmp_03, 256);
wtest(query_03)
{
    wtest_begin(query_03);
    wqserver_t* server;
    wqtree_t* tree;
    wqnode_t* ndf;
    wtest_fassert_soft(wqserver_walloc(&wqmp_03, &server));
    wtest_fassert_soft(wqtree_walloc(&wqmp_03, &tree));
    wtest_fassert_soft(wqtree_addndf(tree, "/float", &ndf));
    wqnode_set_fn(ndf, ndf_fn, NULL);
    wtest_fassert_soft(wqnode_setf(ndf, 27.31));
    wqserver_expose(server, tree);
    wtest_fassert_soft(wqserver_run(server, 1234, 5678));
    wqtree_print(tree);
    wmemp_rmnprint(&wqmp_03_mp);
    wtest_end;
}

// test server-client connect
wpn_declstatic_alloc_mp(wqmp_04, 1024);
wtest(query_04)
{
    wtest_begin(query_04);
    wqserver_t* server;
    wqclient_t* client;
    wqtree_t* tree;
    wqnode_t *ndi, *ndf, *ndb, *ndc, *nds, *root;

    wtest_fassert_soft(wqtree_walloc(&wqmp_04, &tree));
    wtest_fassert_soft(wqtree_addndi(tree, "/foo/bar/int", &ndi));
    wtest_fassert_soft(wqtree_addndf(tree, "/foo/bar/float", &ndf));
    wtest_fassert_soft(wqtree_addndb(tree, "/foo/bar/bool", &ndb));
    wtest_fassert_soft(wqtree_addndc(tree, "/foo/bar/char", &ndc));
    wtest_fassert_soft(wqtree_addnds(tree, "/foo/bar/string", &nds, 32));
    root = wqtree_get_node(tree, "/");
    wtest_assert_soft(root != NULL);
    wtest_assert_soft(wqnode_is_child(root, ndi));
    wtest_assert_soft(wqnode_is_sibling(ndi, ndf));

    wqtree_print(tree);
    wmemp_rmnprint(&wqmp_04_mp);
    wqnode_set_fn(ndi, ndi_fn, tree);
    wqnode_set_fn(ndf, ndf_fn, NULL);
    wtest_fassert_soft(wqnode_set_flags(ndi, WQNODE_CRITICAL | WQNODE_FN_SETPRE));
    wtest_fassert_soft(wqnode_set_flags(ndf, WQNODE_NOREPEAT | WQNODE_FN_SETPRE));
    wtest_fassert_soft(wqnode_set_flags(ndb, WQNODE_CRITICAL));
    wtest_fassert_soft(wqnode_set_flags(ndb, WQNODE_NOREPEAT));
    wtest_fassert_soft(wqnode_set_flags(nds, WQNODE_NOREPEAT | WQNODE_CRITICAL));
    wtest_fassert_soft(wqnode_seti(ndi, 43));
    wtest_fassert_soft(wqnode_setf(ndf, 47.31));
    wtest_fassert_soft(wqnode_setb(ndb, true));
    wtest_fassert_soft(wqnode_setc(ndc, 'A'));
    wtest_fassert_soft(wqnode_sets(nds, "is it about the bunny?"));

    wtest_fassert_soft(wqserver_walloc(&wqmp_04, &server));
    wtest_fassert_soft(wqclient_walloc(&wqmp_04, &client));
    wmemp_rmnprint(&wqmp_04_mp);
    wqserver_expose(server, tree);
    wtest_fassert_soft(wqserver_run(server, 4731, 4389));
    wtest_fassert_soft(wqclient_connect(client, "127.0.0.1", 4389));

//    TODO: check mirrors
//    wqnode_t* ndi_mirror;
//    int ndi_mirror_v;
//    wqclient_getnd(client, "/foo/bar/int", &ndi_mirror);
//    wqnode_geti(ndi_mirror, &ndi_mirror_v);
//    wtest_assert_soft(ndi_mirror_v == 43);
//    wtest_fassert_soft(wqnode_seti(ndi_mirror, 22));
    wtest_end;
}

// tree structure testing
wpn_declstatic_alloc_mp(wqmp_05, 1024);
wtest(query_05)
{
    wtest_begin(query_05);
    wqtree_t* tree;
    wqtree_walloc(&wqmp_05, &tree);
    wqnode_t* foo, *foo_bar, *foo_bar_int, *foo_bar_int2, *foo_bar_int_int;
    wqnode_t *bar, *bar_foo, *bar_foo_float,
             *bar_foo_float2, *bar_foo_float_float, *bar_foo_float2_float;

    wtest_fassert_soft(wqtree_addndN(tree, "/foo", &foo));
    wtest_fassert_soft(wqtree_addndN(tree, "/foo/bar", &foo_bar));
    wtest_fassert_soft(wqtree_addndi(tree, "/foo/bar/int", &foo_bar_int));
    wtest_fassert_soft(wqtree_addndi(tree, "/foo/bar/int2", &foo_bar_int2));
    wtest_fassert_soft(wqtree_addndi(tree, "/foo/bar/int/int", &foo_bar_int_int));
    wtest_fassert_soft(wqtree_addndN(tree, "/bar", &bar));
    wtest_fassert_soft(wqtree_addndN(tree, "/bar/foo", &bar_foo));
    wtest_fassert_soft(wqtree_addndf(tree, "/bar/foo/float", &bar_foo_float));
    wtest_fassert_soft(wqtree_addndf(tree, "/bar/foo/float2", &bar_foo_float2));
    wtest_fassert_soft(wqtree_addndf(tree, "/bar/foo/float/float", &bar_foo_float_float));
    wtest_fassert_soft(wqtree_addndf(tree, "/bar/foo/float2/float", &bar_foo_float2_float));

    wtest_assert_soft(wqnode_is_child(foo, foo_bar));
    wtest_assert_soft(wqnode_is_child(foo_bar, foo_bar_int));
    wtest_assert_soft(wqnode_is_sibling(foo_bar_int, foo_bar_int2));
    wtest_assert_soft(wqnode_is_child(foo_bar_int, foo_bar_int_int));

    wtest_assert_soft(wqnode_is_sibling(bar, foo));
    wtest_assert_soft(wqnode_is_child(bar, bar_foo));
    wtest_assert_soft(wqnode_is_child(bar_foo, bar_foo_float));
    wtest_assert_soft(wqnode_is_sibling(bar_foo_float, bar_foo_float2));
    wtest_assert_soft(wqnode_is_child(bar_foo_float, bar_foo_float_float));
    wtest_assert_soft(wqnode_is_child(bar_foo_float2, bar_foo_float2_float));

    wtest_assert_soft((bool)wqtree_get_node(tree, "/"));
    wtest_assert_soft(wqtree_get_node(tree, "/foo/bar") == foo_bar);
    wtest_assert_soft(wqtree_get_node(tree, "/foo/bar/int") == foo_bar_int);
    wtest_assert_soft(wqtree_get_node(tree, "/foo/bar/int2") == foo_bar_int2);
    wtest_assert_soft(wqtree_get_node(tree, "/foo/bar/int/int") == foo_bar_int_int);
    wtest_assert_soft(wqtree_get_node(tree, "/bar") == bar);
    wtest_assert_soft(wqtree_get_node(tree, "/bar/foo") == bar_foo);
    wtest_assert_soft(wqtree_get_node(tree, "/bar/foo/float") == bar_foo_float);
    wtest_assert_soft(wqtree_get_node(tree, "/bar/foo/float2") == bar_foo_float2);
    wtest_assert_soft(wqtree_get_node(tree, "/bar/foo/float/float") == bar_foo_float_float);
    wtest_assert_soft(wqtree_get_node(tree, "/bar/foo/float2/float") == bar_foo_float2_float);

    wqtree_print(tree);
    wtest_end;
}

int
main(void)
{
    int err = 0;
    err += wpn_unittest_query_01();
    err += wpn_unittest_query_02();
    err += wpn_unittest_query_03();
    err += wpn_unittest_query_04();
    err += wpn_unittest_query_05();
    return err;
}
