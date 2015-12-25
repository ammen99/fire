#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <libudev.h>
#include <libinput.h>
#include <wayland-server.h>

#include "fd.hpp"
#include "udev.hpp"

static struct input {
   struct libinput *handle;
   struct wl_event_source *event_source;
} input;

static struct udev {
   struct udev *handle;
   struct udev_monitor *monitor;
   struct wl_event_source *event_source;
} udev;

static int
input_open_restricted(const char *path, int flags, void *user_data)
{
   (void)user_data;
   return fire_fd_open(path, flags, FIRE_FD_INPUT);
}

static void
input_close_restricted(int fd, void *user_data)
{
   (void)user_data;
   fire_fd_close(fd);
}

static const struct libinput_interface libinput_implementation = {
   .open_restricted = input_open_restricted,
   .close_restricted = input_close_restricted,
};

static double
pointer_abs_x(void *internal, uint32_t width)
{
   struct libinput_event_pointer *pev = (libinput_event_pointer*)internal;
   return libinput_event_pointer_get_absolute_x_transformed(pev, width);
}

static double
pointer_abs_y(void *internal, uint32_t height)
{
   struct libinput_event_pointer *pev = (libinput_event_pointer*)internal;
   return libinput_event_pointer_get_absolute_y_transformed(pev, height);
}

static double
touch_abs_x(void *internal, uint32_t width)
{
   struct libinput_event_touch *tev = (libinput_event_touch*)internal;
   return libinput_event_touch_get_x_transformed(tev, width);
}

static double
touch_abs_y(void *internal, uint32_t height)
{
   struct libinput_event_touch *tev = (libinput_event_touch*)internal;
   return libinput_event_touch_get_y_transformed(tev, height);
}

static fire_touch_type
fire_touch_type_for_libinput_type(enum libinput_event_type type)
{
   switch (type) {
      case LIBINPUT_EVENT_TOUCH_UP:
         return FIRE_TOUCH_UP;
      case LIBINPUT_EVENT_TOUCH_DOWN:
         return FIRE_TOUCH_DOWN;
      case LIBINPUT_EVENT_TOUCH_MOTION:
         return FIRE_TOUCH_MOTION;
      case LIBINPUT_EVENT_TOUCH_FRAME:
         return FIRE_TOUCH_FRAME;
      case LIBINPUT_EVENT_TOUCH_CANCEL:
         return FIRE_TOUCH_CANCEL;

      default: break;
   }

   assert(0 && "should not happen");
   return FIRE_TOUCH_CANCEL;
}

static int
input_event(int fd, uint32_t mask, void *data)
{
   (void)fd, (void)mask;
   struct input *input = data;

   if (libinput_dispatch(input->handle) != 0)
      fire_log(FIRE_LOG_WARN, "Failed to dispatch libinput");

   struct libinput_event *event;
   while ((event = libinput_get_event(input->handle))) {
      struct libinput *handle = libinput_event_get_context(event);
      struct libinput_device *device = libinput_event_get_device(event);
      (void)handle;

      switch (libinput_event_get_type(event)) {
         case LIBINPUT_EVENT_DEVICE_ADDED:
            FIRE_INTERFACE_EMIT(input.created, device);
            break;

         case LIBINPUT_EVENT_DEVICE_REMOVED:
            FIRE_INTERFACE_EMIT(input.destroyed, device);
            break;

         case LIBINPUT_EVENT_POINTER_MOTION:
         {
            struct libinput_event_pointer *pev = libinput_event_get_pointer_event(event);
            struct fire_input_event ev;
            ev.type = FIRE_INPUT_EVENT_MOTION;
            ev.time = libinput_event_pointer_get_time(pev);
            ev.motion.dx = libinput_event_pointer_get_dx(pev);
            ev.motion.dy = libinput_event_pointer_get_dy(pev);
            wl_signal_emit(&fire_system_signals()->input, &ev);
         }
         break;

         case LIBINPUT_EVENT_POINTER_MOTION_ABSOLUTE:
         {
            struct libinput_event_pointer *pev = libinput_event_get_pointer_event(event);
            struct fire_input_event ev;
            ev.type = FIRE_INPUT_EVENT_MOTION_ABSOLUTE;
            ev.time = libinput_event_pointer_get_time(pev);
            ev.motion_abs.x = pointer_abs_x;
            ev.motion_abs.y = pointer_abs_y;
            ev.motion_abs.internal = pev;
            wl_signal_emit(&fire_system_signals()->input, &ev);
         }
         break;

         case LIBINPUT_EVENT_POINTER_BUTTON:
         {
            struct libinput_event_pointer *pev = libinput_event_get_pointer_event(event);
            struct fire_input_event ev;
            ev.type = FIRE_INPUT_EVENT_BUTTON;
            ev.time = libinput_event_pointer_get_time(pev);
            ev.button.code = libinput_event_pointer_get_button(pev);
            ev.button.state = (enum wl_pointer_button_state)libinput_event_pointer_get_button_state(pev);
            wl_signal_emit(&fire_system_signals()->input, &ev);
         }
         break;

         case LIBINPUT_EVENT_POINTER_AXIS:
         {
            struct libinput_event_pointer *pev = libinput_event_get_pointer_event(event);
            struct fire_input_event ev;
            memset(&ev.scroll, 0, sizeof(ev));
            ev.type = FIRE_INPUT_EVENT_SCROLL;
            ev.time = libinput_event_pointer_get_time(pev);

#if LIBINPUT_VERSION_MAJOR == 0 && LIBINPUT_VERSION_MINOR < 8
            /* < libinput 0.8.x (at least to 0.6.x) */
            const enum wl_pointer_axis axis = libinput_event_pointer_get_axis(pev);
            ev.scroll.amount[(axis == LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL)] = libinput_event_pointer_get_axis_value(pev);
            ev.scroll.axis_bits |= (axis == LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL ? FIRE_SCROLL_AXIS_HORIZONTAL : FIRE_SCROLL_AXIS_VERTICAL);
#else
            /* > libinput 0.8.0 */
            if (libinput_event_pointer_has_axis(pev, LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL)) {
               ev.scroll.amount[0] = libinput_event_pointer_get_axis_value(pev, LIBINPUT_POINTER_AXIS_SCROLL_VERTICAL);
               ev.scroll.axis_bits |= FIRE_SCROLL_AXIS_VERTICAL;
            }

            if (libinput_event_pointer_has_axis(pev, LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL)) {
               ev.scroll.amount[1] = libinput_event_pointer_get_axis_value(pev, LIBINPUT_POINTER_AXIS_SCROLL_HORIZONTAL);
               ev.scroll.axis_bits |= FIRE_SCROLL_AXIS_HORIZONTAL;
            }
#endif

            // We should get other axis information from libinput as well, like source (finger, wheel) (v0.8)
            wl_signal_emit(&fire_system_signals()->input, &ev);
         }
         break;

         case LIBINPUT_EVENT_KEYBOARD_KEY:
         {
            struct libinput_event_keyboard *kev = libinput_event_get_keyboard_event(event);
            struct fire_input_event ev;
            ev.type = FIRE_INPUT_EVENT_KEY;
            ev.time = libinput_event_keyboard_get_time(kev);
            ev.key.code = libinput_event_keyboard_get_key(kev);
            ev.key.state = (enum wl_keyboard_key_state)libinput_event_keyboard_get_key_state(kev);
            wl_signal_emit(&fire_system_signals()->input, &ev);
         }
         break;

         case LIBINPUT_EVENT_TOUCH_UP:
         case LIBINPUT_EVENT_TOUCH_DOWN:
         case LIBINPUT_EVENT_TOUCH_MOTION:
         case LIBINPUT_EVENT_TOUCH_FRAME:
         case LIBINPUT_EVENT_TOUCH_CANCEL:
         {
            struct libinput_event_touch *tev = libinput_event_get_touch_event(event);
            struct fire_input_event ev;
            ev.type = FIRE_INPUT_EVENT_TOUCH;
            ev.time = libinput_event_touch_get_time(tev);
            ev.touch.type = fire_touch_type_for_libinput_type(libinput_event_get_type(event));
            ev.touch.x = touch_abs_x;
            ev.touch.y = touch_abs_y;
            ev.touch.slot = libinput_event_touch_get_seat_slot(tev);
            wl_signal_emit(&fire_system_signals()->input, &ev);
         }
         break;

         default: break;
      }

      libinput_event_destroy(event);
   }

   return 0;
}

static bool
input_set_event_loop(struct wl_event_loop *loop)
{
   if (input.event_source) {
      wl_event_source_remove(input.event_source);
      input.event_source = NULL;
   }

   if (input.handle && loop && !(input.event_source = wl_event_loop_add_fd(loop, libinput_get_fd(input.handle), WL_EVENT_READABLE, input_event, &input)))
      return false;
   return true;
}

static bool
is_hotplug(uint32_t drm_id, struct udev_device *device)
{
   uint32_t id;
   const char *sysnum;
   if (!(sysnum = udev_device_get_sysnum(device)) || !chck_cstr_to_u32(sysnum, &id) || id != drm_id)
      return false;

   const char *val;
   if (!(val = udev_device_get_property_value(device, "HOTPLUG")))
      return false;

   return chck_cstreq(val, "1");
}

static int
udev_event(int fd, uint32_t mask, void *data)
{
   (void)fd, (void)mask;
   struct udev *udev = data;
   struct udev_device *device;

   if (!(device = udev_monitor_receive_device(udev->monitor)))
      return 0;

   fire_log(FIRE_LOG_INFO, "udev: got device %s", udev_device_get_sysname(device));

   // FIXME: pass correct drm id
   if (is_hotplug(0, device)) {
      fire_log(FIRE_LOG_INFO, "udev: hotplug");
      struct fire_output_event ev = { .type = FIRE_OUTPUT_EVENT_UPDATE };
      wl_signal_emit(&fire_system_signals()->output, &ev);
      goto out;
   }

   const char *action;
   if (!(action = udev_device_get_action(device)))
      goto out;

   if (!chck_cstrneq("event", udev_device_get_sysname(device), sizeof("event")))
      goto out;

   // XXX: Free event loop for any other stuff. We probably should expose this to api.
   if (chck_cstreq(action, "add"))
      fire_log(FIRE_LOG_INFO, "udev: device added");
   else if (chck_cstreq(action, "remove")) {
      fire_log(FIRE_LOG_INFO, "udev: device removed");
   }

out:
   udev_device_unref(device);
   return 0;
}

static bool
udev_set_event_loop(struct wl_event_loop *loop)
{
   if (udev.event_source) {
      wl_event_source_remove(udev.event_source);
      udev.event_source = NULL;
   }

   if (udev.handle && udev.monitor && loop && !(udev.event_source = wl_event_loop_add_fd(loop, udev_monitor_get_fd(udev.monitor), WL_EVENT_READABLE, udev_event, &udev)))
      return false;

   return true;
}

static void
activate_event(struct wl_listener *listener, void *data)
{
   (void)listener;

   struct fire_activate_event *ev = data;
   if (input.handle) {
      if (!ev->active) {
         fire_log(FIRE_LOG_INFO, "libinput: suspend");
         libinput_suspend(input.handle);
      } else {
         fire_log(FIRE_LOG_INFO, "libinput: resume");
         libinput_resume(input.handle);
      }
   }
}

static struct wl_listener activate_listener = {
   .notify = activate_event,
};

static void
cb_input_log_handler(struct libinput *input, enum libinput_log_priority priority, const char *format, va_list args)
{
   (void)input, (void)priority;
   fire_vlog(FIRE_LOG_INFO, format, args);
}

FIRE_PURE bool
fire_input_has_init(void)
{
   return (input.handle ? true : false);
}

void
fire_input_terminate(void)
{
   input_set_event_loop(NULL);
   libinput_unref(input.handle);
   memset(&input, 0, sizeof(input));
}

bool
fire_input_init(void)
{
   assert(udev.handle && "call fire_udev_init first");

   if (input.handle)
      return true;

   if (!(input.handle = libinput_udev_create_context(&libinput_implementation, &input, udev.handle)))
      goto failed_to_create_context;

   const char *xdg_seat = getenv("XDG_SEAT");
   if (libinput_udev_assign_seat(input.handle, (xdg_seat ? xdg_seat : "seat0")) != 0)
      goto failed_to_assign_seat;

   libinput_log_set_handler(input.handle, &cb_input_log_handler);
   libinput_log_set_priority(input.handle, LIBINPUT_LOG_PRIORITY_ERROR);
   return input_set_event_loop(fire_event_loop());

failed_to_create_context:
   fire_log(FIRE_LOG_WARN, "Failed to create libinput udev context");
   goto fail;
failed_to_assign_seat:
   fire_log(FIRE_LOG_WARN, "Failed to assign seat to libinput");
fail:
   fire_input_terminate();
   return false;
}

void
fire_udev_terminate(void)
{
   if (udev.handle)
      wl_list_remove(&activate_listener.link);

   udev_set_event_loop(NULL);
   udev_monitor_unref(udev.monitor);
   udev_unref(udev.handle);
   memset(&udev, 0, sizeof(udev));
}

bool
fire_udev_init(void)
{
   if (udev.handle)
      return true;

   if (!(udev.handle = udev_new()))
      return false;

   if (!(udev.monitor = udev_monitor_new_from_netlink(udev.handle, "udev")))
      goto monitor_fail;

   udev_monitor_filter_add_match_subsystem_devtype(udev.monitor, "drm", NULL);
   udev_monitor_filter_add_match_subsystem_devtype(udev.monitor, "input", NULL);

   if (udev_monitor_enable_receiving(udev.monitor) < 0)
      goto monitor_receiving_fail;

   if (!udev_set_event_loop(fire_event_loop()))
      goto fail;

   wl_signal_add(&fire_system_signals()->activate, &activate_listener);
   return true;

monitor_fail:
   fire_log(FIRE_LOG_WARN, "Failed to create udev-monitor from netlink");
   goto fail;
monitor_receiving_fail:
   fire_log(FIRE_LOG_WARN, "Failed to enable udev-monitor receiving");
fail:
   fire_udev_terminate();
   return false;
}
