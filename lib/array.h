#ifndef ARRAY_H
#define ARRAY_H

/*
 * array.h - array of pointers
 * 
 * include LICENSE
 */
/*
 array_new( MimeObj, 8)
 * 
 */
 
#include <appclass.h>

typedef void (*ElmDestroyFP_func) ( void *ptr);
typedef int    (*ArrayFP_Cmp)     ( AppClass *d1, AppClass *d2 );
typedef int    (*ArraySortFP_Cmp) ( const void *, const void * );
typedef int    (*ArrayFP_Iter) ( AppClass *data, void *user_data );

#define DEF_ELEM_COUNT 8

typedef struct _Array Array;

struct _Array {
   AppClass parent;
   void *ptr;       /* the array */
   int nAlloc;      /* usable number element allocated */
   int nelem;       /* number element actually added */
   int elSize;      /* size of one table entry */
   ElmDestroyFP_func destroyFunc;
};

/*
 * prototypes
 */

Array *array_new(  int elSize, int count );
void array_construct( Array *ary, int elSize, int count );
void array_destroy(void *ary);
void array_set_elem_destroyFunc(Array *ary, ElmDestroyFP_func func);

void array_tbl_expand(Array *ary, int n);
void *array_table_get(Array *ary);
void array_tbl_delete_all(Array *ary);
void array_tbl_delete(Array *ary, int index);
void *array_tbl_get(Array *ary, int index);
void *array_tbl_remove(Array *ary, int index);
void array_tbl_add(Array *ary, void *elPtr);
void array_tbl_insert(Array *ary, void *elPtr, int index);
void *array_tbl_replace(Array *ary, void *elPtr, int index);
void array_tbl_replace_kill(Array *ary, void *elPtr, int index);

void array_set_nelem (Array *ary, int nelem);
int array_get_nelem (Array *ary);
int array_tbl_index(Array *ary, void *elPtr);
AppClass *array_lookup( Array *ary, AppClass *data, ArrayFP_Cmp cmp, int *index );
AppClass *array_lookup_sorted( Array *ary, AppClass *data, ArrayFP_Cmp cmp, int *index );

void array_sort(Array *ary, ArraySortFP_Cmp cmp_func );
void array_iterator(Array *ary, ArrayFP_Iter iter_func, void *user_data );

#endif /* ARRAY_H */
