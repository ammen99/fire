#include <dlfcn.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <linux/major.h>

#include <xf86drm.h>
#include <iostream>

#include "fd.hpp"
#include "tty.hpp"
#include "logind.hpp"
#include <core.hpp>

#ifndef EVIOCREVOKE
#  define EVIOCREVOKE _IOW('E', 0x91, int)
#endif

#define DRM_MAJOR 226

#define len(x) (sizeof(x) / sizeof(x[0]))

struct msg_request_fd_open {
   char path[32];
   int flags;
   enum fire_fd_type type;
};

struct msg_request_fd_close {
   dev_t st_dev;
   ino_t st_ino;
};

struct msg_request_activate_vt {
   int vt;
};

enum msg_type {
   TYPE_CHECK,
   TYPE_FD_OPEN,
   TYPE_FD_CLOSE,
   TYPE_ACTIVATE,
   TYPE_DEACTIVATE,
   TYPE_ACTIVATE_VT,
};

struct msg_request {
   enum msg_type type;
   union {
      struct msg_request_fd_open fd_open;
      struct msg_request_fd_close fd_close;
      struct msg_request_activate_vt vt_activate;
   };
};

struct msg_response {
   enum msg_type type;
   union {
      bool activate;
      bool deactivate;
   };
};

struct fire_fd {
      dev_t st_dev;
      ino_t st_ino;
      int fd;
      enum fire_fd_type type;
};

static struct {
   fire_fd fds[32];
   int socket;
   pid_t child;
   bool has_logind;
} fire;

static ssize_t
write_fd(int sock, int fd, const void *buffer, ssize_t buffer_size)
{
   char control[CMSG_SPACE(sizeof(int))];
   memset(control, 0, sizeof(control));
   struct msghdr message = {
      .msg_name = NULL,
      .msg_namelen = 0,
      .msg_iov = new iovec{
         .iov_base = (void*)buffer,
         .iov_len = (size_t)buffer_size
      },
      .msg_iovlen = 1,
      .msg_control = control,
      .msg_controllen = sizeof(control),
   };

   struct cmsghdr *cmsg = CMSG_FIRSTHDR(&message);
   if (fd >= 0) {
      cmsg->cmsg_len = CMSG_LEN(sizeof(fd));
      cmsg->cmsg_level = SOL_SOCKET;
      cmsg->cmsg_type = SCM_RIGHTS;
      memcpy(CMSG_DATA(cmsg), &fd, sizeof(fd));
   } else {
      cmsg->cmsg_len = CMSG_LEN(0);
   }

   return sendmsg(sock, &message, 0);
}

static ssize_t
recv_fd(int sock, int *out_fd, void *out_buffer, ssize_t buffer_size)
{
   assert(out_fd && out_buffer);
   *out_fd = -1;

   char control[CMSG_SPACE(sizeof(int))];
   struct msghdr message = {
      .msg_name = NULL,
      .msg_namelen = 0,
      .msg_iov = new iovec{
         .iov_base = out_buffer,
         .iov_len = (size_t)buffer_size
      },
      .msg_iovlen = 1,
      .msg_control = &control,
      .msg_controllen = sizeof(control)
   };
   struct cmsghdr *cmsg;

   errno = 0;
   ssize_t read;
   if ((read = recvmsg(sock, &message, 0)) < 0)
      return read;

   if (!(cmsg = CMSG_FIRSTHDR(&message)))
      return read;

   if (cmsg->cmsg_len == CMSG_LEN(sizeof(int))) {
      if (cmsg->cmsg_level != SOL_SOCKET || cmsg->cmsg_type != SCM_RIGHTS)
         return read;

      memcpy(out_fd, CMSG_DATA(cmsg), sizeof(int));
   } else if (cmsg->cmsg_len == CMSG_LEN(0)) {
      *out_fd = -1;
   }

   return read;
}

static int
fd_open(const char *path, int flags, enum fire_fd_type type)
{
   assert(path);

   fire_fd *pfd = NULL;
   for (uint32_t i = 0; i < len(fire.fds); ++i) {
      if (fire.fds[i].fd < 0) {
         pfd = &fire.fds[i];
         break;
      }
   }

   if (!pfd) {
       std::cout << "Maximum number of fds opened" << std::endl;
      return -1;
   }

   /* we will only open allowed paths */
#define FILTER(x, m) { x, (sizeof(x) > 32 ? 32 : sizeof(x)) - 1, m }
   static struct {
      const char *base;
      const size_t size;
      uint32_t major;
   } allow[FIRE_FD_LAST] = {
      FILTER("/dev/input/", INPUT_MAJOR), // WLC_FD_INPUT
      FILTER("/dev/dri/card", DRM_MAJOR), // WLC_FD_DRM
   };
#undef FILTER

   if (type > FIRE_FD_LAST || memcmp(path, allow[type].base, allow[type].size)) {
       std::cout << "Denying open from: " << path << std::endl;
      return -1;
   }

   struct stat st;
   if (stat(path, &st) < 0 || major(st.st_rdev) != allow[type].major)
      return -1;

   int fd;
   if ((fd = open(path, flags | O_CLOEXEC)) < 0)
      return fd;

   pfd->fd = fd;
   pfd->type = type;
   pfd->st_dev = st.st_dev;
   pfd->st_ino = st.st_ino;

   if (pfd->type == FIRE_FD_DRM)
      drmSetMaster(pfd->fd);

   if (pfd->fd < 0)
      std::cout <<  "Error opening (%m): " << path << std::endl;

   return pfd->fd;
}

static void
fd_close(dev_t st_dev, ino_t st_ino)
{
   struct fire_fd *pfd = NULL;
   for (uint32_t i = 0; i < len(fire.fds); ++i) {
      if (fire.fds[i].st_dev == st_dev && fire.fds[i].st_ino == st_ino) {
         pfd = &fire.fds[i];
         break;
      }
   }

   if (!pfd) {
       std::cout << "Tried to close fd that we did not open: (%zu, %zu)" << st_dev << " " << st_ino << std::endl;;
      return;
   }

   if (pfd->type == FIRE_FD_DRM)
      drmDropMaster(pfd->fd);

   close(pfd->fd);
   pfd->fd = -1;
}

static bool
activate(void)
{
   for (uint32_t i = 0; i < len(fire.fds); ++i) {
      if (fire.fds[i].fd < 0)
         continue;

      switch (fire.fds[i].type) {
         case FIRE_FD_DRM:
            if (drmSetMaster(fire.fds[i].fd)) {
                std::cout << "Could not set master for drm fd " << fire.fds[i].fd << std::endl;
               return false;
            }
            break;

         case FIRE_FD_INPUT:
         case FIRE_FD_LAST:
            break;
      }
   }

   return fire_tty_activate();
}

static bool
deactivate(void)
{
   // try drop drm fds first before we kill input
   for (uint32_t i = 0; i < len(fire.fds); ++i) {
      if (fire.fds[i].fd < 0 || fire.fds[i].type != FIRE_FD_DRM)
         continue;

      if (drmDropMaster(fire.fds[i].fd)) {
          std::cout << "Could not drop master for drm fd " << fire.fds[i].fd <<std::endl;
         return false;
      }
   }

   for (uint32_t i = 0; i < len(fire.fds); ++i) {
      if (fire.fds[i].fd < 0)
         continue;

      switch (fire.fds[i].type) {
         case FIRE_FD_INPUT:
            if (ioctl(fire.fds[i].fd, EVIOCREVOKE, 0) == -1) {
                std::cout << "Kernel does not support EVIOCREVOKE, can not revoke input devices" << std::endl;
               return false;
            }
            close(fire.fds[i].fd);
            fire.fds[i].fd = -1;
            break;

         case FIRE_FD_DRM:
         case FIRE_FD_LAST:
            break;
      }
   }

   return fire_tty_deactivate();
}

static void
handle_request(int sock, int fd, const struct msg_request *request)
{
   struct msg_response response;
   memset(&response, 0, sizeof(response));
   response.type = request->type;

   switch (request->type) {
      case TYPE_CHECK:
         write_fd(sock, fd, &response, sizeof(response));
         break;
      case TYPE_FD_OPEN:
         fd = fd_open(request->fd_open.path, request->fd_open.flags, request->fd_open.type);
         write_fd(sock, fd, &response, sizeof(response));
         break;
      case TYPE_FD_CLOSE:
         /* we will only close file descriptors opened by us. */
         fd_close(request->fd_close.st_dev, request->fd_close.st_ino);
         break;
      case TYPE_ACTIVATE:
         response.activate = activate();
         write_fd(sock, fd, &response, sizeof(response));
         break;
      case TYPE_DEACTIVATE:
         response.deactivate = deactivate();
         write_fd(sock, fd, &response, sizeof(response));
         break;
      case TYPE_ACTIVATE_VT:
         response.activate = fire_tty_activate_vt(request->vt_activate.vt);
         write_fd(sock, fd, &response, sizeof(response));
   }
}

static void
communicate(int sock, pid_t parent)
{
   memset(fire.fds, -1, sizeof(fire.fds));

   do {
      int fd = -1;
      struct msg_request request;
      while (recv_fd(sock, &fd, &request, sizeof(request)) == sizeof(request))
         handle_request(sock, fd, &request);
   } while (kill(parent, 0) == 0);

   // Close all open fds
   for (uint32_t i = 0; i < len(fire.fds); ++i) {
      if (fire.fds[i].fd < 0)
         continue;

      if (fire.fds[i].type == FIRE_FD_DRM)
         drmDropMaster(fire.fds[i].fd);

      close(fire.fds[i].fd);
   }

   std::cout << "Parent exit %u" << parent << std::endl;
   core->exit();
}

static bool
read_response(int sock, int *out_fd, struct msg_response *response, enum msg_type expected_type)
{
   if (out_fd)
      *out_fd = -1;

   memset(response, 0, sizeof(struct msg_response));

   fd_set set;
   FD_ZERO(&set);
   FD_SET(sock, &set);

   struct timeval timeout;
   memset(&timeout, 0, sizeof(timeout));
   timeout.tv_sec = 1;

   if (select(sock + 1, &set, NULL, NULL, &timeout) != 1)
      return false;

   int fd = -1;
   ssize_t ret = 0;
   do {
      ret = recv_fd(sock, &fd, response, sizeof(struct msg_response));
   } while (ret < 0 && (errno == EAGAIN || errno == EWOULDBLOCK));

   if (out_fd)
      *out_fd = fd;

   return (ret == sizeof(struct msg_response) && response->type == expected_type);
}

static void
write_or_die(int sock, int fd, const void *buffer, ssize_t size)
{
   ssize_t wrt;
   if ((wrt = write_fd(sock, fd, buffer, size)) != size) {
       std::cout << "Failed to write bytes to socket (wrote )" <<  size <<  wrt << std::endl;
   }
}

static bool
check_socket(int sock)
{
   struct msg_request request;
   memset(&request, 0, sizeof(request));
   write_or_die(sock, -1, &request, sizeof(request));
   struct msg_response response;
   return read_response(sock, NULL, &response, TYPE_CHECK);
}



int
fire_fd_open(const char *path, int flags, enum fire_fd_type type)
{

#ifdef HAS_LOGIND
   if (fire.has_logind)
      return logind_open(path, flags);
#endif

   struct msg_request request;
   memset(&request, 0, sizeof(request));
   request.type = TYPE_FD_OPEN;
   strncpy(request.fd_open.path, path, sizeof(request.fd_open.path));
   request.fd_open.flags = flags;
   request.fd_open.type = type;
   write_or_die(fire.socket, -1, &request, sizeof(request));

   int fd = -1;
   struct msg_response response;
   if (!read_response(fire.socket, &fd, &response, TYPE_FD_OPEN))
      return -1;

   return fd;
}

void
fire_fd_close(int fd)
{
#ifdef HAS_LOGIND
   if (fire.has_logind) {
      logind_close(fd);
      goto close;
   }
#endif

   struct stat st;
   if (fstat(fd, &st) == 0) {
      struct msg_request request;
      memset(&request, 0, sizeof(request));
      request.type = TYPE_FD_CLOSE;
      request.fd_close.st_dev = st.st_dev;
      request.fd_close.st_ino = st.st_ino;
      write_or_die(fire.socket, -1, &request, sizeof(request));
   }

#ifdef HAS_LOGIND
close:
#endif
   close(fd);
}

bool
fire_fd_activate(void)
{
#ifdef HAS_LOGIND
   if (fire.has_logind)
      return fire_tty_activate();
#endif

   struct msg_response response;
   struct msg_request request;
   memset(&request, 0, sizeof(request));
   request.type = TYPE_ACTIVATE;
   write_or_die(fire.socket, -1, &request, sizeof(request));
   return read_response(fire.socket, NULL, &response, TYPE_ACTIVATE) && response.activate;
}

bool
fire_fd_deactivate(void)
{
#ifdef HAS_LOGIND
   if (fire.has_logind)
      return fire_tty_deactivate();
#endif

   struct msg_response response;
   struct msg_request request;
   memset(&request, 0, sizeof(request));
   request.type = TYPE_DEACTIVATE;
   write_or_die(fire.socket, -1, &request, sizeof(request));
   return read_response(fire.socket, NULL, &response, TYPE_DEACTIVATE) && response.deactivate;
}

bool
fire_fd_activate_vt(int vt)
{
#ifdef HAS_LOGIND
   if (fire.has_logind)
      return fire_tty_activate_vt(vt);
#endif

   struct msg_response response;
   struct msg_request request;
   memset(&request, 0, sizeof(request));
   request.type = TYPE_ACTIVATE_VT;
   request.vt_activate.vt = vt;
   write_or_die(fire.socket, -1, &request, sizeof(request));
   return read_response(fire.socket, NULL, &response, TYPE_ACTIVATE_VT) && response.activate;
}

void
fire_fd_terminate(void)
{
   if (fire.child >= 0) {
      if (fire.has_logind) {
         // XXX: We need fd passer to cleanup sessionless tty
         kill(fire.child, SIGTERM);
      }

      fire.child = 0;

   }
}

static void
signal_handler(int signal)
{
   if (fire.has_logind && (signal == SIGTERM || signal == SIGINT))
      _exit(EXIT_SUCCESS);

   // For non-logind no-op all catched signals
   // we might need to restore tty
}

void
fire_fd_init(int argc, char *argv[], bool has_logind)
{
   fire.has_logind = has_logind;

   int sock[2];
   if (socketpair(AF_LOCAL, SOCK_SEQPACKET | SOCK_CLOEXEC, 0, sock) != 0)  {
       std::cout <<"Failed to create fd passing unix domain socket pair: %m" << std::endl;
       std::exit(-1);
   }

   if ((fire.child = fork()) == 0) {
      close(sock[0]);

      if (clearenv() != 0) {
          std::cout << "Failed to clear environment" << std::endl;
          std::exit(-1);
      }

      struct sigaction action = {
         .sa_handler = signal_handler
      };

      sigaction(SIGUSR1, &action, NULL);
      sigaction(SIGUSR2, &action, NULL);
      sigaction(SIGINT, &action, NULL);
      sigaction(SIGTERM, &action, NULL);

      for (int i = 0; i < argc; ++i)
         strncpy(argv[i], (i == 0 ? "fire" : ""), strlen(argv[i]));

      communicate(sock[1], getppid());
      _exit(EXIT_SUCCESS);
   } else if (fire.child < 0) {
       std::cout << "Fork failed" <<std::endl;
       std::exit(-1);
   } else {
      close(sock[1]);

      if (getuid() != geteuid() || getgid() != getegid())
         std::cout << "Work done, dropping permissions and checking communication" << std::endl;

      if (setgid(getgid()) != 0 || setuid(getuid()) != 0)
         std::cout << "Could not drop permissions: %m" << std::endl;

      if (kill(fire.child, 0) != 0) {
          std::cout << "Child process died" << std::endl;
          std::exit(-1);
      }

      if (!check_socket(sock[0])) {
          std::cout << "Communication between parent and child process seems to be broken" << std::endl;
          std::exit(-1);
      }

      fire.socket = sock[0];
   }
}
