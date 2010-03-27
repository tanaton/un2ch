#ifndef INCLUDE_READ_H
#define INCLUDE_READ_H

#include "unstring.h"
#include "unarray.h"

typedef struct nich_st {
	unstr_t *server;
	unstr_t *board;
	unstr_t *thread;
} nich_t;

typedef struct databox_st {
	unarray_t	*bl;
	unstr_t		*key;
} databox_t;

#endif
