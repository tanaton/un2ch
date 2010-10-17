#ifndef INCLUDE_UNMAP_H
#define INCLUDE_UNMAP_H

#include <stdlib.h>

#define UNMAP_HEAP_ARRAY_SIZE			(2)
#define UNMAP_HEAP_EXTENSION_SIZE(n)	((n) * 2)

#define unmap_free(unmap, free_func)			\
	do { unmap_free_func((unmap), (free_func)); (unmap) = NULL; } while(0)

/* 型の種類 */
typedef enum {
	UNMAP_TYPE_NONE = 0,
	UNMAP_TYPE_DATA,
	UNMAP_TYPE_TREE
} unmap_type_enum_t;

/* 32(64)bitハッシュ値 */
typedef unsigned long unmap_hash_t;

/* ハッシュ値詰め合わせ */
typedef struct unmap_box_st {
	unmap_hash_t hash;	/* ハッシュ値 */
	unmap_hash_t node;	/* ノード値(ハッシュ値から算出) */
} unmap_box_t;

/* 木とデータの混合 */
typedef struct unmap_mixed_st {
	unmap_type_enum_t type;
	void *mixed;
} unmap_mixed_t;

/* 木構造 */
typedef struct unmap_tree_st {
	unmap_mixed_t tree[2];		/* 枝を二つまで管理 */
} unmap_tree_t;

/* 連結リスト */
typedef struct unmap_data_st {
	unmap_box_t box;			/* ハッシュ値 */
	void *data;					/* 保存データ */
	struct unmap_data_st *next;	/* 次のリスト */
} unmap_data_t;

/* メモリ空間管理 */
typedef struct unmap_storage_st {
	void **heap;				/* メモリ */
	size_t heap_size;			/* 一度に確保するサイズ */
	size_t list_size;			/* 確保する最大回数 */
	size_t list_index;			/* 参照しているメモリブロック */
	size_t data_index;			/* 参照しているメモリアドレス */
	size_t type_size;			/* 管理する型のサイズ */
} unmap_storage_t;

/* unmap情報管理 */
typedef struct unmap_st {
	unmap_storage_t *tree_heap;	/* メモリ空間ツリー用 */
	unmap_storage_t *data_heap;	/* メモリ空間データ用 */
	unmap_tree_t *tree;			/* 最初の枝 */
	size_t max_level;			/* 最大階層 */
} unmap_t;

/* プロトタイプ宣言 */
extern unmap_t *unmap_init(size_t max_level, size_t tree_heap_size, size_t data_heap_size);
extern void unmap_free_func(unmap_t *list, void (*free_func)(void *));
extern int unmap_set(unmap_t *list, const char *key, size_t key_size, void *data, void (*free_func)(void *));
extern void *unmap_get(unmap_t *list, const char *key, size_t key_size);
extern size_t unmap_size(unmap_t *list);
extern void *unmap_at(unmap_t *list, size_t at);

#endif /* INCLUDE_UNMAP_H */
