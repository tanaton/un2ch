#ifndef UNARRAY_H_INCLUDE
#define UNARRAY_H_INCLUDE

#include <stdio.h>
#include <stdbool.h>

// 定数
#define UNARRAY_MAIN_SIZE		(16)

typedef struct unarray_st {
	void		**data;
	size_t		length;
	size_t		heap;
} unarray_t;

unarray_t *unarray_init(void);
void unarray_free(unarray_t *array, void (*free_func)(void *));
bool unarray_push(unarray_t *array, void *data);
void* unarray_pop(unarray_t *array);
void* unarray_at(unarray_t *array, size_t at);
bool unarray_insert(unarray_t *array, void *data, size_t at);
bool unarray_delete(unarray_t *array, size_t at, void (*free_func)(void *));

#endif /* UNARRAY_H_INCLUDE */

