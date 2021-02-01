package main

import osc ".."
import "core:fmt"

#assert(size_of(osc.Result(f32)) == 8);

test_01 :: proc()
{
    using osc;
    buf: [64]byte;
    msg: osc.Message;

    assert(osc.encode(&msg, buf[:], "/foo/bar") == Error.None);
    assert(
      osc.write_lock(&msg, 31.6, 47, true,  'Y', "owls are not what they seem")
      == Error.None
    );
    assert(osc.remaining(&msg) == 5);
    osc.print_raw(&msg);
    fmt.println("remaining data:", osc.remaining(&msg), "bytes");
    
    // now decode
    err := osc.decode(&msg, buf[:]);
    assert(osc.uri(&msg) == "/foo/bar");
    assert(osc.tag(&msg) == "fiTcs");
    assert(osc.argc(&msg) == 5);

    for c in osc.tag(&msg) do
        fmt.println(rune(c));

    fmt.println(osc.read_f32(&msg)); // 31.6
    fmt.println(osc.read_i32(&msg)); // 47
    fmt.println(osc.read_bool(&msg)); // true
    fmt.println(osc.read_rune(&msg)); // 'Y';
    fmt.println(osc.read_string(&msg)); // owls are not what they seem
}

main :: proc() {
    test_01();
}
