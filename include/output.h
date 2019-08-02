#pragma once
#include <wayland-server-core.h>
void output_destroy_notify(struct wl_listener *listener, void *data);
void new_output_notify(struct wl_listener *listener, void *data);
