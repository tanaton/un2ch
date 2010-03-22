#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <unhash.h>
#include "un2ch.h"
#include "unstring.h"
#include "unarray.h"

typedef struct nich_st {
	unstr_t *server;
	unstr_t *board;
	unstr_t *thread;
} nich_t;

typedef struct databox_st {
	unarray_t	*bl;
	unstr_t		*key;
} databox_t;

static void *unmalloc(size_t size);
static void sl_free(void *data);
static void nich_free(void *p);
static unh_t *get_server(bool flag);
static unarray_t *get_board(un2ch_t *get, nich_t *nich);
static unh_t *get_board_res(unstr_t *filename);
static void get_thread(un2ch_t *get, unarray_t *tl);
static void *mainThread(void *data);
static void retryThread(databox_t *databox);

static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool g_stop_flag = false;

int main(void)
{
	unh_t *sl = get_server(false);
	databox_t *databox = 0;
	unh_data_t *box = 0;
	nich_t *nich = 0;
	unarray_t *ar = 0;
	size_t sl_len = unh_size(sl);
	size_t i = 0;
	for(i = 0; i < sl_len; i++){
		box = unh_at(sl, i);
		ar = box->data;
		if(ar != NULL){
			nich = unarray_at(ar, 0);
			if(nich != NULL){
				pthread_t tid;
				/* スレッド作成 */
				databox = unmalloc(sizeof(databox_t));
				databox->bl = ar;
				databox->key = unstr_init(nich->server->data);
				if(pthread_create(&tid, NULL, mainThread, databox)){
					printf("%s スレッド生成エラー\n", nich->server->data);
				}
				databox = NULL;
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
	/* unh_free(sl); */
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

static unh_t *get_server(bool flag)
{
	unh_t *hash = 0;
	un2ch_t *get = un2ch_init();
	unh_data_t *p = 0;
	unarray_t *list = 0;
	unstr_t *bl = 0;
	unstr_t *line = 0;
	unstr_t *p1 = 0;
	unstr_t *p2 = 0;
	size_t index = 0;
	bool ok = un2ch_get_server(get);

	if(flag && (ok == false)){
		return NULL;
	}
	bl = unstr_file_get_contents(get->board_list);
	if(bl == NULL){
		perror("板一覧ファイルが無いよ。\n");
	}
	hash = unh_init(8, 32, 2);
	p1 = unstr_init_memory(32);
	p2 = unstr_init_memory(32);
	line = unstr_strtok(bl, "\n", &index);
	while(line != NULL){
		if(unstr_sscanf(line, "$/$<>", p1, p2) == 2){
			nich_t *nich = unmalloc(sizeof(nich_t));
			nich->server = unstr_init(p1->data);
			nich->board = unstr_init(p2->data);
			nich->thread = NULL;
			p = unh_get(hash, p1->data, p1->length);
			if(p->data == NULL){
				list = unarray_init();
				unarray_push(list, nich);
				p->data = list;
				p->free_func = sl_free;
			} else {
				list = p->data;
				unarray_push(list, nich);
			}
			/* listは開放しない */
		}
		unstr_free(line);
		line = unstr_strtok(bl, "\n", &index);
	}
	unstr_delete(4, bl, line, p1, p2);
	un2ch_free(get);
	return hash;
}

static unarray_t *get_board(un2ch_t *get, nich_t *nich)
{
	unstr_t *data = 0;
	unarray_t *tl = 0;
	unstr_t *p1 = 0;
	unstr_t *p2 = 0;
	unstr_t *line = 0;
	unstr_t *filename = 0;
	unh_t *resmap = 0;
	unh_data_t *box = 0;
	size_t index = 0;
	char *p = 0;
	int nres = 0;
	if(get == NULL || nich == NULL){
		return NULL;
	}
	tl = unarray_init();
	p1 = unstr_init_memory(16);
	p2 = unstr_init_memory(256);

	un2ch_set_info(get, nich->server, nich->board, NULL);
	filename = unstr_init(get->logfile->data);
	/* 必要箇所のみ更新するための準備 */
	resmap = get_board_res(filename);
	data = un2ch_get_data(get);
	if(data == NULL){
		unstr_delete(4, data, p1, p2, filename);
		unarray_free(tl, NULL);
		printf("ita error\n");
		return NULL;
	}
	line = unstr_strtok(data, "\n", &index);
	while(line != NULL){
		if(unstr_sscanf(line, "$.dat<>$", p1, p2) == 2){
			nich_t *n = unmalloc(sizeof(nich_t));
			n->server = unstr_init(nich->server->data);
			n->board = unstr_init(nich->board->data);
			n->thread = unstr_init(p1->data);
			if(resmap != NULL){
				p = strrchr(p2->data, ' ');
				/* +2は、空白と開きカッコをスキップ */
				nres = (int)strtol(p + 2, NULL, 10);
				box = unh_get(resmap, p1->data, p1->length);
				if(box->flag != nres){
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
	unh_free(resmap);
	data = un2ch_get_board_name(get);
	unstr_free(data);
	return tl;
}

static unh_t *get_board_res(unstr_t *filename)
{
	unh_t *resmap = 0;
	unstr_t *data = unstr_file_get_contents(filename);
	unstr_t *p1 = 0;
	unstr_t *p2 = 0;
	unstr_t *line = 0;
	size_t index = 0;
	char *p = 0;
	int nres = 0;
	if(data == NULL){
		return NULL;
	}
	resmap = unh_init(16, 128, 4);
	p1 = unstr_init_memory(32);
	p2 = unstr_init_memory(128);
	
	line = unstr_strtok(data, "\n", &index);
	while(line != NULL){
		if(unstr_sscanf(line, "$.dat<>$", p1, p2) == 2){
			p = strrchr(p2->data, ' ');
			nres = (int)strtol(p + 2, NULL, 10); /* +2は、空白と開きカッコをスキップ */
			unh_set(resmap, p1->data, p1->length, NULL, nres, NULL);
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
	size_t i = 0;
	if(get == NULL || tl == NULL) return;

	for(i = 0; i < tl->length; i++){
		nich_t *nich = unarray_at(tl, i);
		un2ch_set_info(get, nich->server, nich->board, nich->thread);
		data = un2ch_get_data(get);
		if(data != NULL){
			printf("code:%ld OK %s/%s/%s\n", get->code, nich->server->data,
					nich->board->data, nich->thread->data);
		} else {
			printf("error %ld %s/%s/%s\n", get->code, nich->server->data,
					nich->board->data, nich->thread->data);
		}
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
		printf("get_thread %s/%s\n", nich->server->data, nich->board->data);
		tl = get_board(get, nich);
		if(tl != NULL){
			get_thread(get, tl);
			/* unarrayの開放 */
			unarray_free(tl, nich_free);
		}
		sleep(4);
	}
	un2ch_free(get);
	/* 10分止める */
	sleep(600);
	retryThread(databox);
	/* スレッドを終了する */
	return NULL;
}

static void retryThread(databox_t *databox)
{
	unh_t *nsl = 0;
	/* ロック */
	pthread_mutex_lock(&g_mutex);
	nsl = get_server(true);
	if((nsl == NULL) && (g_stop_flag == false)){
		pthread_t tid;
		/* スレッド作成 */
		if(pthread_create(&tid, NULL, mainThread, databox)){
			printf("%s スレッド生成エラー\n", databox->key->data);
			g_stop_flag = true;
		}
	} else {
		g_stop_flag = true;
		unh_free(nsl);
	}
	/* ロック解除 */
	pthread_mutex_unlock(&g_mutex);
}

