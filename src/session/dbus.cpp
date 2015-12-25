#include <iostream>
#include <unistd.h>
#include <sys/eventfd.h>
#include <wayland-server.h>
#include "dbus.hpp"
#include <errno.h>
#include <assert.h>

static int
dispatch_watch(int fd, uint32_t mask, void *data)
{
   (void)fd;
   DBusWatch *watch = (DBusWatch*)data;

   if (dbus_watch_get_enabled(watch)) {
      uint32_t flags = 0;
      if (mask & WL_EVENT_READABLE)
         flags |= DBUS_WATCH_READABLE;
      if (mask & WL_EVENT_WRITABLE)
         flags |= DBUS_WATCH_WRITABLE;
      if (mask & WL_EVENT_HANGUP)
         flags |= DBUS_WATCH_HANGUP;
      if (mask & WL_EVENT_ERROR)
         flags |= DBUS_WATCH_ERROR;

      dbus_watch_handle(watch, flags);
   }

   return 0;
}

static dbus_bool_t
add_watch(DBusWatch *watch, void *data)
{
   struct wl_event_loop *loop = (wl_event_loop*) data;

   uint32_t mask = 0;
   if (dbus_watch_get_enabled(watch)) {
      uint32_t flags = dbus_watch_get_flags(watch);
      if (flags & DBUS_WATCH_READABLE)
         mask |= WL_EVENT_READABLE;
      if (flags & DBUS_WATCH_WRITABLE)
         mask |= WL_EVENT_WRITABLE;
   }

   int fd = dbus_watch_get_unix_fd(watch);
   struct wl_event_source *s;
   if (!(s = wl_event_loop_add_fd(loop, fd, mask, dispatch_watch, watch)))
      return false;

   dbus_watch_set_data(watch, s, NULL);
   return true;
}

static void
remove_watch(DBusWatch *watch, void *data)
{
   (void)data;
   struct wl_event_source *s;
   if (!(s = (wl_event_source*)dbus_watch_get_data(watch)))
      return;

   wl_event_source_remove(s);
}

static void
toggle_watch(DBusWatch *watch, void *data)
{
   (void)data;
   struct wl_event_source *s;
   if (!(s = (wl_event_source*)dbus_watch_get_data(watch)))
      return;

   uint32_t mask = 0;
   if (dbus_watch_get_enabled(watch)) {
      uint32_t flags = dbus_watch_get_flags(watch);
      if (flags & DBUS_WATCH_READABLE)
         mask |= WL_EVENT_READABLE;
      if (flags & DBUS_WATCH_WRITABLE)
         mask |= WL_EVENT_WRITABLE;
   }

   wl_event_source_fd_update(s, mask);
}

static int
dispatch_timeout(void *data)
{
   DBusTimeout *timeout = (DBusTimeout*)data;
   if (dbus_timeout_get_enabled(timeout))
      dbus_timeout_handle(timeout);
   return 0;
}

static int
adjust_timeout(DBusTimeout *timeout, struct wl_event_source *s)
{
   int64_t t = 0;
   if (dbus_timeout_get_enabled(timeout))
      t = dbus_timeout_get_interval(timeout);
   return wl_event_source_timer_update(s, t);
}

static dbus_bool_t
add_timeout(DBusTimeout *timeout, void *data)
{
   struct wl_event_loop *loop = (wl_event_loop*)data;

   struct wl_event_source *s;
   if (!(s = wl_event_loop_add_timer(loop, dispatch_timeout, timeout)))
      return false;

   if (adjust_timeout(timeout, s) < 0)
      goto error0;

   dbus_timeout_set_data(timeout, s, NULL);
   return true;

error0:
   wl_event_source_remove(s);
   return false;
}

static void
remove_timeout(DBusTimeout *timeout, void *data)
{
   (void)data;
   struct wl_event_source *s;
   if (!(s = (wl_event_source*)dbus_timeout_get_data(timeout)))
      return;

   wl_event_source_remove(s);
}

static void
toggle_timeout(DBusTimeout *timeout, void *data)
{
   (void)data;
   struct wl_event_source *s;
   if (!(s = (wl_event_source*)dbus_timeout_get_data(timeout)))
      return;

   adjust_timeout(timeout, s);
}

static int
dispatch(int fd, uint32_t mask, void *data)
{
   (void)fd, (void)mask;
   DBusConnection *c = (DBusConnection*)data;
   int r;

   do {
      r = dbus_connection_dispatch(c);
      if (r == DBUS_DISPATCH_COMPLETE)
         r = 0;
      else if (r == DBUS_DISPATCH_DATA_REMAINS)
         r = -EAGAIN;
      else if (r == DBUS_DISPATCH_NEED_MEMORY)
         r = -ENOMEM;
      else
         r = -EIO;
   } while (r == -EAGAIN);

   if (r)
      std::cout << "cannot dispatch dbus events: " <<  r << std::endl;

   return 0;
}

static bool
dbus_bind(struct wl_event_loop *loop, DBusConnection *c, struct wl_event_source **ctx_out)
{
   assert(loop && c && ctx_out);
   *ctx_out = NULL;

   /* Idle events cannot reschedule themselves, therefore we use a dummy
    * event-fd and mark it for post-dispatch. Hence, the dbus
    * dispatcher is called after every dispatch-round.
    * This is required as dbus doesn't allow dispatching events from
    * within its own event sources. */

   int fd;
   if ((fd = eventfd(0, EFD_CLOEXEC)) < 0)
      return false;

   *ctx_out = wl_event_loop_add_fd(loop, fd, 0, dispatch, c);
   close(fd);

   if (!*ctx_out)
      return false;

   wl_event_source_check(*ctx_out);

   if (!dbus_connection_set_watch_functions(c, add_watch, remove_watch, toggle_watch, loop, NULL))
      goto error0;

   if (!dbus_connection_set_timeout_functions(c, add_timeout, remove_timeout, toggle_timeout, loop, NULL))
      goto error0;

   dbus_connection_ref(c);
   return true;

error0:
   dbus_connection_set_timeout_functions(c, NULL, NULL, NULL, NULL, NULL);
   dbus_connection_set_watch_functions(c, NULL, NULL, NULL, NULL, NULL);
   wl_event_source_remove(*ctx_out);
   *ctx_out = NULL;
   return false;
}

static void
dbus_unbind(DBusConnection *c, struct wl_event_source *ctx)
{
   assert(c && ctx);
   dbus_connection_set_timeout_functions(c, NULL, NULL, NULL, NULL, NULL);
   dbus_connection_set_watch_functions(c, NULL, NULL, NULL, NULL, NULL);
   dbus_connection_unref(c);
   wl_event_source_remove(ctx);
}

bool
fire_dbus_add_match(DBusConnection *c, const char *format, ...)
{
   assert(c && format);

   va_list list;
   va_start(list, format);

   char buf[2048];
   std::vsprintf(buf, format, list);
   va_end(list);

   DBusError err;
   dbus_error_init(&err);
   dbus_bus_add_match(c, buf, &err);

   if (dbus_error_is_set(&err)) {
       std::cout << "dbus: " << err.name << std::endl;
       std::cout << "dbus: " << err.message << std::endl;

        dbus_error_free(&err);
      return false;
   }

   return true;
}

bool
fire_dbus_add_match_signal(DBusConnection *c, const char *sender, const char *iface, const char *member, const char *path)
{
   assert(c && sender && iface && member && path);
   return fire_dbus_add_match(c, "type='signal',sender='%s',interface='%s',member='%s',path='%s'", sender, iface, member, path);
}

void
fire_dbus_remove_match(DBusConnection *c, const char *format, ...)
{
   assert(c && format);

   va_list list;
   va_start(list, format);
   char buf[2048];
   std::vsprintf(buf, format, list);
   va_end(list);

    dbus_bus_remove_match(c, buf, NULL);
}

void
fire_dbus_remove_match_signal(DBusConnection *c, const char *sender, const char *iface, const char *member, const char *path)
{
   assert(c && sender && iface && member && path);
   fire_dbus_remove_match(c, "type='signal',sender='%s',interface='%s',member='%s',path='%s'", sender, iface, member, path);
}

void
fire_dbus_close(DBusConnection *c, struct wl_event_source *ctx)
{
   assert(c && ctx);
   dbus_unbind(c, ctx);
   dbus_connection_close(c);
   dbus_connection_unref(c);
}

bool
fire_dbus_open(struct wl_event_loop *loop, DBusBusType bus, DBusConnection **out, struct wl_event_source **ctx_out)
{
   assert(loop && out && ctx_out);

   dbus_connection_set_change_sigpipe(false);

   /* This is actually synchronous. It blocks for some authentication and
    * setup. We just trust the dbus-server here and accept this blocking
    * call. There is no real reason to complicate things further and make
    * this asynchronous/non-blocking. A context should be created during
    * thead/process/app setup, so blocking calls should be fine. */

   DBusConnection *c;
   if (!(c = dbus_bus_get_private(bus, NULL)))
      return false;

   dbus_connection_set_exit_on_disconnect(c, false);

   if (!dbus_bind(loop, c, ctx_out))
      goto error0;

   *out = c;
   return true;

error0:
   dbus_connection_close(c);
   dbus_connection_unref(c);
   return false;
}
