#define _POSIX_C_SOURCE 200809L

#include <assert.h>
#include <stdlib.h>
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/backend.h>
#include <wlr/types/wlr_gamma_control_v1.h>
#include <wlr/types/wlr_idle.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_primary_selection_v1.h>
#include <wlr/types/wlr_screencopy_v1.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_compositor.h>

#include "kaiju_output.h"
#include "kaiju_server.h"
#include "output.h"

int main(int argc, char **argv) {
    struct kaiju_server server;

    server.wl_display = wl_display_create();
    assert(server.wl_display);
    server.wl_event_loop = wl_display_get_event_loop(server.wl_display);
    assert(server.wl_event_loop);

    server.backend = wlr_backend_autocreate(server.wl_display, NULL);
    assert(server.backend);

    wl_list_init(&server.outputs);
    server.new_output.notify = new_output_notify;
    wl_signal_add(&server.backend->events.new_output, &server.new_output);

    const char *socket = wl_display_add_socket_auto(server.wl_display);
    assert(socket);

    if (!wlr_backend_start(server.backend)) {
        fprintf(stdout, "Failed to start backend\n");
        wl_display_destroy(server.wl_display);
        return 1;
    }

    fprintf(stdout, "Running compositor on wayland display '%s'\n", socket);
    setenv("WAYLAND_DISPLAY", socket, true);

    wl_display_init_shm(server.wl_display);
    wlr_gamma_control_manager_v1_create(server.wl_display);
    wlr_screencopy_manager_v1_create(server.wl_display);
    wlr_primary_selection_v1_device_manager_create(server.wl_display);
    wlr_idle_create(server.wl_display);

    server.compositor = wlr_compositor_create(server.wl_display,
            wlr_backend_get_renderer(server.backend));

    // Gives a surface a role
    wlr_xdg_shell_create(server.wl_display);

    wl_display_run(server.wl_display);
    wl_display_destroy(server.wl_display);

    fprintf(stdout, "\x1B[32mIt runs!\x1B[0m\n");
    return 0;
}
