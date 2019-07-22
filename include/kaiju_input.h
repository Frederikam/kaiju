#include "../include/kaiju_server.h"

enum kaiju_cursor_mode {
    KAIJU_CURSOR_PASSTHROUGH,
    KAIJU_CURSOR_MOVE,
    KAIJU_CURSOR_RESIZE,
};

struct kaiju_keyboard {
    struct wl_list link;
    struct kaiju_server *server;
    struct wlr_input_device *device;

    struct wl_listener modifiers;
    struct wl_listener key;
};

void configure_cursor(struct kaiju_server* server);
