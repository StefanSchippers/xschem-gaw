#ifndef STRCATDUP_H
#define STRCATDUP_H

/*
 * strcatdup.h - Interface to app_strcatdup
 * 
 * include LICENSE
 */

char *app_strcatdup (const char *str, ...);
char *app_strappend ( char *ptr, char *str, char *sep);

#endif /* STRCATDUP_H */
