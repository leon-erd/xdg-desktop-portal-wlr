#ifndef VIRTUAL_KEYBOARD_H
#define VIRTUAL_KEYBOARD_H

#define VIRTUAL_KEYBOARD_VERSION 1
#define VIRTUAL_KEYBOARD_VERSION_MIN 1

#include "remotedesktop_common.h"

struct xdpw_state;

int xdpw_virtual_keyboard_init(struct xdpw_state *state);

void xdpw_virtual_keyboard_finish(struct xdpw_remotedesktop_context *ctx);

#endif
