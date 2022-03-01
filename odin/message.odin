package osc

import slice "core:slice"
import str "core:strings"
import fmt "core:fmt"
import mem "core:mem"

Flags :: enum {
     Invalid,
  Tag_Locked,
    Tag_Free,
       Write,
        Read,
}

Error :: enum i32 {
            None,
       Read_Only,
         Tag_End,
     Tag_Mismatch,
      Tag_Invalid,
  Message_Invalid,
      Uri_Invalid,
  Buffer_Overflow,
}

Result :: union(T: typeid) #no_nil { T, Error }

Message :: struct {
     buf: []byte,  // byte buffer holding the entire message
    ttag: int,     // typetag starting index
    data: int,     // data starting index
    rw_d: int,     // r/w data pointer
    rw_t: int,     // read/write tag index
    mode: Flags,   // flags
}

// ----------------------------------------------------------------------------
// encoding
// ----------------------------------------------------------------------------

@(require_results)
encode :: proc(msg: ^Message, buf: []byte, uri: string) -> Error {
    msg.buf  = buf;
    msg.mode = Flags.Tag_Free;
    return set_uri(msg, uri);
}

@(require_results)
set_uri :: proc(msg: ^Message, uri: string) -> Error {
    if check_uri(uri) {
        return Error.Uri_Invalid;
    }
    size := len(uri);
    size += npads(size);
    copy(msg.buf[:], uri);
    msg.ttag = size;
    return Error.None;
}

@(private)
write_data :: proc(using msg: ^Message, v: $T) {
    sz := size_of(T);
    cp := v;
    wptr, ok := slice.get_ptr(buf, data+rw_d);
    assert(ok);
    mem.copy(wptr, &cp, sz);
    rw_d += sz;
}

@(private)
write_ttag :: #force_inline proc(using msg: ^Message, tag: byte) {
    buf[ttag+rw_t] = tag;
    rw_t += 1;
}

// write procedure overloads
write :: proc {
    write_i32,
    write_f32,
    write_rune,
    write_string,
    // todo: cstring, boolean, midi, timetag, etc.
};

@(private, require_results)
write_check :: proc(using msg: ^Message, size: int, tag: byte) -> Error {
    if mode > Flags.Write {
       return Error.Read_Only;
    }
    // check remaining space
    rmn := remaining(msg);
    if size + npads(size) > rmn {
       return Error.Buffer_Overflow;
    }
    // other error checks TODO
    return Error.None;
}

@(require_results)
write_i32 :: proc(using msg: ^Message, value: i32) -> Error {
    err := write_check(msg, 4, 'i');
    if err == .None {
       write_ttag(msg, 'i');
       write_data(msg, value);
    }
    return err;
}

@(require_results)
write_rune :: proc(using msg: ^Message, value: rune) -> Error {
    err := write_check(msg, 4, 'c');
    if err == .None {
       write_ttag(msg, 'c');
       write_data(msg, value);
    }
    return err;
}

@(require_results)
write_f32 :: proc(using msg: ^Message, value: f32) -> Error {
    err := write_check(msg, 4, 'f');
    if err == .None {
       write_ttag(msg, 'f');
       write_data(msg, value);
    }
    return err;
}

@(require_results)
write_string :: proc(using msg: ^Message, value: string) -> Error {
    err := write_check(msg, len(value), 's');
    if err == .None {
        write_ttag(msg, 's');
        copy(buf[data+rw_d:], value);
        rw_d += len(value);
    }
    return err;
}

// sets all message arguments/values
// message's tag is locked from this point on
@(require_results)
write_lock :: proc(using msg: ^Message, args: ..any) -> Error
{
    tlen := len(args)+1;
    tlen += npads(tlen);
    mode = Flags.Tag_Locked;
    data = ttag+tlen;
    write_ttag(msg, ',');

    for value in args {
        switch v in value {
        case int: {
        if err := write_i32(msg, i32(v));
           err != .None do return err;
        }
        case f64: {
        if err := write_f32(msg, f32(v));
           err != .None do return err;
        }
        case rune: {
        if err := write_rune(msg, v);
           err != .None do return err;
        }
        case string: {
        if err := write_string(msg, v);
           err != .None do return err;
        }
        case bool: {
            if v do write_ttag(msg, 'T');
            else do write_ttag(msg, 'F');
        }
        }
    }
    return Error.None;
}

// ----------------------------------------------------------------------------
// decoding
// ----------------------------------------------------------------------------

// decodes a raw osc-message stored in <buf>
@(require_results)
decode :: proc(using dst: ^Message, buffer: []byte) -> Error {
    ttag  = bytestr_len(buffer);
    ttag += npads(ttag);
    data  = bytestr_len(buf[ttag:]);
    data += npads(data);
    data += ttag;
    mode = Flags.Read;
    buf  = buffer;
    rw_t = 1; // skip the comma
    rw_d = 0;
    return Error.None;
}

// returns message's typetag line as string
tag :: #force_inline proc(using msg: ^Message) -> string {
    return byte2str(buf[ttag+1:]);
}

// returns message's uri as string
uri :: #force_inline proc(using msg: ^Message) -> string {
    return byte2str(buf);
}

// returns the number of bytes used in message's buffer
nbytes :: #force_inline proc(using msg: ^Message) -> int {
    return data+rw_d;
}

// returns space left in bytes
remaining :: #force_inline proc(using msg: ^Message) -> int {
    return len(buf)-nbytes(msg);
}

// returns argument count
argc :: #force_inline proc(using msg: ^Message) -> int {
    return bytestr_len(buf[ttag:])-1;
}

// reads next value (if any) as f32
@(require_results)
read_f32 :: proc(using msg: ^Message) -> Result(f32) {
    ptr := &buf[data+rw_d];
    v := transmute(^f32)(ptr);
    rw_d += 4;
    rw_t += 1;
    return v^;
}

// reads next value (if any) as i32
@(require_results)
read_i32 :: proc(using msg: ^Message) -> Result(i32) {
    ptr := &buf[data+rw_d];
    v := transmute(^i32)(ptr);
    rw_d += 4;
    rw_t += 1;
    return v^;
}

// reads next value (if any) as a rune
@(require_results)
read_rune :: proc(using msg: ^Message) -> Result(rune) {
    ptr := &buf[data+rw_d];
    v := transmute(^rune)(ptr);
    rw_d += 4;
    rw_t += 1;
    return v^;
}

// reads next value (if any) as boolean
@(require_results)
read_bool :: proc(using msg: ^Message) -> Result(bool) {
    r := buf[ttag+rw_t] == 'T';
    rw_t += 1;
    return r;
}

// reads next value (if any) as a cstring
@(require_results)
read_cstring :: proc(using msg: ^Message) -> Result(cstring) {
    p := cstring(&buf[data+rw_d]);
    l := len(p);
    l += npads(l);
    rw_t += 1;
    rw_d += l;
    return p;
}

// reads next value (if any) as a odin-string
@(require_results)
read_string :: proc(using msg: ^Message) -> Result(string) {
    switch v in read_cstring(msg) {
          case Error: return v;
          case cstring: return string(v);
    }
    return Error.None;
}

print_raw :: proc(using msg: ^Message) {
    fmt.println("osc message raw data:", buf);
}

// ----------------------------------------------------------------------------
// utilities
// ----------------------------------------------------------------------------

@(private)
bytestr_len :: #force_inline proc(s: []byte) -> int {
    ptr := slice.as_ptr(s);
    return len(cstring(ptr));
}

@(private)
byte2str :: #force_inline proc(s: []byte) -> string {
    return string(cstring(slice.as_ptr(s)));
}

@(private)
next_tag :: #force_inline proc(using msg: ^Message) -> byte {
     defer rw_t += 1;
    return buf[ttag + rw_t];
}

@(private)
npads :: #force_inline proc(sz: int) -> int {
    return 4-(sz%4);
}

@(private, require_results)
check_uri :: #force_inline proc(uri: string) -> bool {
    // TODO
    return false;
}
