#include <wayland-util.h>
#include <wlr/types/wlr_keyboard.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_cursor.h>
#include <kaiju_output.h>
#include <kaiju_server.h>
#include "./include/shell/kaiju_view.h"
#include "./include/shell/xdg.h"

/* Called when the surface is mapped, or ready to display on-screen. */
static void xdg_surface_map(struct wl_listener *listener, void *data) {
    struct kaiju_view *view = wl_container_of(listener, view, map);
    view->mapped = true;
    focus_view(view, view->xdg_surface->surface);
}

/* Called when the surface is unmapped, and should no longer be shown. */
static void xdg_surface_unmap(struct wl_listener *listener, void *data) {
    struct kaiju_view *view = wl_container_of(listener, view, unmap);
    view->mapped = false;
}

/* Called when the surface is destroyed and should never be shown again. */
static void xdg_surface_destroy(struct wl_listener *listener, void *data) {
    struct kaiju_view *view = wl_container_of(listener, view, destroy);
    wl_list_remove(&view->link);
    free(view);
}

static void begin_interactive(struct kaiju_view *view, enum kaiju_cursor_mode mode, uint32_t edges) {
    /* This function sets up an interactive move or resize operation, where the
     * compositor stops propagating pointer events to clients and instead
     * consumes them itself, to move or resize windows. */
    struct kaiju_server *server = view->server;
    struct wlr_surface *focused_surface = server->seat->pointer_state.focused_surface;

    // Deny move/resize requests from unfocused clients.
    if (view->xdg_surface->surface != focused_surface) return;
    server->grabbed_view = view;
    server->cursor_mode = mode;
    struct wlr_box geo_box;
    wlr_xdg_surface_get_geometry(view->xdg_surface, &geo_box);
    if (mode == KAIJU_CURSOR_MOVE) {
        server->grab_x = server->cursor->x - view->props.x;
        server->grab_y = server->cursor->y - view->props.y;
    } else {
        server->grab_x = server->cursor->x + geo_box.x;
        server->grab_y = server->cursor->y + geo_box.y;
    }
    server->grab_width = geo_box.width;
    server->grab_height = geo_box.height;
    server->resize_edges = edges;
}

static void xdg_toplevel_request_move(struct wl_listener *listener, void *data) {
    // Invoked when a window says it is being dragged by the cursor
    struct kaiju_view *view = wl_container_of(listener, view, request_move);
    begin_interactive(view, KAIJU_CURSOR_MOVE, 0);
}

static void xdg_toplevel_request_resize(struct wl_listener *listener, void *data) {
    // Invoked when a window says it is being resized by the cursor
    struct wlr_xdg_toplevel_resize_event *event = data;
    struct kaiju_view *view = wl_container_of(listener, view, request_resize);
    begin_interactive(view, KAIJU_CURSOR_RESIZE, event->edges);
}

void server_new_xdg_surface(struct wl_listener *listener, void *data) {
    /* This event is raised when wlr_xdg_shell receives a new xdg surface from a
     * client, either a toplevel (application window) or popup. */
    fprintf(stdout, "New XDG surface\n");
    struct kaiju_server *server = wl_container_of(listener, server, new_xdg_surface);
    struct wlr_xdg_surface *xdg_surface = data;
    if (xdg_surface->role != WLR_XDG_SURFACE_ROLE_TOPLEVEL) {
        return;
    }

    /* Allocate a kaiju_view for this surface */
    struct kaiju_view *view = calloc(1, sizeof(struct kaiju_view));
    view->server = server;
    view->xdg_surface = xdg_surface;

    /* Listen to the various events it can emit */
    view->map.notify = xdg_surface_map;
    wl_signal_add(&xdg_surface->events.map, &view->map);
    view->unmap.notify = xdg_surface_unmap;
    wl_signal_add(&xdg_surface->events.unmap, &view->unmap);
    view->destroy.notify = xdg_surface_destroy;
    wl_signal_add(&xdg_surface->events.destroy, &view->destroy);

    /* cotd */
    struct wlr_xdg_toplevel *toplevel = xdg_surface->toplevel;
    view->request_move.notify = xdg_toplevel_request_move;
    wl_signal_add(&toplevel->events.request_move, &view->request_move);
    view->request_resize.notify = xdg_toplevel_request_resize;
    wl_signal_add(&toplevel->events.request_resize, &view->request_resize);

    /* Add it to the list of views. */
    wl_list_insert(&server->views, &view->link);
}