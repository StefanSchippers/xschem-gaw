#ifndef DATESTRCONV_H
#define DATESTRCONV_H

/*
 * datestrconv.h - convert string to time and time to string
 *
 * include LICENSE
 */
#include <stdio.h>
#include <string.h>
#include <time.h>

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE /* for strptime */
#endif

/*
 * prototypes
 */
void convert_datestr(char *datestr, char *fmt, time_t *int_time, char **str_time );
void convert_time_t_to_date( time_t mytime, char *fmt,  char *buf, int buflen );
void convert_string_to_date( char *arg, char *fmt, char *buf, int buflen );
void convert_difftime_str( time_t mytime, char *fmt, char *buf, int buflen );

#endif /* DATESTRCONV_H */
