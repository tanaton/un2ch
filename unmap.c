/************************************************************
 * Copyright (C) 2009,  tanaton
 *
 * Filename:	unmap.c
 * Version:		0.10
 * Url:			http://d.hatena.ne.jp/heiwaboke/
*************************************************************/

#include <stdio.h>
#include <string.h>

#include "unmap.h"
#include "crc32.h"

/* �v���g�^�C�v�錾 */
static void *unmap_malloc(unmap_t *list, size_t size);
static void *unmap_realloc(unmap_t *list, void *p, size_t size);
static unmap_storage_t *unmap_storage_init(unmap_t *list, size_t tsize, size_t asize, size_t hsize);
static void *unmap_alloc(unmap_t *list, unmap_storage_t *st);
static unmap_data_t *unmap_area_get(unmap_t *list, unmap_tree_t *tree, unmap_box_t *box, size_t level);
static unmap_data_t *unmap_data_next(unmap_t *list, unmap_data_t *data, unmap_box_t *box);
static void unmap_cache_set(unmap_t *list, unmap_data_t *data);
static unmap_data_t *unmap_cache_get(unmap_t *list, unmap_box_t *box);

/* unmap_t�I�u�W�F�N�g�����E������ */
unmap_t *unmap_init(size_t max_level, size_t tree_heap_size, size_t data_heap_size)
{
	unmap_t *list = 0;
	if((max_level > sizeof(size_t) * 8) || (max_level < 4)){
		/* 33bit�ȏ�3bit�ȉ��̏ꍇ */
		return NULL;
	}
	/* �����l�ݒ� */
	if(tree_heap_size == 0){
		tree_heap_size = 1024;
	}
	if(data_heap_size == 0){
		data_heap_size = 512;
	}
	/* unmap_t�I�u�W�F�N�g�m�� */
	list = malloc(sizeof(unmap_t));
	if(list == NULL){
		perror("unmap_init");
		return NULL;
	}
	/* �[���N���A���Ă��� */
	memset(list, 0, sizeof(unmap_t));

	if(setjmp(list->j_buf) == 0){
		/* tree�̈�m�� */
		list->tree_heap = unmap_storage_init(list, sizeof(unmap_tree_t),
			UNMAP_HEAP_ARRAY_SIZE, tree_heap_size);
		list->tree = (unmap_tree_t *)(list->tree_heap->heap[0]);
		list->tree_heap->size_num = 1;
		/* data�̈�m�� */
		list->data_heap = unmap_storage_init(list, sizeof(unmap_data_t),
			UNMAP_HEAP_ARRAY_SIZE, data_heap_size);
		/* �ő�[�x�Z�b�g */
		list->max_level = max_level;
	} else {
		list = NULL;
	}
	return list;
}

/* unmap_t�I�u�W�F�N�g�J�� */
void unmap_free_func(unmap_t *list)
{
	size_t i = 0;
	size_t j = 0;
	size_t length = 0;
	unmap_storage_t *st = 0;
	unmap_data_t *data = 0;
	if((list == NULL) || (list->tree == NULL)) return;
	/* tree�J�� */
	st = list->tree_heap;
	for(i = 0; i <= st->length; i++){
		free(st->heap[i]);
		st->heap[i] = NULL;
	}
	free(st->heap);
	free(st);
	list->tree_heap = NULL;
	/* data�J�� */
	st = list->data_heap;
	for(j = 0; j <= st->length; j++){
		if(j == st->length){
			length = st->size_num;
		} else {
			length = st->heap_size;
		}
		for(i = 0; i < length; i++){
			data = (unmap_data_t *)((char *)st->heap[j] + (st->type_size * i));
			if((data->data != NULL) && (data->free_func != NULL)){
				/* �f�[�^���i�[����Ă��� and �֐����o�^���ꂢ�� */
				data->free_func(data->data);
				data->data = NULL;
			}
		}
		free(st->heap[j]);
		st->heap[j] = NULL;
	}
	free(st->heap);
	free(st);
	list->data_heap = NULL;
	/* unmap_t�J�� */
	free(list);
}

/* unmap_t�I�u�W�F�N�g�ɒl���Z�b�g���� */
int unmap_set(unmap_t *list, const char *key, size_t key_size, void *data, int flag, void (*free_func)(void *))
{
	unmap_data_t *tmp = 0;
	unmap_box_t box = {0};
	if((list == NULL) || (key == NULL) || (key_size == 0)){
		return -1;
	}
	/* hash���v�Z���� */
	unmap_hash_create(key, key_size, list->max_level, &box);
	if(setjmp(list->j_buf) == 0){
		tmp = unmap_area_get(list, list->tree, &box, list->max_level);
		if(tmp->box.hash != box.hash){
			/* �A�����X�g�����ǂ� */
			tmp = unmap_data_next(list, tmp, &box);
		}
	} else {
		return -1;
	}
	if((tmp->data != NULL) && (tmp->free_func != NULL)){
		/* ���łɃf�[�^���i�[����Ă���ꍇ�A�J������ */
		tmp->free_func(tmp->data);
	}
	tmp->data = data;
	tmp->flag = flag;
	tmp->free_func = free_func;
	unmap_cache_set(list, tmp);	/* �L���b�V������ */
	return 0;
}

/* unmap_t�I�u�W�F�N�g����l���擾 */
unmap_data_t *unmap_get(unmap_t *list, const char *key, size_t key_size)
{
	unmap_data_t *data = 0;
	unmap_box_t box = {0};
	if((list == NULL) || (key == NULL) || (key_size == 0)){
		return NULL;
	}
	/* hash���v�Z���� */
	unmap_hash_create(key, key_size, list->max_level, &box);
	/* �L���b�V������T�� */
	data = unmap_cache_get(list, &box);
	if(data == NULL){
		/* unmap_t�c���[����T�� */
		if(setjmp(list->j_buf) == 0){
			data = unmap_area_get(list, list->tree, &box, list->max_level);
			if(data->box.hash != box.hash){
				/* �A�����X�g���T�� */
				data = unmap_data_next(list, data, &box);
			}
			unmap_cache_set(list, data);	/* �L���b�V������ */
		} else {
			return NULL;
		}
	}
	return data;
}

/* �i�[���Ă���f�[�^����Ԃ� */
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

/* �v�f���w��擾 */
unmap_data_t *unmap_at(unmap_t *list, size_t at)
{
	unmap_storage_t *st = 0;
	unmap_data_t *data = 0;
	size_t size = unmap_size(list);
	size_t i = 0;
	size_t j = 0;
	char *p = 0;

	if(at < size){
		/* �i�[���������Ȃ��ꍇ */
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

/* �G���[�����t��malloc */
static void *unmap_malloc(unmap_t *list, size_t size)
{
	void *p = malloc(size);
	if(p == NULL){
		perror("unmap_malloc");
		longjmp(list->j_buf, 1);
		/* NOTREACHED */
	}
	return p;
}

/* �G���[�����t��realloc */
static void *unmap_realloc(unmap_t *list, void *p, size_t size)
{
	p = realloc(p, size);
	if(p == NULL){
		perror("unmap_realloc");
		longjmp(list->j_buf, 1);
		/* NOTREACHED */
	}
	return p;
}

/* �X�g���[�W������ */
static unmap_storage_t *unmap_storage_init(unmap_t *list, size_t tsize, size_t asize, size_t hsize)
{
	unmap_storage_t *st = unmap_malloc(list, sizeof(unmap_storage_t));
	memset(st, 0, sizeof(unmap_storage_t));
	st->array_size = asize;	/* ���̒��� */
	st->heap_size = hsize;	/* ��s�̒���(�s��) */
	st->type_size = tsize;	/* �^�̑傫�� */
	st->heap = unmap_malloc(list, sizeof(void *) * st->array_size);
	st->heap[0] = unmap_malloc(list, st->type_size * st->heap_size);
	memset(st->heap[0], 0, st->type_size * st->heap_size);
	return st;
}

/* �X�g���[�W�̐؂蕪�� */
static void *unmap_alloc(unmap_t *list, unmap_storage_t *st)
{
	char *p = 0;
	if(st->size_num >= st->heap_size){
		/* �p�ӂ����̈���g���؂��� */
		void *heap = 0;
		st->size_num = 0;	/* �擪���w������ */
		st->length++;		/* ���̗̈�� */
		if(st->length >= st->array_size){
			/* �̈�Ǘ��z����g������ */
			st->array_size *= 2;
			st->heap = unmap_realloc(list, st->heap, sizeof(void *) * st->array_size);
		}
		/* �V�����̈���m�ۂ��� */
		heap = unmap_malloc(list, st->type_size * st->heap_size);
		memset(heap, 0, st->type_size * st->heap_size);
		st->heap[st->length] = heap;
	}
	p = st->heap[st->length];
	/* �g�p���Ă��Ȃ��̈��Ԃ� */
	return p + (st->type_size * st->size_num++);
}

/* unmap_t�c���[�𐶐��A�l�̊i�[�ꏊ��p�� */
static unmap_data_t *unmap_area_get(unmap_t *list, unmap_tree_t *tree, unmap_box_t *box, size_t level)
{
	size_t rl = 0;
	size_t node = box->node;
	unmap_data_t *data = 0;
	while(--level){
		rl = (node >> level) & 0x01;	/* �����I�� */
		if(tree->tree[rl] == NULL){
			tree->tree[rl] = (unmap_tree_t *)unmap_alloc(list, list->tree_heap);
		}
		tree = tree->tree[rl];
	}
	rl = node & 0x01;	/* �����I�� */
	/* �Ő[�� */
	data = tree->tree[rl];
	if(data == NULL){
		/* unmap_data_t�I�u�W�F�N�g���� */
		data = (unmap_data_t *)unmap_alloc(list, list->data_heap);
		data->box = *box;
		tree->tree[rl] = data;
	}
	/* unmap_data_t�I�u�W�F�N�g��Ԃ� */
	return data;
}

/* �A�����X�g�����ǂ� */
static unmap_data_t *unmap_data_next(unmap_t *list, unmap_data_t *data, unmap_box_t *box)
{
	unmap_data_t *tmp = data->next;
	unmap_data_t *back = data;
	while(tmp != NULL){
		if(tmp->box.hash == box->hash){
			/* ������ۂ����̂𔭌� */
			return tmp;
		} else {
			/* ���� */
			back = tmp;
			tmp = tmp->next;
		}
	}
	/* unmap_data_t�I�u�W�F�N�g���� */
	tmp = (unmap_data_t *)unmap_alloc(list, list->data_heap);
	tmp->box = *box;
	back->next = tmp;
	return tmp;
}

/* �L���b�V���ɃZ�b�g���� */
static void unmap_cache_set(unmap_t *list, unmap_data_t *data)
{
	list->cache[(data->box.node & UNMAP_CACHE_SIZE)] = data;
}

/* �L���b�V������T�� */
static unmap_data_t *unmap_cache_get(unmap_t *list, unmap_box_t *box)
{
	unmap_data_t *data = list->cache[(box->node & UNMAP_CACHE_SIZE)];
	if((data == NULL) || (data->box.hash != box->hash)){
		data = NULL;
	}
	return data;
}

