/* 
 * alihack - lazy db api
 * support fast encryption/compression
 * copyright (c) 2011, beardbastard.
 * 10/08/2011
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdarg.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <assert.h>
#include "db.h"

int debug_flag = 0;

void debug(const char *fmt, ...)
{
	va_list ap;
	char	buffer[1024];

	va_start(ap, fmt);
	vsnprintf(buffer, sizeof(buffer), fmt, ap);
	va_end(ap);

	if(debug_flag)
		fprintf(stderr, "%s", buffer);
}

char *db_strerror(db_t *db)
{
	switch(db->error) {
		case DB_ERROR_OPEN:
			return ("DB_ERROR_HEADER");
		case DB_ERROR_MAGIC:
			return ("DB_ERROR_MAGIC");
		case DB_ERROR_ITEM_MAGIC:
			return ("DB_ERROR_ITEM_MAGIC");
		case DB_ERROR_ITEM_HEADER:
			return ("DB_ERROR_ITEM_HEADER");
		case DB_ERROR_ITEM_DATA:
			return ("DB_ERROR_ITEM_DATA");
		case DB_ERROR_MALLOC:
			return ("DB_ERROR_MALLOC");
		case DB_ERROR_NONE:
			return ("DB_ERROR_NONE");
	}
	return ("DB_ERROR_UNKNOWN");
}

int db_open(db_t *db, const char *filename, int mode)
{
	int				c, i, icount = -1;
	unsigned int 	magic;
	db_item_t		*item = NULL;
	struct stat st;

	db->nitems = -1;
	db->fd = 0;
	db->items = NULL;
	db->nroot = 0;

	if(mode == DB_OPEN) {

		if((db->fd = open(filename, O_RDONLY)) < 0) {
			db->error = DB_ERROR_OPEN;
			return (-1);
		}

		debug("checking magic: ");
		if(read(db->fd, (char *)&magic, 4) != 4) {
			db->error = DB_ERROR_MAGIC;
			debug("failed!\n");
			goto rdout;
		}

		if(magic != DB_MAGIC) { 
			db->error = DB_ERROR_HEADER;
			debug("failed!\n");
			goto rdout;
		}
		debug("ok!\nchecking items: ");

		if(read(db->fd, (char *)&db->nitems, 4) != 4) {
			db->error = DB_ERROR_HEADER;
			debug("failed\n");
			goto rdout;
		}
		
		if(read(db->fd, (char *)&db->nroot, 4) != 4) {
			db->error = DB_ERROR_HEADER;
			debug("failed\n");
			goto rdout;
		}

		if(db->nitems < 0 || db->nroot < 0) {
			db->error = DB_ERROR_HEADER;
			debug("failed\n");
			goto rdout;
		}

		for(icount = i = 0; i < db->nitems; icount++, i++) {
			debug("\rparsing item %-10d/%-10d", icount, db->nitems);
			if((item = (db_item_t *)malloc(sizeof(db_item_t))) == NULL)  {
				db->error = DB_ERROR_MALLOC;
				break;
			}
			if(read(db->fd, (char *)&magic, 4) != 4) {
				db->error = DB_ERROR_ITEM_HEADER;
				break;
			}
			if(magic != DB_ITEM) {
				db->error = DB_ERROR_ITEM_MAGIC;
				break;
			}

			if((item = (db_item_t *)malloc(sizeof(db_item_t))) == NULL) {
				db->error = DB_ERROR_MALLOC;
				break;
			}
			
			if(((c = read(db->fd, (char *)item, sizeof(db_item_t) - 8)) != (sizeof(db_item_t) - 8)) || c < 0) {
				db->error = DB_ERROR_ITEM_HEADER;
				break;
			}
	
			if((item->data = (unsigned char *)malloc(item->size + 1)) == NULL) {
				db->error = DB_ERROR_MALLOC;
				break;
			}

			if((c = read(db->fd, item->data, item->size)) != item->size) {
				db->error = DB_ERROR_ITEM_DATA;
				break;
			}
			bb_list_add(&db->items, bb_list_node_alloc((void *)item));
		}

rdout:
		if(icount != db->nitems) {
			close(db->fd);
			icount = -1;
		}
		return (icount);

	} else if(mode == DB_NEW || mode == DB_OVERWRITE) {

		if(mode == DB_NEW)
			if(stat(filename, &st) >= 0)
				return (DB_ERROR_OPEN);

		if((db->fd = open(filename, O_CREAT | O_RDWR)) < 0)
			return (-1);

		fchmod(db->fd, 0644);

		db->nitems 	= 0;
		db->items 	= NULL;
		db->error 	= DB_ERROR_NONE;

		return (1);
	}

	return (-1);
}

void db_item_add(db_t *db, char *name, int root, void *data, int size)
{
	db_item_t *item = (db_item_t *)malloc(sizeof(db_item_t));
	assert((item != NULL));

	if(name == NULL)
		name = "null";

	memset(item->name, 0x00, sizeof(item->name));
	sprintf(item->name, "%s", name);
	item->size = size;
	item->data = data;
	item->root = root;
	item->child = 0;

	db->nitems++;

	if(!root) 
		db->nroot++;

	bb_list_add(&db->items, bb_list_node_alloc((void *)item));
	db->error = DB_ERROR_NONE;
}

int db_item_set_data(db_item_t *item, void *data, int size)
{
	if(item->data && item->size) 
		free(item->data);
	else if(item->size <= 0)
		return (0);

	item->data = data;
	item->size = size;

	return (1);
}

db_item_t *db_item_get(db_t *db, char *name, int start)
{
	bb_list_node_t 		*node;
	db_item_t			*item = NULL;
	static db_item_t	*last = NULL;
	int 				noname = 0;

	if(start)
		last = NULL;

	if(name == NULL)
		noname = 1;

	for(node = db->items->head; node; node = node->next) {
		if((item = (db_item_t *)node->dptr) == NULL) 
			continue;
		if(noname) {
			if(last == NULL) {
				last = item;
				return (item);
			}
			if(last == item) {
				if(node->next == NULL)
					return (NULL);
				item = (db_item_t *)node->next->dptr;
				last = item;
				return (item);
			} else 
				continue;
		}

		if(!strcmp(item->name, name)) {
			if(last == NULL) {
				last = item;
				return (item);
			}
			if(last == item) {
				if(node->next == NULL) {
					return (NULL);
				}
				item = (db_item_t *)node->next->dptr;
				if(!strcmp(item->name, name)) {
					last = item;
					return (item);
				} else continue;
			}
		}
	}
	last = NULL;
	return (NULL);
}

int db_close(db_t *db, int save)
{
	int 			i;
	int				magic = DB_ITEM;
	bb_list_node_t	*node;
	db_item_t 		*item;

	if(save) {
		db->magic = DB_MAGIC;
		write(db->fd, (char *)&db->magic, 4);
		write(db->fd, (char *)&db->nitems, 4);
		write(db->fd, (char *)&db->nroot, 4);
		for(i = 0, node = db->items->head; i < db->nitems; node = node->next, i++) {
			if((item = (db_item_t *)node->dptr) != NULL) {
				write(db->fd, (char *)&magic, 4);
				write(db->fd, (char *)&i, 4);
				write(db->fd, (char *)&item->root, 4);
				write(db->fd, (char *)item->name, sizeof(item->name));
				write(db->fd, (char *)&item->size, sizeof(int));
				write(db->fd, (char *)item->data, item->size);
				free(item);
			}
		}
	}
	close(db->fd);
	return (0);
}
