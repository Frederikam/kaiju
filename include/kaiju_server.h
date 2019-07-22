#include <wayland-server-core.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_seat.h>

struct kaiju_server {
    struct wl_display *wl_display;
    struct wl_event_loop *wl_event_loop;
    struct wlr_xdg_shell *xdg_shell;

    struct wlr_cursor *cursor;
    struct wlr_xcursor_manager *cursor_mgr;

    /** Responsible for abstracting the IO devices (keyboard, monitors, etc) */
    struct wlr_backend *backend;
    /** Global which clients can add surfaces to */
    struct wlr_compositor *compositor;
    struct wlr_seat *seat;
    struct wl_list views;
    struct wl_list outputs; // kaiju_output::link
    struct wlr_output_layout *output_layout;

    struct wl_listener new_output;
    struct wl_listener new_xdg_surface;
};
