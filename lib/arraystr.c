/*
 * arraystr.c - array of string pointers
 * 
 * include LICENSE
 */
#include <stdio.h>
#include <string.h>

#include <strmem.h>
#include <arraystr.h>

#ifdef TRACE_MEM
#include <tracemem.h>
#endif

/*
 *** \brief Allocates memory for a new Array object.
 */


ArrayStr *array_strPtr_new( int count )
{
   ArrayStr *ary;

   ary =  app_new0(ArrayStr, 1);
   array_strPtr_construct( ary, count );
   app_class_overload_destroy( (AppClass *) ary, array_strPtr_destroy );
   return ary;
}

/** \brief Constructor for the ArrayStr object. */

void array_strPtr_construct( ArrayStr *ary, int count )
{
   array_construct( (Array *) ary, 0, count );

#ifdef TRACE_MEM
    ((Array *) ary)->destroyFunc = str_free_func;
#else
    ((Array *) ary)->destroyFunc = app_free;
#endif
}

/** \brief Destructor for the ArrayStr object. */

void array_strPtr_destroy(void *ary)
{
//   ArrayStr *this = (ArrayStr *) ary;

   if (ary == NULL) {
      return;
   }
   array_destroy( ary );
}


/*
 * strdup a string and add it
 */
void array_strPtr_nadd(ArrayStr *ary, char *str, int len )
{
   char *newstr = app_strndup(str, len);
   array_tbl_add((Array *) ary, newstr);
}

/*
 * strdup a string and add it
 */
void array_strPtr_add(ArrayStr *ary, char *str )
{
   char *newstr = app_strdup(str);
   array_tbl_add((Array *) ary, newstr);
}


/*
 * strdup a string and insert it
 */
void array_strPtr_insert(ArrayStr *ary, char *str, int index )
{
   char *newstr = app_strdup(str);
   array_tbl_insert((Array *) ary, newstr, index);
}


/*
 * strdup a string and replace it at index
 */
char *array_strPtr_replace(ArrayStr *ary, char *str, int index )
{
   char *newstr = app_strdup(str);
   return (char *) array_tbl_replace((Array *) ary, newstr, index);
}

void array_strPtr_replace_kill(ArrayStr *ary, char *str, int index )
{
   char *oldstr = array_strPtr_replace( ary, str, index );
   if ( oldstr ) {
      ((Array *) ary)->destroyFunc ( oldstr ) ;
   }
}

int array_strPtr_cmp( AppClass *d1, AppClass *d2 )
{
   return (app_strcmp( ((char *) d1), ((char *) d2) ) );
}

