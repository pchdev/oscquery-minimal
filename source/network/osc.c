#include <wpn114/network/osc.h>
#include <wpn114/utilities.h>

enum womsg_err {
    WOMSG_NOERROR,
    WOMSG_READ_ONLY,
    WOMSG_WRITE_ONLY,
    WOMSG_TAG_MISMATCH,
    WOMSG_TAG_END,
    WOMSG_BUFFER_OVERFLOW,
    WOMSG_URI_INVALID
};

int
wuri_check(const char* uri)
{
    char c;
    if (*uri != '/')
        return WOMSG_URI_INVALID;
    // TODO:
    // pattern match the rest,
    // basically, check invalid characters
    return 0;
}

int
wuri_seglen(const char* uri)
{
    const char* ucpy = uri;
    if (*ucpy == '/')
        ucpy++;
    while (*ucpy != '/' && *ucpy != 0)
        ucpy++;
    return ucpy-uri;
}

int
wuri_seglen_max(const char* u1, const char* u2)
{
    return wpnmax(wuri_seglen(u1), wuri_seglen(u2));
}

int
wuri_segcmp(const char* u1, const char* u2, int* len)
{
    // optimizations are hard, but can maybe be done here...
    *len = wpnmax(wuri_seglen(u1), wuri_seglen(u2));
    return strncmp(u1, u2, *len);
}

int
wuri_depth(const char* uri)
{
    unsigned int depth = 0;
    while (*uri)
        if (*uri++ == '/')
            depth++;
    return depth;
}

bool
wuri_depth_eq(const char* uri, unsigned int eq)
{
    unsigned int depth = 0;
    while (*uri)
        if (*uri++ == '/')
            if (++depth > eq)
                return false;
    return depth == eq;
}

const char*
wuri_last(const char* uri)
{
    int len = strlen(uri);
    const char* last = &uri[len-1];
    while (*--last != '/')
          ;
    return last;
}

int
wuri_parent(const char* uri, char* buf)
{
    const char* last = wuri_last(uri);
    memcpy(buf, uri, last-uri);
    // null terminate it
    buf[last-uri] = 0;
    return 0;
}

const char*
wosc_strerr(int err)
{
    switch (err) {
    case WOMSG_NOERROR:
        return "no error";
    case WOMSG_READ_ONLY:
        return "message is in read-only mode";
    case WOMSG_WRITE_ONLY:
        return "message is in write-only mode";
    case WOMSG_TAG_MISMATCH:
        return "mismatching argument tag for read/write function";
    case WOMSG_TAG_END:
        return "reaching end of arguments";
    case WOMSG_BUFFER_OVERFLOW:
        return "buffer overflow";
    case WOMSG_URI_INVALID:
        return "invalid method/uri";
    default:
        return "unknown error code";
    }
}

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

int
womsg_walloc(struct walloc_t* _allocator, womsg_t** _dst)
{
    return _allocator->alloc(_dst,
           sizeof(struct womsg),
           _allocator->data);
}

#define WOMSG_INVALID       0u
#define WOMSG_WTAGLOCKED    1u
#define WOMSG_WTAGFREE      2u
#define WOMSG_W             3u
#define WOMSG_R             4u

#define _tag(_womsg)        ((char*)(&(_womsg->buf[_womsg->tag])))
#define _tagnc(_womsg)      (_tag(_womsg)+1)
#define _nexttag(_womsg)    (*(_tagnc(_womsg)+_womsg->idx))

static __always_inline int
womsg_npads(int sz) { return 4-(sz%4); }

void
womsg_printraw(struct womsg* dst)
{
    int n;
    printf("%d", dst->buf[0]);
    for (n = 1; n < dst->usd; ++n)
        printf(", %d", dst->buf[n]);
    printf("\n");
}

int
womsg_decode(struct womsg* dst, byte_t* src, uint32_t len)
{
    int ulen, tlen;
    dst->buf = src;
    dst->ble = len;
    dst->usd = len;
    dst->mode = WOMSG_R;

    ulen = strlen((char*)src);
    ulen += womsg_npads(ulen);
    dst->tag = ulen;

    tlen = strlen((char*)&src[ulen])+1;
    tlen += womsg_npads(tlen);
    dst->rwi = &src[ulen+tlen];

    return 0;
}

int
womsg_setbuf(struct womsg* msg, byte_t* buf, uint32_t len)
{
    msg->buf = buf;
    msg->ble = len;
    // set message in write mode
    msg->mode = WOMSG_W;
    memset(buf, 0, len);
    return 0;
}

int
womsg_seturi(struct womsg* msg, const char* uri)
{
    // check uri
    int err, len;
    if ((err = wuri_check(uri)))
        return err;
    if (msg->mode > WOMSG_W)
        return WOMSG_READ_ONLY;
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

static int
womsg_checktag(const char* tag)
{
    return 0;
}

int
womsg_settag(struct womsg* msg, const char* tag)
{
    // TODO:
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
womsg_getargc(struct womsg* msg)
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
        if (msg->idx >= womsg_getargc(msg))
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
        else if (next == 0) {
            int len;
            // we are finished writing,
            // we can now set the message in read-only mode,
            // resetting tag and r/w indexes.
            msg->mode = WOMSG_R;
            msg->idx = 0;
            len = womsg_getargc(msg)+1;
            len += womsg_npads(len);
            msg->rwi = (byte_t*)(&(msg->buf[msg->tag])+len);
        }
    }
    return 0;
}

int
womsg_writei(struct womsg* msg, int32_t value)
{
    int err;
    if (!(err = womsg_checkw(msg, 'i', sizeof(int32_t))))
        womsg_write(msg, &value, sizeof(int32_t));
    return err;
}

int
womsg_writef(struct womsg* msg, float value)
{
    int err;
    if (!(err = womsg_checkw(msg, 'f', sizeof(float))))
        womsg_write(msg, &value, sizeof(float));
    return err;
}

int
womsg_writec(struct womsg* msg, char value)
{
    int err;
    // we convert to int32
    int32_t c = value;
    if (!(err = womsg_checkw(msg, 'c', sizeof(int32_t))))
        womsg_write(msg, &c, sizeof(int32_t));
    return err;
}

int
womsg_writeb(struct womsg* msg, bool value)
{
    // no need to set the value if tag has already been set obviously..
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
        if (msg->mode < WOMSG_R)
            // if we're still in write mode
            // after this, add the pads
            msg->rwi += npads;
        msg->usd += npads;
    }
    return 0;
}

static int
womsg_checkr(struct womsg* msg, char tp)
{
    if (msg->mode < WOMSG_R)
        return WOMSG_WRITE_ONLY;
    if (_nexttag(msg) == 0)
        return WOMSG_TAG_END;
    if (tp == 'T') {
        char n = _nexttag(msg);
        if (!(n == 'T' || n == 'F'))
            return WOMSG_TAG_MISMATCH;
    }
    else if (_nexttag(msg) != tp)
        return WOMSG_TAG_MISMATCH;
    return 0;
}

static inline int
womsg_read(struct womsg* msg, void* dst, size_t tpsz)
{
    memcpy(dst, msg->rwi, tpsz);
    msg->idx++;
    msg->rwi += tpsz;
    return 0;
}

int
womsg_readi(struct womsg* msg, int32_t* dst)
{    
    int err;
    if (!(err = womsg_checkr(msg, 'i')))
        womsg_read(msg, dst, sizeof(int32_t));
    return err;
}

int
womsg_readf(struct womsg* msg, float* dst)
{
    int err;
    if (!(err = womsg_checkr(msg, 'f')))
        womsg_read(msg, dst, sizeof(float));
    return err;
}

int
womsg_readb(struct womsg* msg, bool* dst)
{
    int err;
    if (!(err = womsg_checkr(msg, 'T'))) {
        *dst = _nexttag(msg) == 'T';
        msg->idx++;
    }
    return err;
}

int
womsg_readc(struct womsg* msg, char* dst)
{
    int err, c;
    if (!(err = womsg_checkr(msg, 'c'))) {
        womsg_read(msg, &c, sizeof(int32_t));
        *dst = (char) c;
    }
    return err;
}

int
womsg_reads(struct womsg* msg, char** dst)
{
    int err;
    if (!(err = womsg_checkr(msg, 's'))) {
        *dst = (char*)msg->rwi;
        msg->idx++;
    }
    return err;
}

int
womsg_readv(struct womsg* msg, wvalue_t* v)
{
    v->t = *womsg_gettag(msg);
    switch (v->t) {
    case WOSC_TYPE_INT:
        return womsg_readi(msg, &v->u.i);
    case WOSC_TYPE_TRUE:
    case WOSC_TYPE_FALSE:
        return womsg_readb(msg, &v->u.b);
    case WOSC_TYPE_CHAR:
        return womsg_readc(msg, &v->u.c);
    case WOSC_TYPE_FLOAT:
        return womsg_readf(msg, &v->u.f);
    case WOSC_TYPE_STRING: {
        int err;
        char* s;
        if (!(err = womsg_reads(msg, &s))) {
            if (strlen(s) > v->u.s->cap)
                return 32;
            strcpy(v->u.s->dat, s);
        }
        return err;
    }
    default: return 1;
    }
}
