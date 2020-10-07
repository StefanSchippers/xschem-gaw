/*
 * dbuf.c - dynamic buffer functions
 * 
 * include LICENSE
 */

#define _GNU_SOURCE      /* for vasprintf */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include <dbuf.h>
 
#ifdef TRACE_MEM
#include <tracemem.h>
#endif
        

/**
 * \brief Allocates memory for a new DBuf object.
 * size   : the initial size of the buffer
 * buffer : if not null, an external buffer that can be worked as a dbuf
 *          except reallocation. (Use for read only, for example)
 */

DBuf *
dbuf_new( int size, char *ext_buf )
{
   DBuf *dbuf;
   
   dbuf = app_new0(DBuf, 1 );
   dbuf_construct( dbuf, size, ext_buf );
   app_class_overload_destroy( (AppClass *) dbuf, dbuf_destroy );
   
   return dbuf;
}

/** \brief Constructor for the DBuf object. */

void dbuf_construct( DBuf *dbuf, int size, char *ext_buf )
{
   app_class_construct( (AppClass *) dbuf );

   if (size == 0 ){
      size = BUFSIZ;
   }
   if ( ext_buf ) {
      dbuf->ext_buf = dbuf->s = ext_buf;
      dbuf->len = size;
   } else {
      size = app_power_of_2(size);
      dbuf->s =  app_new(char, size );
      *(dbuf->s) = 0;
   }
   dbuf->size = size ;
}

/** \brief Destructor for the DBuf object. */

void dbuf_destroy( void *dbuf)
{
   DBuf *this = (DBuf *) dbuf;

   if ( dbuf == NULL ) {
      return;
   }
   if ( ! this->ext_buf ){
      app_free(this->s);
   }
   app_class_destroy( dbuf );
}

void dbuf_set_fill_cb( DBuf *dbuf, Dbuf_Fill_FP fill_func, AppClass *caller)
{
   dbuf->fill_func = fill_func;
   dbuf->caller = caller;
}

void dbuf_renew( DBuf *dbuf, int newsize)
{
   if ( dbuf->ext_buf ){
      msg_fatal("external buffer to allowed to reallocate");
   }
   dbuf->size = app_power_of_2(newsize + 1);
   dbuf->nRenew++;
   dbuf->s = app_renew( char, dbuf->s , dbuf->size);
}

void dbuf_set_flags( DBuf *dest, int flags)
{
   dest->flags = flags ;
}

int dbuf_is_empty( DBuf *dest)
{
   if ( dest->pos < dest->len ){
      return 0 ;
   }
   return 1 ;
}

int dbuf_get_available( DBuf *dest)
{
   return dest->len - dest->pos;
}

void dbuf_set_pos( DBuf *dest, int pos)
{
   dest->pos = pos ;
}

int dbuf_get_size( DBuf *dest)
{
   return dest->size ;
}

int dbuf_get_pos( DBuf *dest)
{
   return dest->pos ;
}

void dbuf_set_len( DBuf *dest, int len)
{
   dest->len = len ;
   if ( dest->len > dest->size - 1) {
      dbuf_renew( dest, dest->len );
   }
   char *p = dest->s + dest->len ;
   *p = 0;
}

int dbuf_get_len( DBuf *dest)
{
   return dest->len ;
}

void dbuf_inc_len( DBuf *dest)
{
   dbuf_set_len( dest, dest->len + 1);
}

void dbuf_clear( DBuf *dest)
{
   dest->pos = 0 ;
   dest->len = 0 ;
   dest->lineno = 0;
   *(dest->s) = 0;  
}

/*
 * concatenate n bytes from (src->s + pos) to dest
 */
void dbuf_ncat( DBuf *dest, DBuf *src, int slen)
{
   int len = dest->len ;
   
   dest->len += slen ;
   if ( dest->len >= dest->size - 1) {
      dbuf_renew( dest, dest->len );
   }
   memmove(dest->s + len, src->s + src->pos, slen );
   *(dest->s + dest->len) = 0;
   src->pos += slen;
}
/*
 * Warning : src start at src->s + src->pos and not at start of buffer
 */

void dbuf_cat( DBuf *dest, DBuf *src)
{
   int slen = src->len - src->pos;

   dbuf_ncat( dest, src, slen);
}

void dbuf_ncopy( DBuf *dest, DBuf *src, int slen)
{
   dest->pos = 0 ;
   dest->len = 0 ;
   dbuf_ncat( dest, src, slen);
}

void dbuf_copy( DBuf *dest, DBuf *src)
{
   dbuf_ncopy( dest, src, src->len);
}

void dbuf_strncpy( DBuf *dest, char *src, int slen)
{
   dest->pos = 0 ;
   dest->len = slen ;
   if ( dest->len >= dest->size - 1) {
      dbuf_renew( dest, dest->len );
   }
   memmove(dest->s, src,  slen);  
   *(dest->s + dest->len) = 0;
}

void dbuf_strcpy( DBuf *dest, char *src)
{
   dbuf_strncpy( dest, src, app_strlen(src));
}

void dbuf_strncat( DBuf *dest, char *src, int slen)
{
   int dlen = dest->len ;
   
   dest->len += slen ;
   if ( dest->len >= dest->size - 1) {
      dbuf_renew( dest, dest->len );
   }
   memmove(dest->s + dlen, src, slen );
   *(dest->s + dest->len) = 0;
}

void dbuf_strcat( DBuf *dest, char *src)
{
   dbuf_strncat( dest, src, app_strlen(src));
}

/*
 * concatanate all arguments of a NULL terminated list
 */

void dbuf_concat( DBuf *dest, ...)
{
   va_list args;
   const char *p;

   if ((p = ( const char *) dest) == NULL) {
      return ;
   }

   va_start (args, dest);
   p = va_arg (args, const char *);
   while (p != NULL) {
      dbuf_strcat( dest, (char *) p);
      p = va_arg (args, const char *);
   }
   va_end (args);
}


/*
 * return a pointer on the next char in buffer
 * instead of the char itself
 */
char *dbuf_next_char(DBuf *sbuf)
{
   
   if (sbuf->pos >= sbuf->len) {
      if ( sbuf->fill_func == NULL) {
         return NULL;
      } else {
         int lineno = sbuf->lineno;
         dbuf_clear(sbuf);
         if ( sbuf->fill_func(sbuf->caller, (AppClass *) sbuf) == NULL ) {
            return NULL;
         }
         sbuf->lineno = lineno;
      }
   }
   char *p = sbuf->s + sbuf->pos++;
   if (*p == '\n') {
      sbuf->lineno++;
   }
   return p;
}

 /*
 * return a single char from buffer
 *  or -1 if no more chars.
 */
int dbuf_get_char(DBuf *sbuf)
{
   char *p = dbuf_next_char(sbuf);
   if ( ! p ){
      return -1;
   }
   return (unsigned char) *p;
}

/*
 * return a duped binary string from dbuf of len chars
 *   string may countain 0.
 * Caller must free the returned ptr.
 */
char *dbuf_get_dupchars(DBuf *sbuf, int len)
{
   if (sbuf->pos + len >= sbuf->len) {
      return NULL;
   }
   char *ptr = app_new0( char, len + 1) ;
   memmove(ptr, sbuf->s + sbuf->pos, len);
   sbuf->pos += len;
   return ptr;
}

char *dbuf_get_line(DBuf *dest, DBuf *src)
{
   char *p;
   int c = 0;  /* compiler warning */
   int lastc = 0;
   
   dbuf_clear(dest);
   while ( (p = dbuf_next_char(src)) ) {
      c = *p;
      if ( c == '\n' && lastc == '\\' && (dest->flags & DB_JOIN_LINES) ) {
         dbuf_unput_char( dest ); /* remove lastc */
         c = ' ';
      }
      if (c == '\n' || c == 0 ) {
         break;
      }
      if ( c == '\r' && (dest->flags & DB_STRIP_CR) ) {
         continue;
      }
      dbuf_put_char( dest,  c);
      lastc = c;
   }
   if ( p == NULL && dest->len == 0) {
     return NULL;
   }
   if ( dest->flags & DB_KEEP_LF) {
      dbuf_put_char( dest,  c);
   }
   return dest->s;
}
/*
 * undo a get_char
 */
int dbuf_unget_char(DBuf *sbuf)
{
   if (--sbuf->pos < 0) {
      return -1;
   }
   char c = sbuf->s[sbuf->pos];
   return (unsigned char) c;
}

/*
 * undo a put_char
 */
int dbuf_unput_char(DBuf *sbuf)
{
   if (--sbuf->len < 0) {
      return -1;
   }
   char c = sbuf->s[sbuf->len];
   sbuf->s[sbuf->len] = 0;
   return (unsigned char) c;
}

void dbuf_put_char( DBuf *dest, char c)
{
   if ( dest->len >= dest->size - 2) {
      dbuf_renew( dest, dest->len + 1 );
   }
   char *p = dest->s + dest->len++ ;
   *p++ = c;
   *p = 0;
}

/*
 * dbuf_printf is like sprintf but to dbuf ; concatatenate the result
 */
void dbuf_vprintf( DBuf *dest, const char *format, va_list args )
{
   char *buffer = NULL;

   /* vasprintf returns malloc-allocated memory */
   int len = vasprintf(&buffer, format, args);

   if ( len < 0 ){
      msg_error("vasprintf returned -1.");
      return;
   }
   dbuf_strncat(dest, buffer, len);
   app_free (buffer);
}

void dbuf_printf( DBuf *dest, const char *format, ...)
{
   va_list args;

   va_start (args, format);
   dbuf_vprintf( dest, format, args );
   va_end (args);
}

/*
 * move remaining data at beginning of buffer
 */
void dbuf_move_beg( DBuf *dbuf)
{
   if ( dbuf->pos && dbuf->pos != dbuf->len ){
      dbuf->len -= dbuf->pos;
      memmove(dbuf->s, dbuf->s + dbuf->pos, dbuf->len );
      dbuf->pos = 0;
   }
}

int dbuf_get_lineno( DBuf *dbuf)
{
   return dbuf->lineno;
}
