#include "remotedesktop.h"

#include <stdint.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <xkbcommon/xkbcommon.h>
#include <linux/input-event-codes.h>

#include "wlr_virtual_pointer.h"
#include "virtual_keyboard.h"
#include "xdpw.h"

#include "keymap.h"

static const char object_path[] = "/org/freedesktop/portal/desktop";
static const char interface_name[] = "org.freedesktop.impl.portal.RemoteDesktop";
static const uint32_t delay_frame_end_ns = 5000;

static uint32_t get_timestamp_ms(struct xdpw_remotedesktop_session_data *remote) {
	struct timespec *t_start, t_stop;

	t_start = &remote->t_start;
	clock_gettime(CLOCK_REALTIME, &t_stop);

	return 1000 * (t_stop.tv_sec - t_start->tv_sec) +
		(t_stop.tv_nsec - t_start->tv_nsec) / 1000000;
}

static struct xdpw_session *get_session_from_handle(struct xdpw_state *state, char *session_handle) {
	struct xdpw_session *sess;
	wl_list_for_each_reverse(sess, &state->xdpw_sessions, link) {
		if (strcmp(sess->session_handle, session_handle) == 0) {
			return sess;
		}
	}
	return NULL;
}

static int method_remotedesktop_create_session(sd_bus_message *msg, void *data,
		sd_bus_error *ret_error) {
	struct xdpw_state *state = data;

	int ret = 0;
	char *request_handle, *session_handle, *app_id, *key;
	struct xdpw_request *req;
	struct xdpw_session *sess;

	logprint(DEBUG, "remotedesktop: create session: method invoked");

	ret = sd_bus_message_read(msg, "oos", &request_handle, &session_handle, &app_id);
	if (ret < 0) {
		return ret;
	}

	logprint(DEBUG, "remotedesktop: create session: request_handle: %s", request_handle);
	logprint(DEBUG, "remotedesktop: create session: session_handle: %s", session_handle);
	logprint(DEBUG, "remotedesktop: create session: app_id: %s", app_id);

	ret = sd_bus_message_enter_container(msg, 'a', "{sv}");
	if (ret < 0) {
		return ret;
	}
	while ((ret = sd_bus_message_enter_container(msg, 'e', "sv")) > 0) {
		ret = sd_bus_message_read(msg, "s", &key);
		if (ret < 0) {
			return ret;
		}

		if (strcmp(key, "session_handle_token") == 0) {
			char *token;
			sd_bus_message_read(msg, "v", "s", &token);
			logprint(DEBUG, "remotedesktop: create session: session handle token: %s", token);
		} else {
			logprint(WARN, "remotedesktop: create session: unknown option: %s", key);
			sd_bus_message_skip(msg, "v");
		}

		ret = sd_bus_message_exit_container(msg);
		if (ret < 0) {
			return ret;
		}
	}
	if (ret < 0) {
		return ret;
	}

	ret = sd_bus_message_exit_container(msg);
	if (ret < 0) {
		return ret;
	}

	req = xdpw_request_create(sd_bus_message_get_bus(msg), request_handle);
	if (req == NULL) {
		return -ENOMEM;
	}

	sess = xdpw_session_create(state, sd_bus_message_get_bus(msg), strdup(session_handle));
	if (sess == NULL) {
		return -ENOMEM;
	}

	ret = sd_bus_reply_method_return(msg, "ua{sv}", PORTAL_RESPONSE_SUCCESS, 0);
	if (ret < 0) {
		return ret;
	}
	return 0;
}

static int method_remotedesktop_select_devices(sd_bus_message *msg, void *data,
		sd_bus_error *ret_error) {
	struct xdpw_state *state = data;

	int ret = 0;
	char *request_handle, *session_handle, *app_id, *key;
	struct xdpw_session *sess;

	logprint(DEBUG, "remotedesktop: select devices: method invoked");

	ret = sd_bus_message_read(msg, "oos", &request_handle, &session_handle, &app_id);
	if (ret < 0) {
		return ret;
	}

	logprint(DEBUG, "remotedesktop: select devices: request_handle: %s", request_handle);
	logprint(DEBUG, "remotedesktop: select devices: session_handle: %s", session_handle);
	logprint(DEBUG, "remotedesktop: select devices: app_id: %s", app_id);

	sess = get_session_from_handle(state, session_handle);
	if (!sess) {
		logprint(WARN, "remotedesktop: select devices: session not found");
		return -1;
	}
	logprint(DEBUG, "remotedesktop: select devices: session found");

	ret = sd_bus_message_enter_container(msg, 'a', "{sv}");
	if (ret < 0) {
		return ret;
	}
	while ((ret = sd_bus_message_enter_container(msg, 'e', "sv")) > 0) {
		ret = sd_bus_message_read(msg, "s", &key);
		if (ret < 0) {
			return ret;
		}

		if (strcmp(key, "types") == 0) {
			uint32_t types;
			ret = sd_bus_message_read(msg, "v", "u", &types);
			if (ret < 0) {
				return ret;
			}
			logprint(DEBUG, "remotedesktop: select devices: option types: %x", types);
		} else {
			logprint(WARN, "remotedesktop: select devices: unknown option: %s", key);
			sd_bus_message_skip(msg, "v");
		}

		ret = sd_bus_message_exit_container(msg);
		if (ret < 0) {
			return ret;
		}
	}
	if (ret < 0) {
		return ret;
	}
	ret = sd_bus_message_exit_container(msg);
	if (ret < 0) {
		return ret;
	}

	ret = sd_bus_reply_method_return(msg, "ua{sv}", PORTAL_RESPONSE_SUCCESS, 0);
	if (ret < 0) {
		return ret;
	}
	return 0;
}

int init_keymap() {
	char template[] = "/xdpw-keymap-XXXXXX";
	char const *path;
	char *name;

	path = getenv("XDG_RUNTIME_DIR");
	if (!path) {
		logprint(ERROR, "remotedestop: init_keymap: failed to get XDG_RUNTIME_DIR");
		errno = ENOENT;
		return -1;
	}

	name = malloc(strlen(path) + sizeof(template) + 1);
	if (!name) {
		logprint(ERROR, "remotedestop: init_keymap: failed to allocate keymap path");
		return -1;
	}

	strcpy(name, path);
	strcat(name, template);

	int keymap_fd = mkstemp(name);
	if (keymap_fd == -1) {
		logprint(ERROR, "remotedesktop: init_keymap: failed to create temporary keymap file");
		logprint(ERROR, name);
		logprint(ERROR, strerror(errno));
		return -1;
	}

	int flags = fcntl(keymap_fd, F_GETFD);
	if (flags == -1) {
		logprint(ERROR, "remotedesktop: init_keymap: failed to query temporary keymap file flags");
		close(keymap_fd);
		return -1;
	}

	if (fcntl(keymap_fd, F_SETFD, flags | FD_CLOEXEC) == -1) {
		logprint(ERROR, "remotedesktop: init_keymap: failed to set temporary keymap file flags");
		close(keymap_fd);
		return -1;
	}
	unlink(name);
	free(name);

	off_t keymap_size = strlen(keymap)+64;
	if (posix_fallocate(keymap_fd, 0, keymap_size)) {
		logprint(ERROR, "remotedestop: init_keymap: failed to allocate keymap memory");
		return -1;
	}

	void *keymap_memory_ptr = mmap(NULL, keymap_size, PROT_READ | PROT_WRITE, MAP_SHARED,
		keymap_fd, 0);
	if (keymap_memory_ptr == (void*)-1) {
		logprint(ERROR, "remotedesktop: init_keymap: failed to mmap keymap data");
		return -1;
	}
	strcpy(keymap_memory_ptr, keymap);
	return keymap_fd;
}

void remote_desktop_send_frame_motion(void *data) {
	struct xdpw_session *sess = data;

	zwlr_virtual_pointer_v1_frame(sess->remotedesktop_data.virtual_pointer);
	sess->remotedesktop_data.motion_timer = NULL;
}

void remote_desktop_send_frame_axis(void *data) {
	struct xdpw_session *sess = data;

	zwlr_virtual_pointer_v1_frame(sess->remotedesktop_data.virtual_pointer);
	sess->remotedesktop_data.axis_timer = NULL;
}

static int method_remotedesktop_start(sd_bus_message *msg, void *data, sd_bus_error *ret_error) {
	struct xdpw_state *state = data;

	int ret = 0;
	char *request_handle, *session_handle, *app_id, *parent_window, *key;
	struct xdpw_session *sess;
	struct xdpw_remotedesktop_session_data *remote;

	logprint(DEBUG, "remotedesktop: start: method invoked");

	ret = sd_bus_message_read(msg, "oos", &request_handle, &session_handle, &app_id);
	if (ret < 0) {
		return ret;
	}

	logprint(DEBUG, "remotedesktop: start: request_handle: %s", request_handle);
	logprint(DEBUG, "remotedesktop: start: session_handle: %s", session_handle);
	logprint(DEBUG, "remotedesktop: start: app_id: %s", app_id);

	sess = get_session_from_handle(state, session_handle);
	if (!sess) {
		logprint(WARN, "remotedesktop: start: session not found");
		return -1;
	}
	logprint(DEBUG, "remotedesktop: start: session found");

	remote = &sess->remotedesktop_data;
	remote->virtual_pointer = zwlr_virtual_pointer_manager_v1_create_virtual_pointer(
		state->remotedesktop.virtual_pointer_manager, state->remotedesktop.seat);

	int keymap_fd = init_keymap();
	if (keymap_fd > 0)
	{
		int keymap_size = strlen(keymap)+64;
		remote->virtual_keyboard = zwp_virtual_keyboard_manager_v1_create_virtual_keyboard(
			state->remotedesktop.virtual_keyboard_manager, state->remotedesktop.seat);
		zwp_virtual_keyboard_v1_keymap(remote->virtual_keyboard, WL_KEYBOARD_KEYMAP_FORMAT_XKB_V1,
			keymap_fd, keymap_size);
	}

	clock_gettime(CLOCK_REALTIME, &remote->t_start);

	ret = sd_bus_message_read(msg, "s", &parent_window);
	if (ret < 0) {
		return ret;
	}
	logprint(DEBUG, "remotedesktop: start: parent window: %s", parent_window);

	ret = sd_bus_message_enter_container(msg, 'a', "{sv}");
	if (ret < 0) {
		return ret;
	}
	while ((ret = sd_bus_message_enter_container(msg, 'e', "sv")) > 0) {
		ret = sd_bus_message_read(msg, "s", &key);
		if (ret < 0) {
			return ret;
		}

		logprint(WARN, "remotedesktop: start: unknown option: %s", key);
		sd_bus_message_skip(msg, "v");

		ret = sd_bus_message_exit_container(msg);
		if (ret < 0) {
			return ret;
		}
	}
	if (ret < 0) {
		return ret;
	}
	ret = sd_bus_message_exit_container(msg);
	if (ret < 0) {
		return ret;
	}

	ret = sd_bus_reply_method_return(msg, "ua{sv}", PORTAL_RESPONSE_SUCCESS,
		1, "devices", "u", POINTER | KEYBOARD);
	if (ret < 0) {
		return ret;
	}

	return 0;
}

static int method_remotedesktop_notify_pointer_motion(sd_bus_message *msg,
		void *data, sd_bus_error *ret_error) {
	struct xdpw_state *state = data;

	int ret = 0;
	char *session_handle;
	struct xdpw_session *sess;
	double dx = 0, dy = 0;

	logprint(DEBUG, "remotedesktop: npm: method invoked");

	ret = sd_bus_message_read(msg, "o", &session_handle);
	if (ret < 0) {
		return ret;
	}
	logprint(DEBUG, "remotedesktop: npm: session_handle: %s", session_handle);

	wl_list_for_each_reverse(sess, &state->xdpw_sessions, link) {
		if (strcmp(sess->session_handle, session_handle) == 0) {
			break;
		}
	}
	if (!sess) {
		logprint(WARN, "remotedesktop: npm: session not found");
		return -1;
	}
	logprint(DEBUG, "remotedesktop: npm: session found");

	ret = sd_bus_message_skip(msg, "a{sv}");
	if (ret < 0) {
		return ret;
	}
	ret = sd_bus_message_read(msg, "d", &dx);
	if (ret < 0) {
		return ret;
	}
	ret = sd_bus_message_read(msg, "d", &dy);
	if (ret < 0) {
		return ret;
	}

	zwlr_virtual_pointer_v1_motion(sess->remotedesktop_data.virtual_pointer,
		get_timestamp_ms(&sess->remotedesktop_data),
		wl_fixed_from_double(dx), wl_fixed_from_double(dy));

	if(sess->remotedesktop_data.motion_timer != NULL) {
		xdpw_destroy_timer(sess->remotedesktop_data.motion_timer);
	}
	sess->remotedesktop_data.motion_timer = xdpw_add_timer(state, delay_frame_end_ns,
		(xdpw_event_loop_timer_func_t) remote_desktop_send_frame_motion, sess);

	return 0;
}

static int method_remotedesktop_notify_pointer_motion_absolute(
		sd_bus_message *msg, void *data, sd_bus_error *ret_error) {
	struct xdpw_state *state = data;

	int ret = 0;
	char *session_handle;
	struct xdpw_session *sess;
	double x = 0, y = 0;

	logprint(DEBUG, "remotedesktop: npma: method invoked");

	ret = sd_bus_message_read(msg, "o", &session_handle);
	if (ret < 0) {
		return ret;
	}
	logprint(DEBUG, "remotedesktop: npma: session_handle: %s", session_handle);

	wl_list_for_each_reverse(sess, &state->xdpw_sessions, link) {
		if (strcmp(sess->session_handle, session_handle) == 0) {
			break;
		}
	}
	if (!sess) {
		logprint(WARN, "remotedesktop: npma: session not found");
		return -1;
	}
	logprint(DEBUG, "remotedesktop: npma: session found");

	ret = sd_bus_message_skip(msg, "a{sv}");
	if (ret < 0) {
		return ret;
	}
	ret = sd_bus_message_read(msg, "d", &x);
	if (ret < 0) {
		return ret;
	}
	ret = sd_bus_message_read(msg, "d", &y);
	if (ret < 0) {
		return ret;
	}

	struct xdpw_wlr_output *output = sess->screencast_data.screencast_instance->target->output;
	zwlr_virtual_pointer_v1_motion_absolute(sess->remotedesktop_data.virtual_pointer,
		get_timestamp_ms(&sess->remotedesktop_data),
		wl_fixed_from_double(x), wl_fixed_from_double(y),
		output->width, output->height);

	return 0;
}

static int method_remotedesktop_notify_pointer_button(sd_bus_message *msg,
		void *data, sd_bus_error *ret_error) {
	struct xdpw_state *state = data;

	int ret = 0;
	char *session_handle;
	struct xdpw_session *sess;
	int32_t button;
	uint32_t btn_state;

	logprint(DEBUG, "remotedesktop: npb: method invoked");

	ret = sd_bus_message_read(msg, "o", &session_handle);
	if (ret < 0) {
		return ret;
	}
	logprint(DEBUG, "remotedesktop: npb: session_handle: %s", session_handle);

	wl_list_for_each_reverse(sess, &state->xdpw_sessions, link) {
		if (strcmp(sess->session_handle, session_handle) == 0) {
			break;
		}
	}
	if (!sess) {
		logprint(WARN, "remotedesktop: npb: session not found");
		return -1;
	}
	logprint(DEBUG, "remotedesktop: npb: session found");

	ret = sd_bus_message_skip(msg, "a{sv}");
	if (ret < 0) {
		return ret;
	}
	ret = sd_bus_message_read(msg, "i", &button);
	if (ret < 0) {
		return ret;
	}
	ret = sd_bus_message_read(msg, "u", &btn_state);
	if (ret < 0) {
		return ret;
	}

	zwlr_virtual_pointer_v1_button(sess->remotedesktop_data.virtual_pointer,
		get_timestamp_ms(&sess->remotedesktop_data),
		button, btn_state);
	zwlr_virtual_pointer_v1_frame(sess->remotedesktop_data.virtual_pointer);
	return 0;
}

static int method_remotedesktop_notify_pointer_axis(sd_bus_message *msg,
		void *data, sd_bus_error *ret_error) {
	struct xdpw_state *state = data;

	int ret = 0, finish = 0;
	char *session_handle, *key;
	struct xdpw_session *sess;
	double dx = 0, dy = 0;

	logprint(DEBUG, "remotedesktop: npa: method invoked");

	ret = sd_bus_message_read(msg, "o", &session_handle);
	if (ret < 0) {
		return ret;
	}
	logprint(DEBUG, "remotedesktop: npa: session_handle: %s", session_handle);

	wl_list_for_each_reverse(sess, &state->xdpw_sessions, link) {
		if (strcmp(sess->session_handle, session_handle) == 0) {
			break;
		}
	}
	if (!sess) {
		logprint(WARN, "remotedesktop: npa: session not found");
		return -1;
	}
	logprint(DEBUG, "remotedesktop: npa: session found");

	ret = sd_bus_message_enter_container(msg, 'a', "{sv}");
	if (ret < 0) {
		return ret;
	}
	while ((ret = sd_bus_message_enter_container(msg, 'e', "sv")) > 0) {
		ret = sd_bus_message_read(msg, "s", &key);
		if (ret < 0) {
			return ret;
		}

		if (strcmp(key, "finish") == 0) {
			sd_bus_message_read(msg, "v", "b", &finish);
			logprint(DEBUG, "remotedesktop: npa: finish: %d", finish);
		} else {
			logprint(WARN, "remotedesktop: npa: unknown option: %s", key);
			sd_bus_message_skip(msg, "v");
		}

		ret = sd_bus_message_exit_container(msg);
		if (ret < 0) {
			return ret;
		}
	}
	if (ret < 0) {
		return ret;
	}
	ret = sd_bus_message_exit_container(msg);
	if (ret < 0) {
		return ret;
	}

	ret = sd_bus_message_read(msg, "d", &dx);
	if (ret < 0) {
		return ret;
	}
	ret = sd_bus_message_read(msg, "d", &dy);
	if (ret < 0) {
		return ret;
	}

	zwlr_virtual_pointer_v1_axis(sess->remotedesktop_data.virtual_pointer,
		get_timestamp_ms(&sess->remotedesktop_data),
		WL_POINTER_AXIS_VERTICAL_SCROLL, wl_fixed_from_double(dy * 10));
	zwlr_virtual_pointer_v1_axis(sess->remotedesktop_data.virtual_pointer,
		get_timestamp_ms(&sess->remotedesktop_data),
		WL_POINTER_AXIS_HORIZONTAL_SCROLL, wl_fixed_from_double(dx * 10));

	if (finish) {
		zwlr_virtual_pointer_v1_axis_stop(sess->remotedesktop_data.virtual_pointer,
			get_timestamp_ms(&sess->remotedesktop_data),
			WL_POINTER_AXIS_VERTICAL_SCROLL);
		zwlr_virtual_pointer_v1_axis_stop(sess->remotedesktop_data.virtual_pointer,
			get_timestamp_ms(&sess->remotedesktop_data),
			WL_POINTER_AXIS_HORIZONTAL_SCROLL);
	}

	if(sess->remotedesktop_data.axis_timer != NULL) {
		xdpw_destroy_timer(sess->remotedesktop_data.axis_timer);
	}
	sess->remotedesktop_data.axis_timer = xdpw_add_timer(state, delay_frame_end_ns,
		(xdpw_event_loop_timer_func_t) remote_desktop_send_frame_axis, sess);

	return 0;
}

static int method_remotedesktop_notify_pointer_axis_discrete(
		sd_bus_message *msg, void *data, sd_bus_error *ret_error) {
	struct xdpw_state *state = data;

	int ret = 0;
	char *session_handle;
	struct xdpw_session *sess;
	uint32_t axis;
	int32_t steps;

	logprint(DEBUG, "remotedesktop: npad: method invoked");

	ret = sd_bus_message_read(msg, "o", &session_handle);
	if (ret < 0) {
		return ret;
	}
	logprint(DEBUG, "remotedesktop: npad: session_handle: %s", session_handle);

	wl_list_for_each_reverse(sess, &state->xdpw_sessions, link) {
		if (strcmp(sess->session_handle, session_handle) == 0) {
			break;
		}
	}
	if (!sess) {
		logprint(WARN, "remotedesktop: npad: session not found");
		return -1;
	}
	logprint(DEBUG, "remotedesktop: npad: session found");

	ret = sd_bus_message_skip(msg, "a{sv}");
	if (ret < 0) {
		return ret;
	}
	ret = sd_bus_message_read(msg, "u", &axis);
	if (ret < 0) {
		return ret;
	}
	ret = sd_bus_message_read(msg, "i", &steps);
	if (ret < 0) {
		return ret;
	}

	zwlr_virtual_pointer_v1_axis_discrete(sess->remotedesktop_data.virtual_pointer,
			get_timestamp_ms(&sess->remotedesktop_data),
			axis, wl_fixed_from_double(0.1), steps);
	return 0;
}

static int method_remotedesktop_notify_keyboard_keycode(
		sd_bus_message *msg, void *data, sd_bus_error *ret_error) {
	struct xdpw_state *state = data;

	int ret = 0;
	char *session_handle;
	struct xdpw_session *sess;
	struct xdpw_remotedesktop_session_data *remote;
	int32_t keycode;
	uint32_t key_state;

	logprint(DEBUG, "remotedesktop: notify_keyboard_keycode: method invoked");

	ret = sd_bus_message_read(msg, "o", &session_handle);
	if (ret < 0) {
		return ret;
	}
	logprint(DEBUG, "remotedesktop: notify_keyboard_keycode: session_handle: %s", session_handle);

	wl_list_for_each_reverse(sess, &state->xdpw_sessions, link) {
		if (strcmp(sess->session_handle, session_handle) == 0) {
			break;
		}
	}
	if (!sess) {
		logprint(WARN, "remotedesktop: notify_keyboard_keycode: session not found");
		return -1;
	}
	logprint(DEBUG, "remotedesktop: notify_keyboard_keycode: session found");

	remote = &sess->remotedesktop_data;

	ret = sd_bus_message_skip(msg, "a{sv}");
	if (ret < 0) {
		return ret;
	}
	ret = sd_bus_message_read(msg, "i", &keycode);
	if (ret < 0) {
		return ret;
	}
	ret = sd_bus_message_read(msg, "u", &key_state);
	if (ret < 0) {
		return ret;
	}

	// this is not the right way, but at least this enables minimal usage
	// TODO: Map evdev keycodes to xkb_symbols properly
	uint32_t key = 0;
	switch(keycode)
	{
		case KEY_BACKSPACE: {
			key = XKB_KEY_BackSpace - 0xff00;
			break;
		}
		case KEY_ENTER: {
			key = XKB_KEY_Return - 0xff00;
			break;
		}
		default:
			return -1;
	}

	zwp_virtual_keyboard_v1_key(remote->virtual_keyboard,
		get_timestamp_ms(remote),
		key-7, key_state);
	return 0;
}

static int method_remotedesktop_notify_keyboard_keysym(
		sd_bus_message *msg, void *data, sd_bus_error *ret_error) {
	struct xdpw_state *state = data;

	int ret = 0;
	char *session_handle;
	struct xdpw_session *sess;
	struct xdpw_remotedesktop_session_data *remote;
	int32_t keysym;
	uint32_t key_state;

	logprint(DEBUG, "remotedesktop: notify_keyboard_keysym: method invoked");

	ret = sd_bus_message_read(msg, "o", &session_handle);
	if (ret < 0) {
		return ret;
	}
	logprint(DEBUG, "remotedesktop: notify_keyboard_keysym: session_handle: %s", session_handle);

	wl_list_for_each_reverse(sess, &state->xdpw_sessions, link) {
		if (strcmp(sess->session_handle, session_handle) == 0) {
			break;
		}
	}
	if (!sess) {
		logprint(WARN, "remotedesktop: notify_keyboard_keysym: session not found");
		return -1;
	}
	logprint(DEBUG, "remotedesktop: notify_keyboard_keysym: session found");

	remote = &sess->remotedesktop_data;

	ret = sd_bus_message_skip(msg, "a{sv}");
	if (ret < 0) {
		return ret;
	}
	ret = sd_bus_message_read(msg, "i", &keysym);
	if (ret < 0) {
		return ret;
	}
	ret = sd_bus_message_read(msg, "u", &key_state);
	if (ret < 0) {
		return ret;
	}

	// map char to xkb_keycode_t
	uint32_t keycode = keysym;
	if(keycode < 0x20) {
		keycode += 0xff00;
	}
	else if(keycode > 127) {
		// FIXME: create more temporary keymaps and switch between them if needed
		logprint(WARN, "remotedestop: notify_keyboard_keysym: keysym > 127 not supported for now");
		return -1;
	}
	else {
		keycode -= 7;
	}

	zwp_virtual_keyboard_v1_key(remote->virtual_keyboard,
		get_timestamp_ms(remote),
		keycode, key_state);
	return 0;
}

static int method_remotedesktop_notify_touch_down(
		sd_bus_message *msg, void *data, sd_bus_error *ret_error) {
	return 0;
}

static int method_remotedesktop_notify_touch_motion(
		sd_bus_message *msg, void *data, sd_bus_error *ret_error) {
	return 0;
}

static int method_remotedesktop_notify_touch_up(
		sd_bus_message *msg, void *data, sd_bus_error *ret_error) {
	return 0;
}

static const sd_bus_vtable remotedesktop_vtable[] = {
	SD_BUS_VTABLE_START(0),
	SD_BUS_METHOD("CreateSession", "oosa{sv}", "ua{sv}",
		method_remotedesktop_create_session, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_METHOD("SelectDevices", "oosa{sv}", "ua{sv}",
		method_remotedesktop_select_devices, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_METHOD("Start", "oossa{sv}", "ua{sv}",
		method_remotedesktop_start, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_METHOD("NotifyPointerMotion", "oa{sv}dd", NULL,
		method_remotedesktop_notify_pointer_motion, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_METHOD("NotifyPointerMotionAbsolute", "oa{sv}udd", NULL,
		method_remotedesktop_notify_pointer_motion_absolute, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_METHOD("NotifyPointerButton", "oa{sv}iu", NULL,
		method_remotedesktop_notify_pointer_button, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_METHOD("NotifyPointerAxis", "oa{sv}dd", NULL,
		method_remotedesktop_notify_pointer_axis, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_METHOD("NotifyPointerAxisDiscrete", "oa{sv}ui", NULL,
		method_remotedesktop_notify_pointer_axis_discrete, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_METHOD("NotifyKeyboardKeycode", "oa{sv}iu", NULL,
		method_remotedesktop_notify_keyboard_keycode, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_METHOD("NotifyKeyboardKeysym", "oa{sv}iu", NULL,
		method_remotedesktop_notify_keyboard_keysym, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_METHOD("NotifyTouchDown", "oa{sv}uudd", NULL,
		method_remotedesktop_notify_touch_down, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_METHOD("NotifyTouchMotion", "oa{sv}uudd", NULL,
		method_remotedesktop_notify_touch_motion, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_METHOD("NotifyTouchUp", "oa{sv}u", NULL,
		method_remotedesktop_notify_touch_up, SD_BUS_VTABLE_UNPRIVILEGED),
	SD_BUS_PROPERTY("AvailableDeviceTypes", "u", NULL,
		offsetof(struct xdpw_state, remotedesktop_available_device_types),
		SD_BUS_VTABLE_PROPERTY_CONST),
	SD_BUS_PROPERTY("version", "u", NULL,
		offsetof(struct xdpw_state, remotedesktop_version),
		SD_BUS_VTABLE_PROPERTY_CONST),
	SD_BUS_VTABLE_END
};

int xdpw_remotedesktop_init(struct xdpw_state *state) {
	sd_bus_slot *slot = NULL;

	state->remotedesktop = (struct xdpw_remotedesktop_context) { 0 };
	state->remotedesktop.state = state;

	int err;
	err = xdpw_wlr_virtual_pointer_init(state);
	if (err) {
		goto fail_virtual_pointer;
	}

	err = xdpw_virtual_keyboard_init(state);
	if (err) {
		goto fail_virtual_keyboard;
	}

	return sd_bus_add_object_vtable(state->bus, &slot, object_path,
		interface_name, remotedesktop_vtable, state);

fail_virtual_keyboard:
	xdpw_virtual_keyboard_finish(&state->remotedesktop);

fail_virtual_pointer:
	xdpw_wlr_virtual_pointer_finish(&state->remotedesktop);

	return err;
}
