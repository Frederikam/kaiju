#include <assert.h>
#include <iostream>
#include <stdlib.h>
extern "C" {
#include <wayland-server-core.h>
#include <wayland-util.h>
#include <wlr/backend.h>
#include <wlr/types/wlr_gamma_control_v1.h>
#include <wlr/types/wlr_idle.h>
#include <wlr/types/wlr_output.h>
#include <wlr/types/wlr_primary_selection_v1.h>
#include <wlr/types/wlr_screencopy_v1.h>
#include <wlr/types/wlr_xdg_shell.h>
#define static
#include <wlr/types/wlr_matrix.h>
#include <wlr/types/wlr_compositor.h>
#include <wlr/render/wlr_renderer.h>
#undef static
}

#include "kaiju_output.hpp"
#include "kaiju_server.hpp"

static void output_destroy_notify(struct wl_listener *listener, void *data) {
    struct kaiju_output *output = (struct kaiju_output *)wl_container_of(listener, output, destroy);
    wl_list_remove(&output->link);
    wl_list_remove(&output->destroy.link);
    wl_list_remove(&output->frame.link);
    free(output);
}

static void output_frame_notify(struct wl_listener *listener, void *data) {
    struct kaiju_output *output = (struct kaiju_output *)wl_container_of(listener, output, frame);
    struct kaiju_server *server = output->server;
    struct wlr_output *wlr_output = (struct wlr_output *)data;
    struct wlr_renderer *renderer = wlr_backend_get_renderer(
            wlr_output->backend);

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);

    wlr_output_attach_render(wlr_output, NULL);
    wlr_renderer_begin(renderer, wlr_output->width, wlr_output->height);

    float color[4] = {1.0, 0, 0, 1.0};
    wlr_renderer_clear(renderer, color);

    struct wl_resource *_surface;
    wl_resource_for_each(_surface, &server->compositor->surface_resources) {
        struct wlr_surface *surface = wlr_surface_from_resource(_surface);

        if (!wlr_surface_has_buffer(surface)) continue;
        
        struct wlr_texture *texture = wlr_surface_get_texture(surface);

        struct wlr_box render_box = {
                .x = 20,
                .y = 20,
                .width = surface->current.width,
                .height = surface->current.height};

        float matrix[16];
        wlr_matrix_project_box(matrix, &render_box, surface->current.transform, 0, wlr_output->transform_matrix);
        wlr_render_texture_with_matrix(renderer, texture, matrix, 1.0f);
        wlr_surface_send_frame_done(surface, &now);
    }

    //https://github.com/swaywm/wlroots/commit/5e6766a165bd4bc71f1dc24c4348f7be0f020ddd
    //wlr_output_swap_buffers(wlr_output, NULL, NULL);
    wlr_output_commit(output->wlr_output);
    wlr_renderer_end(renderer);
}

static void new_output_notify(struct wl_listener *listener, void *data) {
    struct kaiju_server *server = wl_container_of(listener, server, new_output);
    struct wlr_output *wlr_output = (struct wlr_output *)data;

    if (!wl_list_empty(&wlr_output->modes)) {
        struct wlr_output_mode *mode =
                wl_container_of(wlr_output->modes.prev, mode, link);
        wlr_output_set_mode(wlr_output, mode);
    }

    struct kaiju_output *output = (struct kaiju_output *)calloc(1, sizeof(struct kaiju_output));
    clock_gettime(CLOCK_MONOTONIC, &output->last_frame);
    output->server = server;
    output->wlr_output = wlr_output;
    wl_list_insert(&server->outputs, &output->link);

    output->destroy.notify = output_destroy_notify;
    wl_signal_add(&wlr_output->events.destroy, &output->destroy);
    output->frame.notify = output_frame_notify;
    wl_signal_add(&wlr_output->events.frame, &output->frame);
}

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
        std::cerr << "Failed to start backend\n";
        wl_display_destroy(server.wl_display);
        return 1;
    }

    std::cout << "Running compositor on wayland display " << socket << "\n";
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

    std::cout << "\u001B[32mIt runs!\u001B[0m\n";
    return 0;
}
