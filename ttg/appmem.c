/**
 * \file appmem.c
 * \brief Methods to provide memory interfaces to the AppClass structure.
 *
 * interface to memory allocation
 * 
 * stolen from simulavr
 * 
 * include LICENSE
 */

#include <appmem.h>


/**
 * \brief Memory Management Functions.
 *
 * This module provides facilities for managing memory.
 *
 *
 * There is no need to check the returned values from any of these
 * functions. Any memory allocation failure is considered fatal and the
 * program is terminated.
 *
 * We want to wrap all functions that allocate memory. This way we can
 * add secret code to track memory usage and debug memory leaks if we 
 * want. Right now, I don't want to ;).
 */


/* These macros are only here for documentation purposes. */

#ifdef MACRO_DOCUMENTATION

/** \brief Macro for allocating memory.
 *  \param type  The C type of the memory to allocate.
 *  \param count Allocate enough memory hold count types.
 *
 *  This macro is just a wrapper for app_malloc() and should be used to avoid
 *  the repetitive task of casting the returned pointer.
 */

#define app_new(type, count)          \
    ((type *) app_malloc ((unsigned) sizeof (type) * (count)))

/** \brief Macro for allocating memory and initializing it to zero.
 *  \param type  The C type of the memory to allocate.
 *  \param count Allocate enough memory hold count types.
 *
 *  This macro is just a wrapper for app_malloc0() and should be used to avoid
 *  the repetitive task of casting the returned pointer.
 */

#define app_new0(type, count)         \
    ((type *) app_malloc0 ((unsigned) sizeof (type) * (count)))

/** \brief Macro for allocating memory.
 *  \param type  The C type of the memory to allocate.
 *  \param mem   Pointer to existing memory.
 *  \param count Allocate enough memory hold count types.
 *
 *  This macro is just a wrapper for app_malloc() and should be used to avoid
 *  the repetitive task of casting the returned pointer.
 */

#define app_renew(type, mem, count)   \
   ((type *) app_realloc (mem, (unsigned) sizeof (type) * (count)))

#endif				/* MACRO_DOCUMENTATION */

/** \brief Allocate memory and initialize to zero.
 *
 *  Use the app_new() macro instead of this function.
 *
 *  There is no need to check the returned value, since this function will
 *  terminate the program if the memory allocation fails.
 *
 *  No memory is allocated if passed a size of zero.
 */

void *app_malloc(size_t size)
{
   if (size) {
      void *ptr;

      ptr = malloc(size);
      if (ptr) {
	 return ptr;
      }
      msg_fatal(_("malloc failed"));
   }
   return NULL;
}

/** \brief Allocate memory and initialize to zero.
 *
 *  Use the app_new0() macro instead of this function.
 *
 *  There is no need to check the returned value, since this function will
 *  terminate the program if the memory allocation fails.
 *
 *  No memory is allocated if passed a size of zero.
 */

void *app_malloc0(size_t size)
{
   if (size) {
      void *ptr;

      ptr = calloc(1, size);
      if (ptr) {
	 return ptr;
      }
      msg_fatal(_("malloc0 failed"));
   }
   return NULL;
}

/** \brief Wrapper for realloc().
 *
 *  Resizes and possibly allocates more memory for an existing memory block.
 *
 *  Use the app_renew() macro instead of this function.
 *
 *  There is no need to check the returned value, since this function will
 *  terminate the program if the memory allocation fails.
 *
 *  No memory is allocated if passed a size of zero.
 */

void *app_realloc(void *ptr, size_t size)
{
   if (size) {
      ptr = realloc(ptr, size);
      if (ptr) {
	 return ptr;
      }
      msg_fatal(_("realloc failed"));
   }
   return NULL;
}

/** \brief Free malloc'd memory.
 *
 *  It is safe to pass a null pointer to this function.
 */

void app_free(void *ptr)
{
   if (ptr) {
      free(ptr);
   }
}
