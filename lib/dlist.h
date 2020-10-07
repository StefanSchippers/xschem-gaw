/*
 * dlist.h -  A generic doubly linked list.
 * 
 * include LICENSE
 */

/*
 *     *----------------------------------------* 
 *     |                                        |
 *  o--o---o                                    |
 *  | prev |>-----------------------------*     |
 *  | next |>------*       *-----------*  |     |
 *  o--o---o       |       |           |  |     |
 *     |        o--o---o   |         o-o--o-o   |
 *     |        | next |>--*         | next |>--*
 *     *-------<| prev |    *-------<| prev |
 *              o--o---o    |        o------o
 *                 |        |
 *                 *--------*        
 *   head         node1    ...        noden      
 */

#ifndef APP_DLIST_H
#define APP_DLIST_H

#include <appclass.h>

/***************************************************************************
 *
 * DList(AppClass) Methods : A doubly linked list.
 * A special node is created to hold a pointer to the first node and
 * the last node.
 * The list acts as Generic FIFO First In First Out.
 ***************************************************************************/


#define  dlist_isempty(head)  ((head) == NULL || (head)->next == (head))
#define  dlist_getfirst(head) ((head)->next)
#define  dlist_getlast(head)  ((head)->prev)

/*
 * return value for dlist_iterator
 */
#define DLIST_RM_NODE       0x7fff0000	/* remove node and return   */
#define DLIST_RM_NODE_CONT  0x7fff0001	/* remove node and continue */
#define DLIST_ITER_STOP     0x7fff0002	/* end the iteration loop   */
#define DLIST_CONT          0           /* continue iteration loop   */

typedef struct _DList DList;

typedef int (*DListFP_Cmp) (AppClass * d1, AppClass * d2);
typedef int (*DListFP_Iter) (AppClass * data, void *user_data);


struct _DList {
   AppClass parent;
   struct _DList *prev;
   struct _DList *next;
   AppClass *data;
};


/*
 * prototypes
 */

extern void dlist_destroy_node(void *node);

extern DList *dlist_add(DList * head, AppClass * data, DListFP_Cmp cmp,
			int *res);
DList *dlist_add_at(DList * node, AppClass * data);
DList *dlist_add_head(DList * head, AppClass * data);
DList *dlist_add_tail(DList * head, AppClass * data);
void dlist_remove(DList * node);
AppClass *dlist_get_data(DList * head, DList * node);
AppClass *dlist_get_data_rm_node(DList * head, DList * node);
AppClass *dlist_dequeue_head(DList * head);
AppClass *dlist_dequeue_tail(DList * head);
DList *dlist_delete(DList * head, AppClass * data, DListFP_Cmp cmp);
void dlist_delete_all(DList * head);
void dlist_delete_list(DList ** head);
AppClass *dlist_lookup(DList * head, AppClass * data,
			      DListFP_Cmp cmp);
int dlist_iterator(DList * head, DListFP_Iter func,
			  void *user_data);
int dlist_iterator_cmp(AppClass * n1, AppClass * n2);
AppClass *dlist_get_ndata(DList * head, int n);

int dlist_get_nelem(DList * head);
int dlist_charPtr_cmp(AppClass * n1, AppClass * n2);

#endif				/* APP_DLIST_H */
