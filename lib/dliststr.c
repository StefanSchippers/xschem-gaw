/*
 * dliststr.c - a list of StrObj's
 * 
 * include LICENSE
 */
#include <stdio.h>
#include <string.h>

#include <msglog.h>
#include <dliststr.h>
 
#ifdef TRACE_MEM
#include <tracemem.h>
#endif
        


/** \brief Add a new node to the end of the list. */

DList *dlist_str_add( DList *head, char *pstr, DListFP_Cmp cmp, int *res )
{

   StrObj *strobj = str_new( pstr );
   return dlist_add( head, (AppClass *) strobj, cmp, res );
}

/** \brief Add a new node at the head of the list. */

DList *dlist_str_add_head( DList *head, char *pstr )
{
   StrObj *strobj = str_new( pstr );
   return dlist_add_head( head, (AppClass *) strobj );
}

/** \brief Add a new node at the tail of the list. */

DList *dlist_str_add_tail( DList *head, char *pstr )
{
   StrObj *strobj = str_new( pstr );
   return dlist_add_tail( head, (AppClass *) strobj );
}


/*
 * get a member of the list at index
 */
char *dlist_str_node_get_data(DList *node)
{
   if ( node ) {
      StrObj *strobj = (StrObj *) node->data;
      return strobj->pstr;
   }
   return NULL;
}

/*
 * get a member of the list at index
 */
char *dlist_str_get_ndata(DList *head, int index)
{
   StrObj *strobj;
   if ( ( strobj = (StrObj *) dlist_get_ndata(head, index)) ) {
      return strobj->pstr;
   }
   return NULL;
}

DList *dlist_str_set_ndata( DList *head, char *nstr, int index)
{
   StrObj *strobj;
   
   if ( ( strobj = (StrObj *) dlist_get_ndata(head, index)) ) {
      str_replace( strobj, nstr);
   } else {
      strobj = str_new(nstr);
      head = dlist_add_tail( head, (AppClass *) strobj );
   }
   return head;
}


