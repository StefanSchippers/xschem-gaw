#ifndef STR_MEM_H
#define STR_MEM_H
/*
 * strmem.h - interface to str functions
 * 
 * include LICENSE
 */

#include <appmem.h>

extern char *app_strndup(const char *s, int size);
extern unsigned int app_power_of_2(unsigned int size);
extern char *app_strdup(const char *s);
extern int app_strlen(const char *s);
extern char *app_strcpy(char *s1, const char *s2);
extern char *app_strncpy(char *s1, const char *s2, int n);
extern int app_strcmp(const char *s1, const char *s2);
extern int app_strncmp(const char *s1, const char *s2, int n);
extern int app_strcasecmp(const char *s1, const char *s2);
extern int app_strncasecmp(const char *s1, const char *s2, int n);

char *app_strstr(const char *s1, const char *s2);
char *app_strcasestr(const char *s1, const char *s2);

void app_dup_str(char **varp, char *str);

#ifdef TRACE_MEM
void str_free_func(void *ptr);
#endif

#endif				/* STR_MEM_H */
