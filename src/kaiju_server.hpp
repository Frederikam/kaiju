#include <wayland-server-core.h>

class kaiju_server {
    public:
    struct wl_display *wl_display;
    struct wl_event_loop *wl_event_loop;
    /** Responsible for abstracting the IO devices (keyboard, monitors, etc) */
    struct wlr_backend *backend;
    struct wl_listener new_output;
    struct wl_list outputs; // mcw_output::link
};
