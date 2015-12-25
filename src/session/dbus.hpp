#ifndef DBUS_H_
#define DBUS_H_

#include <dbus/dbus.h>
#include <stdbool.h>

struct wl_event_loop;
struct wl_event_source;

bool fire_dbus_add_match(DBusConnection *c, const char *format, ...);
bool fire_dbus_add_match_signal(DBusConnection *c, const char *sender, const char *iface, const char *member, const char *path);
void fire_dbus_remove_match(DBusConnection *c, const char *format, ...);
void fire_dbus_remove_match_signal(DBusConnection *c, const char *sender, const char *iface, const char *member, const char *path);

void fire_dbus_close(DBusConnection *c, struct wl_event_source *ctx);
bool fire_dbus_open(struct wl_event_loop *loop, DBusBusType bus, DBusConnection **out, struct wl_event_source **ctx_out);

#endif /* DBUS_H_ */
