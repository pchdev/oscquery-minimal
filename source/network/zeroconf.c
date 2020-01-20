#include <wpn114/network/zeroconf.h>
#include <dependencies/mongoose/mongoose.h>

// there surely is a lighter udp socket lib than mgws here...
struct wzservice {
    struct mg_mgr mgr;
};

struct wzbrowser {
    struct mg_mgr mgr;
};
