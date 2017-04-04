/*
 * beardbastards source code
*/
#ifndef BB_LIST_H
#define BB_LIST_H

#include <stdio.h>
#include <stdlib.h>

typedef struct bb_list 		bb_list_t;
typedef struct bb_list_node	bb_list_node_t;

struct bb_list {
	bb_list_node_t		*head, *tail;
};

struct bb_list_node {
	void				*dptr;
	bb_list_node_t		*next, *prev;
};

bb_list_node_t *bb_list_node_alloc(void *);
void bb_list_add(bb_list_t **, bb_list_node_t *);
void bb_list_insert(bb_list_t **, bb_list_node_t *, bb_list_node_t *);
void bb_list_del(bb_list_t **, bb_list_node_t *);
bb_list_node_t *bb_list_find_next(bb_list_t **, void *);
bb_list_node_t *bb_list_find_prev(bb_list_t **, void *);
void bb_list_print(bb_list_t **, unsigned char);

#endif
