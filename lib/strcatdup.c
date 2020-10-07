/**
 * \file appstrcatdup.c
 * \brief app_strcatdup function
 * 
 * stolen from glib
 * 
 * include LICENSE
 */

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include <appmem.h>
#include <strcatdup.h>
 
#ifdef TRACE_MEM
#include <tracemem.h>
#endif
        

/*
 * concatanate all arguments of a NULL terminated list
 * and strdup the result
 */
char *app_strcatdup (const char *str, ...)
{
   char *buffer, *cur;
   const char *p;
   unsigned int len = 0;
   va_list args;
   
   if ((p = str) == NULL) {
      return NULL;
   }
   
   va_start (args, str);
   while (p != NULL) {
      len += strlen (p);
      p = va_arg (args, const char *);
   }
   va_end (args);
   
   cur = buffer = app_new(char, len + 1);
   
   va_start (args, str);
   p = str;
   while (p != NULL) {
      strcpy (cur, p); /* safe */
      p = va_arg (args, const char *);
      cur += strlen (cur);
   }
   va_end (args);
   
   return buffer;
}

/*
 * concatenate and dup <res> = <ptr> <sep> <str>
 * ptr and str must be duped before.
 * res should be freed after usage.
 */
char *app_strappend ( char *ptr, char *str, char *sep)
{
   if ( ptr == NULL ) {
      return str;
   }
   char *newptr = app_strcatdup( ptr, sep, str, NULL );
   app_free( ptr );
   app_free( str );
   return newptr;
}
