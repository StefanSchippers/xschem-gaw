/*
 * array.c - array interface functions
 * 
 * include LICENSE
 */
#include <stdio.h>
#include <string.h>

#include <strmem.h>
#include <array.h>

#ifdef TRACE_MEM
#include <tracemem.h>
#endif

/*
 *** \brief Allocates memory for a new Array object.
 */



Array *array_new( int elSize, int count )
{
   Array *ary;

   ary =  app_new0(Array, 1);
   array_construct( ary, elSize, count );
   app_class_overload_destroy( (AppClass *) ary, array_destroy );
   return ary;
}

/** \brief Constructor for the Array object. */

void array_construct( Array *ary, int elSize, int count )
{
   app_class_construct( (AppClass *) ary );

   if (count == 0 ){
      count = DEF_ELEM_COUNT;
   } else if ( count < 0 ){
      count = app_power_of_2(count);
   }
   if (elSize == 0 ){
      elSize = sizeof(char *); /* sizeof pointer */
   }

   ary->nAlloc = count;
   ary->elSize = elSize;
   /*
    * always keep a null element at the end of array
    * actual allocated = nAlloc + 1, but usable is nAlloc
    */
   ary->ptr = app_malloc0 ( elSize * (count + 1) );
   /*
    *  default function to destroy an element
    *  Must be overloaded for non AppClass element : char *, dlist *
    */
   ary->destroyFunc = (ElmDestroyFP_func) app_class_unref ;
}

/** \brief Destructor for the Array object. */

void array_destroy(void *ary)
{
   Array *this = (Array *) ary;

   if (ary == NULL) {
      return;
   }

   array_tbl_delete_all(this);
   
   app_free(this->ptr);
   app_class_destroy( ary );
}

void array_tbl_delete_all(Array *ary)
{
   int i;
   void **pptr;
   ElmDestroyFP_func func = ary->destroyFunc;
   
   if ( func ) {
      for ( i = 0 ; i < ary->nelem ; i++ ){
         /* pointer of type void used in arithmetic wpointer arith */
         pptr = (void *)((char *) ary->ptr + (i * ary->elSize));
         if ( *pptr ){
            func ( *pptr );
         }
      }
   }
   ary->nelem = 0;
}

void array_set_elem_destroyFunc(Array *ary, ElmDestroyFP_func func)
{
   ary->destroyFunc = (ElmDestroyFP_func) func;
}


void *array_table_get(Array *ary)
{
   return ary->ptr ;
}

void *array_tbl_get(Array *ary, int index)
{
   if ( index >= ary->nelem) {
      msg_error ( "Index %d >= Nb elem %d", index, ary->nelem);
      return NULL;
   }
   void **pptr = (void **) ary->ptr;
   return pptr[index] ;
}

/*
 * suppress the element in array and destroy it
 */
void array_tbl_delete(Array *ary, int index)
{
   if ( index >= ary->nelem) {
      msg_error ( "Index %d > Nb elem %d", index, ary->nelem);
      return;
   }
   void **pptr = (void **) ary->ptr;
   if ( pptr[index] ){
      ary->destroyFunc ( pptr[index] );
   }
   memmove ( &pptr[index], &pptr[index + 1], ary->elSize * (ary->nelem - index));
   ary->nelem--;
}

/*
 * get the element and remove it from array without destroying the element
 */
void *array_tbl_remove(Array *ary, int index)
{
   if ( index >= ary->nelem) {
      msg_error ( "Index %d > Nb elem %d", index, ary->nelem);
      return NULL;
   }
   void **pptr = (void **) ary->ptr;
   void *ptr = pptr[index] ;
   memmove ( &pptr[index], &pptr[index + 1], ary->elSize * (ary->nelem - index));
   ary->nelem--;
   return ptr ;
}

/*
 * allocate 1 more to always get a free element set to null
 */
void array_tbl_expand(Array *ary, int n)
{
   if ( n > ary->nAlloc ) {
      ary->nAlloc <<= 1;
      ary->ptr = app_realloc(ary->ptr, ary->elSize * (ary->nAlloc + 1));
      memset((char *) ary->ptr + (ary->nelem * ary->elSize), 0,
             ary->elSize * ( ary->nAlloc + 1 - ary->nelem) );
   }
}

void array_tbl_add(Array *ary, void *elPtr)
{
   array_tbl_expand(ary, ary->nelem + 1);
   void **pptr = (void **) ary->ptr;
   pptr[ary->nelem++] = elPtr;
}


/*
 *  insert before index
 */
void array_tbl_insert(Array *ary, void *elPtr, int index)
{
   if ( index > ary->nelem) {
      msg_error ( "Index %d > Nb elem %d", index, ary->nelem);
      return;
   }
      
   array_tbl_expand(ary, ary->nelem + 1);
   void **pptr = (void **) ary->ptr;
   memmove ( &pptr[index + 1], &pptr[index], ary->elSize * (ary->nelem - index));
   pptr[index] = elPtr;
   ary->nelem++;
}

/*
 *  replace element at index
 *  return the old element
 */
void *array_tbl_replace(Array *ary, void *elPtr, int index)
{
   if ( index >= ary->nelem) {
      /* just add it */
      array_tbl_add(ary, elPtr);
//      msg_error ( "Index %d > Nb elem %d", index, ary->nelem);
      return NULL;
   }
      
   void **pptr = (void **) ary->ptr;
   void *ptr = pptr[index] ;
   pptr[index] = elPtr;
   return ptr ;
}

/*
 * replace the element with the new one
 *  and kill the old one
 */
void array_tbl_replace_kill(Array *ary, void *elPtr, int index)
{
   void *ptr = array_tbl_replace(ary, elPtr, index) ;
   if ( ptr ) {
      ary->destroyFunc ( ptr ) ;
   }
}

int array_tbl_index(Array *ary, void *elPtr)
{
   int i;
   void **pptr = (void **) ary->ptr;
   
   for ( i = 0 ; i < ary->nelem ; i++ ){
      if (  pptr[i] == elPtr ) {
	 return i;
      }
   }
   return -1;
}

void array_set_nelem (Array *ary, int nelem)
{
   ary->nelem = nelem;
   return ;
}

int array_get_nelem (Array *ary)
{
   return ary->nelem ;
}


/** \brief Lookup an item in the array.
 *
 *  Walk the array pointed to by ary and return a pointer to the data if
 *  found. If not found, return NULL. 
 *
 *  \param ary The array to be iterated.
 *  \param data The data to be passed to the func when it is applied.
 *  \param cmp  A function to be used for comparing the items.
 *
 *  \return     A pointer to the data found, or NULL if not found.
 *
 *  Warning: element list Must be sorted before creating the table
 */
AppClass *array_lookup_sorted( Array *ary, AppClass *data, ArrayFP_Cmp cmp, int *index  )
{
   int i;
   int ret;
   int lo ;
   int hi ;
   AppClass *entry ;
   
   if (cmp == NULL) {
      msg_fatal( "compare function not specified" );
   }
   lo = 0;
   hi =  ary->nelem;

   while (lo != hi) {
      i = (hi + lo)  / 2 ;

      entry = (AppClass *) array_tbl_get( ary, i );
      ret = (*cmp)(entry, data);
      if ( ret > 0 ) {  /*  entry->symbol > symbol */
         hi = i;
      } else if ( ret < 0 ) {  /*  entry->symbol < symbol */
         lo = i + 1;
      } else {         /*  entry->symbol == symbol  */
         /* symbol found */
         if ( index ){
            *index = i;
         }
         return entry;
      }
   }
   if ( index ){
      *index = lo;
   }
   return NULL;
}

/*
 * lookup an unsorted list
 */
AppClass *array_lookup( Array *ary, AppClass *data, ArrayFP_Cmp cmp, int *index )
{
   int i;
   AppClass *entry ;
   
   if (cmp == NULL) {
      msg_fatal( "compare function not specified" );
   }

   for ( i = 0; i < ary->nelem ; i++) {
      entry = (AppClass *) array_tbl_get( ary, i );
      if ( entry && (*cmp)(entry, data) == 0 ) {  /*  entry->symbol == symbol */
         /* symbol found */
         if ( index ){
            *index = i;
         }
         return entry;
      }
   }
   return NULL;
}

void array_sort(Array *ary, ArraySortFP_Cmp cmp_func )
{
   qsort( ary->ptr, ary->nelem, ary->elSize, cmp_func);
}

void array_iterator(Array *ary, ArrayFP_Iter iter_func, void *user_data)
{
   int i;
   AppClass *entry ;
   
   for ( i = 0; i < ary->nelem ; i++) {
      entry = (AppClass *) array_tbl_get( ary, i );
      if ( entry ){
         (*iter_func)(entry, user_data);
      }
   }
}
