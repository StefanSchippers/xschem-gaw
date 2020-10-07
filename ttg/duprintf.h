#ifndef DUPRINTF_H
#define DUPRINTF_H

/*
 * duprintf.h - Interface to app_strdup_printf
 * 
 * include LICENSE
 */

#include <stdarg.h>

#include <strmem.h>
#include <appmem.h>

char *app_strdup_vprintf(const char *format, va_list args);
char *app_strdup_printf(const char *format, ...);

#endif				/* DUPRINTF_H */
