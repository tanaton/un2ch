#include <pthread.h>
#include <mysql.h>
#include <string.h>
#include <unistd.h>
#include <unmap.h>
#include <unstring.h>
#include "un2ch.h"
#include "unarray.h"
#include "read2.h"

typedef struct mysql_config_st {
	unstr_t *host;
	unstr_t *user;
	unstr_t *db;
	unstr_t *passwd;
	unsigned int port;
	unstr_t *unix_socket;
	unsigned long client_flag;
} mysql_config_t;

static void *unmalloc(size_t size);
static void sl_free(void *data);
static void nich_free(void *p);
static unmap_t *get_server(bool flag);
static unarray_t *get_board(un2ch_t *get, nich_t *nich);
static unmap_t *get_board_res(unstr_t *filename);
static void get_thread(un2ch_t *get, unarray_t *tl, int board_no, MYSQL *mysql);
static void *mainThread(void *data);
static void retryThread(databox_t *databox);
static void get_mysql_config(mysql_config_t *m);
static void set_mysql_res(unstr_t *data, unstr_t *moto, nich_t *nich, MYSQL *mysql);
static void set_mysql_res_query(unstr_t *data, nich_t *nich, size_t res_no, MYSQL *mysql);
static unstr_t* create_date_query(nich_t *nich, size_t res_no, unstr_t *data);
static int get_board_no(nich_t *nich, MYSQL *mysql);
static int get_board_no_query(nich_t *nich, MYSQL *mysql);
static void set_board_no_query(nich_t *nich, MYSQL *mysql);

static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_onig_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool g_stop_flag = false;
static mysql_config_t g_mysql_config;
static char *g_pattern = "(\\d{4})\\/(\\d{2})\\/(\\d{2}).*(\\d{2}):(\\d{2}):(\\d{2})";

int main(void)
{
	pthread_t tid;
	unmap_t *sl = get_server(false);
	databox_t *databox = 0;
	nich_t *nich = 0;
	unarray_t *ar = 0;
	size_t sl_len = unmap_size(sl);
	size_t i = 0;
	get_mysql_config(&g_mysql_config);

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
	unstr_delete(
		5,
		g_mysql_config.host,
		g_mysql_config.user,
		g_mysql_config.db,
		g_mysql_config.passwd,
		g_mysql_config.unix_socket
	);
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

static void get_thread(un2ch_t *get, unarray_t *tl, int board_no, MYSQL *mysql)
{
	unstr_t *data = 0;
	unstr_t *moto = 0;
	nich_t *nich = 0;
	size_t i = 0;
	regex_t *reg;
	OnigErrorInfo einfo;
	int r;

	if(get == NULL || tl == NULL){
		return;
	}
	pthread_mutex_lock(&g_onig_mutex);
	r = onig_new(&reg, (UChar *)g_pattern, (UChar *)(g_pattern + strlen(g_pattern)),
			ONIG_OPTION_NONE, ONIG_ENCODING_SJIS, ONIG_SYNTAX_PERL, &einfo);
	pthread_mutex_unlock(&g_onig_mutex);
	if(r != ONIG_NORMAL){
		UChar s[ONIG_MAX_ERROR_MESSAGE_LEN];
		onig_error_code_to_str(s, r, &einfo);
		printf("ERROR: %s\n", s);
		return;
	}

	for(i = 0; i < tl->length; i++){
		nich = unarray_at(tl, i);
		un2ch_set_info(get, nich->server, nich->board, nich->thread);
		moto = unstr_file_get_contents(get->logfile);
		data = un2ch_get_data(get);
		if(unstr_empty(data)){
			printf("error %ld %s/%s/%s\n", get->code, nich->server->data,
					nich->board->data, nich->thread->data);
		} else {
			printf("code:%ld OK %s/%s/%s\n", get->code, nich->server->data,
					nich->board->data, nich->thread->data);
			if(board_no >= 0){
				nich->reg = reg;
				nich->board_no = board_no;
				set_mysql_res(data, moto, nich, mysql);
			}
		}
		nich = NULL;
		unstr_free(moto);
		unstr_free(data);
		/* 4秒止める */
		sleep(4);
	}
	pthread_mutex_lock(&g_onig_mutex);
	onig_free(reg);
	pthread_mutex_unlock(&g_onig_mutex);
}

static void *mainThread(void *data)
{
	un2ch_t *get = 0;
	databox_t *databox = (databox_t *)data;
	unarray_t *bl = databox->bl;
	unarray_t *tl = 0;
	nich_t *nich = 0;
	size_t i = 0;
	int board_no = 0;
	MYSQL *mysql;
	/* スレッドを親から切り離す */
	pthread_detach(pthread_self());
	if((mysql = mysql_init(NULL)) != NULL){
		mysql_real_connect(
			mysql,
			g_mysql_config.host->data,
			g_mysql_config.user->data,
			g_mysql_config.passwd->data,
			g_mysql_config.db->data,
			g_mysql_config.port,
			NULL,
			0
		);
	}
	get = un2ch_init();
	/* get->bourbon = true; */
	for(i = 0; i < bl->length; i++){
		/* スレッド取得 */
		nich = unarray_at(bl, i);
		tl = get_board(get, nich);
		board_no = get_board_no(nich, mysql);
		sleep(4);
		if(tl != NULL){
			get_thread(get, tl, board_no, mysql);
			/* unarrayの開放 */
			unarray_free(tl, nich_free);
		}
	}
	un2ch_free(get);
	mysql_close(mysql);
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

static void get_mysql_config(mysql_config_t *m)
{
	unstr_free(m->host);
	unstr_free(m->user);
	unstr_free(m->db);
	unstr_free(m->passwd);
	m->port = 0;
	unstr_free(m->unix_socket);
	m->client_flag = 0;

	unstr_t *filename = unstr_init("./un2ch.conf");
	unstr_t *data = unstr_file_get_contents(filename);
	unstr_t *line = 0;
	unstr_t *p1 = unstr_init_memory(32);
	unstr_t *p2 = unstr_init_memory(32);
	size_t index = 0;

	line = unstr_strtok(data, "\n", &index);
	while(line != NULL){
		if(unstr_sscanf(line, "$=$", p1, p2) == 2){
			if(unstr_strcmp_char(p1, "host") == 0){
				m->host = unstr_copy(p2);
			} else if(unstr_strcmp_char(p1, "user") == 0){
				m->user = unstr_copy(p2);
			} else if(unstr_strcmp_char(p1, "db") == 0){
				m->db = unstr_copy(p2);
			} else if(unstr_strcmp_char(p1, "passwd") == 0){
				m->passwd = unstr_copy(p2);
			} else if(unstr_strcmp_char(p1, "port") == 0){
				m->port = strtol(p2->data, NULL, 10);
			} else if(unstr_strcmp_char(p1, "unix_socket") == 0){
				m->unix_socket = unstr_copy(p2);
			} else if(unstr_strcmp_char(p1, "client_flag") == 0){
				m->client_flag = strtol(p2->data, NULL, 10);
			}
		}
		unstr_free(line);
		line = unstr_strtok(data, "\n", &index);
	}
	unstr_delete(5, filename, data, p1, p2, line);
}

static void set_mysql_res(unstr_t *data, unstr_t *moto, nich_t *nich, MYSQL *mysql)
{
	size_t moto_len = 0;
	size_t res_no = 0;
	size_t *array = 0;
	unstr_t *str = 0;
	unstr_t *search = 0;
	if(data == NULL) return;
	search = unstr_init("\n");
	if(moto == NULL){
		str = unstr_copy(data);
		array = unstr_quick_search(str, search, &res_no);
		set_mysql_res_query(str, nich, res_no, mysql);
	} else {
		moto_len = unstr_strlen(moto);
		if(unstr_strlen(data) > (moto_len + 1)){
			str = unstr_init(&(data->data[moto_len]));
			array = unstr_quick_search(moto, search, &res_no);
			set_mysql_res_query(str, nich, res_no, mysql);
		}
	}
	free(array);
	unstr_free(str);
}

static void set_mysql_res_query(unstr_t *data, nich_t *nich, size_t res_no, MYSQL *mysql)
{
	unstr_t *tmp_query = 0;
	unstr_t *query = unstr_init("INSERT INTO restime (board_id,thread,number,date) VALUES");
	unstr_t *line = 0;
	unstr_t *p1 = unstr_init_memory(128);
	unstr_t *p2 = unstr_init_memory(128);
	unstr_t *p3 = unstr_init_memory(128);
	unarray_t *list = unarray_init(128);
	size_t index = 0;
	size_t i = 0;

	line = unstr_strtok(data, "\n", &index);
	while(line != NULL){
		res_no++;
		if(unstr_sscanf(line, "$<>$<>$<>", p1, p2, p3) == 3){
			tmp_query = create_date_query(nich, res_no, p3);
			if(tmp_query != NULL){
				unarray_push(list, tmp_query);
			}
		}
		unstr_free(line);
		line = unstr_strtok(data, "\n", &index);
	}
	if(list->length > 0){
		unstr_strcat(query, unarray_at(list, 0));
		for(i = 1; i < list->length; i++){
			unstr_strcat_char(query, ",");
			unstr_strcat(query, unarray_at(list, i));
		}
		if(mysql_query(mysql, query->data)){
			printf("error: mysql挿入失敗\n");
		} else {
			printf("mysql挿入数:%d\n", (int)list->length);
		}
	}
	unstr_delete(5, query, line, p1, p2, p3);
	unarray_free(list, (void (*)(void *))unstr_free_func);
}

static unstr_t* create_date_query(nich_t *nich, size_t res_no, unstr_t *data)
{
	OnigRegion *region = onig_region_new();
	unstr_t *match[6] = {0};
	unstr_t *query = 0;
	UChar *start;
	UChar *end;
	UChar *range;
	int ret;

	end = (UChar *)(data->data + unstr_strlen(data));
	start = (UChar *)(data->data);
	range = end;

	ret = onig_search(nich->reg, (UChar *)data->data, end, start, range, region, ONIG_OPTION_NONE);
	if(ret >= 0){
		match[0] = unstr_init_memory(8);
		match[1] = unstr_init_memory(8);
		match[2] = unstr_init_memory(8);
		match[3] = unstr_init_memory(8);
		match[4] = unstr_init_memory(8);
		match[5] = unstr_init_memory(8);
		unstr_substr_char(match[0], data->data + region->beg[1], region->end[1] - region->beg[1]);
		unstr_substr_char(match[1], data->data + region->beg[2], region->end[2] - region->beg[2]);
		unstr_substr_char(match[2], data->data + region->beg[3], region->end[3] - region->beg[3]);
		unstr_substr_char(match[3], data->data + region->beg[4], region->end[4] - region->beg[4]);
		unstr_substr_char(match[4], data->data + region->beg[5], region->end[5] - region->beg[5]);
		unstr_substr_char(match[5], data->data + region->beg[6], region->end[6] - region->beg[6]);
		query = unstr_sprintf(
			NULL,
			"(%d,%$,%d,'%$%$%$%$%$%$')",
			nich->board_no,
			nich->thread,
			res_no,
			match[0],
			match[1],
			match[2],
			match[3],
			match[4],
			match[5]
		);
	} else { /* Miss match or error */
		UChar s[ONIG_MAX_ERROR_MESSAGE_LEN];
		onig_error_code_to_str(s, ret);
		printf("ERROR: %s\n", s);
		return NULL;
	}
	onig_region_clear(region);
	onig_region_free(region, 1);
	unstr_delete(6, match[0], match[1], match[2], match[3], match[4], match[5]);
	return query;
}

static int get_board_no(nich_t *nich, MYSQL *mysql)
{
	int board_no = get_board_no_query(nich, mysql);
	if(board_no < 0){
		set_board_no_query(nich, mysql);
		board_no = get_board_no_query(nich, mysql);
	}
	return board_no;
}

static int get_board_no_query(nich_t *nich, MYSQL *mysql)
{
	MYSQL_RES *res;
	MYSQL_ROW row;
	unstr_t *query = unstr_sprintf(NULL, "SELECT id, board FROM boardlist WHERE board = '%$'", nich->board);
	int board_no = -1;
	if(!mysql_query(mysql, query->data)){
		res = mysql_use_result(mysql);
		if((row = mysql_fetch_row(res)) != NULL){
			board_no = strtol(row[0], NULL, 10);
		}
		mysql_free_result(res);
	} else {
		printf("%s\n", query->data);
		printf("板番号取得失敗 %s\n", nich->board->data);
	}
	unstr_free(query);
	return board_no;
}

static void set_board_no_query(nich_t *nich, MYSQL *mysql)
{
	unstr_t *query = unstr_sprintf(NULL, "INSERT INTO boardlist (board) VALUES(%$)", nich->board);
	mysql_query(mysql, query->data);
}

