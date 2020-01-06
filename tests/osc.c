#include <wpn114/network/osc.h>

void
test_01(void)
{
    int err;
    byte_t buf[512];
    womsg_t* msg;
    womsg_alloca(&msg);
    womsg_setbuf(msg, buf, 512);
    womsg_seturi(msg, "/foo/bar");
    womsg_settag(msg, "fibs");
    womsg_writef(msg, 32.4);
    womsg_writei(msg, 47);
    womsg_writeb(msg, true);
    womsg_writes(msg, "owls are not what they seem");

    const char* tag = womsg_gettag(msg);
    char t;

    // this would be an example for unknown tag messages
    while ((t = *tag++)) {
        switch (t) {
        case 'f': {
            float f;
            womsg_readf(msg, &f);
            printf("value (float): %f\n", f);
            break;
        }
        case 'i': {
            int i;
            womsg_readi(msg, &i);
            printf("value (int): %d\n", i);
            break;
        }
            // etc.
        }
    }
}
