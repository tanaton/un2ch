#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <unmap.h>
#include <unstring.h>
#include "un2ch.h"
#include "unarray.h"
#include "read.h"

static void *unmalloc(size_t size);
static void sl_free(void *data);
static void nich_free(void *p);
static unmap_t *get_server(bool flag);
static unarray_t *get_board(un2ch_t *get, nich_t *nich);
static unmap_t *get_board_res(unstr_t *filename);
static void get_thread(un2ch_t *get, unarray_t *tl);
static void *mainThread(void *data);
static void retryThread(databox_t *databox);

static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool g_stop_flag = false;

int main(void)
{
	pthread_t tid;
	unmap_t *sl = get_server(false);
	databox_t *databox = 0;
	nich_t *nich = 0;
	unarray_t *ar = 0;
	size_t sl_len = unmap_size(sl);
	size_t i = 0;
	for(i = 0; i < sl_len; i++){
		ar = unmap_at(sl, i);
		if(ar != NULL){
			nich = unarray_at(ar, 0);
			if(nich != NULL){
				/* スレッド作成 */
				databox = unmalloc(sizeof(databox_t));
				databox->bl = ar;
				databox->key = unstr_copy(nich->server);
				if(pthread_create(&tid, NULL, mainThread, databox)){
					unstr_free(databox->key);
					free(databox);
					printf("%s スレッド生成エラー\n", nich->server->data);
				}
				databox = NULL;
				/* 1秒止める */
				sleep(1);
			}
		}
	}
	/* 仮 */
	while(1){
		pthread_mutex_lock(&g_mutex);
		if(g_stop_flag){
			printf("Exit threads\n");
			break;
		}
		pthread_mutex_unlock(&g_mutex);
		sleep(600);
	}
	/* unmap_free(sl, sl_free); */
	return 0;
}

static void *unmalloc(size_t size)
{
	void *p = malloc(size);
	if(p == NULL){
		perror("unmalloc");
	}
	memset(p, 0, size);
	return p;
}

static void sl_free(void *data)
{
	unarray_free(data, nich_free);
}

static void nich_free(void *p)
{
	nich_t *nich = p;
	if(nich != NULL){
		unstr_free(nich->server);
		unstr_free(nich->board);
		unstr_free(nich->thread);
	}
	free(nich);
}

static unmap_t *get_server(bool flag)
{
	unmap_t *hash = 0;
	un2ch_t *get = un2ch_init();
	unarray_t *list = 0;
	nich_t *nich = 0;
	unstr_t *bl = 0;
	unstr_t *line = 0;
	unstr_t *server = 0;
	unstr_t *board = 0;
	size_t index = 0;
	bool ok = un2ch_get_server(get);

	if(flag && (ok == false)){
		un2ch_free(get);
		return NULL;
	}
	bl = unstr_file_get_contents(get->board_list);
	if(bl == NULL){
		un2ch_free(get);
		perror("板一覧ファイルが無いよ。\n");
	}
	hash = unmap_init(16);
	server = unstr_init_memory(32);
	board = unstr_init_memory(32);
	line = unstr_strtok(bl, "\n", &index);
	while(line != NULL){
		if(unstr_sscanf(line, "$/$<>", server, board) == 2){
			nich = unmalloc(sizeof(nich_t));
			nich->server = unstr_copy(server);
			nich->board = unstr_copy(board);
			nich->thread = NULL;
			list = unmap_find(hash, server->data, server->length);
			if(list == NULL){
				list = unarray_init(32);
				unarray_push(list, nich);
				unmap_set(hash, server->data, server->length, list);
			} else {
				unarray_push(list, nich);
			}
			/* listは開放しない */
		}
		unstr_free(line);
		line = unstr_strtok(bl, "\n", &index);
	}
	unstr_delete(4, bl, line, server, board);
	un2ch_free(get);
	return hash;
}

static unarray_t *get_board(un2ch_t *get, nich_t *nich)
{
	unstr_t *data = 0;
	unarray_t *tl = 0;
	nich_t *n = 0;
	unstr_t *p1 = 0;
	unstr_t *p2 = 0;
	unstr_t *line = 0;
	unstr_t *filename = 0;
	unmap_t *resmap = 0;
	int *res = 0;
	size_t index = 0;
	char *p = 0;
	int nres = 0;
	if(get == NULL || nich == NULL){
		return NULL;
	}
	tl = unarray_init(512);
	p1 = unstr_init_memory(16);
	p2 = unstr_init_memory(256);

	un2ch_set_info(get, nich->server, nich->board, NULL);
	filename = unstr_copy(get->logfile);
	/* 必要箇所のみ更新するための準備 */
	resmap = get_board_res(filename);
	data = un2ch_get_data(get);
	if(unstr_empty(data)){
		unstr_delete(4, data, p1, p2, filename);
		unarray_free(tl, NULL);
		unmap_free(resmap, free);
		printf("ita error\n");
		return NULL;
	}
	line = unstr_strtok(data, "\n", &index);
	while(line != NULL){
		if(unstr_sscanf(line, "$.dat<>$", p1, p2) == 2){
			n = unmalloc(sizeof(nich_t));
			n->server = unstr_copy(nich->server);
			n->board = unstr_copy(nich->board);
			n->thread = unstr_copy(p1);
			p = strrchr(p2->data, ' ');
			if((resmap != NULL) && (p != NULL)){
				/* +2は、空白と開きカッコをスキップ */
				nres = (int)strtol(p + 2, NULL, 10);
				res = unmap_find(resmap, p1->data, p1->length);
				if((res != NULL) && (*res != nres)){
					/* レス数が違う */
					unarray_push(tl, n);
				} else {
					/* 解放 */
					nich_free(n);
				}
			} else {
				/* 板未取得 */
				unarray_push(tl, n);
			}
		}
		unstr_free(line);
		line = unstr_strtok(data, "\n", &index);
	}
	unstr_delete(5, data, p1, p2, line, filename);
	unmap_free(resmap, free);
	data = un2ch_get_board_name(get);
	unstr_free(data);
	return tl;
}

static unmap_t *get_board_res(unstr_t *filename)
{
	unmap_t *resmap = 0;
	unstr_t *data = 0;
	unstr_t *p1 = 0;
	unstr_t *p2 = 0;
	unstr_t *line = 0;
	size_t index = 0;
	char *p = 0;
	int *nres = 0;

	data = unstr_file_get_contents(filename);
	if(unstr_empty(data)){
		return NULL;
	}
	resmap = unmap_init(16);
	p1 = unstr_init_memory(32);
	p2 = unstr_init_memory(128);
	
	line = unstr_strtok(data, "\n", &index);
	while(line != NULL){
		if(unstr_sscanf(line, "$.dat<>$", p1, p2) == 2){
			p = strrchr(p2->data, ' ');
			if(p != NULL){
				nres = unmalloc(sizeof(int));
				*nres = (int)strtol(p + 2, NULL, 10); /* +2は、空白と開きカッコをスキップ */
				free(unmap_find(resmap, p1->data, p1->length));
				unmap_set(resmap, p1->data, p1->length, nres);
			}
		}
		unstr_free(line);
		line = unstr_strtok(data, "\n", &index);
	}
	unstr_delete(4, data, p1, p2, line);
	return resmap;
}

static void get_thread(un2ch_t *get, unarray_t *tl)
{
	unstr_t *data = 0;
	nich_t *nich = 0;
	size_t i = 0;
	if(get == NULL || tl == NULL) return;

	for(i = 0; i < tl->length; i++){
		nich = unarray_at(tl, i);
		un2ch_set_info(get, nich->server, nich->board, nich->thread);
		data = un2ch_get_data(get);
		if(unstr_empty(data)){
			printf("error %ld %s/%s/%s\n", get->code, nich->server->data,
					nich->board->data, nich->thread->data);
		} else {
			printf("code:%ld OK %s/%s/%s\n", get->code, nich->server->data,
					nich->board->data, nich->thread->data);
		}
		nich = NULL;
		unstr_free(data);
		/* 4秒止める */
		sleep(4);
	}
}

static void *mainThread(void *data)
{
	un2ch_t *get = 0;
	databox_t *databox = (databox_t *)data;
	unarray_t *bl = databox->bl;
	unarray_t *tl = 0;
	nich_t *nich = 0;
	size_t i = 0;
	/* スレッドを親から切り離す */
	pthread_detach(pthread_self());
	get = un2ch_init();
	/* get->bourbon = true; */
	for(i = 0; i < bl->length; i++){
		/* スレッド取得 */
		nich = unarray_at(bl, i);
		tl = get_board(get, nich);
		sleep(4);
		if(tl != NULL){
			get_thread(get, tl);
			/* unarrayの開放 */
			unarray_free(tl, nich_free);
		}
	}
	un2ch_free(get);
	/* 1分止める */
	sleep(60);
	retryThread(databox);
	/* スレッドを終了する */
	return NULL;
}

static void retryThread(databox_t *databox)
{
	pthread_t tid;
	unmap_t *nsl = 0;
	/* ロック */
	pthread_mutex_lock(&g_mutex);
	nsl = get_server(true);
	if((nsl == NULL) && (g_stop_flag == false)){
		/* スレッド作成 */
		if(pthread_create(&tid, NULL, mainThread, databox)){
			printf("%s スレッド生成エラー\n", databox->key->data);
			unstr_free(databox->key);
			free(databox);
			g_stop_flag = true;
		}
	} else {
		unstr_free(databox->key);
		free(databox);
		g_stop_flag = true;
		unmap_free(nsl, sl_free);
	}
	/* ロック解除 */
	pthread_mutex_unlock(&g_mutex);
}

