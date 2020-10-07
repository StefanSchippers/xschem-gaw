#ifndef DBUF_H
#define DBUF_H

/*
 * dbuf.c - dynamic buffer interface
 * 
 * include LICENSE
 */

#include <strmem.h>
#include <appclass.h>
#include <stdarg.h>

/*
 * a function called to refill the buffer
 */
typedef char * (*Dbuf_Fill_FP)( AppClass *caller, AppClass *app );

/* merged with fdbuf flags */
enum _DbufFlagsInfo {
   DB_STRIP_CR    = ( 1 << 0 ),   /* if set remove crlf  - \r\n -> \n */
   DB_JOIN_LINES  = ( 1 << 1 ),   /* if set join lines - \\n -> ' ' */
   DB_KEEP_LF     = ( 1 << 2 ),   /* if set join lines - \\n -> ' ' */
};

#define dbuf_check_size(x, y) if ( (y) >= (x)->size - 1) dbuf_renew( (x), (y) );

typedef struct _DBuf DBuf;

struct _DBuf {
   AppClass parent;
   char *s;           /* pointer to string */
   char *ext_buf;     /* pointer to external buffer */
   Dbuf_Fill_FP fill_func;     /* application call function to fill the buffer */
   AppClass *caller;           /* parameter to fill_func */
   int  pos;    /* use pos to get  - position on string */
   int  len;    /* use len to fill - string len         */
   int  size;   /* allocated size   */
   int  nRenew;   /* count re-new   */
   int flags;     /* flags for get_line */
   int lineno;    /* keep track of line number for get_line */
};

DBuf *dbuf_new( int size, char *ext_buf );
void dbuf_construct( DBuf *dbuf, int size, char *ext_buf);
void dbuf_destroy(void *dbuf);

void dbuf_set_fill_cb( DBuf *dbuf, Dbuf_Fill_FP fill_func, AppClass *caller);
void dbuf_renew( DBuf *dbuf, int newsize);

void dbuf_clear( DBuf *dest);
void dbuf_set_flags( DBuf *dest, int flags);
void dbuf_set_pos( DBuf *dest, int pos);
int dbuf_is_empty( DBuf *dest);
int dbuf_get_lineno( DBuf *dest);
int dbuf_get_available( DBuf *dest);
int dbuf_get_pos( DBuf *dest);
int dbuf_get_size( DBuf *dest);
void dbuf_set_len( DBuf *dest, int len);
int dbuf_get_len( DBuf *dest);
void dbuf_inc_len( DBuf *dest);
void dbuf_copy( DBuf *dest, DBuf *src);
void dbuf_ncopy( DBuf *dest, DBuf *src, int slen);
void dbuf_ncat( DBuf *dest, DBuf *src, int slen);
void dbuf_cat( DBuf *dest, DBuf *src);
void dbuf_strncpy( DBuf *dest, char *src, int slen);
void dbuf_strcpy( DBuf *dest, char *src);
void dbuf_strncat( DBuf *dest, char *src, int slen);
void dbuf_strcat( DBuf *dest, char *src);
void dbuf_concat( DBuf *dest, ...);
char *dbuf_next_char(DBuf *sbuf);
int dbuf_get_char(DBuf *sbuf);
char *dbuf_get_dupchars(DBuf *sbuf, int len);
int dbuf_unget_char(DBuf *sbuf);
int dbuf_unput_char(DBuf *sbuf);
void dbuf_put_char( DBuf *dest, char c);
char *dbuf_get_line(DBuf *dest, DBuf *src);
void dbuf_vprintf( DBuf *dest, const char *format, va_list args );
void dbuf_printf( DBuf *dest, const char *format, ...);
void dbuf_move_beg( DBuf *dbuf);

#endif /* DBUF_H */
