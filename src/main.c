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
#include <wlr/types/wlr_output_layout.h>
#include <wlr/types/wlr_data_device.h>

#include <kaiju_output.h>
#include <shell/xdg.h>
#include <output.h>
#include <config_loader.h>
#include <kaiju_input.h>

int main(int argc, char **argv) {
    struct kaiju_server server;
    config_load();

    server.wl_display = wl_display_create();
    assert(server.wl_display);
    server.wl_event_loop = wl_display_get_event_loop(server.wl_display);
    assert(server.wl_event_loop);

    server.backend = wlr_backend_autocreate(server.wl_display, NULL);
    assert(server.backend);

    server.renderer = wlr_backend_get_renderer(server.backend);
    wlr_renderer_init_wl_display(server.renderer, server.wl_display);

    /* Creates an output layout, which a wlroots utility for working with an
	 * arrangement of screens in a physical layout. */
    server.output_layout = wlr_output_layout_create();

    wl_list_init(&server.outputs);
    server.new_output.notify = new_output_notify;
    wl_signal_add(&server.backend->events.new_output, &server.new_output);

    /* Set up our list of views and the xdg-shell.
	 * https://drewdevault.com/2018/07/29/Wayland-shells.html
	 */
    wl_list_init(&server.views);
    server.xdg_shell = wlr_xdg_shell_create(server.wl_display);
    server.new_xdg_surface.notify = server_new_xdg_surface;
    wl_signal_add(&server.xdg_shell->events.new_surface, &server.new_xdg_surface);

    const char *socket = wl_display_add_socket_auto(server.wl_display);
    assert(socket);

    configure_input(&server);

    if (!wlr_backend_start(server.backend)) {
        fprintf(stdout, "Failed to start backend\n");
        wlr_backend_destroy(server.backend);
        wl_display_destroy(server.wl_display);
        return 1;
    }

    printf("\x1B[32mRunning compositor on wayland display '%s'\x1B[0m\n", socket);
    setenv("WAYLAND_DISPLAY", socket, true);

    wl_display_init_shm(server.wl_display);
    wlr_gamma_control_manager_v1_create(server.wl_display);
    wlr_screencopy_manager_v1_create(server.wl_display);
    wlr_primary_selection_v1_device_manager_create(server.wl_display);
    wlr_idle_create(server.wl_display);

    server.compositor = wlr_compositor_create(
            server.wl_display,
            wlr_backend_get_renderer(server.backend)
    );
    wlr_data_device_manager_create(server.wl_display);

    wl_display_run(server.wl_display);
    wl_display_destroy_clients(server.wl_display);
    wl_display_destroy(server.wl_display);

    return 0;
}
