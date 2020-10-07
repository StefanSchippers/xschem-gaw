/*
 *
 * arraystruct.h - array of structures
 *   same thing as array but with structure instead of pointer
 * 
 * include LICENSE
 */
#include <stdio.h>
#include <string.h>

#include <arraystruct.h>


/*
 *** \brief Allocates memory for a new ArrayStruct object.
 */

ArrayStruct *array_struct_new( int elSize, int count, ElmDestroyFP_func func )
{
   ArrayStruct *aryst;

   aryst =  app_new0(ArrayStruct, 1);
   array_struct_construct( aryst, elSize, count, func );
   app_class_overload_destroy( (AppClass *) aryst, array_struct_destroy );
   return aryst;
}

/** \brief Constructor for the ArrayStruct object. */

void array_struct_construct( ArrayStruct *aryst, int elSize, int count,
                             ElmDestroyFP_func func )
{
   array_construct( (Array *) aryst, elSize, count );
   array_set_elem_destroyFunc( (Array *) aryst, NULL);
   aryst->func = func;
}

/** \brief Destructor for the ArrayStruct object. */

void array_struct_destroy(void *aryst)
{
   ArrayStruct *this = (ArrayStruct *) aryst;

   if (aryst == NULL) {
      return;
   }
   array_struct_delete_all(this);
      
   array_destroy(aryst);
}

void array_struct_delete_all(ArrayStruct *aryst)
{
   int i;
   void **pptr = (void **) ((Array *) aryst)->ptr;
   ElmDestroyFP_func func = aryst->func;
   
   if ( func ) {
      for ( i = 0 ; i < ((Array *) aryst)->nelem ; i++ ){
         pptr = ((Array *) aryst)->ptr + (i * ((Array *) aryst)->elSize);
         func ( pptr );
      }
   }
   ((Array *) aryst)->nelem = 0;
}

/*
 * get a pointer to the structure at index
 */
void *array_struct_get( ArrayStruct *aryst, int index)
{
   Array *ary = (Array *) aryst;
   if ( index >= ary->nelem) {
      msg_error ( "Index %d > Nb elem %d", index, ary->nelem);
      return NULL;
   }
   return (void *) ary->ptr + (ary->elSize * index);
}

/*
 * add a new element in array
 * return a pointer to the next elem
 *    to let the user fill it
 */
void *array_struct_next(ArrayStruct *aryst )
{
   Array *ary = (Array *) aryst;
   array_tbl_expand( ary, ary->nelem + 1);
   return (void *) ary->ptr + ( ary->elSize * ary->nelem++);
}
