#ifndef ARRAYSTRUCT_H
#define ARRAYSTRUCT_H

/*
 * arraystruct.h - array of structures
 *   same thing as array but with structure instead of pointer
 * 
 * include LICENSE
 */

#include <array.h>

typedef struct _ArrayStruct ArrayStruct;

struct _ArrayStruct {
   Array parent;
   ElmDestroyFP_func func; /* function to destroy the element */
};

#define array_struct_table_get(ary) array_table_get((Array *) ary)
#define array_struct_get_nelem(ary) array_get_nelem((Array *) ary)
/*
 * prototypes
 */
ArrayStruct *array_struct_new( int elSize, int count, ElmDestroyFP_func func );
void array_struct_construct( ArrayStruct *aryst, int elSize,
                             int count, ElmDestroyFP_func func );
void array_struct_destroy(void *aryst);

void array_struct_delete_all(ArrayStruct *aryst);

void *array_struct_get(ArrayStruct *aryst, int index);
void *array_struct_next(ArrayStruct *aryst );


#endif /* ARRAYSTRUCT_H */
