#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <core.hpp>

#include "tty.hpp"

#include <assert.h>

#if defined(__linux__)
#  include <linux/kd.h>
#  include <linux/major.h>
#  include <linux/vt.h>
#endif

#ifndef KDSKBMUTE
#  define KDSKBMUTE 0x4B51
#endif

/**
 * XXX: Use pam for session control, when no logind?
 */

static struct {
   struct {
      long kb_mode;
      int vt;
   } old_state;
   int tty, vt;
} fire = {
   .old_state = {0},
   .tty = -1,
   .vt = 0,
};

static int
find_vt(const char *vt_string, bool *out_replace_vt)
{
   assert(out_replace_vt);

   if (vt_string) {
      int vt;
      char *dummy;
      vt = std::strtod(vt_string, &dummy);

      //if () {
         *out_replace_vt = true;
         return vt;
      //} else {
      //   fire_log(WLC_LOG_WARN, "Invalid vt '%s', trying to find free vt", vt_string);
      //}
   }

   int tty0_fd;
   if ((tty0_fd = open("/dev/tty0", O_RDWR | O_CLOEXEC)) < 0) {
       std::cout << "Could not open /dev/tty0 to find free vt" << std::endl;
       std::exit(-1);
   }

   int vt;
   if (ioctl(tty0_fd, VT_OPENQRY, &vt) != 0) {
       std::cout << "Could not find free vt" << std::endl;
       std::exit(-1);
   }

   close(tty0_fd);
   return vt;
}

static int
open_tty(int vt)
{
   char tty_name[64];
   snprintf(tty_name, sizeof tty_name, "/dev/tty%d", vt);

   /* check if we are running on the desired vt */
   if (ttyname(STDIN_FILENO) && std::string(tty_name) == ttyname(STDIN_FILENO)) {
      return STDIN_FILENO;
   }

   int fd;
   if ((fd = open(tty_name, O_RDWR | O_NOCTTY | O_CLOEXEC)) < 0) {
       std::cout << "Could not open " << tty_name << std::endl;
       std::exit(-1);
   }

   return fd;
}

void die(std::string arg) {
    std::cout << arg << std::endl;
    std::exit(-1);
}

static bool
setup_tty(int fd, bool replace_vt)
{
   if (fd < 0)
      return false;

   struct stat st;
   if (fstat(fd, &st) == -1)
      die("Could not stat tty fd");

   fire.vt = minor(st.st_rdev);

   if (major(st.st_rdev) != TTY_MAJOR || fire.vt == 0)
      die("Not a valid vt");

   if (!replace_vt) {
      int kd_mode;
      if (ioctl(fd, KDGETMODE, &kd_mode) == -1)
         die("Could not get vt mode");

      if (kd_mode != KD_TEXT)
         die("vt is already in graphics mode . Is another display server running?");
   }

   struct vt_stat state;
   if (ioctl(fd, VT_GETSTATE, &state) == -1)
      die("Could not get current vt");

   fire.old_state.vt = state.v_active;

   if (ioctl(fd, VT_ACTIVATE, fire.vt) == -1)
      die("Could not activate vt");

   if (ioctl(fd, VT_WAITACTIVE, fire.vt) == -1)
      die("Could not wait for vt to become active");

   if (ioctl(fd, KDGKBMODE, &fire.old_state.kb_mode) == -1)
      die("Could not get keyboard mode");

   // vt will be restored from now on
   fire.tty = fd;

   if (ioctl(fd, KDSKBMUTE, 1) == -1 && ioctl(fd, KDSKBMODE, K_OFF) == -1) {
      fire_tty_terminate();
      die("Could not set keyboard mode to K_OFF");
   }

   if (ioctl(fd, KDSETMODE, KD_GRAPHICS) == -1) {
      fire_tty_terminate();
      die("Could not set console mode to KD_GRAPHICS");
   }

   struct vt_mode mode = {
      .mode = VT_PROCESS,
      .relsig = SIGUSR1,
      .acqsig = SIGUSR2
   };

   if (ioctl(fd, VT_SETMODE, &mode) == -1) {
      fire_tty_terminate();
      die("Could not set vt mode");
   }

   return true;
}

static void
sigusr_handler(int signal)
{
   switch (signal) {
      case SIGUSR1:
          core->set_active(false);
         break;
      case SIGUSR2:
         core->set_active(true);
         break;
   }
}

// Do not call this function directly!
// Use fire_fd_activate instead.
bool
fire_tty_activate(void)
{
   return (ioctl(fire.tty, VT_RELDISP, VT_ACKACQ) != -1);
}

// Do not call this function directly!
// Use fire_fd_deactivate instead.
bool
fire_tty_deactivate(void)
{
   return (ioctl(fire.tty, VT_RELDISP, 1) != -1);
}

// Do not call this function directly!
// Use fire_fd_activate_vt instead.
bool
fire_tty_activate_vt(int vt)
{
   if (fire.tty < 0 || vt == fire.vt)
      return false;

   return (ioctl(fire.tty, VT_ACTIVATE, vt) != -1);
}

int
fire_tty_get_vt(void)
{
   return fire.vt;
}

void
fire_tty_terminate(void)
{
   if (fire.tty >= 0) {
      // The ACTIVATE / WAITACTIVE may be potentially bad here.
      // However, we need to make sure the vt we initially opened is also active on cleanup.
      // We can't make sure this is synchronized due to unclean exits.
      if (ioctl(fire.tty, VT_ACTIVATE, fire.vt) != -1 && ioctl(fire.tty, VT_WAITACTIVE, fire.vt) != -1) {
          std::printf("Restoring vt %d (0x%lx) (fd %d)", fire.vt, fire.old_state.kb_mode, fire.tty);

         if (ioctl(fire.tty, KDSKBMUTE, 0) == -1 &&
             ioctl(fire.tty, KDSKBMODE, fire.old_state.kb_mode) == -1 &&
             ioctl(fire.tty, KDSKBMODE, K_UNICODE) == -1)
            std::printf("Failed to restore vt%d KDSKMODE", fire.vt);

         if (ioctl(fire.tty, KDSETMODE, KD_TEXT) == -1)
            std::printf("Failed to restore vt%d mode to VT_AUTO", fire.vt);

         struct vt_mode mode = { .mode = VT_AUTO };
         if (ioctl(fire.tty, VT_SETMODE, &mode) == -1)
            std::printf("Failed to restore vt%d mode to VT_AUTO", fire.vt);
      } else {
          std::printf("Failed to activate vt%d for restoration", fire.vt);
      }

      if (ioctl(fire.tty, VT_ACTIVATE, fire.old_state.vt) == -1)
         std::printf("Failed to switch back to vt%d", fire.old_state.vt);

      close(fire.tty);
   }

   memset(&fire, 0, sizeof(fire));
   fire.tty = -1;
}

void
fire_tty_init(int vt)
{
   if (fire.tty >= 0)
      return;

   // if vt was forced (logind) or if XDG_VTNR was valid vt (checked by find_vt)
   // we skip the text mode check for current vt.
   bool replace_vt = (vt > 0);

   if (!vt && !(vt = find_vt(getenv("XDG_VTNR"), &replace_vt)))
      die("Could not find vt");

   if (!setup_tty(open_tty(vt), replace_vt))
      die("Could not open tty with vt%d");

   struct sigaction action = {
      .sa_handler = sigusr_handler
   };

   sigaction(SIGUSR1, &action, NULL);
   sigaction(SIGUSR2, &action, NULL);
}
