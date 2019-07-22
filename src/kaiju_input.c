#include "../include/kaiju_input.h"
#include <wlr/types/wlr_cursor.h>

void configure_cursor(struct kaiju_server *server) {
    /*
	 * Creates a cursor, which is a wlroots utility for tracking the cursor
	 * image shown on screen.
	 */
    server->cursor = wlr_cursor_create();
    wlr_cursor_attach_output_layout(server->cursor, server->output_layout);
}
