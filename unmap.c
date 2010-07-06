#include <stdio.h>
#include <string.h>

#include "unmap.h"
#include "crc32.h"

/* プロトタイプ宣言 */
static void *unmap_malloc(size_t size);
static void *unmap_realloc(void *p, size_t size, size_t len);
static unmap_storage_t *unmap_storage_init(unmap_t *list, size_t tsize, size_t asize, size_t hsize);
static void *unmap_alloc(unmap_t *list, unmap_storage_t *st);
static unmap_data_t *unmap_area_get(unmap_t *list, unmap_tree_t *tree, unmap_box_t *box, size_t level);
static unmap_data_t *unmap_data_next(unmap_t *list, unmap_data_t *data, unmap_box_t *box);
static void unmap_cache_set(unmap_t *list, unmap_data_t *data);
static unmap_data_t *unmap_cache_get(unmap_t *list, unmap_box_t *box);

/* unmap_tオブジェクト生成・初期化 */
unmap_t *unmap_init(size_t max_level, size_t tree_heap_size, size_t data_heap_size)
{
	unmap_t *list = 0;
	if((max_level > sizeof(size_t) * 8) || (max_level < 4)){
		/* 33bit以上3bit以下の場合 */
		return NULL;
	}
	/* 初期値設定 */
	if(tree_heap_size == 0){
		tree_heap_size = 1024;
	}
	if(data_heap_size == 0){
		data_heap_size = 512;
	}
	/* unmap_tオブジェクト確保 */
	list = unmap_malloc(sizeof(unmap_t));
	/* tree領域確保 */
	list->tree_heap = unmap_storage_init(list, sizeof(unmap_tree_t),
		UNMAP_HEAP_ARRAY_SIZE, tree_heap_size);
	list->tree = (unmap_tree_t *)(list->tree_heap->heap[0]);
	list->tree_heap->size_num = 1;
	/* data領域確保 */
	list->data_heap = unmap_storage_init(list, sizeof(unmap_data_t),
		UNMAP_HEAP_ARRAY_SIZE, data_heap_size);
	/* 最大深度セット */
	list->max_level = max_level;
	return list;
}

/* unmap_tオブジェクト開放 */
void unmap_free_func(unmap_t *list)
{
	size_t i = 0;
	size_t j = 0;
	unmap_storage_t *st = 0;
	unmap_data_t *data = 0;
	if((list == NULL) || (list->tree == NULL)) return;
	/* tree開放 */
	st = list->tree_heap;
	for(i = 0; i <= st->length; i++){
		free(st->heap[i]);
		st->heap[i] = NULL;
	}
	free(st->heap);
	free(st);
	list->tree_heap = NULL;
	/* data開放 */
	st = list->data_heap;
	for(j = 0; j <= st->length; j++){
		for(i = 0; i < st->heap_size; i++){
			data = (unmap_data_t *)((char *)st->heap[j] + (st->type_size * i));
			if((data->data != NULL) && (data->free_func != NULL)){
				/* データが格納されている and 関数が登録されいる */
				data->free_func(data->data);
			}
			data->data = NULL;
		}
		free(st->heap[j]);
		st->heap[j] = NULL;
	}
	free(st->heap);
	free(st);
	list->data_heap = NULL;
	/* unmap_t開放 */
	free(list);
}

/* unmap_tオブジェクトに値をセットする */
int unmap_set(unmap_t *list, const char *key, size_t key_size, void *data, int flag, void (*free_func)(void *))
{
	unmap_data_t *tmp = 0;
	unmap_box_t box = {0};
	if((list == NULL) || (key == NULL) || (key_size == 0)){
		return -1;
	}
	/* hashを計算する */
	unmap_hash_create(key, key_size, list->max_level, &box);
	tmp = unmap_area_get(list, list->tree, &box, list->max_level);
	if(tmp->box.hash != box.hash){
		/* 連結リストをたどる */
		tmp = unmap_data_next(list, tmp, &box);
	}
	if((tmp->data != NULL) && (tmp->free_func != NULL)){
		/* すでにデータが格納されている場合、開放する */
		tmp->free_func(tmp->data);
	}
	tmp->data = data;
	tmp->flag = flag;
	tmp->free_func = free_func;
	unmap_cache_set(list, tmp);	/* キャッシュする */
	return 0;
}

/* unmap_tオブジェクトから値を取得 */
unmap_data_t *unmap_get(unmap_t *list, const char *key, size_t key_size)
{
	unmap_data_t *data = 0;
	unmap_box_t box = {0};
	if((list == NULL) || (key == NULL) || (key_size == 0)){
		return NULL;
	}
	/* hashを計算する */
	unmap_hash_create(key, key_size, list->max_level, &box);
	/* キャッシュから探索 */
	data = unmap_cache_get(list, &box);
	if(data == NULL){
		/* unmap_tツリーから探索 */
		data = unmap_area_get(list, list->tree, &box, list->max_level);
		if(data->box.hash != box.hash){
			/* 連結リスト上を探索 */
			data = unmap_data_next(list, data, &box);
		}
		unmap_cache_set(list, data);	/* キャッシュする */
	}
	return data;
}

/* 格納しているデータ数を返す */
size_t unmap_size(unmap_t *list)
{
	unmap_storage_t *st = 0;
	size_t size = 0;
	if((list == NULL) || (list->data_heap == NULL)){
		return 0;
	}
	st = list->data_heap;
	size = (st->heap_size * st->length) + st->size_num;
	return size;
}

/* 要素数指定取得 */
unmap_data_t *unmap_at(unmap_t *list, size_t at)
{
	unmap_storage_t *st = 0;
	unmap_data_t *data = 0;
	size_t size = unmap_size(list);
	size_t i = 0;
	size_t j = 0;
	char *p = 0;

	if(at < size){
		/* 格納数よりも少ない場合 */
		st = list->data_heap;
		i = at / st->heap_size;
		j = at % st->heap_size;
		p = st->heap[i];
		data = (unmap_data_t *)(p + (st->type_size * j));
	} else {
		data = NULL;
	}
	return data;
}

/* エラー処理付きmalloc */
static void *unmap_malloc(size_t size)
{
	void *p = malloc(size);
	if(p == NULL){
		perror("unmap_malloc");
	} else {
		memset(p, 0, size);
	}
	return p;
}

/* エラー処理付きrealloc */
static void *unmap_realloc(void *p, size_t size, size_t len)
{
	p = realloc(p, size);
	if(p == NULL){
		perror("unmap_realloc");
	} else {
		len++;
		if(size > len){
			memset(((char *)p) + len, 0, size - len);
		}
	}
	return p;
}

/* ストレージ初期化 */
static unmap_storage_t *unmap_storage_init(unmap_t *list, size_t tsize, size_t asize, size_t hsize)
{
	unmap_storage_t *st = unmap_malloc(sizeof(unmap_storage_t));
	st->array_size = asize;	/* 一列の長さ */
	st->heap_size = hsize;	/* 一行の長さ(不変) */
	st->type_size = tsize;	/* 型の大きさ */
	st->heap = unmap_malloc(sizeof(void *) * st->array_size);
	st->heap[0] = unmap_malloc(st->type_size * st->heap_size);
	return st;
}

/* ストレージの切り分け */
static void *unmap_alloc(unmap_t *list, unmap_storage_t *st)
{
	char *p = 0;
	if(st->size_num >= st->heap_size){
		/* 用意した領域を使い切った */
		st->size_num = 0;	/* 先頭を指し直す */
		st->length++;		/* 次の領域へ */
		if(st->length >= st->array_size){
			/* 領域管理配列を拡張する */
			st->array_size += st->length >> 1;
			st->heap = unmap_realloc(st->heap, sizeof(void *) * st->array_size, sizeof(void *) * st->length);
		}
		/* 新しい領域を確保する */
		st->heap[st->length] = unmap_malloc(st->type_size * st->heap_size);
	}
	p = st->heap[st->length];
	/* 使用していない領域を返す */
	return p + (st->type_size * st->size_num++);
}

/* unmap_tツリーを生成、値の格納場所を用意 */
static unmap_data_t *unmap_area_get(unmap_t *list, unmap_tree_t *tree, unmap_box_t *box, size_t level)
{
	size_t rl = 0;
	size_t node = box->node;
	unmap_data_t *data = 0;
	while(--level){
		rl = (node >> level) & 0x01;	/* 方向選択 */
		if(tree->tree[rl] == NULL){
			tree->tree[rl] = (unmap_tree_t *)unmap_alloc(list, list->tree_heap);
		}
		tree = tree->tree[rl];
	}
	rl = node & 0x01;	/* 方向選択 */
	/* 最深部 */
	data = tree->tree[rl];
	if(data == NULL){
		/* unmap_data_tオブジェクト生成 */
		data = (unmap_data_t *)unmap_alloc(list, list->data_heap);
		data->box = *box;
		tree->tree[rl] = data;
	}
	/* unmap_data_tオブジェクトを返す */
	return data;
}

/* 連結リストをたどる */
static unmap_data_t *unmap_data_next(unmap_t *list, unmap_data_t *data, unmap_box_t *box)
{
	unmap_data_t *tmp = data->next;
	unmap_data_t *back = data;
	while(tmp != NULL){
		if(tmp->box.hash == box->hash){
			/* それっぽいものを発見 */
			return tmp;
		} else {
			/* 次へ */
			back = tmp;
			tmp = tmp->next;
		}
	}
	/* unmap_data_tオブジェクト生成 */
	tmp = (unmap_data_t *)unmap_alloc(list, list->data_heap);
	tmp->box = *box;
	back->next = tmp;
	return tmp;
}

/* キャッシュにセットする */
static void unmap_cache_set(unmap_t *list, unmap_data_t *data)
{
	list->cache[(data->box.node & UNMAP_CACHE_SIZE)] = data;
}

/* キャッシュから探索 */
static unmap_data_t *unmap_cache_get(unmap_t *list, unmap_box_t *box)
{
	unmap_data_t *data = list->cache[(box->node & UNMAP_CACHE_SIZE)];
	if((data == NULL) || (data->box.hash != box->hash)){
		data = NULL;
	}
	return data;
}

