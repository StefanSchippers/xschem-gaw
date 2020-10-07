/*
 * fdsel.c - file descriptor select object. Use this if you don't have a loop
 * 
 * include LICENSE
 */
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/select.h>

#include <fdsel.h>

#ifdef TRACE_MEM
#include <tracemem.h>
#endif

/*
 *** \brief Allocates memory for a new FdescSelect object.
 */

FdescSelect *fdsel_new( int fd, long sec, long usec )
{
   FdescSelect *fds;

   fds =  app_new0(FdescSelect, 1);
   fdsel_construct( fds, fd,  sec, usec );
   app_class_overload_destroy( (AppClass *) fds, fdsel_destroy );
   return fds;
}

/** \brief Constructor for the FdescSelect object. */

void fdsel_construct( FdescSelect *fds, int fd, long sec, long usec)
{
   app_class_construct( (AppClass *) fds );

   fds->fd = fd;
   fdsel_set_timeout(fds, sec, usec);
}

/** \brief Destructor for the FdescSelect object. */

void fdsel_destroy(void *fds)
{
//   FdescSelect *this = (FdescSelect *) fds;

   if (fds == NULL) {
      return;
   }

   app_class_destroy( fds );
}

void fdsel_set_fd(FdescSelect *fds, int fd)
{
   fds->fd = fd;
}

void fdsel_set_timeout(FdescSelect *fds, long sec, long usec)
{
   fds->timeout_sec = sec;
   fds->timeout_usec = usec;
}

int fdsel_is_ready(FdescSelect *fds, int check)
{   
   int i = 0;
   fd_set fdset ;
   int nfds;
   struct timeval sel_timeout ;

   sel_timeout.tv_sec = 0 ;
   sel_timeout.tv_usec = 0 ;
   if ( check & FDSEL_CHECK_TIMEOUT ) {
      sel_timeout.tv_sec = fds->timeout_sec ;
      sel_timeout.tv_usec = fds->timeout_usec ;
   }

      
   FD_ZERO(&fdset) ;
   FD_SET(fds->fd, &fdset) ;
   nfds = fds->fd + 1;

   if ( check & FDSEL_CHECK_READ) {
      i = select(nfds, &fdset, NULL, NULL, &sel_timeout ) ;
   } else if ( check & FDSEL_CHECK_WRITE) {
      i = select(nfds, NULL, &fdset, NULL, &sel_timeout ) ;
   }
   if ( i == -1 ) {
      msg_error(_("select error '%s'"), strerror(errno) ) ;
   } else if ( i > 0 ) {
      return 1;
   }
   return 0;
}
