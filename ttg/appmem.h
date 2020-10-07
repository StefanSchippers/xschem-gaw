#ifndef APP_MEM_H
#define APP_MEM_H
/*
 * appmem.h - interface to memory allocation
 * 
 * include LICENSE
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
/*
 * msglog.h is needed for msg_fatal, ...
 * CENTOS : msglog include stdlib.h that need to be included before glib.h
 */
#include <msglog.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) ( sizeof(a) / sizeof((a)[0]) )
#endif

#ifdef HAVE_GLIB
#include <glib.h>
#undef malloc
#undef calloc
#undef realloc
#undef free
#define malloc  g_malloc
#define calloc(x,y) g_malloc0(x*y)
#define realloc g_realloc
#define free    g_free
  /* this is for CENTOS */
//  #define __need_malloc_and_calloc
#endif

#ifndef HAVE_GLIB
# define GUINT_TO_POINTER(u)	((void *) (unsigned long) (u))
# define GINT_TO_POINTER(i)	((void *) (long) (i))
# define GPOINTER_TO_UINT(p)	((unsigned int) (unsigned long) (p))
# define GPOINTER_TO_INT(p)	((int) (unsigned long) (p))
#endif

/* disable some trace_mem functions remaining in the code */
#ifndef TRACE_MEM
#define tmem_init( file )
#define tmem_destroy(tm)
#define tmem_alloced(p,size)
#define tmem_freed(p)
#define app_memcheck()
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

/*
 * Macros for allocating new memory. Automatically performs type cast.
 */

/** \brief Macro for allocating memory.
 *  \param type  The C type of the memory to allocate.
 *  \param count Allocate enough memory hold count types.
 *
 *  This macro is just a wrapper for app_malloc() and should be used to avoid
 *  the repetitive task of casting the returned pointer.
 */


#define app_new(type, count)          \
    ((type *) app_malloc ((unsigned) sizeof (type) * (count)))

#define app_new0(type, count)         \
    ((type *) app_malloc0 ((unsigned) sizeof (type) * (count)))

#define app_renew(type, mem, count)   \
    ((type *) app_realloc (mem, (unsigned) sizeof (type) * (count)))

/*
 * Malloc and free wrappers. Perform sanity checks for you.
 */

extern void *app_malloc(size_t size);
extern void *app_malloc0(size_t size);
extern void *app_realloc(void *ptr, size_t size);

extern void app_free(void *ptr);

#endif				/* APP_MEM_H */
