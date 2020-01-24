#include <wpn114/alloc.h>
#include <wpn114/utilities.h>
#include <assert.h>
#include "tests.h"
#include <check.h>

wpn_declstatic_mp(ut01mp, 100);

wtest(memp_01)
{
    wtest_begin(memp_01);
    int *ptr1, *ptr2, *ptr3;
    wtest_assert_soft(wmemp_chk(&ut01mp, 50) == 50);
    wtest_assert_soft(wmemp_chk(&ut01mp, 100) == 0);
    wtest_assert_soft(wmemp_req(&ut01mp, 50, (void**)&ptr1) == 50);
    wtest_assert_soft(wmemp_req0(&ut01mp, 50, (void**)&ptr2) == 0);
    wtest_assert_soft(ptr2[31] == 0);
    wtest_assert_soft(wmemp_req0(&ut01mp, 50, (void**)&ptr3) == -50);
    wtest_assert_soft(ut01mp.cap == 100);
    wtest_assert_soft(ut01mp.usd == 30);
    wtest_end;
}

wpn_declstatic_alloc_mp(walloc_02, 256);

wtest(memp_02)
{
    wtest_begin(memp_02);
    wtest_assert_soft(walloc_02.data == &walloc_02_mp);
    wtest_assert_soft(walloc_02.alloc == WPN_MEMP);
    wtest_assert_soft(walloc_02.free == wfree_memp);
    wtest_end;
}

int
main(void)
{
    int nerr = 0;
    nerr += wpn_unittest_memp_01();
    nerr += wpn_unittest_memp_02();
    return nerr;
}
