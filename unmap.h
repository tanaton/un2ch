/************************************************************
 * Copyright (C) 2009,  tanaton
 *
 * Filename:	unmap.h
 * Version:		0.10
 * Url:			http://d.hatena.ne.jp/heiwaboke/
*************************************************************/

#ifndef INCLUDE_UNMAP_H
#define INCLUDE_UNMAP_H

#include <stdlib.h>
#include <setjmp.h>

#define UNMAP_PRIMES_TABLE			(257)
#define UNMAP_HEAP_ARRAY_SIZE		(16)
#define UNMAP_CACHE_SIZE			(0x0F)

#define unmap_free(unmap)	do { unmap_free_func(unmap); (unmap) = NULL; } while(0)

/* 32(64)bit�n�b�V���l */
typedef unsigned long unmap_hash_t;

/* �n�b�V���l�l�ߍ��킹 */
typedef struct unmap_box_st {
	unmap_hash_t hash;	/* �n�b�V���l */
	unmap_hash_t sum;	/* �m�F�p */
	unmap_hash_t node;	/* �m�[�h�l(�n�b�V���l����Z�o) */
} unmap_box_t;

/* �؍\�� */
typedef struct unmap_tree_st {
	void *tree[2];		/* �}���܂ŊǗ� */
} unmap_tree_t;

/* �A�����X�g */
typedef struct unmap_data_st {
	unmap_box_t box;				/* �n�b�V���l */
	int flag;					/* �t���[(�p�r����) */
	void (*free_func)(void *);	/* �J���p�֐� */
	void *data;					/* �ۑ��f�[�^ */
	struct unmap_data_st *next;	/* ���̃��X�g */
} unmap_data_t;

/* ��������ԊǗ� */
typedef struct unmap_storage_st {
	void **heap;				/* ������ */
	size_t heap_size;			/* ��x�Ɋm�ۂ���T�C�Y */
	size_t array_size;			/* �m�ۂ���ő�� */
	size_t length;				/* �Q�Ƃ��Ă��郁�����u���b�N */
	size_t size_num;			/* �Q�Ƃ��Ă��郁�����A�h���X */
	size_t type_size;			/* �Ǘ�����^�̃T�C�Y */
} unmap_storage_t;

/* unmap���Ǘ� */
typedef struct unmap_st {
	unmap_storage_t *tree_heap;	/* ��������ԃc���[�p */
	unmap_storage_t *data_heap;	/* ��������ԃf�[�^�p */
	unmap_tree_t *tree;			/* �ŏ��̎} */
	unmap_data_t *cache[UNMAP_CACHE_SIZE + 1];	/* �L���b�V�� */
	size_t max_level;			/* �ő�K�w */
	jmp_buf j_buf;				/* �W�����v�o�b�t�@ */
} unmap_t;

/* �v���g�^�C�v�錾 */
unmap_t *unmap_init(size_t max_level, size_t tree_heap_size, size_t data_heap_size);
void unmap_free_func(unmap_t *list);
int unmap_set(unmap_t *list, const char *key, size_t key_size, void *data, int flag, void (*free_func)(void *));
unmap_data_t *unmap_get(unmap_t *list, const char *key, size_t key_size);
size_t unmap_size(unmap_t *list);
unmap_data_t *unmap_at(unmap_t *list, size_t at);

#endif /* INCLUDE_UNMAP_H */
