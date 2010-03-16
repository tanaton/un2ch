#include "unarray.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

/* 非公開関数 */
static void *unarray_malloc(size_t size);
static void *unarray_realloc(void *data, size_t size, size_t len);
static size_t unsizeof(size_t size);
static void unarray_alloc_memory(unarray_t *array, size_t size);

static void *unarray_malloc(size_t size)
{
	void *data = malloc(size);
	if(data == NULL){
		perror("unarray_malloc");
	} else {
		memset(data, 0, size);
	}
	return data;
}

static void *unarray_realloc(void *data, size_t size, size_t len)
{
	data = realloc(data, size);
	if(data == NULL){
		perror("unarray_realloc");
	} else {
		memset(((char *)data) + len, 0, size - len);
	}
	return data;
}

static size_t unsizeof(size_t size)
{
	return sizeof(void *) * size;
}

unarray_t *unarray_init(void)
{
	unarray_t *array = unarray_malloc(sizeof(unarray_t));
	memset(array, 0, sizeof(unarray_t));
	array->data = unarray_malloc(unsizeof(UNARRAY_MAIN_SIZE));
	memset(array->data, 0, unsizeof(UNARRAY_MAIN_SIZE));
	array->heap = UNARRAY_MAIN_SIZE;
	return array;
}

void unarray_free_func(unarray_t *array, void (*free_func)(void *))
{
	int i = 0;
	if(array != NULL){
		for(i = 0; i < array->heap; i++){
			if(array->data[i] != NULL && free_func != NULL){
				free_func(array->data[i]);
			}
		}
		free(array->data);
		array->heap = 0;
		array->length = 0;
	}
	free(array);
}

static void unarray_alloc_memory(unarray_t *array, size_t size)
{
	if((array->length + size) >= array->heap){
		array->heap = (((array->heap + size) / UNARRAY_MAIN_SIZE) + 1) * UNARRAY_MAIN_SIZE;
		array->data = unarray_realloc(array->data, unsizeof(array->heap), unsizeof(array->length));
	}
}

bool unarray_push(unarray_t *array, void *data)
{
	if(!array) return false;
	unarray_alloc_memory(array, 1);
	array->data[array->length] = data;
	array->length++;
	return true;
}

void* unarray_pop(unarray_t *array)
{
	void *data = 0;
	if(!array) return NULL;
	if(array->length == 0) return NULL;
	data = array->data[0];
	if(array->length >= 2){
		memmove(array->data, array->data + 1, unsizeof(array->length - 1));
	}
	array->length--;
	array->data[array->length] = NULL;
	return data;
}

void* unarray_at(unarray_t *array, size_t at)
{
	if(!array) return NULL;
	if(at >= array->length) return NULL;
	return array->data[at];
}

bool unarray_insert(unarray_t *array, void *data, size_t at)
{
	if(array == NULL || array->data == NULL) return false;
	unarray_alloc_memory(array, 1);
	if(at >= array->length){
		unarray_push(array, data);
	} else {
		memmove(array->data + at + 1,
				array->data + at,
				unsizeof(array->length - at));
		array->data[at] = data;
		array->length++;
	}
	return true;
}

bool unarray_delete(unarray_t *array, size_t at, void (*free_func)(void *))
{
	if(array == NULL || array->data == NULL) return false;
	if(at >= array->length) return false;
	unarray_alloc_memory(array, 1);
	if(at == 0){
		void *data = unarray_pop(array);
		if(free_func){
			free_func(data);
		}
	} else {
		if(free_func){
			free_func(array->data[at]);
		}
		memmove(array->data + at,
				array->data + at + 1,
				unsizeof(array->length - at));
		array->length--;
	}
	return true;
}
