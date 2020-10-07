#ifndef DLISTSTR_H
#define DLISTSTR_H

/*
 * dliststr.h - function to handle string in DList
 * 
 * include LICENSE
 */

#include <strobj.h>
#include <dlist.h>


/*
 * prototypes
 */
DList *dlist_str_add( DList *head, char *pstr, DListFP_Cmp cmp, int *res );
DList *dlist_str_add_head( DList *head, char *pstr );
DList *dlist_str_add_tail( DList *head, char *pstr );

char *dlist_str_node_get_data(DList *node);
char *dlist_str_get_ndata(DList *head, int index);
DList *dlist_str_set_ndata( DList *head, char *nstr, int index);

#endif /* DLISTSTR_H */
