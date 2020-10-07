#ifndef SOCKCON_H
#define SOCKCON_H

/*
 * sockcon.h - socket type connection handling
 * 
 * include LICENSE
 */

#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdarg.h>
#include <netdb.h>

#include <fdsel.h>
#include <dbuf.h>

#define CON_RECVBUF_SIZE          8192

enum _ConNewFlagsInfo {
   CON_BIND     = (1 << 0),
   CON_CONNECT  = (1 << 1),
   CON_SENDTO   = (1 << 2),  /* use sendto */
   CON_REUSEAD  = (1 << 3),  /* call set socket option REUSEADDR */
   CON_SET_ADDR = (1 << 4),  /* copy dest addr in adstore */
   CON_ACQUIRE =  (1 << 5),  /* connection from inetd    */
};

enum _ConTypeInfo {
   CON_SERVER = 1 << 0,
      CON_CLIENT = 1 << 1,
      CON_TEMP = 1 << 2, /* temporary socket: before connect or accept */
};


/*
 * Can be used in flags
 */
enum _ConCheckInfo {
   CON_CHECK_READ = FDSEL_CHECK_READ,           /* is socket ready for read ? */
      CON_CHECK_WRITE = FDSEL_CHECK_WRITE,       /* is socket ready for write ? */
      CON_CHECK_TIMEOUT = FDSEL_CHECK_TIMEOUT,     /* set a timeout or not */
      CON_CHECK_UPDATE = 1 << 3,      /* request for update */

      CON_NOBLOCKING = 1 << 4,
      CON_BLOCKING = 1 << 5,
      CON_WCONNECT = 1 << 6,   /* wait connect to succeed */
      CON_BLOCKING_MSK = (CON_NOBLOCKING | CON_BLOCKING | CON_WCONNECT),
};

#define con_is_server(cnx) (cnx->type == CON_SERVER)

typedef struct _AddrStore AddrStore;

struct _AddrStore {
   struct sockaddr src_addr; /* source address */
   struct sockaddr dest_addr; /* destination address */
   socklen_t src_addr_len;            /* Length of sockaddr type */
} ;

typedef struct _SockCon SockCon;

typedef int (*NetRw_FP)( SockCon *cnx, char *buf, int len, int flags );
typedef int (*NetClose_FP)( SockCon *cnx, int shutdown );

struct _SockCon {
   FdescSelect parent;
   int s ;          /* connection socket */
   unsigned int port ;       /* port number     */
   int type ;       /* server or client */
   int status ;     /* set to -1 if connection cannot be opened */
   int blocking;    /* blocking mode or not */
   char *host_to_connect ; /* host to connect */
   char *full_name_host ; /* host to connect full name */
   int domain;        /* Protocol family */
   int sockType;      /* stream or dgram ... */
   int timeout;       /* connect, receive timeout */
   char *connected_to;
   char *connected_ip;
   struct sockaddr addr; /* address selected from getaddrinfo */
   socklen_t addr_len;           /* Length of sockaddr type */   
   DBuf   *recvbuf; /* receive buffer struct */
   void *udata;  /* for specific app user data associated with the connection */
   NetRw_FP rdfunc;      /* net read function */ 
   NetRw_FP wrfunc;      /* net write function */
   NetClose_FP closefunc; /* net close function */
   AddrStore *adstore;    /* connectionless addresses */
} ;

extern int sigalrm_seen;

#define MAX_LOOP 1
#define SIZE_OF_BUF  BUFSIZ * 2

#define CNX_VOIP  1


SockCon *con_new( char *host, int domain, int sockType,  int proto,
                  int port, int flags);
void con_construct( SockCon *cnx, char *host, int domain, int sockType, int proto,
                    int port, int flags);
void con_destroy(void *cnx);

struct addrinfo *con_getaddrinfo( char *host, int domain, int sockType, int proto, int port);

SockCon *con_acquire( int domain, int sockType, int proto, int checkflags );
SockCon *con_accept (SockCon *pser, int checkFlags);
void con_set_blocking(SockCon *p, int blocking);
int con_connect (SockCon *pcli);
int con_bind(SockCon *pcli);
int con_listen (SockCon *pser, int backog);
char *con_get_host_name(char *name);
char *con_get_host_addr(char *name);
int con_set_port(char *service, char *proto, int *port);
int con_is_ready(SockCon *p, int check); /*  is ready (ready for read, write) */
void con_set_timeout(SockCon *cnx, int timeout);
void con_update_dest_addr(SockCon *cnx);
int con_check_dest_addr(SockCon *cnx);

void con_SetLinger(int sockfd);
void con_SetKeepalive(int sockfd);
void con_SetTcpNoDelay(int sockfd, int on);
void con_SetPriority(int sockfd);
void con_SetSndbuf(int sockfd, int size);
void con_SetReuseaddr(int sockfd, int on);
int con_get_name_info(const struct sockaddr *sa, int flags, char **phost,
                       char **pservice );


int con_send (SockCon *cnx, char *buf, int len, int flags);
int con_dbuf_send (SockCon *cnx, DBuf *dbuf );
int con_fmt_send (SockCon *cnx, char *format, ...);
int con_recv (SockCon *cnx, char *buf, int len, int flags);
int con_dbuf_recv (SockCon *cnx);

#ifdef CON_DBUF_CB
char *con_dbuf_fill_cb(AppClass *xcnx, AppClass *xdbuf );
#endif /* CON_DBUF_CB */

#endif /* SOCKCON_H */

