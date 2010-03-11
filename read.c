#include "un2ch.h"
#include "unstring.h"
#include "unarray.h"
#include <unhash.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

typedef struct nich_st {
	unstr_t *server;
	unstr_t *board;
	unstr_t *thread;
} nich_t;

typedef struct databox_st {
	unarray_t	*bl;
	unh_t		*sl;
	unstr_t		*key;
} databox_t;

void *unmalloc(size_t size);
void sl_free(void *data);
void nich_free(void *p);
unh_t *get_server();
unarray_t *get_board(nich_t *nich);
unh_t *get_board_res(unstr_t *filename);
void get_thread(unarray_t *tl);
void *mainThread(void *data);

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

int main(void)
{
	unh_t *sl = get_server();
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
				printf("%s\n", nich->server->data);
				/* スレッド作成 */
				databox = unmalloc(sizeof(databox_t));
				databox->sl = sl;
				databox->bl = ar;
				databox->key = unstr_init(nich->server->data);
				if(pthread_create(&tid, NULL, mainThread, databox) != 0){
					printf("%s スレッド生成エラー\n", nich->server->data);
				}
			}
		}
	}
	/* 仮 */
	while(1)
		sleep(1000000);
	return 0;
}

void *unmalloc(size_t size)
{
	void *p = malloc(size);
	if(p == NULL){
		perror("unmalloc");
	}
	memset(p, 0, size);
	return p;
}

void sl_free(void *data)
{
	unarray_free(data, nich_free);
}

void nich_free(void *p)
{
	nich_t *nich = p;
	if(nich != NULL){
		unstr_free(nich->server);
		unstr_free(nich->board);
		unstr_free(nich->thread);
	}
	free(nich);
}

unh_t *get_server()
{
	unh_t *hash = unh_init(8, 32, 2);
	un2ch_t *init = un2ch_init();
	unh_data_t *p = 0;
	unarray_t *list = 0;
	unstr_t *bl = 0;
	unstr_t *line = 0;
	unstr_t *p1 = unstr_init_memory(32);
	unstr_t *p2 = unstr_init_memory(32);
	unstr_t *data = 0;
	size_t index = 0;

	un2ch_get_server(init);
	bl = unstr_file_get_contents(init->board_list);
	if(bl == NULL){
		perror("板一覧ファイルが無いよ。\n");
	}
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
	unstr_delete(5, bl, line, p1, p2, data);
	un2ch_free(init);
	return hash;
}

unarray_t *get_board(nich_t *nich)
{
	unstr_t *data = 0;
	unarray_t *tl = 0;
	un2ch_t *get = 0;
	unstr_t *p1 = 0;
	unstr_t *p2 = 0;
	unstr_t *line = 0;
	unstr_t *filename = 0;
	unh_t *resmap = 0;
	unh_data_t *box = 0;
	size_t index = 0;
	char *p = 0;
	int nres = 0;
	if(nich == NULL){
		return NULL;
	}
	tl = unarray_init();
	get = un2ch_init();
	p1 = unstr_init_memory(32);
	p2 = unstr_init_memory(128);

	un2ch_set_info(get, nich->server, nich->board, NULL);
	filename = unstr_init(get->logfile->data);
	/* 必要箇所のみ更新するための準備 */
	resmap = get_board_res(filename);
	data = un2ch_get_data(get);
	if(data == NULL){
		unstr_delete(4, data, p1, p2, filename);
		un2ch_free(get);
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
	un2ch_free(get);
	return tl;
}

unh_t *get_board_res(unstr_t *filename)
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

void get_thread(unarray_t *tl)
{
	unstr_t *data = 0;
	un2ch_t *get = 0;
	size_t i = 0;
	if(tl == NULL) return;
	get = un2ch_init();

	for(i = 0; i < tl->length; i++){
		nich_t *nich = unarray_at(tl, i);
		un2ch_set_info(get, nich->server, nich->board, nich->thread);
		data = un2ch_get_data(get);
		if(data != NULL){
			printf("thread:%u code:%ld OK %s/%s/%s\n", (unsigned int)pthread_self(), get->code, nich->server->data,
					nich->board->data, nich->thread->data);
		} else {
			printf("error %ld %s/%s/%s\n", get->code, nich->server->data,
					nich->board->data, nich->thread->data);
		}
		unstr_free(data);
		/* 4秒止める */
		sleep(4);
	}
	un2ch_free(get);
}

void *mainThread(void *data)
{
	databox_t *databox = (databox_t *)data;
	unarray_t *tl = 0;
	unstr_t *key = databox->key;
	unh_t *sl = 0;
	unh_t *nsl = 0;
	size_t i = 0;
	/* スレッドを親から切り離す */
	pthread_detach(pthread_self());
	for(i = 0; i < databox->bl->length; i++){
		/* スレッド取得 */
		tl = get_board(unarray_at(databox->bl, i));
		if(tl != NULL){
			get_thread(tl);
		}
		/* unarrayの開放 */
		unarray_free(tl, nich_free);
	}
	/* 10分止める */
	//sleep(600);

	/* ロック */
	pthread_mutex_lock(&mutex);
	printf("thread切り替え\n");
	sl = databox->sl;
	//nsl = get_server();
	nsl = sl;
	//databox->sl = nsl;
	if(nsl != NULL){
		unh_data_t *d = unh_get(nsl, key->data, key->length);
		size_t nsl_len = unh_size(nsl);
		printf("担当鯖引き継ぎ\n");
		if(d->data != NULL){
			pthread_t tid;
			printf("スレッド生成直前\n");
			/* スレッド作成 */
			if(pthread_create(&tid, NULL, mainThread, databox) != 0){
				printf("%s スレッド生成エラー\n", key->data);
			}
			printf("スレッド生成直後\n");
		} else {
			/* もし、サーバに変更があった場合（バグまみれ） */
			for(i = 0; i < nsl_len; i++){
				unh_data_t *nsldata = unh_at(nsl, i);
				nich_t *ni = unarray_at(nsldata->data, 0);
				if(ni != NULL){
					unh_data_t *sldata = unh_get(sl, ni->server->data, ni->server->length);
					if(sldata->data == NULL){
						/* NULLだった場合新規スレッド生成 */
						pthread_t tid;
						databox_t *box = unmalloc(sizeof(databox_t));
						box->sl = nsl;
						box->bl = nsldata->data;
						box->key = unstr_init(ni->server->data);
						if(pthread_create(&tid, NULL, mainThread, box) != 0){
							printf("%s スレッド生成エラー\n", ni->server->data);
						}
					}
				}
			}
		}
	}
	printf("thread切り替え終了\n");
	/* ロック解除 */
	pthread_mutex_unlock(&mutex);
	/* 10秒止める(不要？) */
	//sleep(10);
	/* スレッドを終了する */
	pthread_exit(NULL);
	return NULL;
}


