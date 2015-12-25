#ifndef TTY_H_
#define TTY_H_

// Do not call these functions directly!
// Use the fd.h counterparts instead.
bool fire_tty_activate(void);
bool fire_tty_deactivate(void);
bool fire_tty_activate_vt(int vt);

void fire_tty_setup_signal_handlers(void);
int fire_tty_get_vt(void);
void fire_tty_terminate(void);
void fire_tty_init(int vt);

#endif /* TTY_H_ */
