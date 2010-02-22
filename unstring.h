/*
 * unstring.h
 */
#ifndef UNSTRING_H_INCLUDE
#define UNSTRING_H_INCLUDE

#include <stdlib.h>

#define UNSTRING_HEAP_SIZE				256
#define unstr_free(str)				\
	unstr_free_func(str); (str) = NULL;

typedef struct unstr_st {
	char *data;
	size_t length;
	size_t heap;
} unstr_t;

void unstr_alloc(unstr_t *str, size_t size);
unstr_t *unstr_init(const char *str);
unstr_t *unstr_init_memory(size_t size);
int unstr_check_heap_size(unstr_t *str, size_t size);
void unstr_free_func(unstr_t *str);
void unstr_delete(size_t size, ...);
void unstr_zero(unstr_t *str);
unstr_t *unstr_copy(unstr_t *str);
unstr_t *unstr_substr(unstr_t *s1, unstr_t *s2, size_t len);
unstr_t *unstr_substr_char(const char *str, size_t len);
unstr_t *unstr_strcat(unstr_t *s1, unstr_t *s2);
unstr_t *unstr_strcat_char(unstr_t *str, const char *c);
int unstr_strcmp(unstr_t *str1, unstr_t *str2);
unstr_t *unstr_split(unstr_t *str, const char *tmp, char c);
unstr_t *unstr_sprintf(unstr_t *str, const char *format, ...);
size_t unstr_sscanf(unstr_t *data, const char *format, ...);
unstr_t *unstr_reverse(unstr_t *str);
unstr_t *unstr_itoa(int num, int kisu, int moji);
unstr_t *unstr_file_get_contents(unstr_t *filename);
void unstr_file_put_contents(unstr_t *filename, unstr_t *data, const char *mode);
unstr_t *unstr_replace(unstr_t *data, unstr_t *search, unstr_t *replace);
size_t *unstr_quick_search(unstr_t *text, unstr_t *search, size_t *size);

#endif /* UNSTRING_H_INCLUDE */
