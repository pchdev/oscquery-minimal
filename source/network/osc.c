#include <wpn114/network/osc.h>
#include <wpn114/utilities.h>

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
#define WOMSG_WTAGLOCKED    1u
#define WOMSG_WTAGFREE      2u
#define WOMSG_W             3u
#define WOMSG_R             4u

#define _tag(_womsg)        ((char*)(&(_womsg->buf[_womsg->tag])))
#define _tagnc(_womsg)      (_tag(_womsg)+1)
#define _nexttag(_womsg)    (*(_tagnc(_womsg)+_womsg->idx))

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
    if (msg->usd+len > msg->ble)
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
    int len = strlen(tag)+1;
    strcpy(ptr++, ",");
    strcpy(ptr, tag);
    msg->usd += len+womsg_npads(len);
    msg->rwi = &msg->buf[msg->usd];
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
        if (msg->idx >= womsg_getcnt(msg))
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

static inline int
womsg_write(struct womsg* msg, void* value, size_t sz)
{
    memcpy(msg->rwi, value, sz);
    msg->rwi += sz;
    msg->idx++;
    if (msg->mode == WOMSG_WTAGLOCKED) {
        char next;
        next = _nexttag(msg);
        msg->usd += sz;
        if (next == 'T' || next == 'F')
            msg->idx++;
        if (next == 0)
            // we finished writing, now we can set it in read only mode
            msg->mode = WOMSG_READ_ONLY;
    }
    return 0;
}

int
womsg_writei(struct womsg* msg, int32_t value)
{
    int err;
    if (!(err = womsg_checkw(msg, 'i', sizeof(int32_t)))) {
        womsg_write(msg, &value, sizeof(int32_t));
    }
    return err;
}

int
womsg_writef(struct womsg* msg, float value)
{
    int err;
    if (!(err = womsg_checkw(msg, 'f', sizeof(float)))) {
        womsg_write(msg, &value, sizeof(float));
    }
    return err;
}

int
womsg_writec(struct womsg* msg, int8_t value)
{
    int err;
    if (!(err = womsg_checkw(msg, 'c', sizeof(int8_t)))) {
        womsg_write(msg, &value, sizeof(int8_t));
        msg->rwi += 3;
        msg->usd += 3;
    }
    return err;
}

int
womsg_writeb(struct womsg* msg, bool value)
{
    // no need to set the value is tag has already been set obviously..
    if (msg->mode == WOMSG_WTAGLOCKED)
        return 4;
    return 1;
}

int
womsg_writes(struct womsg* msg, const char* str)
{
    int err, len, npads;
    len = strlen(str);
    npads = womsg_npads(len);
    if (!(err = womsg_checkw(msg, 's', len))) {
        womsg_write(msg, str, len);
        msg->rwi += npads;
        msg->usd += npads;
    }
    return 0;
}

int
womsg_readi(struct womsg* msg, int32_t* dst)
{
    // check read-mode, check type...
    memcpy(dst, (int*)msg->idx, sizeof(int));
    msg->idx += sizeof(int);
    return 1;
}

int
womsg_readf(struct womsg* msg, float* dst)
{
    return 1;
}

int
womsg_readb(struct womsg* msg, bool* dst)
{
    *dst = _nexttag(msg) == 'T';
    msg->idx++;
    return 0;
}

int
womsg_readc(struct womsg* msg, int8_t* dst)
{

}

int
womsg_reads(struct womsg* msg, char** dst)
{
    return 1;
}




