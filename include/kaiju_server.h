#include <wayland-server-core.h>
#include <wlr/types/wlr_xdg_shell.h>
#include <wlr/types/wlr_seat.h>
#include <wlr/backend.h>

enum kaiju_cursor_mode {
    KAIJU_CURSOR_PASSTHROUGH,
    KAIJU_CURSOR_MOVE,
    KAIJU_CURSOR_RESIZE,
};

struct kaiju_server {
    struct wl_display *wl_display;
    struct wl_event_loop *wl_event_loop;
    struct wlr_xdg_shell *xdg_shell;

    // *** Input and cursor lifecycle ***
    struct wlr_seat *seat;
    struct wl_listener new_input;
    struct wl_listener request_cursor;
    struct wl_list keyboards;

    // *** Cursor ***
    struct wlr_cursor *cursor;
    struct wlr_xcursor_manager *cursor_mgr;
    struct wl_listener cursor_motion;
    struct wl_listener cursor_motion_absolute;
    struct wl_listener cursor_button;
    struct wl_listener cursor_axis;
    struct wl_listener cursor_frame;

    // *** Grabbing ***
    enum kaiju_cursor_mode cursor_mode;
    struct kaiju_view *grabbed_view;
    double grab_x, grab_y;
    int grab_width, grab_height;
    uint32_t resize_edges;

    // *** Output ***
    /** Responsible for abstracting the IO devices (keyboard, monitors, etc) */
    struct wlr_backend *backend;
    /** Global which clients can add surfaces to */
    struct wlr_compositor *compositor;
    struct wlr_renderer *renderer;
    struct wl_list outputs; // kaiju_output::link
    struct wlr_output_layout *output_layout;
    struct wl_listener new_output;
    struct wl_listener new_xdg_surface;
    struct wl_list views;
};
