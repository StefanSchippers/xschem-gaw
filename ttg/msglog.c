/**
 *  \file msglog.c
 *  \brief Functions for printing messages, warnings and errors.
 *
 *  This module provides output printing facilities.
 * 
 * include LICENSE
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <signal.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

#include <msglog.h>
#include <duprintf.h>

#ifdef TRACE_MEM
#include <tracemem.h>
#endif

#define MSG_FMT_DATE \
"%2d %3s %4d  %2d:%02d:%02d",  \
ptm->tm_mday, msg_month_of_year[ptm->tm_mon], (1900 + ptm->tm_year), \
ptm->tm_hour, ptm->tm_min, ptm->tm_sec

const char *msg_month_of_year[] = {
   "Jan", "Feb", "Mar", "Apr", "May", "Jun",
   "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
};

const char *msg_day_of_week[] = {
   "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
};

const char *msg_typename[] = {
   "DEBUG",
   "INFO",
   "WARNING",
   "ERROR",
   "FATAL",
};

/* see man console_codes */
#define COL(x)  "\033[" #x ";1m"
#define RED     COL(31)
#define GREEN   COL(32)
#define YELLOW  COL(33)
#define BLUE    COL(34)
#define MAGENTA COL(35)
#define CYAN    COL(36)
#define WHITE   COL(37)
#define GRAY    "\033[0m"	/* end of color */

const char *msg_color[] = {
   "",				/* Debug */
   "",				/* info  */
   GREEN,			/* Warning */
   RED,				/* Error */
   BLUE,			/* Fatal */
};

int prog_debug;  /* global turn debug on */
MsgLogData *msglogp; /* global pointeur to MsgLogData in main */
/*
 * struct _MsgLogData moved from msglog.h
 * Let use different msglog.c without changing msglog.h
 */
struct _MsgLogData {
   FILE *msg_fd;
   FILE *dbg_fd;   /* debug message file desc */
   char *msg_progname;
   int msg_flags;
   int msg_verbose_level;
   int msg_dbg_msk;
//   int msg_dump_msk;
   char *msg_filename;
   int log_facility;            /* storage for facility used in syslog */
   int logger_is_open;          /* set if openlog has been called */
   MsgFunc msg_displayFunc;
};

/*
 * if given filename store message in file
 * if given displayFunc call func to display messge
 *   except for debug message
 */
void msg_set_func(MsgFunc displayFunc)
{
   msglogp->msg_displayFunc = displayFunc;
   if (displayFunc) {
      msglogp->msg_flags &= ~MSG_F_TYPE;
   }
}

void msg_set_facility(int facility)
{
   msglogp->log_facility = facility;
}
int msg_get_facility(void)
{
   return msglogp->log_facility;
}

void msg_set_logger_is_open(int is_open)
{
   msglogp->logger_is_open = is_open;
}
int msg_get_logger_is_open(void)
{
   return msglogp->logger_is_open;
}

void msg_initlog(char *name, int flags, char *filename,
		 MsgFunc displayFunc)
{
   if (!msglogp) {
      msglogp = app_new0(MsgLogData, 1);
   }
   msglogp->msg_progname = name;
   msglogp->msg_flags = flags;
   msglogp->msg_verbose_level = 1;
   msglogp->msg_dbg_msk = 1;
   msg_set_func(displayFunc);
   msglogp->msg_fd = stderr;
   msglogp->dbg_fd = stderr;
   if (filename) {
      char *omode = "a";

      if (flags & MSG_F_OVERWRITE) {
	 omode = "w";
      }
      msg_openlog(filename, omode);
   }
   atexit(msg_closelog);
}

FILE * msg_get_msg_fd(void)
{
   return msglogp->msg_fd;
}

void msg_set_dbg_fd(FILE * msg_fd )
{
   msglogp->dbg_fd = msg_fd;
}

void msg_atexit(void)
{
   if (msglogp) {
      if (msglogp->msg_flags & MSG_F_SIGNAL) {
	 kill(getppid(), SIGQUIT);
      }
      msg_closelog();
      app_free(msglogp);
      msglogp = NULL;
   }
}

void msg_set_flags(int flags)
{
   msglogp->msg_flags = flags;
}

void msg_set_level(int level)
{
   msglogp->msg_verbose_level = level;
}

int msg_get_level(void)
{
   return msglogp->msg_verbose_level;
}

#ifdef MSG_DEBUG
void private_msg_set_dbg_msk(int msk)
{
   msglogp->msg_dbg_msk = msk;
}

int  private_msg_get_dbg_msk(void)
{
   return msglogp->msg_dbg_msk;
}

void private_msg_get_dbg_msk_str(char *msg)
{
   unsigned int i;
   int k = 0;
   
   int msk = msglogp->msg_dbg_msk;
   for ( i = 0 ; i < 8 * sizeof(int) ; i++ ){
      if ( msk & (1 << i) ){
         if ( k ) {
            msg[k] = ',';
            k++;
         }
         sprintf(&msg[k], "%d", i);
         k++;
         if ( i > 9) {
            k++;
         }
      }
   }
   msg[k] = 0;
}

unsigned int private_msg_set_dbg_msk_str(char *p)
{
   unsigned int ui = 1;
   int i;

   if ( *p == 0 ){
      prog_debug = 0;
      goto finish;
   }
   while (1) {
      i = strtoul(p, &p, 10);
      ui |= (1 << i);
      if (*p == 0) {
	 break;
      }
      p++;
   }
   prog_debug = 1;
finish:
   msglogp->msg_dbg_msk = ui;
   return ui;
}
#endif

#ifdef MSG_DUMP
void private_msg_dump(int level, char *buf, int len, int start, FILE *fd,
                      char *fmt, ...)
{
   int i;
   int n;
   char msgbuf[50];
   char lastline[50];
   char cbuf[18];
   int dup = 0;
   int print_offset = 1;
   va_list ap;

   if ((level & msglogp->msg_dbg_msk) == 0) {
      return;
   }
   if ( fd == NULL ){
      fd = msglogp->msg_fd;
   }
   if (fmt) {
      char *ffmt;
      va_start(ap, fmt);
      ffmt = app_strdup_vprintf(fmt, ap);
      va_end(ap);
      fprintf(fd, "%s: %d\n", ffmt, len );
      app_free(ffmt);
   }
   if ( start == -1 ){
      print_offset = 0;
   }
   lastline[0] = 0;
   while (len > 0) {
      int k = 0;
      n = 16;
      if (len < n) {
	 n = len;
      }
      for (i = 0; i < n; i++) {
	 sprintf(&msgbuf[k], "%02X ", (unsigned char) *buf);
         k +=  3;
         if ( i == 7){
            msgbuf[k] =' ';
            k++;
         }
	 if (isprint(*buf & 0x7F)) {
	    cbuf[i] = *buf & 0x7F;
	 } else {
	    cbuf[i] = '.';
	 }
	 buf++;
      }
      cbuf[i] = 0;
      msgbuf[k] = 0;
      len -= i;
      if (strcmp(msgbuf, lastline) == 0 && len > 0) {
         dup++;
      } else {
         if (dup > 0) {
            fprintf(fd, "  -- prev line repeats %d times --\n", dup);
         }
         dup = 0;
         strcpy(lastline, msgbuf);
         if ( print_offset ){
            fprintf(fd, "%8X: ", (unsigned int) start);
         }
         fprintf(fd, "%-49s  %s\n", msgbuf, cbuf);
      }
      start += n;
   }
}
#endif

/*
 * msg_openlog :
 * send log messages to a file if needed
 */

FILE *msg_openlog(char *filename, char *mode)
{
   msglogp->msg_filename = filename;
   if ((msglogp->msg_fd = fopen(filename, mode)) == NULL) {
      msglogp->msg_fd = stderr; /* just to get the next mesg */
      msg_fatal(_("Can't open file '%s': %s"), filename, strerror(errno));
   }
   if ( prog_debug ){
      msglogp->dbg_fd = msglogp->msg_fd;
   }
   return msglogp->msg_fd;
}

void msg_closelog(void)
{
   if (msglogp->msg_fd != stderr) {
      fclose(msglogp->msg_fd);
      msglogp->msg_fd = stderr;
   }
}

void msg_str_time_date(char *strdate);

void msg_str_time_date(char *strdate)
{
   time_t now;
   struct tm *ptm;

   time(&now);
   ptm = localtime(&now);
   sprintf(strdate, MSG_FMT_DATE);
}

void private_message(int level, int msgtype, const char *func, char *file,
		     int line, char *fmt, ...)
{
   va_list ap;
   char *ffmt;
   char date[128];
   char text[128];
   char *p;
   char *color_s = "";
   char *color_e = "";

   if (msgtype == MSG_T_DEBUG) {
      if ((msglogp->msg_dbg_msk & level) == 0) {
	 return;
      }
   } else if (level > msglogp->msg_verbose_level) {
      return;
   }
   date[0] = 0;
   if (msglogp->msg_flags & MSG_F_DATE) {
      msg_str_time_date(date);
      strcat(date, ": ");
   }
   if (msglogp->msg_flags & MSG_F_PROG) {
      if (msglogp->msg_progname && *msglogp->msg_progname) {
	 p = date + strlen(date);
	 snprintf(p, sizeof(date) - strlen(date), "%s: ",
		  msglogp->msg_progname);
      }
   }

   text[0] = 0;
   if (msgtype == MSG_T_DEBUG || msglogp->msg_flags & MSG_F_TYPE) {
      p = text + strlen(text);
      snprintf(p, sizeof(text) - strlen(text), "%s: ",
	       msg_typename[msgtype]);
   }
   if (msglogp->msg_flags & MSG_F_FUNC) {
      p = text + strlen(text);
      snprintf(p, sizeof(text) - strlen(text), "%s: ", func);
   }
   if (msglogp->msg_flags & MSG_F_FILE) {
      p = text + strlen(text);
      snprintf(p, sizeof(text) - strlen(text), "%s:%d: ", file, line);
   }
   if (msglogp->msg_flags & MSG_F_COLOR) {
      color_s = (char *) msg_color[msgtype];
      if (msgtype > MSG_T_INFO) {
	 color_e = GRAY;
      }
   }

   va_start(ap, fmt);
   ffmt = app_strdup_vprintf(fmt, ap);
   va_end(ap);

   if (msglogp->msg_flags & MSG_F_FOLD) {
      if (strlen(date) + strlen(text) + strlen(ffmt) > 80) {
	 strcat(text, "\n    ");
      }
   }

   if (msgtype == MSG_T_DEBUG) {
      fprintf(msglogp->dbg_fd, "%s%s%s\n", date, text, ffmt);
      fflush(msglogp->dbg_fd);
   } else if (msglogp->msg_displayFunc) {
      char message[1024];

      snprintf(message, sizeof(message), "%s%s%s\n", date, text, ffmt);
      (msglogp->msg_displayFunc) (msgtype, message);
   } else {
      fprintf(msglogp->msg_fd, "%s%s%s%s%s\n", color_s, date, text, ffmt,
	      color_e);
      fflush(msglogp->msg_fd);
   }
   app_free(ffmt);
}
