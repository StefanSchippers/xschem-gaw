#ifndef ARRAYSTR_H
#define ARRAYSTR_H

/*
 * arraystr.h - array of string pointers
 * 
 * include LICENSE
 */
 
#include <array.h>

#define array_strPtr_get(ary, index) \
   (char *) array_tbl_get((Array *) ary, index)
#define array_strPtr_nelem(ary) \
   array_get_nelem((Array *) ary)
#define array_strPtr_lookup(ary, str, pindex) \
   (char *) array_lookup((Array *) ary, (AppClass *) str, array_strPtr_cmp, pindex)

typedef struct _ArrayStr ArrayStr;

struct _ArrayStr {
   Array parent;   /* the Array Stucture */
};

/*
 * prototypes
 */

ArrayStr *array_strPtr_new( int count );
void array_strPtr_construct( ArrayStr *ary, int count );
void array_strPtr_destroy(void *ary);

void array_strPtr_nadd(ArrayStr *ary, char *str, int len );
void array_strPtr_add(ArrayStr *ary, char *str );
void array_strPtr_insert(ArrayStr *ary, char *str, int index );
char *array_strPtr_replace(ArrayStr *ary, char *str, int index );
void array_strPtr_replace_kill(ArrayStr *ary, char *str, int index );

int array_strPtr_cmp( AppClass *d1, AppClass *d2 );

#endif /* ARRAYSTR_H */
