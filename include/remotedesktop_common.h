#ifndef REMOTEDESKTOP_COMMON_H
#define REMOTEDESKTOP_COMMON_H

#include <stdbool.h>
#include <time.h>

#include "virtual-keyboard-unstable-v1-client-protocol.h"
#include "wlr-virtual-pointer-unstable-v1-client-protocol.h"
#include <wayland-client-protocol.h>
#include <wayland-util.h>

#define XDP_REMOTE_PROTO_VER 1

struct xdpw_remotedesktop_context {
	// xdpw
	struct xdpw_state *state;

	// wlroots
	struct wl_registry *registry;
	struct zwp_virtual_keyboard_manager_v1 *virtual_keyboard_manager;
	struct zwlr_virtual_pointer_manager_v1 *virtual_pointer_manager;
	struct wl_seat *seat;

	// sessions
	struct wl_list remotedesktop_instances;
};

struct xdpw_remotedesktop_session_data {
	struct zwp_virtual_keyboard_v1 *virtual_keyboard;
	struct zwlr_virtual_pointer_v1 *virtual_pointer;
	struct timespec t_start;

	struct xdpw_timer *motion_timer;
	struct xdpw_timer *axis_timer;
};

enum device_types {
  KEYBOARD = 1,
  POINTER = 2,
	TOUCHSCREEN = 4,
};

#endif
