#ifndef STROBJ_H
#define STROBJ_H

/*
 * strobj.h - simple object to store a string
 *    - needed to store string in DList
 * 
 * include LICENSE
 */

#include <strmem.h>
#include <appclass.h>

typedef struct _StrObj StrObj;

struct _StrObj {
   AppClass parent;
   char *pstr;         /* pointer to string */
};

/*
 * prototypes
 */
StrObj *str_new( char *pstr );
void str_construct( StrObj *str, char *pstr );
void str_destroy(void *str);

void str_replace( StrObj *str, char *pstr );
int str_func_cmp ( AppClass *d1, AppClass *d2 );
int str_func_str_cmp ( AppClass *d1, AppClass *d2 );
int str_qsort_cmp(const void *d1, const void *d2 );

#endif /* STROBJ_H */
