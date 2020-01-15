#include <wpn114/network/osc.h>

struct womsg {
    byte_t* buf;         // message buffer
    byte_t* rwi;         // mandatory, data read/write index
    unsigned short ble;  // total buffer length (in bytes)
    unsigned short usd;  // used length (in bytes)
    unsigned short tag;  // tag index (in bytes) (0 is comma)
    byte_t idx;          // data index
    byte_t mode;         // read-write mode
};

int _womsg_sizeof(void) { return sizeof(struct womsg); }

enum womsg_err {
    WOMSG_NOERROR,
    WOMSG_READ_ONLY,
    WOMSG_TAG_MISMATCH,
    WOMSG_TAG_END,
    WOMSG_BUFFER_OVERFLOW,
    WOMSG_URI_INVALID
};

#define WOMSG_INVALID       0u
#define WOMSG_W             1u
#define WOMSG_WTAGLOCKED    2u
#define WOMSG_WTAGFREE      3u
#define WOMSG_R             4u

#define _tag(_womsg)        ((char*)(&(_womsg->buf[_womsg->tag])))
#define _tagnc(_womsg)      (_tag(_womsg)+1)
#define _nexttag(_womsg)    (_womsg->buf[_womsg->tag+_womsg->idx])
#define _taglen(_womsg)     ((byte_t) strlen((char*)&_womsg->buf[_womsg->tag]))

int
wosc_checkuri(const char* uri)
{
    char c;
    if (*uri != '/')
        return WOMSG_URI_INVALID;
    // pattern match the rest,
    // basically, check invalid characters
    return 0;
}

int
womsg_decode(struct womsg* dst, byte_t* src, uint32_t len)
{
    // set message in read mode and return
    dst->buf = src;
    dst->ble = len;
    dst->usd = len;
    dst->mode = WOMSG_R;
    return 0;
}

int
womsg_setbuf(struct womsg* msg, byte_t* buf, uint32_t len)
{
    msg->buf = buf;
    msg->ble = len;
    // set message in write mode
    msg->mode = WOMSG_W;
    return 0;
}

static __always_inline int
womsg_npads(int sz)
{
    return 4-(sz%4);
}

int
womsg_seturi(struct womsg* msg, const char* uri)
{
    // check uri
    int err, len;
    if ((err = wosc_checkuri(uri)))
        return err;
    // check size
    len = strlen(uri);
    len += womsg_npads(len);
    // we count the comma + the pads
    if (msg->usd+len+4 > msg->ble)
        return WOMSG_BUFFER_OVERFLOW;

    strcpy((char*)msg->buf, uri);
    // set tag index
    msg->tag = len;
    msg->usd = len;
    return err;
}

int
womsg_settag(struct womsg* msg, const char* tag)
{
    // check tag first
    // check buffer size
    char* ptr = (char*) &msg->buf[msg->tag];
    int len = strlen(tag);
    strcpy(ptr++, ",");
    strcpy(ptr, tag);
    msg->usd += len+1;
    msg->mode = WOMSG_WTAGLOCKED;
    return 0;
}

const char*
womsg_geturi(struct womsg* msg)
{
    return (char*)msg->buf;
}

int
womsg_getlen(struct womsg* msg)
{
    return msg->usd;
}

int
womsg_getcnt(struct womsg* msg)
{
    return strlen(_tagnc(msg));
}

const char*
womsg_gettag(struct womsg* msg)
{
    return _tagnc(msg);
}

static inline int
womsg_checkw(struct womsg* msg, char tag, byte_t tpsz)
{
    // check if <msg> is in write mode
    int nsz = msg->usd+tpsz;
    if (msg->mode > WOMSG_W)
        return WOMSG_READ_ONLY;
    if (msg->mode == WOMSG_WTAGLOCKED) {
        // if tag is locked and there's no next tag, return error
        if (msg->idx >= _taglen(msg))
            return WOMSG_TAG_END;
        // check if <value> type matches next tag in line
        else if (_nexttag(msg) != tag)
            return WOMSG_TAG_MISMATCH;
    } else {
        // don't forget to add +1 for newtag!
        nsz++;
    }
    // finally, check if there's enough space in the buffer
    if (nsz > msg->ble)
        return WOMSG_BUFFER_OVERFLOW;
    return 0;
}

int
womsg_writei(struct womsg* msg, int value)
{
    int err;
    if (!(err = womsg_checkw(msg, 'i', sizeof(int))))
        memcpy(msg->rwi, &value, sizeof(int));
    return err;
}

int
womsg_writef(struct womsg* msg, float value)
{
    int err;
    if (!(err = womsg_checkw(msg, 'f', sizeof(float))))
        memcpy(msg->rwi, &value, sizeof(float));
    return err;
}

int
womsg_writec(struct womsg* msg, char value)
{
    int err;
    if (!(err = womsg_checkw(msg, 'c', sizeof(char))))
        memcpy(msg->rwi, &value, sizeof(char));
    return err;
}

int
womsg_writeb(struct womsg* msg, bool value)
{
    int err;
    return err;
}

int
womsg_writes(struct womsg* msg, const char* str)
{
    int err;
    return err;
}

int
womsg_readi(struct womsg* msg, int* dst)
{
    // check read-mode, check type...
    memcpy(dst, (int*)msg->idx, sizeof(int));
    msg->idx += sizeof(int);
    return 0;
}

int
womsg_readf(struct womsg* msg, float* dst)
{
    return 0;
}


