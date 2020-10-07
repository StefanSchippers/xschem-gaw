/**
 * \file strmem.c
 * \brief Methods to str functions.
 * 
 * include LICENSE
 */

#define _GNU_SOURCE		/* for strcasestr */

#include <string.h>

#include <strmem.h>

#ifdef TRACE_MEM
#include <tracemem.h>
#endif


/** \brief Wrapper for strdup().
 *
 *  Returns a copy of the passed in string. The returned copy must be
 *  free'd.
 *
 *  There is no need to check the returned value, since this function will
 *  terminate the program if the memory allocation fails.
 *
 *  It is safe to pass a NULL pointer. No memory is allocated if a NULL is
 *  passed.
 */

char *app_strndup(const char *s, int size)
{
   if (s) {
      char *ptr = app_new(char, size + 1);

      if (ptr) {
	 memcpy(ptr, s, size);
	 ptr[size] = '\0';
	 return ptr;
      }
      msg_fatal(_("strndup failed"));
   }
   return NULL;
}

char *app_strdup(const char *s)
{
   if (s) {
      return app_strndup(s, strlen(s));
   }
   return NULL;
}


/** \brief return a power of 2 value greater than size.
 *
 */

unsigned int app_power_of_2(unsigned int size)
{
   unsigned int newsize = 1;

   while (newsize < size) {
      newsize <<= 1;
   }
   return newsize;
}

void app_dup_str(char **varp, char *str)
{
   app_free(*varp);
   if (str) {
      *varp = app_strdup(str);
   } else {
      *varp = NULL;
   }
}

/*
 * just to avoid segmentation fault
 */
int app_strlen(const char *s)
{
   if (s) {
      return strlen(s);
   }
   return 0;
}

char *app_strcpy(char *s1, const char *s2)
{
   if (s1 && s2) {
      return strcpy(s1, s2);
   }
   return NULL;
}

char *app_strncpy(char *s1, const char *s2, int n)
{
   if (s1 && s2) {
      return strncpy(s1, s2, n);
   }
   return NULL;
}

int app_strcmp(const char *s1, const char *s2)
{
   if (s1 && s2) {
      return strcmp(s1, s2);
   }
   return 1;			/* not egal */
}

int app_strncmp(const char *s1, const char *s2, int n)
{
   if (s1 && s2) {
      return strncmp(s1, s2, n);
   }
   return 1;			/* not egal */
}

int app_strcasecmp(const char *s1, const char *s2)
{
   if (s1 && s2) {
      return strcasecmp(s1, s2);
   }
   return 1;			/* not egal */
}

int app_strncasecmp(const char *s1, const char *s2, int n)
{
   if (s1 && s2) {
      return strncasecmp(s1, s2, n);
   }
   return 1;			/* not egal */
}

char *app_strcasestr(const char *s1, const char *s2)
{
   if (s1 && s2) {
      return strcasestr(s1, s2);
   }
   return NULL;
}

char *app_strstr(const char *s1, const char *s2)
{
   if (s1 && s2) {
      return strstr(s1, s2);
   }
   return NULL;
}

/*
 * special for trace memory
 */

#ifdef TRACE_MEM
void str_free_func(void *ptr)
{
   app_free(ptr);
}
#endif
