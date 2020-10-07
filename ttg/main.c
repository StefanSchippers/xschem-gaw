/*
 * Test client for Talk to Gaw
 */
#define _GNU_SOURCE 1   /* netdb.h */

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <libgen.h>
#include <unistd.h>

#include <msglog.h>
#include <sockcon.h>

void server_loop(SockCon *cnx);
void client_loop(SockCon *cnx, FILE *ifd );

void usage(char *prog)
{
   fprintf(stderr, "%s  [options] [<host>]\n", prog);
   fprintf(stderr, "                : Test client for Talk to Gaw\n");
   fprintf(stderr, " [-h]           : print this message\n");
   fprintf(stderr, " [-s]           : enter in server mode\n");
   fprintf(stderr, " [-f file]      : client takes input from file instead stdin\n");
   fprintf(stderr, " [-p port]      : specify the port to connect or listen\n");
   fprintf(stderr, " [host]         : host to connect in client mode\n");
   
   exit(1);
}


int main(int argc, char **argv, char **envp)
{
   int i ;
   int server_mode = 0;
   char *host = NULL;
   char *service = NULL;
   char *filename = NULL;
   char *prog;
   int port = 1946;
   SockCon *cnx;
   FILE *ifd = stdin;
 
   prog = basename(argv[0]);
   msg_initlog(prog, MSG_F_NO_DATE, NULL, NULL );
   for (i = 1 ; i < argc ; i++) {
      if (*argv[i] == '-') {
         if (strcmp(argv[i], "-d") == 0) {
            prog_debug++ ;
         } else if (strcmp(argv[i], "-h") == 0) {
            usage(prog);
         } else if (strcmp(argv[i], "-s") == 0) {
            server_mode = 1 ;
	 } else if (strcmp(argv[i], "-p") == 0) {
            port = atoi(argv[++i]) ;
	 } else if (strcmp(argv[i], "-f") == 0) {
            filename = argv[++i] ;
	 } else if (strcmp(argv[i], "-ser") == 0) {
            service = argv[++i] ;
	 }
      } else {
         if ( host == 0) {
	    host = argv[i] ;
	 }
      }
   }
   if ( filename ) {
      ifd = fopen(filename, "r");
      if ( ! ifd ){ 
	 msg_fatal("Can't open '%s'", filename);
      }
   }
   if ( service ) {
      con_set_port(service, "tcp", &port);
   }
   
   cnx = con_new( host, PF_INET, SOCK_STREAM, IPPROTO_IP, port, 0 );
   if ( cnx->status < 0) {
      msg_errorl( 2, "Can't connect to server %s", host );
      con_destroy(cnx);
      return -1;
   }

   if ( server_mode ) {
      con_set_blocking(cnx, CON_NOBLOCKING);
      if ( con_listen(cnx, 5) < 0 ) {
	 return -1;
      }
      server_loop(cnx);
   } else {
      if ( ! host ){
	 usage(prog);
      }
      con_set_blocking(cnx, CON_BLOCKING);
      if ( con_connect (cnx) < 0 ) {
	 msg_error("connect");
	 return -1;
      }
      msg_info( "Connected: %s %s \n", cnx->connected_to, cnx->connected_ip);
      client_loop(cnx, ifd);
   }
	  
   /* let server to finish before closing */
   sleep(5);
   con_destroy(cnx);
   if ( filename ) {
      fclose(ifd);
   }
   return 0;
}

void run_command(char *line)
{
   fprintf( stdout, "%s\n", line);
}

void process_con(SockCon *cnx)
{
   char linebuf[BUFSIZ];
   char recbuf[BUFSIZ];
   int n;
   char *p;
   char *d = linebuf;
   
   for ( ; ; ) {
      if ( con_is_ready(cnx, CON_CHECK_READ | CON_CHECK_TIMEOUT) ){ 
	 n = recv(cnx->s, recbuf, sizeof(recbuf), MSG_DONTWAIT );
	 msg_dbg("%d", n);
	 if ( n < 0  ) {
	      if ( errno == EAGAIN ) {
		 continue;
	      }
	    return;
	 }
	 if ( n == 0  ) {
	    return;
	 }

	 for ( p = recbuf ; n > 0 ; n-- ) {
	    if ( *p == '\n' ) {
	       *d = 0 ;
	       run_command(linebuf);
	       d = linebuf;
	       p++;
	       continue;
	    }
	    *d++ = *p++;
	 }
	 if ( con_is_ready(cnx, CON_CHECK_WRITE) ){
	    char *done = "done";
	    n = send(cnx->s, done, sizeof(done), MSG_DONTWAIT );
	 }
      }
   }
}

void server_loop(SockCon *listen)
{
   SockCon *cnx;
   
   for ( ; ; ) {
      if ( con_is_ready(listen, CON_CHECK_READ | CON_CHECK_TIMEOUT) && 
	   (cnx = con_accept(listen, CON_CHECK_READ )) >= 0 ) {
	 msg_info( "Connected: %s %s \n", cnx->connected_to, cnx->connected_ip);
	 process_con(cnx);
	 msg_info( "Disconnected: %s %s \n", cnx->connected_to, cnx->connected_ip);
	 con_destroy(cnx);
      }
   }
}
 

void client_read(SockCon *cnx)
{
   char recbuf[BUFSIZ];
   int n;

   if ( con_is_ready(cnx, CON_CHECK_READ | CON_CHECK_TIMEOUT ) ){
      n = recv(cnx->s, recbuf, sizeof(recbuf), MSG_DONTWAIT );
      if ( n == 0  ) {
	 fprintf (stderr, "server closing\n");
	 return;
      }
      if ( n > 0 ){
	 char *s = recbuf;
	 int i;
	 int j = 0;
	 for (i = 0; i < n ; i++, s++ ){
	    if ( *s == '\n' ) {
	       continue;
	    }
	    fputc(*s, stderr );
	    j++;
	 }
	 if ( j ) {
	    fputc('\n', stderr );
	    fflush(stderr);
	 }
      }
   }
}

void client_loop(SockCon *cnx, FILE *fd)
{
   char linebuf[BUFSIZ];
   int done = 0;
   
   con_set_blocking(cnx, CON_NOBLOCKING);
   con_set_timeout(cnx, 0);
   for ( ; ! done ; ) {
      if ( con_is_ready(cnx, CON_CHECK_WRITE) ){
	 if ( fgets(linebuf, sizeof(linebuf), fd) == NULL ) {
	    done = 1;
	 } else {
	    con_send( cnx, linebuf, strlen(linebuf), 0);
	 }
      }
      client_read(cnx);
   }
   int i;
   con_set_timeout(cnx, 1);
   for ( i = 0 ; i < 10; i++ ){
      client_read(cnx);
   }
}
