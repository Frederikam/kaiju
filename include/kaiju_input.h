#pragma once
#include "../include/kaiju_server.h"

struct kaiju_keyboard {
    struct wl_list link;
    struct kaiju_server *server;
    struct wlr_input_device *device;

    struct wl_listener modifiers;
    struct wl_listener key;
};

void configure_input(struct kaiju_server *server);
