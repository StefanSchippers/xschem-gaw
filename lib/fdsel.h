#ifndef FDSEL_H
#define FDSEL_H

/*
 * fdsel.h - file descriptor select object.
 * 
 * include LICENSE
 */

#include <appclass.h>

enum _FdselCheckInfo {
   FDSEL_CHECK_READ    = 1 << 0,     /* is socket ready for read ? */
   FDSEL_CHECK_WRITE   = 1 << 1,     /* is socket ready for write ? */
   FDSEL_CHECK_TIMEOUT = 1 << 2,     /* set a timeout or not */
};

typedef struct _FdescSelect FdescSelect;

struct _FdescSelect {
   AppClass parent;
   long    timeout_sec;         /* timeout seconds */
   long    timeout_usec;        /* timeout microseconds */
   int fd;                 /* file descriptor */
};

/*
 * prototypes
 */
FdescSelect *fdsel_new( int fd, long sec, long usec );
void fdsel_construct( FdescSelect *fds, int fd, long sec, long usec );
void fdsel_destroy(void *fds);

void fdsel_set_fd(FdescSelect *fds, int fd);
void fdsel_set_timeout(FdescSelect *fds, long sec, long usec);
int fdsel_is_ready(FdescSelect *fds, int check);

#endif /* FDSEL_H */
