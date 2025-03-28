/*
 * sockcon.c - socket type connection handling
 * 
 * include LICENSE
 */
#define _GNU_SOURCE 1   /* netdb.h */
#include <stdio.h>
#include <string.h>

#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/time.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/tcp.h>

#include <strmem.h>
#include <duprintf.h>
#include <sockcon.h>
 
#ifdef TRACE_MEM
#include <tracemem.h>
#endif
        

static int con_write (SockCon *cnx, char *buf, int len, int flags);
static int con_read (SockCon *cnx, char *buf, int len, int flags);
static int con_write_to (SockCon *cnx, char *buf, int len, int flags);
static int con_read_from (SockCon *cnx, char *buf, int len, int flags);


/*
 *** \brief Allocates memory for a new SockCon object.
 *
 * con_new - connection init
 */
SockCon *con_new( char *host, int domain, int sockType, int proto, int port, int flags)
{
   SockCon *cnx;
   
   cnx = app_new0( SockCon, 1);
   con_construct( cnx, host, domain, sockType, proto, port, flags);
   app_class_overload_destroy( (AppClass *) cnx, con_destroy );

   return cnx;
}
   
/** \brief Constructor for the SockCon object. */

void con_construct( SockCon *cnx, char *host, int domain, int sockType, int proto, int port, int flags)
{
   struct addrinfo *info;
   struct addrinfo *rp;
   int sfd;

   fdsel_construct( (FdescSelect *) cnx, 0, 0, 0 );

   cnx->host_to_connect = host;
   cnx->domain = domain;
   cnx->sockType = sockType;
   cnx->port = port;
   cnx->type = CON_TEMP;
   cnx->wrfunc = con_write;
   cnx->rdfunc = con_read;
   cnx->s = -1;

   if ( flags & CON_SENDTO  ){
      cnx->wrfunc = con_write_to;
      cnx->rdfunc = con_read_from;
      cnx->adstore = app_new0(AddrStore, 1);
   }
   cnx->recvbuf = dbuf_new( CON_RECVBUF_SIZE, 0 );
   if ( flags & CON_ACQUIRE ){
      return;
   }
      
   info = con_getaddrinfo( host, domain, sockType, proto, port);
   if (info == NULL) {
      cnx->status = -1;
      return ;
   }
   /* getaddrinfo() returns a list of address structures.*/
   for (rp = info ; rp != NULL ; rp = rp->ai_next ) {
      sfd = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
      if (sfd == -1) {
         continue;
      }
      fdsel_set_fd((FdescSelect *) cnx, sfd);
      cnx->s = sfd;
      memcpy(&cnx->addr, rp->ai_addr, rp->ai_addrlen);
      cnx->addr_len = rp->ai_addrlen;

#ifdef CON_OPT_REUSEADDR
      if ( flags & CON_REUSEAD ){
         con_SetReuseaddr(sfd, 1);
      }
#endif
      if (flags & CON_BIND){
         if (bind(sfd, rp->ai_addr, rp->ai_addrlen) == 0) {
            break;                  /* Success */
         }
      } else if (flags & CON_CONNECT ){
         if (con_connect(cnx) >= 0) {
            break;                  /* Success */
         }
      } else {
          break;                  /* socket ok */
      }
      close(sfd);
      cnx->s = -1;
   }

   if (rp == NULL) {               /* No address succeeded */
      if ( cnx->s < 0 ){
         msg_error(_("Can't open socket - err = %s"), strerror (errno) ) ;
      } else {
         msg_error(_("Can't bind - err = %s"), strerror (errno) ) ;
      }
      cnx->status = -1;
      return ;
   } else {

      if ( (flags & CON_SET_ADDR) && cnx->adstore ){
         memcpy(&cnx->adstore->dest_addr, rp->ai_addr, rp->ai_addrlen);
      }

      if ( port == 0 ){
         struct sockaddr addr;
         socklen_t addrlen = sizeof(addr);
         if ( getsockname(cnx->s, &addr, &addrlen) == 0) {
            cnx->port = ntohs( ((struct sockaddr_in *) &addr)->sin_port);
         }
      }
   }
   msg_dbgl(DBG_8, "init ok: socket %d, ai_family %d, port %d", 
            cnx->s, rp->ai_family, cnx->port );

   freeaddrinfo(info);           /* No longer needed */
}


/** \brief Destructor for the SockCon object. */

void con_destroy(void *cnx)
{
   SockCon *this = (SockCon *) cnx;

   if (cnx == NULL) {
      return;
   }
   con_set_blocking(this, CON_BLOCKING);
   if ( this->closefunc ) {
      this->closefunc ( cnx, 1 );
   }
   shutdown( this->s, 2);
   if ( this->s >= 0 && close(this->s) != 0 ){ 
      msg_warning(_("Can't close socket %d - %s"), this->s, strerror(errno) ) ;
   }
   app_free(this->adstore);
   app_free(this->full_name_host);
   app_free(this->connected_ip);
   app_free(this->connected_to);
   
   dbuf_destroy(this->recvbuf);
   fdsel_destroy( cnx );
}

/*
 * call to getaddrinfo
 */
struct addrinfo *con_getaddrinfo( char *host, int domain, int sockType, int proto, int port)
{
   struct addrinfo hints;
   struct addrinfo *info;
   int ret;
   char *port_str = NULL;

   memset(&hints, 0, sizeof(struct addrinfo));
   /* AF_* = PF_*, see bits/socket.h */
   hints.ai_family = domain;
   hints.ai_socktype = sockType;
   hints.ai_protocol = proto;
   
   if ( ! host ){
      /* server connection */
      hints.ai_flags |= AI_PASSIVE;    /* For wildcard IP address */
   } else {
       /* client connection */
   }
   if ( port ){
      port_str = app_strdup_printf("%d", port );
#ifndef NB4
      hints.ai_flags |= AI_NUMERICSERV;
#endif
   }
   if ( ! host && ! port ) {
      /* get a free port on this host */
      host = "localhost";
   }
   ret = getaddrinfo( host, port_str, &hints, &info );
   if (ret != 0) {
      msg_error("Can't get address info '%s' '%s' - err = %s",
                host, port_str, gai_strerror(ret) ) ;
      info = NULL;
   }
   app_free(port_str);
   return info;
}

/*
 * Set non blocking i/o.
 */
 
void con_set_blocking(SockCon *p, int blocking)
{
   p->blocking = blocking &  CON_BLOCKING_MSK;
   int ret = fcntl(p->s, F_GETFL ) ;
   ret &= ~O_NONBLOCK;
   if ( p->blocking & CON_BLOCKING ) {
      fcntl(p->s, F_SETFL, ret ) ;
   } else {
      fcntl(p->s, F_SETFL, ret | O_NONBLOCK ) ;
   }
}

void con_set_timeout(SockCon *cnx, int timeout)
{
   fdsel_set_timeout((FdescSelect *) cnx, timeout, 0);
   cnx->timeout = timeout;
}

/*
 * update usefull info about peers
 */
static void con_update(SockCon *cnx)
{
   struct sockaddr *sa = &cnx->addr;
   struct sockaddr_in *sai = (struct sockaddr_in *) sa;
   socklen_t l = sizeof( struct sockaddr );
   
   if ( cnx->type & CON_TEMP ) {
      return;
   }
   if ( getpeername( cnx->s, sa, &l ) == 0 ) {
      app_free(cnx->connected_ip);
      app_free(cnx->connected_to);
      cnx->connected_ip = app_strdup(inet_ntoa(sai->sin_addr));
      cnx->connected_to = con_get_host_name(cnx->connected_ip);
      msg_dbgl(DBG_8, "Connected: %s %s", cnx->connected_to, cnx->connected_ip);
   } else {
      msg_warning(_("Can't get peer name.- %s"), strerror(errno) );
   }
}

SockCon *con_accept(SockCon *pser, int checkFlags)
{
   int s ;
   SockCon *cnx;
 
   if ( (checkFlags & CON_CHECK_READ) && ! con_is_ready(pser, checkFlags ) ){
      return NULL;
   }
   s = accept(pser->s, (struct sockaddr *) &pser->addr, &pser->addr_len )  ;
   if ( s > 0 ) {
      cnx = app_new0(SockCon, 1);
      memcpy(cnx, pser, sizeof(SockCon));
      cnx->s = s;
      fdsel_set_fd((FdescSelect *) cnx, s);
      con_set_blocking( cnx, pser->blocking );
      cnx->type = CON_SERVER;
      con_is_ready(cnx, CON_CHECK_WRITE | CON_CHECK_UPDATE );
      app_class_ref( (AppClass *) cnx );
      cnx->recvbuf = dbuf_new( CON_RECVBUF_SIZE, 0 );

      msg_dbgl(DBG_8, "Accepted: %s %s", cnx->connected_to, cnx->connected_ip);
      return(cnx) ;
   } else {
     msg_error( "Can't accept socket - %s", strerror(errno) ) ;
   }
      
   return(NULL) ;
}
/*
 * get connection from inetd
 */
SockCon *con_acquire( int domain, int sockType, int proto, int checkflags )
{
   SockCon *cnx;
   
   cnx = con_new( NULL, domain, sockType, proto, 0, CON_ACQUIRE);
   cnx->s = dup(0); /* 0 in daemon mode */;
   fdsel_set_fd((FdescSelect *) cnx, cnx->s );
   close(0);
   con_set_blocking( cnx, checkflags);
   cnx->type = CON_SERVER;
   
   con_is_ready(cnx, CON_CHECK_WRITE | CON_CHECK_UPDATE);

   msg_dbgl(DBG_8, "Accepted: %s %s", cnx->connected_to, cnx->connected_ip);
   return cnx;
}


/*
 * client connection open
 */

int con_connect (SockCon *pcli)
{
   if ( connect(pcli->s, (struct sockaddr *) &pcli->addr, pcli->addr_len) < 0) {
      if ( errno != EINPROGRESS ) {
	 msg_warning(_("Can't connect socket - err = %d %s\n"), errno, strerror(errno) ) ;
	 return(-1) ;
      }
   }
   pcli->type = CON_CLIENT;
   
   if ( ! con_is_ready(pcli, CON_CHECK_WRITE | CON_CHECK_UPDATE ) ) {
      return -1;
   }
      
   return(pcli->s) ;
}

/*
 * bind address to socket
 */

int con_bind(SockCon *pcli)
{
   if (bind(pcli->s, (struct sockaddr *) &pcli->addr, pcli->addr_len) < 0) {
      msg_error("Can't bind socket - err = %d %s\n", errno, strerror(errno) ) ;
      return -1;
   }
   return 0;
}

int con_listen(SockCon *pser, int backlog )
{
   if ( listen(pser->s, backlog)  < 0 ) {
      msg_error( "Can't listen socket - %s", strerror(errno) ) ;
      return(-1) ;
   }
   return(pser->s) ;
}


/*
 * Check if connection is ready (ready to read, write)
 */
int con_is_ready(SockCon *cnx, int check)
{
   int ret = fdsel_is_ready((FdescSelect *) cnx, check);

   if ( ret > 0 ) {
      if ( (check & CON_CHECK_WRITE) && cnx->connected_ip == NULL ) { 
	 con_update(cnx);
      }
      return 1;
   }
   return 0;
}

/*
 * update udp address from received packet address
 */

void con_update_dest_addr(SockCon *cnx)
{
   memcpy(&cnx->adstore->dest_addr, &cnx->adstore->src_addr,
                   sizeof(struct sockaddr) );
}

/*
 * check udp address
 * return 0 if OK
 */
int con_check_dest_addr(SockCon *cnx)
{
   return memcmp(&cnx->adstore->dest_addr, &cnx->adstore->src_addr,
                    cnx->adstore->src_addr_len  );
}

		
#ifdef CON_OPT_LINGER
void con_SetLinger(int sockfd)
{
   struct linger li;
 
   li.l_onoff = 1;
   li.l_linger = 120;      /* 2 minutes, but system ignores field. */

   /*
    * Have the system make an effort to deliver any unsent data,
    * even after we close the connection.
    */
   if (setsockopt(sockfd, SOL_SOCKET, SO_LINGER, (char *) &li, (int) sizeof(li)) < 0)
      msg_warning(_("Linger mode could not be enabled."));
}       /* SetLinger */
#endif


#ifdef CON_OPT_KEEPALIVE
void con_SetKeepalive(int sockfd)
{
   int on = 1;

   if (setsockopt(sockfd, SOL_SOCKET, SO_KEEPALIVE, (char *) &on, (int) sizeof(on)) < 0)
      msg_warning(_("Keepalive mode could not be enabled."));
}       /* SetKeepalive */
#endif

#ifdef CON_OPT_TCP_NODELAY
void con_SetTcpNoDelay(int sockfd, int on)
{
   if (setsockopt(sockfd, IPPROTO_TCP, TCP_NODELAY, (char *) &on, (int) sizeof(on)) < 0)
      msg_warning(_("TcpNoDelay mode could not be enabled."));
}       /* TcpNoDelay */
#endif

#ifdef CON_OPT_PRIORITY
void con_SetPriority(int sockfd)
{
/* Mac OS X doesn't implement SO_PRIORITY.
 * Maybe this should be rather handled on the autoconf level with a test program and a macro in config.h.
 */
#ifdef SO_PRIORITY
   int on = 1;

   if (setsockopt(sockfd, SOL_SOCKET, SO_PRIORITY, (char *) &on, (int) sizeof(on)) < 0)
      msg_warning(_("Priority mode could not be enabled."));
#else /* SO_PRIORITY */
   msg_warning(_("Priority mode not supported by the OS."));
#endif /* SO_PRIORITY */
}       /* SetKeepalive */
#endif

#ifdef CON_OPT_SNDBUF
void con_SetSndbuf(int sockfd, int size)
{
   int on ;

   on = size;
   if (setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, (char *) &on, (int) sizeof(on)) < 0)
     msg_warning(_("SetSndbuf mode could not be enabled."));
}       /* SetSndbuf */
#endif

#ifdef CON_OPT_REUSEADDR
void con_SetReuseaddr(int sockfd, int on)
{
   if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *) &on, (int) sizeof(on)) < 0){
      msg_warning("SetReuseaddr mode could not be enabled. - %s", strerror(errno));
   }
}
#endif

#ifdef CON_OPT_GETNAMEINFO
/*
 * return name for a struct sockaddr - see man 3 getnameinfo
 * phost and pservice must be freed by caller
 */
int con_get_name_info(const struct sockaddr *sa, int flags, char **phost,
                       char **pservice )
{
   int ret; 
   char host[100];
   char service[100];
   
   ret = getnameinfo( sa, sizeof(struct sockaddr),
                host, sizeof(host),
                service, sizeof(service), flags );
   if ( ret ) {
      msg_error( "getnameinfo failed - %s", gai_strerror(ret) );
      return ret;
   }
   *phost = app_strdup(host);
   *pservice = app_strdup(service);
   return 0;
}
#endif


int con_set_port(char *service, char *proto, int *port)
{
   struct servent *sp ;
   
   if ( service && service[0] ) {
      if ((sp = getservbyname(service, proto)) == NULL) {
	 if ( *port == 0 ) {
	    msg_error( "Unkown service '%s'- port %d", service, *port) ;
	    return(-1) ;
	 }
      } else {
	 *port = (unsigned int) ntohs(sp->s_port);
      }
      return *port;
   }
   return -1;
}

/*
 * full qualified name
 * If name is null try to return local host
 */
char *con_get_host_name(char *name)
{
   struct hostent *hp;
   struct in_addr in_ad;
   char localHost[128];

   if ( name == NULL ) {
      gethostname(localHost, sizeof(localHost));
      name = localHost;
   }
   if ( name[0] == 0 ) {
       return NULL ;
   }
   if (inet_aton(name, &in_ad) == 0 ) {
      /* ad is not an ip address */
      hp = gethostbyname( name );
   } else {
      /* ad is an ip address */
      hp = gethostbyaddr(&in_ad, sizeof (struct in_addr), AF_INET);
   }
   if (hp == NULL) {
      msg_error("host information from name %s not found", name );
      return NULL;
   }
   return app_strdup(hp->h_name);
}

char *con_get_host_addr(char *name)
{
   struct hostent *hp;
   struct in_addr in_ad;

   if ( name == NULL || name[0] == 0 ) {
       return NULL ;
   }
   if (inet_aton(name, &in_ad) != 0 ) {
      /* ad is an ip address */
      return app_strdup(name) ;
   }

   hp = gethostbyname(name);
   if (hp == NULL) {
      msg_error( "host information for '%s' not found", name );
      return(NULL);
   }
   memcpy(&in_ad.s_addr, *(hp->h_addr_list), sizeof (struct in_addr));
   return app_strdup(inet_ntoa(in_ad));
}

static int con_write (SockCon *cnx, char *buf, int len, int flags)
{
   int ret;

   /* leave this stuff here, to be compatible with tls_read, tls_write  */
   msg_dbgl( DBG_8, "Sending: '%s' len %d", buf, len);
   ret = send( cnx->s, buf, len, flags );
   if ( ret < 0 ){
      msg_error( "Can't send - %s", strerror(errno) );
   }
   return ret;
}

static int con_write_to (SockCon *cnx, char *buf, int len, int flags)
{
   int ret;

   /* leave this stuff here, to be compatible with tls_read, tls_write  */
   msg_dbgl( DBG_8, "Sending: '%s' len %d", buf, len);

   ret = sendto( cnx->s, buf, len, flags,
                 (struct sockaddr *)  &cnx->adstore->dest_addr,
                  sizeof(cnx->adstore->dest_addr) );
   if ( ret < 0 ){
      msg_error("Can't send - %s", strerror(errno) );
   }
   return ret;
}

/*
 * send over the connection
 */

int con_send (SockCon *cnx, char *buf, int len, int flags)
{
   if ( len == 0 ) {
      len = strlen(buf);
   }
    return cnx->wrfunc ( cnx, buf, len, flags );
}

int con_dbuf_send (SockCon *cnx, DBuf *dbuf )
{
   int size = dbuf->len - dbuf->pos;
   int sent;

   sent = con_send (cnx, dbuf->s + dbuf->pos, size, 0);
   if ( sent > 0 ) {
      if ( sent < size ) {
         dbuf_set_pos( dbuf, dbuf->pos + sent );
      } else {
	 /* buffer completely sent */
	 dbuf_clear(dbuf);
      }
   }
   return sent;
}

/*
 * formatted send over the connection
 */

int con_fmt_send (SockCon *cnx, char *format, ...)
{
   va_list args;
   char buffer[BUFSIZ];
   
   va_start(args, format);
   vsnprintf(buffer, sizeof(buffer), format, args);
   va_end(args);

   return con_send( cnx, buffer, strlen(buffer), 0);
}

static int con_read (SockCon *cnx, char *buf, int len, int flags)
{
   int size;

   size = recv(cnx->s, buf, len, flags);

   /* leave this stuff here, to be compatible with tls_read, tls_write  */
   if ( size < 0 ){  
      if ( errno == EAGAIN ) {
	 return -2 ;
      }
      msg_error("Receive error - %s", strerror(errno) );
   }
   msg_dbgl( DBG_8, "Read: '%d' bytes", size);
   return size;
}

static int con_read_from (SockCon *cnx, char *buf, int len, int flags)
{
   int size;
   
   cnx->adstore->src_addr_len = sizeof(struct sockaddr);
   size = recvfrom(cnx->s, buf, len, flags,
                   (struct sockaddr *) &cnx->adstore->src_addr,
                   &cnx->adstore->src_addr_len);

   if ( size < 0 ){  
      if ( errno == EAGAIN ) {
	 return -2 ;
      }
      msg_error( "Receive error - %s", strerror(errno) );
   }
   msg_dbgl( DBG_8, "Read: '%d' bytes", size);
   return size;
}

int con_recv (SockCon *cnx, char *buf, int len, int flags)
{
   return  cnx->rdfunc(cnx, buf, len, flags);
}

int con_dbuf_recv (SockCon *cnx)
{
   int size;
   DBuf *dbuf = cnx->recvbuf; 

   if ( dbuf->len && dbuf->pos >= dbuf->len ){
      dbuf_clear( dbuf ); /* clear if buffer comsumed */
   }
   size = cnx->rdfunc(cnx, dbuf->s + dbuf->len, dbuf->size - dbuf->len - 1, 0);

   if ( size > 0 ){  
      dbuf_set_len( dbuf, dbuf->len + size  );
   }
   return size;
}

#ifdef CON_DBUF_CB
/*
 * callback to try to fill again the dbuf when buffer is empty
 * called from dbuf_get_char
 *
 * When an application is reading char from a stream with dbuf_get_char,
 * and the buffer becomes empty,
 * this callback is called without the need of the application itself
 */
char *con_dbuf_fill_cb(AppClass *xcnx, AppClass *xdbuf )
{
   SockCon *cnx = (SockCon *) xcnx;
   DBuf *dbuf = (DBuf *) xdbuf;
   
   /* see is there is a chunk to read */
   if ( con_is_ready(cnx, CON_CHECK_READ | CON_CHECK_TIMEOUT) ){
      if ( con_dbuf_recv (cnx) > 0 ){
         return dbuf->s;
      }
   }
   msg_info("Got timeout");
   /* timeout */
   return NULL;
}
#endif /* CON_DBUF_CB */
