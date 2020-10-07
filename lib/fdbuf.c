/*
 * fdbuf.c - file dynamic buffer functions
 * 
 * include LICENSE
 */
#define _GNU_SOURCE      /* stdio.h fseeko */ 
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <fdbuf.h>
 
#ifdef TRACE_MEM
#include <tracemem.h>
#endif
        

/** \brief Allocates memory for a new FDBuf object. */

FDBuf *
fdbuf_new( char *filename, char *mode, int size)
{
   FDBuf *fdbuf;
   
   fdbuf = app_new0(FDBuf, 1 );
   fdbuf_construct( fdbuf, filename, mode, size);
   app_class_overload_destroy( (AppClass *) fdbuf, fdbuf_destroy );
   
   return fdbuf;
}

/** \brief Constructor for the FDBuf object. */

void
fdbuf_construct( FDBuf *fdbuf, char *filename, char *mode, int size)
{
   dbuf_construct( (DBuf *) fdbuf, size, NULL );
   fdbuf->filename = app_strdup(filename);
   fdbuf->flags = FDB_STRIP_CR;      /* remove cr by default */
   fdbuf->fd = fopen( filename, mode );
   if ( fdbuf->fd == NULL ) {
      msg_error(_("Can't open file '%s'"), filename );
      fdbuf->status = -1;
   }
}

/** \brief Destructor for the FDBuf object. */

void fdbuf_destroy( void *fdbuf)
{
   FDBuf *this = (FDBuf *) fdbuf;

   if ( fdbuf == NULL ) {
      return;
   }
   app_free(this->filename);
   if ( this->fd ) {
      fclose(  this->fd );
   }
   
   dbuf_destroy( fdbuf );
}

/*
 * remove cr in crlf
 */

void fdbuf_set_flags(FDBuf *dest, int flags)
{
   dest->flags = flags;
}

char *fdbuf_get_line(FDBuf *dest)
{
   int c;
   int lastc = 0;

   dbuf_clear((DBuf *) dest);
   while ((c = fgetc(dest->fd)) >= 0) {
      if ( c == '\n' && lastc == '\\' && (dest->flags & FDB_JOIN_LINES) ) {
	 dbuf_unput_char( (DBuf *) dest ); /* remove lastc */
	 c = ' ';
	 dest->lineno++;
      }
      if (c == '\n' || c == 0 ) {
         break;
      }
      dbuf_put_char( (DBuf *) dest,  c);
      lastc = c;
   }
   if ( (dest->flags & FDB_STRIP_CR) && c == '\n' && lastc == '\r' ) {
       dbuf_unput_char( (DBuf *) dest );
   }
   if ( c < 0 && dbuf_get_len((DBuf *) dest ) == 0) {
     return NULL;
   }
   dest->lineno++;
   return  ((DBuf *) dest)->s;
}

/*
 * read stream in DBuf at start
 */

int fdbuf_read(FDBuf *dest, int nbytes )
{
   int n;
   DBuf *dbuf = (DBuf *) dest;
   
   dbuf_check_size( dbuf, nbytes);

   n = fread( dbuf->s, 1, nbytes, dest->fd );

   dbuf->len = 0;
   if ( n > 0 ){
      dbuf->len = n;
   }
   *(dbuf->s + dbuf->len) = 0;
   return n;
}

void fdbuf_rewind( FDBuf *fdbuf)
{
   fdbuf->lineno = 0;
   if (fseeko(fdbuf->fd, 0L, SEEK_SET) < 0) {
      msg_fatal(_("Seek error '%s': %s"), fdbuf->filename, strerror(errno) );
   }
}

int fdbuf_fseeko( FDBuf *fdbuf, off_t offset, int whence)
{
   return fseeko(fdbuf->fd, offset, whence);
}

off_t fdbuf_tello( FDBuf *fdbuf)
{
   return ftello(fdbuf->fd);
}

int fdbuf_get_lineno( FDBuf *fdbuf)
{
   return fdbuf->lineno;
}

char *fdbuf_get_buffer( FDBuf *fdbuf )
{
   return ((DBuf *) fdbuf)->s;
}
