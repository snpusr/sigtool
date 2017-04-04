#ifndef DB_H
#define DB_H

#ifndef LIST_H
#include "list.h"
#endif

#define DB_MAGIC	0xdeadbeef
#define DB_ITEM		0xc00ffee

#define DB_OPEN				0
#define DB_NEW				1
#define DB_OVERWRITE		2

#define DB_TYPE_FLAT		0
#define DB_TYPE_CRYPTO		2
#define DB_TYPE_COMPRESS	4

enum {
	DB_ERROR_OPEN,
	DB_ERROR_HEADER,
	DB_ERROR_MAGIC,
	DB_ERROR_ITEM_MAGIC,
	DB_ERROR_ITEM_HEADER,
	DB_ERROR_ITEM_DATA,
	DB_ERROR_MALLOC,
	DB_ERROR_NONE
};

typedef struct db {
	unsigned int	magic;
	unsigned int	type;
	int				nitems;
	int				nroot;
	bb_list_t		*items;
	int				fd;
	unsigned char	error;
} db_t;

typedef struct db_item {
	int				id;
	int				root;
	char			name[64];
	unsigned int	size;
	int				child;
	unsigned char	*data;
} db_item_t;

int 		db_open(db_t *db, const char *filename, int mode);
int 		db_close(db_t *db, int save);

void 		db_item_add(db_t *db, char *name, int root, void *data, int size);
db_item_t 	*db_item_get(db_t *, char *, int);
char		*db_strerror(db_t *db);

#endif
