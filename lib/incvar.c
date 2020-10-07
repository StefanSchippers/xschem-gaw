/*
 * incvar.c - keep track of user variables for rewriting
 *             line interface
 * 
 * include LICENSE
 */
#include <stdio.h>
#include <string.h>

#include <msglog.h>
#include <array.h>
#include <dliststr.h>
#include <strcatdup.h>
#include <incvar.h>
#include <duprintf.h>
 
#ifdef TRACE_MEM
#include <tracemem.h>
#endif
        


/*
 *** \brief Allocates memory for a new IncVar object.
 */

IncVar *var_new(  unsigned int linenum,  void *varptr, int vartype )
{
   IncVar *var;

   var =  app_new0(IncVar, 1);
   var_construct( var, linenum, varptr, vartype );
   app_class_overload_destroy( (AppClass *) var, var_destroy );
   return var;
}

/** \brief Constructor for the IncVar object. */

void var_construct( IncVar *var,  unsigned int linenum,  void *varptr, int vartype )
{
   app_class_construct( (AppClass *) var );

   var->linenum = linenum;
   var->varptr = varptr;
   var->vartype = vartype;
}

/** \brief Destructor for the IncLine object. */

void var_destroy(void *var)
{
//   IncVar *this = (IncVar *) var;

   if (var == NULL) {
      return;
   }
   app_class_destroy( var );
}


char *var_mkstr(void *varptr, int vartype)
{
   char *pstr;
   char *tmpstr;

   switch( vartype) {
    case TBOOL:
      {
	 int *pi = (int *) varptr;
	 return app_strdup_printf("%d", (*pi ? 1 : 0) ) ;
      }
      break ;
    case TDECI: {
       int *pi = (int *) varptr;
       return app_strdup_printf( "%d", *pi ) ;
    }
      break ;
    case THEXA :
      {
	 int *pi = (int *) varptr;
	 return app_strdup_printf( "0x%X", *pi ) ;
      }
      break ;
    case TPTR:
      {
	 char *p = *((char **) varptr);
	 if ( ! p ) {
	    return NULL;
	 }
         /* always put string in quotes */ 
	 return app_strdup_printf("\"%s\"", p ) ;
      }
      break ;
    case TARYSTR:
      {
	 int i;
	 Array *ary = *((Array **) varptr);
	 int num = array_get_nelem(ary);
	 char *retptr = app_strdup("");
	 char *sep = "";
	 
	 for ( i = 0 ; i < num ; i++ ) {
	    pstr = array_tbl_get(ary, i);
            /* always put string in quotes */ 
	    tmpstr = app_strdup_printf("\"%s\"", pstr );
	    retptr = app_strappend( retptr, tmpstr, sep);
	    sep = ", ";
	 }
	 return retptr;
      }
      break;
    case TLIST:
      {
	 DList *head = *((DList **) varptr);
	 DList *node = head;
         if ( head ) {
            node = head->next;
         }
	 char *retptr = app_strdup("");
	 char *sep = "";
	 int i = 0;
       
	 while (node != head) {
	    pstr =  dlist_str_node_get_data(node);
            /* always put string in quotes */ 
	    tmpstr = app_strdup_printf("\"%s\"", pstr );
	    retptr = app_strappend( retptr, tmpstr, sep );
	    sep = ", ";
	    i++;
	    node = node->next;
	 }
	 return retptr;
      }
   }
   return NULL;
}

char *var_get_str(IncVar *var)
{
   return var_mkstr(var->varptr, var->vartype);
}
