#include <stdlib.h>
#include <wayland-util.h>
#include <wlr/types/wlr_cursor.h>
#include <wlr/util/edges.h>
#include <wlr/types/wlr_xcursor_manager.h>
#include "../include/kaiju_view.h"
#include "../include/kaiju_input.h"

static void keyboard_handle_modifiers(struct wl_listener *listener, void *data) {
    /* This event is raised when a modifier key, such as shift or alt, is
     * pressed. We simply communicate this to the client. */
    struct kaiju_keyboard *keyboard = wl_container_of(listener, keyboard, modifiers);
    /*
     * A seat can only have one keyboard, but this is a limitation of the
     * Wayland protocol - not wlroots. We assign all connected keyboards to the
     * same seat. You can swap out the underlying wlr_keyboard like this and
     * wlr_seat handles this transparently.
     */
    wlr_seat_set_keyboard(keyboard->server->seat, keyboard->device);
    /* Send modifiers to the client. */
    wlr_seat_keyboard_notify_modifiers(
            keyboard->server->seat,
            &keyboard->device->keyboard->modifiers
    );
}

static bool handle_keybinding(struct kaiju_server *server, xkb_keysym_t sym) {
    return false;
}

static void keyboard_handle_key(struct wl_listener *listener, void *data) {
    /* This event is raised when a key is pressed or released. */
    struct kaiju_keyboard *keyboard = wl_container_of(listener, keyboard, key);
    struct kaiju_server *server = keyboard->server;
    struct wlr_event_keyboard_key *event = data;
    struct wlr_seat *seat = server->seat;

    /* Translate libinput keycode -> xkbcommon */
    uint32_t keycode = event->keycode + 8;
    /* Get a list of keysyms based on the keymap for this keyboard */
    const xkb_keysym_t *syms;
    int nsyms = xkb_state_key_get_syms(
            keyboard->device->keyboard->xkb_state,
            keycode,
            &syms
    );

    bool handled = false;
    uint32_t modifiers = wlr_keyboard_get_modifiers(keyboard->device->keyboard);
    if ((modifiers & WLR_MODIFIER_ALT) && event->state == WLR_KEY_PRESSED) {
        /* If alt is held down and this button was _pressed_, we attempt to
         * process it as a compositor keybinding. */
        for (int i = 0; i < nsyms; i++) {
            handled = handle_keybinding(server, syms[i]);
        }
    }

    if (handled) return;
    /* Otherwise, we pass it along to the client. */
    wlr_seat_set_keyboard(seat, keyboard->device);
    wlr_seat_keyboard_notify_key(seat, event->time_msec, event->keycode, event->state);
}

static void server_new_keyboard(struct kaiju_server *server, struct wlr_input_device *device) {
    struct kaiju_keyboard *keyboard = calloc(1, sizeof(struct kaiju_keyboard));
    keyboard->server = server;
    keyboard->device = device;

    /* We need to prepare an XKB keymap and assign it to the keyboard. This
     * assumes the defaults (e.g. layout = "us"). TODO set proper keymap */
    struct xkb_rule_names rules = {0};
    struct xkb_context *context = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
    struct xkb_keymap *keymap = xkb_map_new_from_names(context, &rules, XKB_KEYMAP_COMPILE_NO_FLAGS);

    wlr_keyboard_set_keymap(device->keyboard, keymap);
    xkb_keymap_unref(keymap);
    xkb_context_unref(context);
    wlr_keyboard_set_repeat_info(device->keyboard, 25, 600);

    /* Here we set up listeners for keyboard events. */
    keyboard->modifiers.notify = keyboard_handle_modifiers;
    wl_signal_add(&device->keyboard->events.modifiers, &keyboard->modifiers);
    keyboard->key.notify = keyboard_handle_key;
    wl_signal_add(&device->keyboard->events.key, &keyboard->key);

    wlr_seat_set_keyboard(server->seat, device);

    /* And add the keyboard to our list of keyboards */
    wl_list_insert(&server->keyboards, &keyboard->link);
}

static void server_new_pointer(struct kaiju_server *server, struct wlr_input_device *device) {
    /* We don't do anything special with pointers. All of our pointer handling
     * is proxied through wlr_cursor. On another compositor, you might take this
     * opportunity to do libinput configuration on the device to set
     * acceleration, etc. */
    wlr_cursor_attach_input_device(server->cursor, device);
}

static void server_new_input(struct wl_listener *listener, void *data) {
    /* This event is raised by the backend when a new input device becomes
     * available. */
    struct kaiju_server *server = wl_container_of(listener, server, new_input);
    struct wlr_input_device *device = data;
    switch (device->type) {
        case WLR_INPUT_DEVICE_KEYBOARD:
            printf("New keyboard\n");
            server_new_keyboard(server, device);
            break;
        case WLR_INPUT_DEVICE_POINTER:
            printf("New pointer\n");
            server_new_pointer(server, device);
            break;
        default:
            printf("Unsupported input type: %d\n", device->type);
            break;
    }
    /* We need to let the wlr_seat know what our capabilities are, which is
     * communicated to the client. In TinyWL we always have a cursor, even if
     * there are no pointer devices, so we always include that capability. */
    uint32_t caps = WL_SEAT_CAPABILITY_POINTER;
    if (!wl_list_empty(&server->keyboards)) {
        caps |= WL_SEAT_CAPABILITY_KEYBOARD;
    }
    wlr_seat_set_capabilities(server->seat, caps);
}

static void seat_request_cursor(struct wl_listener *listener, void *data) {
    struct kaiju_server *server = wl_container_of(listener, server, request_cursor);
    /* This event is raised by the seat when a client provides a cursor image */
    struct wlr_seat_pointer_request_set_cursor_event *event = data;
    struct wlr_seat_client *focused_client =
            server->seat->pointer_state.focused_client;
    /* This can be sent by any client, so we check to make sure this one is
     * actually has pointer focus first. */
    if (focused_client == event->seat_client) {
        /* Once we've vetted the client, we can tell the cursor to use the
         * provided surface as the cursor image. It will set the hardware cursor
         * on the output that it's currently on and continue to do so as the
         * cursor moves between outputs. */
        wlr_cursor_set_surface(server->cursor, event->surface,
                               event->hotspot_x, event->hotspot_y);
    }
}

static bool view_at(struct kaiju_view *view,
                    double lx, double ly, struct wlr_surface **surface,
                    double *sx, double *sy) {
    /*
     * XDG toplevels may have nested surfaces, such as popup windows for context
     * menus or tooltips. This function tests if any of those are underneath the
     * coordinates lx and ly (in output Layout Coordinates). If so, it sets the
     * surface pointer to that wlr_surface and the sx and sy coordinates to the
     * coordinates relative to that surface's top-left corner.
     */
    double view_sx = lx - view->props.x;
    double view_sy = ly - view->props.y;

    struct wlr_surface_state *state = &view->xdg_surface->surface->current;

    double _sx, _sy;
    struct wlr_surface *_surface = NULL;
    _surface = wlr_xdg_surface_surface_at(
            view->xdg_surface,
            view_sx, view_sy,
            &_sx, &_sy
    );

    if (_surface != NULL) {
        *sx = _sx;
        *sy = _sy;
        *surface = _surface;
        return true;
    }

    return false;
}

static struct kaiju_view *desktop_view_at(
        struct kaiju_server *server, double lx, double ly,
        struct wlr_surface **surface, double *sx, double *sy) {
    /* This iterates over all of our surfaces and attempts to find one under the
     * cursor. This relies on server->views being ordered from top-to-bottom. */
    struct kaiju_view *view;
    wl_list_for_each(view, &server->views, link) {
        if (view_at(view, lx, ly, surface, sx, sy)) {
            return view;
        }
    }
    return NULL;
}

static void process_cursor_move(struct kaiju_server *server, uint32_t time) {
    // Move the grabbed view to the new position.
    server->grabbed_view->props.x = server->cursor->x - server->grab_x;
    server->grabbed_view->props.y = server->cursor->y - server->grab_y;
}

static void process_cursor_resize(struct kaiju_server *server, uint32_t time) {
    /*
     * Resizing the grabbed view can be a little bit complicated, because we
     * could be resizing from any corner or edge. This not only resizes the view
     * on one or two axes, but can also move the view if you resize from the top
     * or left edges (or top-left corner).
     *
     * Note that I took some shortcuts here. In a more fleshed-out compositor,
     * you'd wait for the client to prepare a buffer at the new size, then
     * commit any movement that was prepared.
     */
    struct kaiju_view *view = server->grabbed_view;
    double dx = server->cursor->x - server->grab_x;
    double dy = server->cursor->y - server->grab_y;
    double x = view->props.x;
    double y = view->props.y;
    int width = server->grab_width;
    int height = server->grab_height;
    if (server->resize_edges & WLR_EDGE_TOP) {
        y = server->grab_y + dy;
        height -= dy;
        if (height < 1) {
            y += height;
        }
    } else if (server->resize_edges & WLR_EDGE_BOTTOM) {
        height += dy;
    }
    if (server->resize_edges & WLR_EDGE_LEFT) {
        x = server->grab_x + dx;
        width -= dx;
        if (width < 1) {
            x += width;
        }
    } else if (server->resize_edges & WLR_EDGE_RIGHT) {
        width += dx;
    }
    view->props.x = x;
    view->props.y = y;
    wlr_xdg_toplevel_set_size(view->xdg_surface, width, height);
}

static void process_cursor_motion(struct kaiju_server *server, uint32_t time) {
    /* If the mode is non-passthrough, delegate to those functions. */
    if (server->cursor_mode == KAIJU_CURSOR_MOVE) {
        process_cursor_move(server, time);
        return;
    } else if (server->cursor_mode == KAIJU_CURSOR_RESIZE) {
        process_cursor_resize(server, time);
        return;
    }

    /* Otherwise, find the view under the pointer and send the event along. */
    double sx, sy;
    struct wlr_seat *seat = server->seat;
    struct wlr_surface *surface = NULL;
    struct kaiju_view *view = desktop_view_at(
            server,
            server->cursor->x, server->cursor->y,
            &surface,
            &sx, &sy
    );
    if (!view) {
        /* If there's no view under the cursor, set the cursor image to a
         * default. This is what makes the cursor image appear when you move it
         * around the screen, not over any views. */
        wlr_xcursor_manager_set_cursor_image(server->cursor_mgr, "left_ptr", server->cursor);
    }
    if (surface) {
        bool focus_changed = seat->pointer_state.focused_surface != surface;
        /*
         * "Enter" the surface if necessary. This lets the client know that the
         * cursor has entered one of its surfaces.
         *
         * Note that this gives the surface "pointer focus", which is distinct
         * from keyboard focus. You get pointer focus by moving the pointer over
         * a window.
         */
        wlr_seat_pointer_notify_enter(seat, surface, sx, sy);
        if (!focus_changed) {
            /* The enter event contains coordinates, so we only need to notify
             * on motion if the focus did not change. */
            wlr_seat_pointer_notify_motion(seat, time, sx, sy);
        }
    } else {
        /* Clear pointer focus so future button events and such are not sent to
         * the last client to have the cursor over it. */
        wlr_seat_pointer_clear_focus(seat);
    }
}

static void server_cursor_motion(struct wl_listener *listener, void *data) {
    /* This event is forwarded by the cursor when a pointer emits a _relative_
     * pointer motion event (i.e. a delta) */
    struct kaiju_server *server = wl_container_of(listener, server, cursor_motion);
    struct wlr_event_pointer_motion *event = data;
    /* The cursor doesn't move unless we tell it to. The cursor automatically
     * handles constraining the motion to the output layout, as well as any
     * special configuration applied for the specific input device which
     * generated the event. You can pass NULL for the device if you want to move
     * the cursor around without any input. */
    wlr_cursor_move(server->cursor, event->device,
                    event->delta_x, event->delta_y);
    process_cursor_motion(server, event->time_msec);
}

static void server_cursor_motion_absolute(struct wl_listener *listener, void *data) {
    /* This event is forwarded by the cursor when a pointer emits an _absolute_
     * motion event, from 0..1 on each axis. This happens, for example, when
     * wlroots is running under a Wayland window rather than KMS+DRM, and you
     * move the mouse over the window. You could enter the window from any edge,
     * so we have to warp the mouse there. There is also some hardware which
     * emits these events. */
    struct kaiju_server *server = wl_container_of(listener, server, cursor_motion_absolute);
    struct wlr_event_pointer_motion_absolute *event = data;
    wlr_cursor_warp_absolute(server->cursor, event->device, event->x, event->y);
    process_cursor_motion(server, event->time_msec);
}

static void server_cursor_button(struct wl_listener *listener, void *data) {
    /* This event is forwarded by the cursor when a pointer emits a button
     * event. */
    struct kaiju_server *server = wl_container_of(listener, server, cursor_button);
    struct wlr_event_pointer_button *event = data;
    /* Notify the client with pointer focus that a button press has occurred */
    wlr_seat_pointer_notify_button(server->seat, event->time_msec, event->button, event->state);
    double sx, sy;
    struct wlr_seat *seat = server->seat;
    struct wlr_surface *surface;
    struct kaiju_view *view = desktop_view_at(server,
                                              server->cursor->x, server->cursor->y, &surface, &sx, &sy);
    if (event->state == WLR_BUTTON_RELEASED) {
        /* If you released any buttons, we exit interactive move/resize mode. */
        server->cursor_mode = KAIJU_CURSOR_PASSTHROUGH;
    } else {
        /* Focus that client if the button was _pressed_ */
        focus_view(view, surface);
    }
}

static void server_cursor_axis(struct wl_listener *listener, void *data) {
    /* This event is forwarded by the cursor when a pointer emits an axis event,
     * for example when you move the scroll wheel. */
    struct kaiju_server *server = wl_container_of(listener, server, cursor_axis);
    struct wlr_event_pointer_axis *event = data;
    /* Notify the client with pointer focus of the axis event. */
    wlr_seat_pointer_notify_axis(
            server->seat,
            event->time_msec,
            event->orientation,
            event->delta,
            event->delta_discrete,
            event->source
    );
}

static void server_cursor_frame(struct wl_listener *listener, void *data) {
    /* This event is forwarded by the cursor when a pointer emits an frame
     * event. Frame events are sent after regular pointer events to group
     * multiple events together. For instance, two axis events may happen at the
     * same time, in which case a frame event won't be sent in between. */
    struct kaiju_server *server = wl_container_of(listener, server, cursor_frame);
    /* Notify the client with pointer focus of the frame event. */
    wlr_seat_pointer_notify_frame(server->seat);
}


void configure_cursor(struct kaiju_server *server) {
    /*
	 * Creates a cursor, which is a wlroots utility for tracking the cursor
	 * image shown on screen.
	 */
    server->cursor = wlr_cursor_create();
    wlr_cursor_attach_output_layout(server->cursor, server->output_layout);
}
