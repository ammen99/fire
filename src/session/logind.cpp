/** adapted from wlc logind.c **/
/** adapted from weston logind-util.c */


#include <string>
#include <iostream>

#include <stdio.h>
#include <unistd.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <core.hpp>

#include <dbus-1.0/dbus/dbus.h>
#include <systemd/sd-login.h>

#include "dbus.hpp"
#include "logind.hpp"

#ifndef DRM_MAJOR
#  define DRM_MAJOR 226
#endif

#ifndef KDSKBMUTE
#  define KDSKBMUTE 0x4B51
#endif

static struct {
   char *seat;
   char *sid;
   DBusConnection *dbus;
   wl_event_source *dbus_ctx;
   std::string spath;
   int vt;

   struct {
      DBusPendingCall *active;
   } pending;
} logind;


static int take_device(uint32_t major, uint32_t minor, bool *out_paused)
{
   if (out_paused)
      *out_paused = false;

   DBusMessage *m;
   if (!(m = dbus_message_new_method_call("org.freedesktop.login1", logind.spath.c_str(), "org.freedesktop.login1.Session", "TakeDevice")))
      return -1;

   if (!dbus_message_append_args(m, DBUS_TYPE_UINT32, &major, DBUS_TYPE_UINT32, &minor, DBUS_TYPE_INVALID))
      goto error0;

   DBusMessage *reply;
   if (!(reply = dbus_connection_send_with_reply_and_block(logind.dbus, m, -1, NULL)))
      goto error0;

   int fd;
   dbus_bool_t paused;
   if (!dbus_message_get_args(reply, NULL, DBUS_TYPE_UNIX_FD, &fd, DBUS_TYPE_BOOLEAN, &paused, DBUS_TYPE_INVALID))
      goto error1;

   if (out_paused)
      *out_paused = paused;

   int fl;
   if ((fl = fcntl(fd, F_GETFL)) < 0 || fcntl(fd, F_SETFD, fl | FD_CLOEXEC) < 0)
      goto error2;

   dbus_message_unref(reply);
   dbus_message_unref(m);
   return fd;

error2:
   close(fd);
error1:
   dbus_message_unref(reply);
error0:
   dbus_message_unref(m);
   return -1;
}

static void release_device(uint32_t major, uint32_t minor)
{
   DBusMessage *m;
   if (!(m = dbus_message_new_method_call("org.freedesktop.login1", logind.spath.c_str(), "org.freedesktop.login1.Session", "ReleaseDevice")))
      return;

   if (dbus_message_append_args(m, DBUS_TYPE_UINT32, &major, DBUS_TYPE_UINT32, &minor, DBUS_TYPE_INVALID))
      dbus_connection_send(logind.dbus, m, NULL);

   dbus_message_unref(m);
}

static void
pause_device(uint32_t major, uint32_t minor)
{
   DBusMessage *m;
   if (!(m = dbus_message_new_method_call("org.freedesktop.login1", logind.spath.c_str(), "org.freedesktop.login1.Session", "PauseDeviceComplete")))
      return;

   if (dbus_message_append_args(m, DBUS_TYPE_UINT32, &major, DBUS_TYPE_UINT32, &minor, DBUS_TYPE_INVALID))
      dbus_connection_send(logind.dbus, m, NULL);

   dbus_message_unref(m);
}

int logind_open(const char *path, int flags) { 

    if(!path) {
        std::printf("WLC_LOGIND_OPEN CALLED WITH NULL!\n");
        std::exit(-1);
    }

   struct stat st;
   if (stat(path, &st) < 0 || !S_ISCHR(st.st_mode))
      return -1;

   int fd;
   if ((fd = take_device(major(st.st_rdev), minor(st.st_rdev), NULL)) < 0)
      return fd;

   return fd;
}

void wlc_logind_close(int fd) {
   struct stat st;
   if (fstat(fd, &st) < 0 || !S_ISCHR(st.st_mode))
      return;

   release_device(major(st.st_rdev), minor(st.st_rdev));
}

static void
parse_active(DBusMessage *m, DBusMessageIter *iter)
{

    if(!iter) {
        std::cout << "parse_active called withnull!" << std::endl;
    }

   if (dbus_message_iter_get_arg_type(iter) != DBUS_TYPE_VARIANT)
      return;

   DBusMessageIter sub;
   dbus_message_iter_recurse(iter, &sub);

   if (dbus_message_iter_get_arg_type(&sub) != DBUS_TYPE_BOOLEAN)
      return;

   dbus_bool_t b;
   dbus_message_iter_get_basic(&sub, &b);

   // TODO: implement activation 
   //wlc_set_active(b);
}

static void
get_active_cb(DBusPendingCall *pending, void *data)
{
    if(!pending) {
        std::cout << "get_active_cb called with null!\n";
        std::exit(-1);
    }

   dbus_pending_call_unref(logind.pending.active);
   logind.pending.active = NULL;

   DBusMessage *m;
   if (!(m = dbus_pending_call_steal_reply(pending)))
      return;

   DBusMessageIter iter;
   if (dbus_message_get_type(m) == DBUS_MESSAGE_TYPE_METHOD_RETURN && dbus_message_iter_init(m, &iter))
      parse_active(m, &iter);

   dbus_message_unref(m);
}

static void
get_active(void)
{
   DBusMessage *m;
   if (!(m = dbus_message_new_method_call("org.freedesktop.login1", logind.spath.c_str(), "org.freedesktop.DBus.Properties", "Get")))
      return;

   const char *iface = "org.freedesktop.login1.Session";
   const char *name = "Active";
   if (!dbus_message_append_args(m, DBUS_TYPE_STRING, &iface, DBUS_TYPE_STRING, &name, DBUS_TYPE_INVALID))
      goto error0;

   DBusPendingCall *pending;
   if (!dbus_connection_send_with_reply(logind.dbus, m, &pending, -1))
      goto error0;

   if (!dbus_pending_call_set_notify(pending, get_active_cb, NULL, NULL))
      goto error1;

   if (logind.pending.active) {
      dbus_pending_call_cancel(logind.pending.active);
      dbus_pending_call_unref(logind.pending.active);
   }

   logind.pending.active = pending;
   return;

error1:
   dbus_pending_call_cancel(pending);
   dbus_pending_call_unref(pending);
error0:
   dbus_message_unref(m);
}

static void
disconnected_dbus(void)
{
    std::cout << "logindd: dbus connection lost. Terminating ...\n";
    //TODO: implement proper exiting
    std::exit(-1);
}

static void
session_removed(DBusMessage *m)
{
   const char *name, *obj;
   if (!dbus_message_get_args(m, NULL, DBUS_TYPE_STRING, &name, DBUS_TYPE_OBJECT_PATH, &obj, DBUS_TYPE_INVALID))
      return;

   if(name != logind.sid)
      return;

    std::cout << "logindd: dbus connection lost. Terminating ...\n";
    //TODO: implement proper exiting
    std::exit(-1);
}

static void
property_changed(DBusMessage *m)
{
   DBusMessageIter iter;
   if (!dbus_message_iter_init(m, &iter) || dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_STRING)
      goto error0;

   const char *interface;
   dbus_message_iter_get_basic(&iter, &interface);

   if (!dbus_message_iter_next(&iter) || dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY)
      goto error0;

   DBusMessageIter sub;
   dbus_message_iter_recurse(&iter, &sub);

   DBusMessageIter entry;
   while (dbus_message_iter_get_arg_type(&sub) == DBUS_TYPE_DICT_ENTRY) {
      dbus_message_iter_recurse(&sub, &entry);

      if (dbus_message_iter_get_arg_type(&entry) != DBUS_TYPE_STRING)
         goto error0;

      const char *name;
      dbus_message_iter_get_basic(&entry, &name);
      if (!dbus_message_iter_next(&entry))
         goto error0;

      if (std::string(name) == "Active") {
         parse_active(m, &entry);
         return;
      }

      dbus_message_iter_next(&sub);
   }

   if (!dbus_message_iter_next(&iter) || dbus_message_iter_get_arg_type(&iter) != DBUS_TYPE_ARRAY)
      goto error0;

   dbus_message_iter_recurse(&iter, &sub);

   while (dbus_message_iter_get_arg_type(&sub) == DBUS_TYPE_STRING) {
      const char *name;
      dbus_message_iter_get_basic(&sub, &name);

      if (std::string(name) == "Active") {
         get_active();
         return;
      }

      dbus_message_iter_next(&sub);
   }

   return;

error0:
   std::cout << "logind: cannot parse PropertiesChanged dbus signal" << std::endl;
}

static void
device_paused(DBusMessage *m)
{
   const char *type;
   uint32_t major, minor;
   if (!dbus_message_get_args(m, NULL, DBUS_TYPE_UINT32, &major, DBUS_TYPE_UINT32, &minor, DBUS_TYPE_STRING, &type, DBUS_TYPE_INVALID))
      return;

   if (std::string(type) ==  "pause")
      pause_device(major, minor);

   if (major == DRM_MAJOR) {
       core->set_active(false);
   }
}

static void
device_resumed(DBusMessage *m)
{
   assert(m);

   uint32_t major;
   if (!dbus_message_get_args(m, NULL, DBUS_TYPE_UINT32, &major, DBUS_TYPE_INVALID))
      return;

   if (major == DRM_MAJOR)
       core->set_active(true);
}

static DBusHandlerResult
filter_dbus(DBusConnection *c, DBusMessage *m, void *data)
{
   assert(m);
   (void)c, (void)data;

   if (dbus_message_is_signal(m, DBUS_INTERFACE_LOCAL, "Disconnected")) {
      disconnected_dbus();
      return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
   }

   struct {
      const char *path, *name;
      void (*function)(DBusMessage *m);
   } map[] = {
      { "org.freedesktop.login1.Manager", "SessionRemoved", session_removed },
      { "org.freedesktop.DBus.Properties", "PropertiesChanged", property_changed },
      { "org.freedesktop.login1.Session", "PauseDevice", device_paused },
      { "org.freedesktop.login1.Session", "ResumeDevice", device_resumed },
      { NULL, NULL, NULL },
   };

   for (uint32_t i = 0; map[i].function; ++i) {
      if (!dbus_message_is_signal(m, map[i].path, map[i].name))
         continue;

      map[i].function(m);
      break;
   }

   return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

static bool
setup_dbus(void)
{

    //TODO: might be wrong
    logind.spath = "/org/freedesktop/login1/session/" + std::string(logind.sid);

   if (!dbus_connection_add_filter(logind.dbus, filter_dbus, NULL, NULL))
      goto error0;

   if (!fire_dbus_add_match_signal(logind.dbus, "org.freedesktop.login1", "org.freedesktop.login1.Manager", "SessionRemoved", "/org/freedesktop/login1"))
      goto error0;

   if (!fire_dbus_add_match_signal(logind.dbus, "org.freedesktop.login1", "org.freedesktop.login1.Session", "PauseDevice", logind.spath.c_str()))
      goto error0;

   if (!fire_dbus_add_match_signal(logind.dbus, "org.freedesktop.login1", "org.freedesktop.login1.Session", "ResumeDevice", logind.spath.c_str()))
      goto error0;

   if (!fire_dbus_add_match_signal(logind.dbus, "org.freedesktop.login1", "org.freedesktop.DBus.Properties", "PropertiesChanged", logind.spath.c_str()))
      goto error0;

   return true;

error0:
   return false;
}

static bool
take_control(void)
{
   DBusError err;
   dbus_error_init(&err);

   DBusMessage *m;
   if (!(m = dbus_message_new_method_call("org.freedesktop.login1", logind.spath.c_str(), "org.freedesktop.login1.Session", "TakeControl")))
      return false;

   if (!dbus_message_append_args(m, DBUS_TYPE_BOOLEAN, (dbus_bool_t[]){false}, DBUS_TYPE_INVALID))
      goto error0;

   DBusMessage *reply;
   if (!(reply = dbus_connection_send_with_reply_and_block(logind.dbus, m, -1, &err))) {
      if (dbus_error_has_name(&err, DBUS_ERROR_UNKNOWN_METHOD)) {
          std::cout << "logind: old systemd version detected" << std::endl;
      } else {
          std::cout << "logind: cannot take control over session " << logind.sid << std::endl;
      }
      dbus_error_free(&err);
      goto error0;
   }

   dbus_message_unref(reply);
   dbus_message_unref(m);
   return true;

error0:
   dbus_message_unref(m);
   return false;
}

static void
release_control(void)
{
   if (!logind.spath.c_str())
      return;

   DBusMessage *m;
   if (!(m = dbus_message_new_method_call("org.freedesktop.login1", logind.spath.c_str(), "org.freedesktop.login1.Session", "ReleaseControl")))
      return;

   dbus_connection_send(logind.dbus, m, NULL);
   dbus_message_unref(m);
}

static bool
get_vt(const char *sid, int *out)
{
   return (sd_session_get_vt(sid, (unsigned*)out) >= 0);
//
//   char *tty;
//   if (sd.sd_session_get_tty(sid, &tty) < 0)
//      return false;
//
//   const uint32_t r = sscanf(tty, "tty%u", out);
//   free(tty);
//   return (r == 1);
}

bool logind_available()
{

   char *sid;
   if (sd_pid_get_session(getpid(), &sid) < 0)
      return false;

   char *seat;
   if (sd_session_is_active(sid) || sd_session_get_seat(sid, &seat) < 0)
      goto error0;

   int vt;
   if (!get_vt(sid, &vt))
      goto error1;

   free(sid);
   free(seat);
   return true;

error1:
   free(seat);
error0:
   free(sid);
   return false;
}

void
logind_terminate(void)
{
   if (logind.pending.active) {
      dbus_pending_call_cancel(logind.pending.active);
      dbus_pending_call_unref(logind.pending.active);
   }

   release_control();
   free(logind.sid);
   free(logind.seat);
   fire_dbus_close(logind.dbus, logind.dbus_ctx);
}

int
logind_init(const char *seat_id)
{
   if (logind.vt != 0)
      return logind.vt;

   if (sd_pid_get_session(getpid(), &logind.sid) < 0)
      goto session_fail;

   if (sd_session_get_seat(logind.sid, &logind.seat) < 0)
      goto seat_fail;

   if(seat_id != logind.seat)
      goto seat_does_not_match;

   if (!get_vt(logind.sid, &logind.vt))
      goto not_a_vt;

   if (!fire_dbus_open(core->get_event_loop(), DBUS_BUS_SYSTEM, &logind.dbus, &logind.dbus_ctx) || !setup_dbus() || !take_control())
      goto dbus_fail;

   std::cout << "logind: session control granted" << std::endl;
   return logind.vt;

session_fail:
   std::cout << "logind: not running in a systemd session" << std::endl;
   goto fail;
seat_fail:
   std::cout  << "logind: failed to get session seat" << std::endl;
   goto fail;
seat_does_not_match:
   std::cout << "logind: seat does not match wlc seat  " << seat_id << " " <<  logind.seat << std::endl;
   goto fail;
not_a_vt:
   std::cout << "logind: session not running on a VT" << std::endl;
   goto fail;
dbus_fail:
   std::cout << "logind: failed to setup dbus" << std::endl;
fail:
   logind_terminate();
   return 0;
}
