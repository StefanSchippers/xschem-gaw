/*
 * fsutil.c - file utility functions
 * 
 * include LICENSE
 */

#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <fileutil.h>
 
#ifdef TRACE_MEM
#include <tracemem.h>
#endif
        


int file_exists (const char *filename)
{
  struct stat statbuf;

  return stat (filename, &statbuf) == 0;
}

/*
 * return size of the file
 *
 * for an open file use this, it is faster !
 *   size = lseek(fd, 0L, SEEK_END);
 *   lseek(fd, 0L, SEEK_SET);
 */
off_t file_size (const char *filename)
{
   struct stat statbuf;

   if ( stat (filename, &statbuf) == 0 ) {
      return statbuf.st_size;
   }
   return -1;
}

