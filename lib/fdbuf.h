#ifndef FILE_DBUF_H
#define FILE_DBUF_H

/*
 * fdbuf.c - file dynamic buffer interface
 * 
 * include LICENSE
 */
#define _GNU_SOURCE      /* off_t */
#include <sys/types.h>

#include <dbuf.h>

enum _FDbufFlagsInfo {
   FDB_STRIP_CR  = ( 1 << 0 ),     /* if set remove crlf  - \r\n -> \n */
   FDB_JOIN_LINES  = ( 1 << 1 ),   /* if set join lines - \\n -> ' ' */
};

typedef struct _FDBuf FDBuf;

struct _FDBuf {
   DBuf parent;
   char *filename;
   FILE *fd;     /* filedesc */
   int flags;      /* see _FDbufFlagsInfo */
   int lineno;     /* line number */
   int status;     /* op status */
};

FDBuf *fdbuf_new( char *filename, char *mode, int size);
void fdbuf_construct( FDBuf *fdbuf, char *filename, char *mode, int size);
void fdbuf_destroy(void *fdbuf);
char *fdbuf_get_line(FDBuf *dest);
void fdbuf_set_flags(FDBuf *dest, int flags);
void fdbuf_rewind( FDBuf *fdbuf);
int  fdbuf_get_lineno( FDBuf *fdbuf);
void fdbuf_set_lineno( FDBuf *fdbuf, int lineno);
int fdbuf_read(FDBuf *dest, int nbytes );
off_t fdbuf_tello( FDBuf *fdbuf);
int fdbuf_fseeko( FDBuf *fdbuf, off_t offset, int whence);
char *fdbuf_get_buffer( FDBuf *fdbuf );

#endif /* FILE_DBUF_H */
