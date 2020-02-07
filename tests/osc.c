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
    wtest_fassert_soft(womsg_setbuf(msg, buf, sizeof(buf)));
    wtest_fassert_soft(womsg_seturi(msg, "/foo/bar"));
    wtest_fassert_soft(womsg_settag(msg, "fiTcs"));
    uri = womsg_geturi(msg);
    tag = womsg_gettag(msg);
    wtest_fassert_soft(strcmp(uri, "/foo/bar"));
    wtest_fassert_soft(strcmp(tag, "fiTcs"));
    wtest_assert_soft(womsg_getargc(msg) == 5);
    wtest_fassert_soft(womsg_writef(msg, 32.4));
    wtest_fassert_soft(womsg_writei(msg, 47));
    wtest_fassert_soft(womsg_writec(msg, 'W'));
    wtest_fassert_soft(womsg_writes(msg, "owls are not what they seem"));
    // in taglocked mode, we should get an error if we try to write another value
    wtest_assert_soft(womsg_writei(msg, 456) == 1);
    wtest_assert_soft(womsg_getlen(msg) == 60);

    char t;
    // this would be an example for unknown tag messages
    while ((t = *tag++)) {
        switch (t) {
        case WOSC_TYPE_FLOAT: {
            float f;
            wtest_fassert_soft(womsg_readf(msg, &f));
            wpnout("value (float): %f\n", f);
            break;
        }
        case WOSC_TYPE_INT: {
            int i;
            wtest_fassert_soft(womsg_readi(msg, &i));
            wpnout("value (int): %d\n", i);
            break;
        }
        case WOSC_TYPE_TRUE: {
            bool b;
            wtest_fassert_soft(womsg_readb(msg, &b));
            wpnout("value (bool): %d\n", b);
            break;
        }
        case WOSC_TYPE_CHAR: {
            char c;
            wtest_fassert_soft(womsg_readc(msg, &c));
            wpnout("value (char): %c\n", c);
            break;
        }
        case WOSC_TYPE_STRING: {
            char* s;
            wtest_fassert_soft(womsg_reads(msg, &s));
            wtest_fassert_soft(strcmp(s, "owls are not what they seem"));
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
wtest(osc_02)
{
    wtest_begin(osc_02);
    byte_t buf[64] = {
        47, 116, 101, 115,
       116,  95,  48,  50,
         0,   0,   0,   0,
        44, 102, 105, 115,
         0,   0,   0,   0,
       113,  61,  61,  66,
        16,   0,   0,   0,
       116, 119, 111,  32,
        99, 111, 111, 112,
       101, 114, 115,   0
    };
    womsg_t* msg;
    float f;
    int i;
    char* c;
    womsg_alloca(&msg);
    womsg_decode(msg, buf, sizeof(buf));
    wtest_fassert_soft(strcmp(womsg_geturi(msg), "/test_02"));
    wtest_fassert_soft(strcmp(womsg_gettag(msg), "fis"));
    wtest_fassert_soft(womsg_readf(msg, &f));
    wtest_fassert_soft(womsg_readi(msg, &i));
    wtest_fassert_soft(womsg_reads(msg, &c));
    wtest_assert_soft(f == 47.31f);
    wtest_assert_soft(i == 16);
    wtest_fassert_soft(strcmp(c, "two coopers"));
    wtest_end;
}

int
main(void)
{
    int err = 0;
    err += wpn_unittest_osc_01();
    err += wpn_unittest_osc_02();
    return err;
}
