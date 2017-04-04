/*
 * alihack - signature creating/searching tool
 * copyright (c) 2011, snape
 * basicblock linked list
 *
*/
#include "list.h"

bb_list_node_t *bb_list_node_alloc(void *dptr)
{
	bb_list_node_t *node;
   
	if((node = (bb_list_node_t *)malloc(sizeof(bb_list_node_t))) == NULL)
		return (NULL);

	node->dptr = dptr;
	node->next = node->prev = NULL;
	return (node);
}

void bb_list_add(bb_list_t **list, bb_list_node_t *new)
{
	if((*list) == NULL) {
		*list = (bb_list_t *)malloc(sizeof(bb_list_t));
		(*list)->head = (*list)->tail = NULL;
	}
	if((*list)->head == NULL) {
		(*list)->head = new;
		new->prev = NULL;
	} else {
		(*list)->tail->next = new;
		new->prev = (*list)->tail;
	}
	(*list)->tail = new;
	new->next = NULL;
}

void bb_list_insert(bb_list_t **list, bb_list_node_t *new, bb_list_node_t *after) 
{
	new->next = after->next;
	new->prev = after;

	if(after->next != NULL) {
		after->next->prev = new;
	} else {
		(*list)->tail = new;
	}

	after->next = new;
}

void bb_list_del(bb_list_t **list, bb_list_node_t *del)
{
	if(del->prev == NULL) {
		(*list)->head = del->next;
	} else {
		del->prev->next = del->next;
	}
	if(del->next == NULL) {
		(*list)->tail = del->prev;
	} else {
		del->next->prev = del->prev;
	}
	free(del);
}

bb_list_node_t *bb_list_find_next(bb_list_t **list, void *dptr)
{
	bb_list_node_t *next;
	for(next = (*list)->head; next; next = next->next) {
		if(next->dptr == dptr)
			return (next);
	}
	return (NULL);
}

bb_list_node_t *bb_list_find_prev(bb_list_t **list, void *dptr)
{
	bb_list_node_t *prev;
	for(prev = (*list)->tail; prev; prev = prev->prev) {
		if(prev->dptr == dptr)
			return (prev);;
	}
	return (NULL);
}

#ifdef DEBUG_LIST
void bb_list_print(bb_list_t **list, unsigned char dir)
{
	bb_list_node_t *p;
	for(p = (!dir ? (*list)->head : (*list)->tail); p; p = (!dir ? p->next : p->prev))
		printf("%d\n", *(int *)p->dptr);
}

int main(int argc, char **arv)
{
	int 	one, *on[23];

	bb_list_t 			*global;
	bb_list_node_t 	*p;

	for(one = 0; one < 10; one++) {
		on[one] = malloc(sizeof(int));
		memcpy(on[one], &one, 4);
		bb_list_add(&global, bb_list_node_alloc(on[one]));
	}

	p = bb_list_find_next(&global, on[5]);
	printf("===========================================\n");
	bb_list_print(&global, 1);
	printf("===========================================\n");
	p = bb_list_find_next(&global, on[7]);
	bb_list_insert(&global, bb_list_node_alloc(on[5]), p);
	bb_list_print(&global, 1);
	
	exit(0);

}
#endif
