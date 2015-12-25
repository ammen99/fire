#ifndef LOGIND_H_
#define LOGIND_H_

#ifdef YCM
#define HAS_LOGIND 1
#endif

#if HAS_LOGIND

/** Use wlc_fd_open instead, it automatically calls this if logind is used. */
int logind_open(const char *path, int flags);

/** Use wlc_fd_close instead, it automatically calls this if logind is used. */
void logind_close(int fd);

/** Check if logind is available. */
bool wlc_logind_available(void);

void logind_terminate(void);
int logind_init(const char *seat_id);

#else

/** For convenience. */
static inline bool
logind_available(void)
{
   return false;
}

#endif

#endif /* LOGIND_H_ */
