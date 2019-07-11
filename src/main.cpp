#include <iostream>
#include <assert.h>
#include <wayland-server-core.h>
#include "kaiju_server.hpp"

int main(int argc, char **argv) {
    struct kaiju_server server;

    server.wl_display = wl_display_create();
    assert(server.wl_display);
    server.wl_event_loop = wl_display_get_event_loop(server.wl_display);
    assert(server.wl_event_loop);
    std::cout << "\u001B[32mIt runs!\u001B[0m\n";
    return 0;
}
