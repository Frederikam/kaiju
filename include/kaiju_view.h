#include <stdlib.h>
#include <wayland-util.h>
#include <wayland-server-core.h>
#include <bridge/view.h>

struct kaiju_view {
    struct wl_list link;
	struct kaiju_server *server;
	struct wlr_xdg_surface *xdg_surface;
	struct wl_listener map;
	struct wl_listener unmap;
	struct wl_listener destroy;
	struct wl_listener request_move;
	struct wl_listener request_resize;
	bool mapped;
	struct view_props props;
};

void server_new_xdg_surface(struct wl_listener *listener, void *data);