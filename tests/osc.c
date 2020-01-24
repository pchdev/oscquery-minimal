#include <wpn114/network/osc.h>
#include <wpn114/utilities.h>
#include <stdio.h>
#include <assert.h>
#include "tests.h"

// #01 - simple encoding/decoding

wtest(osc_01)
{
    wtest_begin(osc_01);
    const char* uri, *tag;
    byte_t buf[64];
    womsg_t* msg;
    womsg_alloca(&msg);
    wtest_assert_soft(womsg_setbuf(msg, buf, 512) == 0);
    wtest_assert_soft(womsg_seturi(msg, "/foo/bar") == 0);
    wtest_assert_soft(womsg_settag(msg, "fiTcs") == 0);
    uri = womsg_geturi(msg);
    tag = womsg_gettag(msg);
    wtest_assert_soft(strcmp(uri, "/foo/bar") == 0);
    wtest_assert_soft(strcmp(tag, "fiTcs") == 0);
    wtest_assert_soft(womsg_getcnt(msg) == 5);
    wtest_assert_soft(womsg_writef(msg, 32.4) == 0);
    wtest_assert_soft(womsg_writei(msg, 47) == 0);
    wtest_assert_soft(womsg_writec(msg, 'W') == 0);
    wtest_assert_soft(womsg_writes(msg, "owls are not what they seem") == 0);
    // in taglocked mode, we should get an error if we try to write another value
    wtest_assert_soft(womsg_writei(msg, 456) == 1);
    wtest_assert_soft(womsg_getlen(msg) == 60);

    char t;
    // this would be an example for unknown tag messages
    while ((t = *tag++)) {
        switch (t) {
        case 'f': {
            float f;
            wtest_assert_soft(womsg_readf(msg, &f) == 0);
            wpnout("value (float): %f\n", f);
            break;
        }
        case 'i': {
            int i;
            wtest_assert_soft(womsg_readi(msg, &i) == 0);
            wpnout("value (int): %d\n", i);
            break;
        }
        case 'T': {
            bool b;
            wtest_assert_soft(womsg_readb(msg, &b) == 0);
            wpnout("value (bool): %d\n", b);
            break;
        }
        case 'c': {
            char c;
            wtest_assert_soft(womsg_readc(msg, &c) == 0);
            wpnout("value (char): %c\n", c);
            break;
        }
        case 's': {
            char* s;
            wtest_assert_soft(womsg_reads(msg, &s) == 0);
            wtest_assert_soft(strcmp(s, "owls are not what they seem") == 0);
            wpnout("value (str): %s\n", s);
            break;
        }
        default:
            assert(0);
        }
    }
    wtest_end;
}

// get a raw message
void
wosc_unittest_02(void)
{
    womsg_t* msg;
    womsg_alloca(&msg);
}

int
main(void)
{
    int err = 0;
    err += wpn_unittest_osc_01();
    return err;
}
