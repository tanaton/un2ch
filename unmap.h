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

#define unmap_free(unmap)			\
	do { unmap_free_func(unmap); (unmap) = NULL; } while(0)

/* 32(64)bitハッシュ値 */
typedef unsigned long unmap_hash_t;

/* 木構造 */
typedef struct unmap_tree_st {
	void *tree[2];		/* 枝を二つまで管理 */
} unmap_tree_t;

/* 連結リスト */
typedef struct unmap_data_st {
	unmap_hash_t hash;			/* ハッシュ値 */
	int flag;					/* フリー(用途無し) */
	void (*free_func)(void *);	/* 開放用関数 */
	void *data;					/* 保存データ */
	struct unmap_data_st *next;	/* 次のリスト */
} unmap_data_t;

/* メモリ空間管理 */
typedef struct unmap_storage_st {
	void **heap;				/* メモリ */
	size_t heap_size;			/* 一度に確保するサイズ */
	size_t array_size;			/* 確保する最大回数 */
	size_t length;				/* 参照しているメモリブロック */
	size_t size_num;			/* 参照しているメモリアドレス */
	size_t type_size;			/* 管理する型のサイズ */
} unmap_storage_t;

/* unmap情報管理 */
typedef struct unmap_st {
	unmap_storage_t *tree_heap;	/* メモリ空間ツリー用 */
	unmap_storage_t *data_heap;	/* メモリ空間データ用 */
	unmap_tree_t *tree;			/* 最初の枝 */
	unmap_data_t *cache[UNMAP_CACHE_SIZE + 1];	/* キャッシュ */
	size_t max_level;			/* 最大階層 */
	jmp_buf j_buf;				/* ジャンプバッファ */
} unmap_t;

/* プロトタイプ宣言 */
unmap_t *unmap_init(size_t max_level, size_t tree_heap_size, size_t data_heap_size);
void unmap_free_func(unmap_t *list);
int unmap_set(unmap_t *list, const char *key, size_t key_size, void *data, int flag, void (*free_func)(void *));
unmap_data_t *unmap_get(unmap_t *list, const char *key, size_t key_size);
size_t unmap_size(unmap_t *list);
unmap_data_t *unmap_at(unmap_t *list, size_t at);

#endif /* INCLUDE_UNMAP_H */
