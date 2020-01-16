#include <wpn114/mempool.h>
#include <wpn114/utilities.h>
#include <assert.h>

wpn_declstatic_mp(ut01mp, 100);
wpn_declstatic_mp(ut02mp, 200);

void
wpn_mp_unittest_01(void)
{
    long err;
    assert(wmemp_chk(&ut01mp, 50) == 50);
    assert(wmemp_chk(&ut01mp, 100) == 0);
    wpnout("wpn_mp_unittest_01_a_ok\n");

    int *ptr1, *ptr2, *ptr3;
    err = wmemp_req(&ut01mp, 50, (void**)&ptr1);
    assert(err == 50);
    wpnout("wpn_mp_unittest_01_b_ok\n");

    err = wmemp_req0(&ut01mp, 50, (void**)&ptr2);
    assert(err == 0);
    assert(ptr2[31] == 0);
    wpnout("wpn_mp_unittest_01_c_ok\n");

    err = wmemp_req0(&ut01mp, 50, (void**)&ptr3);
    assert(err == -50);
    assert(ut01mp.cap == 100);
    assert(ut01mp.usd == 100);
    wpnout("wpn_mp_unittest_01_d_ok\n");
}

void
wpn_mp_unittest_02(void)
{
    long err;
    int *ptr1, *ptr2;
}

int
main(void)
{
    wpn_mp_unittest_01();
//    wpn_mp_unittest_02();
    return 0;
}
