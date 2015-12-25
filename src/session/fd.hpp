#ifndef FD_H_
#define FD_H_

#include <stdbool.h>

enum fire_fd_type {
   FIRE_FD_INPUT,
   FIRE_FD_DRM,
   FIRE_FD_LAST
};

// Use these functions to control tty.
// Reason is that for vt's which we don't have session on, we need root permissions to switch.
bool fire_fd_activate(void);
bool fire_fd_deactivate(void);
bool fire_fd_activate_vt(int vt);

int  fire_fd_open(const char *path, int flags, enum fire_fd_type type);
void fire_fd_close(int fd);
void fire_fd_terminate(void);
void fire_fd_init(int argc, char *argv[], bool has_logind);

#endif /* FD_H_ */
