/**
 * \file duprintf.c
 * \brief strdup printf function
 * 
 * stolen from glib
 * 
 * include LICENSE
 */

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>

#include <duprintf.h>

#ifdef TRACE_MEM
#include <tracemem.h>
#endif


char *app_strdup_vprintf(const char *format, va_list args)
{
   char *string = NULL;
   
   /* vasprintf returns malloc-allocated memory */
   int len = vasprintf(&string, format, args);
   
   if (len < 0) {
      string = NULL;
   }
   return string;
}

char *app_strdup_printf(const char *format, ...)
{
   char *buffer;
   va_list args;

   va_start(args, format);
   buffer = app_strdup_vprintf(format, args);
   va_end(args);

   return buffer;
}
