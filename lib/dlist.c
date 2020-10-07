/*
 * dlist.c -  A generic doubly linked list.
 * 
 * include LICENSE
 */

#include <stdio.h>
#include <sys/time.h>

#include <appclass.h>
#include <dlist.h>

#ifdef TRACE_MEM
#include <tracemem.h>
#endif


/***************************************************************************
 *
 * DList(AppClass) Methods : A doubly linked list.
 *
 ***************************************************************************/

static DList *dlist_new_node(AppClass * data);
static void dlist_construct_node(DList * node, AppClass * data);


static DList *dlist_new_node(AppClass * data)
{
   DList *node;

   node = app_new0(DList, 1);
   dlist_construct_node(node, data);
   app_class_overload_destroy((AppClass *) node, dlist_destroy_node);

   return node;
}

static void dlist_construct_node(DList * node, AppClass * data)
{
   app_class_construct((AppClass *) node);

   node->prev = node;
   node->next = node;

   node->data = data;
}

void dlist_destroy_node(void *node)
{
   DList *_node = (DList *) node;

   if (_node == NULL) {
      return;
   }

   if (_node->data) {
      app_class_unref(_node->data);
   }
   app_class_destroy(node);
}

/** \brief Add a new node to the end of the list.
 *
 * If cmp argument is not NULL, use cmp() to see if node already exists and
 * don't add node if it exists.
 *
 * This function can unref data if not added, if asked by user (*res != 0).
 * The res variable is set to 1 if data is not added.
 * 
 * head is modified only when the list is empty.
 *  So it is safe to add node to an existing list.
 */

DList *dlist_add(DList * head, AppClass * data, DListFP_Cmp cmp, int *res)
{
   if (head == NULL) {
      /* The list is empty, make new node the head. */
      head = dlist_new_node(NULL);	/* special head node */
   }
   DList *node = head->next;

   /* Walk the list to find the end */

   if (cmp) {
      while (node != head) {
	 if ((*cmp) (node->data, data) == 0) {
	    /* node already exists and we were asked to keep nodes unique */
	    if (res) {
	       if (*res) {
		  app_class_unref(data);	/* elem destroyed */
	       }
	       *res = 1;	/* elem not added to the list */
	    }
	    return head;
	 }
	 node = node->next;
      }
   }
   /* reached  the end - add at the tail */
   dlist_add_at(head->prev, data);
   if (res) {
      *res = 0;			/* elem added to the list */
   }
   return head;
}

/** \brief Add a new node after this node
 */

DList *dlist_add_at(DList * node, AppClass * data)
{
   DList *head = node;

   if (head == NULL) {
      head = dlist_new_node(NULL);	/* special head node */
   }

   DList *newnode = dlist_new_node(data);

   newnode->prev = head->next->prev;
   head->next->prev = newnode;
   newnode->next = head->next;
   head->next = newnode;

   return head;
}

/* add a node at the head */
DList *dlist_add_head(DList * head, AppClass * data)
{
   return dlist_add_at(head, data);
}

/* add a node at the tail  */
DList *dlist_add_tail(DList * head, AppClass * data)
{
   if ( ! head) {
      return dlist_add_at(head, data);
   }
   dlist_add_at(head->prev, data);
   return head;
}

/*
 * remove node from list
 * free the node and data if not refed
 */
void dlist_remove(DList * node)
{
   if (node) {
      node->prev->next = node->next;
      node->next->prev = node->prev;
      /* free the node and data */
      app_class_unref((AppClass *) node);
   }
}

/*
 * get next data at head or tail
 * return data
 */
AppClass *dlist_get_data(DList * head, DList * node)
{
   if (head && node != head) {
      return node->data;
   }
   return NULL;
}

/*
 * get next data at head or tail
 * remove node from list
 * return data
 * caller must call app_class_unref  to free the data
 */
AppClass *dlist_get_data_rm_node(DList * head, DList * node)
{
   AppClass *data = dlist_get_data(head, node);

   if (data) {
      app_class_ref(data);
      dlist_remove(node);
   }
   return data;
}

/* get next data at head  */
AppClass *dlist_dequeue_head(DList * head)
{
   if (head) {
      return dlist_get_data_rm_node(head, head->next);
   }
   return NULL;
}

/* get next data at tail  */
AppClass *dlist_dequeue_tail(DList * head)
{
   if (head) {
      return dlist_get_data_rm_node(head, head->prev);
   }
   return NULL;
}

/** \brief Conditionally delete a node from the list.
 *
 *  Delete a node from the list if the node's data matches the specified
 *  data. Returns the head of the modified list.
 */

DList *dlist_delete(DList * head, AppClass * data, DListFP_Cmp cmp)
{
   if (cmp == NULL) {
      msg_fatal("compare function not specified");
   }
   if (head == NULL) {
      return NULL;
   }
   DList *node = head->next;

   while (node != head) {
      if ((*cmp) (node->data, data) == 0) {
	 /* remove the node from the list and also unref the node */
	 dlist_remove(node);

	 return head;
      }
      /* move on to next node */
      node = node->next;
   }
   /* if we get here, data wasn't found, just return original head */
   return head;
}

void dlist_delete_list(DList ** head)
{
   if ( *head ){
      if ( (*head)->next == *head ){
         /* destroy special node as well */
         app_class_unref((AppClass *) *head );
      } else {
         dlist_delete_all(*head);
      }
      *head = NULL;
   }
}
/** \brief Blow away the entire list.
 * Warning: after this function we must not reuse the head without
 * setting it to NULL. Use dlist_delete_list instead.
 */

void dlist_delete_all(DList * head)
{
   if (head == NULL) {
      return;
   }
   DList *node = head->next;
   DList *next;

   while (node != head) {
      next = node->next;

      app_class_unref((AppClass *) node);
      node = next;
   }
   /* destroy special node as well */
   app_class_unref((AppClass *) node);
}

/** \brief Lookup an item in the list.
 *
 *  Walk the list pointed to by head and return a pointer to the data if
 *  found. If not found, return NULL. 
 *
 *  \param head The head of the list to be iterated.
 *  \param data The data to be passed to the func when it is applied.
 *  \param cmp  A function to be used for comparing the items.
 *
 *  \return     A pointer to the data found, or NULL if not found.
 */

AppClass *dlist_lookup(DList * head, AppClass * data, DListFP_Cmp cmp)
{
   if (head == NULL) {
      return NULL;
   }
   DList *node = head->next;

   if (cmp == NULL) {
      msg_fatal("compare function not specified");
   }

   while (node != head) {
      if ((*cmp) (node->data, data) == 0) {
	 return node->data;
      }
      node = node->next;
   }

   /* If we get here, no node was found, return NULL. */

   return NULL;
}

/*
 * a simple node compare function for the iterator.
 */

int dlist_iterator_cmp(AppClass * n1, AppClass * n2)
{
   /*
    * Since this is only used in the iterator, we are guaranteed that it is
    * safe to compare data pointers because both n1 and n2 came from the
    * list.
    */

   return (int) (n1 - n2);
}

/** \brief Iterate over all elements of the list.
 *
 *  For each element, call the user supplied iterator function and pass it the
 *  node data and the user_data. If the iterator function return non-zero,
 *  remove the node from the list.
 *
 *  \param head The head of the list to be iterated.
 *  \param func The function to be applied.
 *  \param user_data The data to be passed to the func when it is applied.
 *
 *  \return An int : return what the function return
 *   func return:
 *   - 0                  = OK all elements visited - return 0
 *   - DLIST_RM_NODE      = remove the node, stop  and return DLIST_RM_NODE
 *   - DLIST_RM_NODE_CONT = remove the node and continue the loop
 *   - any other value    = stop the loop and return the value
 */

int dlist_iterator(DList * head, DListFP_Iter func, void *user_data)
{
   if (func == NULL) {
      msg_fatal("no iteration func supplied");
   }
   if (head == NULL) {
      return 0;
   }
   DList *node = head->next;

   while (node != head) {
      /*
       * WARNING: if next = node->next is placed before the func,
       * we can  suppress the node but we may have problem adding to the list 
       * WARNING: if next = node->next is placed after the func
       * we have problem if node is suppressed by the func,
       * but no problen to add to the list.
       */
      int ret = (*func) (node->data, user_data);
      /* placed here ! : so NEVER destroy the node in the function */
      /* let the following code do the destroy */
      DList *next = node->next;	/* must be before remove node */

      if (ret) {
	 /* remove node also unref the node + node->data */
	 if (ret == DLIST_RM_NODE || ret == DLIST_RM_NODE_CONT) {
	    dlist_remove(node);
	 }
	 if (ret != DLIST_RM_NODE_CONT) {
	    return ret;
	 }
      }
      node = next;
   }

   return 0;
}

/*
 * return number of nodes in list
 */
int dlist_get_nelem(DList * head)
{
   if (dlist_isempty(head)) {
      return 0;
   }
   DList *node = head->next;
   int n = 0;

   while (node != head) {
      n++;
      node = node->next;
   }
   return n;
}

/*
 * get a member of the list at index
 */
AppClass *dlist_get_ndata(DList * head, int n)
{
   if (head == NULL) {
      return NULL;
   }
   DList *node = head->next;
   int i = 0;

   while (node != head) {
      if (i == n) {
	 return node->data;
      }
      i++;
      node = node->next;
   }
   return NULL;
}
