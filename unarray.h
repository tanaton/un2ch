#ifndef UNARRAY_H_INCLUDE
#define UNARRAY_H_INCLUDE

#include <stdio.h>

#define UNARRAY_MAIN_SIZE		(16)
#define unarray_free(array, func)		\
	do { unarray_free_func((array), (func)); (array) = NULL; } while(0)

typedef enum {
	UNARRAY_OK	= 0,
	UNARRAY_NULL,
	UNARRAY_NOTHING_DATA,
	UNARRAY_OVER_LENGTH
} unarray_code_t;

typedef struct unarray_st {
	void		**data;
	size_t		length;
	size_t		heap;
} unarray_t;

unarray_t *unarray_init(size_t size);
void unarray_free_func(unarray_t *array, void (*free_func)(void *));
unarray_code_t unarray_push(unarray_t *array, void *data);
void* unarray_pop(unarray_t *array);
void* unarray_at(unarray_t *array, size_t at);
size_t unarray_size(unarray_t *array);
unarray_code_t unarray_insert(unarray_t *array, void *data, size_t at);
unarray_code_t unarray_delete(unarray_t *array, size_t at, void (*free_func)(void *));

#endif /* UNARRAY_H_INCLUDE */

