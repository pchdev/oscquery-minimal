#include <wpn114/network/osc.h>
#include <wpn114/utilities.h>
#include <stdio.h>
#include <assert.h>

// #01 - simple encoding/decoding
void
wosc_unittest_01(void)
{
    int err;
    const char* uri;
    const char* tag;
    byte_t buf[64];
    womsg_t* msg;

    womsg_alloca(&msg);
    assert((err = womsg_setbuf(msg, buf, 512)) == 0);
    assert((err = womsg_seturi(msg, "/foo/bar")) == 0);
    assert((err = womsg_settag(msg, "fiTcs")) == 0);
    uri = womsg_geturi(msg);
    tag = womsg_gettag(msg);
    assert(strcmp(uri, "/foo/bar") == 0);
    assert(strcmp(tag, "fiTcs") == 0);
    assert(womsg_getcnt(msg) == 5);

    err = womsg_writef(msg, 32.4);
    err = womsg_writei(msg, 47);
    err = womsg_writec(msg, 'W');
    err = womsg_writes(msg, "owls are not what they seem");

    // in taglocked mode, we should get an error if we try to write another value
    err = womsg_writei(msg, 456);
    assert(err == 1);
    assert(womsg_getlen(msg) == 60);

    char t;
    // this would be an example for unknown tag messages
    while ((t = *tag++)) {
        switch (t) {
        case 'f': {
            float f;
            err = womsg_readf(msg, &f);
            assert(err == 0);
            wpnout("value (float): %f\n", f);
            break;
        }
        case 'i': {
            int i;
            err = womsg_readi(msg, &i);
            assert(err == 0);
            wpnout("value (int): %d\n", i);
            break;
        }
        case 'T': {
            bool b;
            err = womsg_readb(msg, &b);
            assert(err == 0);
            wpnout("value (bool): %d\n", b);
            break;
        }
        case 'c': {
            char c;
            err = womsg_readc(msg, &c);
            assert(err == 0);
            wpnout("value (char): %c\n", c);
            break;
        }
        case 's': {
            char* s;
            err = womsg_reads(msg, &s);
            assert(err == 0);
            wpnout("value (str): %s\n", s);
            break;
        }
        default:
            assert(0);
        }
    }
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
    wosc_unittest_01();
    return 0;
}
