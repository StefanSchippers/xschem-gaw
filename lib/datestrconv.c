/*
 * datestrconv.c - convert string to time and time to string
 * 
 * include LICENSE
 *
 */
#define _XOPEN_SOURCE
#include <datestrconv.h>
#include <duprintf.h>

char *deffmt  = "%d-%m-%Y %H:%M:%S";

/*
 * convert a data time string to time_t
 * ex: "27  1 2016  0:02:24" -> 1401197380
 * *str_time must freed by caller if used.
 */
void
convert_datestr(char *datestr, char *fmt, time_t *int_time, char **str_time )
{
   struct tm tm;
   time_t mytime;
   char *format = deffmt;

   if ( fmt ){
      format = fmt;
   }

   memset(&tm, 0, sizeof(struct tm));
   if ( strptime( datestr, format, &tm) == NULL ){
      fprintf(stderr, "Bad format: \"%s\" expected \"%s\"\n", datestr, format );
   }
      
   tm.tm_isdst = -1;
   mytime = mktime(&tm);
   if ( int_time ){
     *int_time = mytime;
   }
   if ( str_time ){
     *str_time = app_strdup_printf("%lu", (unsigned long) mytime );
   }
#if HQDEBUG
   if ( prog_debug ){
      char buf[255];
      strftime(buf, sizeof(buf), format, &tm);
      fprintf(stderr, "\"%s\" converted to \"%s\"\n", datestr, buf );

      struct tm *ptm;
      ptm = localtime(&mytime);
      strftime(buf, sizeof(buf), format, ptm);
      fprintf(stderr, "Result for %lu is \"%s\"\n", (unsigned long) mytime, buf );
   }
#endif
}

/*
 * time_t to date
 * -cd 1418979685
 * -cd 0x547BA1A2
 */
void convert_time_t_to_date( time_t mytime, char *fmt,  char *buf, int buflen )
{
   struct tm *ptm;

   char *format = deffmt;
   if ( fmt ){
      format = fmt;
   }

   ptm = localtime(&mytime);
   strftime(buf, buflen, format, ptm);

#if HQDEBUG
   if ( prog_debug ){
      fprintf(stderr, "%lu -> 0x%lX -> \"%s\"\n",
              (unsigned long) mytime, (unsigned long) mytime, buf );
   }
#endif
}

void convert_string_to_date( char *arg, char *fmt,  char *buf, int buflen )
{
   time_t mytime;

   mytime = strtoul(arg, NULL, 0);

   convert_time_t_to_date( mytime, fmt, buf, buflen );
}

/*
 * convert a time_t representing a diff between 2 dates
 * to a string
 */

void convert_difftime_str( time_t mytime, char *fmt, char *buf, int buflen )
{
   int days = mytime / (24 * 3600);
   mytime = mytime % (24 * 3600);
   int hours = mytime / 3600;
   mytime = mytime % 3600;
   int mins = mytime / 60;
   int secs = mytime % 60;
   snprintf( buf, buflen, fmt, days, hours, mins, secs );
}

