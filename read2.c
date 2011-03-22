#include <pthread.h>
#include <mysql.h>
#include <oniguruma.h>
#include <unmap.h>
#include <unstring.h>
#include <string.h>
#include <unistd.h>
#include "un2ch.h"
#include "unarray.h"
#include "read2.h"

static void *unmalloc(size_t size);
static void sl_free(void *data);
static void nich_free(void *p);
static void get_read2ch_config(int argc, char *argv[], read2ch_config_t *conf);
static void read2ch_config_free(read2ch_config_t *conf);
static unmap_t *get_board_data(unstr_t *path);
static unmap_t *get_server(read2ch_config_t *conf);
static void push_board(unmap_t *hash, const unstr_t *server, const unstr_t *board);
static unarray_t *get_board(un2ch_t *get, nich_t *nich);
static unmap_t *get_board_res(unstr_t *filename);
static void get_thread(un2ch_t *get, unarray_t *tl, int board_no, MYSQL *mysql);
static void *mainThread(void *data);
static void retryThread(databox_t *databox);
static void get_mysql_config(mysql_config_t *m, unstr_t *path);
static void mysql_config_free(mysql_config_t *m);
static void set_mysql_res(unstr_t *data, unstr_t *moto, nich_t *nich, MYSQL *mysql);
static void set_mysql_res_query(unstr_t *data, nich_t *nich, size_t res_no, MYSQL *mysql);
static unstr_t* create_date_query(const nich_t *nich, size_t res_no, const unstr_t *data);
static int get_board_no(nich_t *nich, MYSQL *mysql);
static int get_board_no_query(nich_t *nich, MYSQL *mysql);
static void set_board_no_query(nich_t *nich, MYSQL *mysql);

static pthread_mutex_t g_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t g_onig_mutex = PTHREAD_MUTEX_INITIALIZER;
static bool g_stop_flag = false;
static read2ch_config_t g_conf;
static char *g_pattern = "^.*?<>.*?<>.*?(\\d{4})\\/(\\d{2})\\/(\\d{2}).*(\\d{2}):(\\d{2}):(\\d{2})";

int main(int argc, char *argv[])
{
	pthread_t tid;
	unmap_t *sl = 0;
	databox_t *databox = 0;
	nich_t *nich = 0;
	unarray_t *ar = 0;
	size_t sl_len = 0;
	size_t i = 0;

	get_read2ch_config(argc, argv, &g_conf);
	sl = get_server(&g_conf);
	sl_len = unmap_size(sl);

	for(i = 0; i < sl_len; i++){
		ar = unmap_at(sl, i);
		if(ar != NULL){
			nich = unarray_at(ar, 0);
			if(nich != NULL){
				/* スレッド作成 */
				databox = unmalloc(sizeof(databox_t));
				databox->bl = ar;
				databox->key = unstr_copy(nich->server);
				databox->conf = &g_conf;
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
	read2ch_config_free(&g_conf);
	unmap_free(sl, sl_free);
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

static void get_read2ch_config(int argc, char *argv[], read2ch_config_t *conf)
{
	int result = 0;
	unstr_t *strtmp = unstr_init_memory(128);
	conf->type = READ2CH_NORMAL;
	while((result = getopt(argc, argv, "fnc:m:")) != -1){
		switch(result){
		case 'f':
			conf->type = READ2CH_FAVO;
			break;
		case 'n':
			conf->type = READ2CH_NOFAVO;
			break;
		case 'c':
			unstr_free(conf->favolist_path);
			conf->favolist_path = unstr_init(optarg);
			break;
		case 'm':
			unstr_strcpy_char(strtmp, optarg);
			get_mysql_config(&(conf->mysql_config), strtmp);
			break;
		}
	}
	if(conf->favolist_path == NULL){
		conf->favolist_path = unstr_init(READ2CH_GET_BOARD_PATH);
	}
	if(conf->mysql_config.host == NULL){
		unstr_strcpy_char(strtmp, READ2CH_MYSQL_CONF_PATH);
		get_mysql_config(&(conf->mysql_config), strtmp);
	}
	unstr_free(strtmp);
}

static void read2ch_config_free(read2ch_config_t *conf)
{
	unstr_free(conf->favolist_path);
	mysql_config_free(&(conf->mysql_config));
}

static unmap_t *get_board_data(unstr_t *path)
{
	size_t index = 0;
	size_t length = 0;
	unmap_t *map = unmap_init(16);
	unstr_t *line = 0;
	unstr_t *data = unstr_file_get_contents(path);
	line = unstr_strtok(data, "\n", &index);
	while(line != NULL){
		length = unstr_strlen(line);
		if(length != 0){
			unstr_free_func(unmap_find(map, line->data, length));
			unmap_set(map, line->data, length, unstr_copy(line));
		}
		unstr_free(line);
		line = unstr_strtok(data, "\n", &index);
	}
	unstr_delete(2, line, data);
	return map;
}

static unmap_t *get_server(read2ch_config_t *conf)
{
	unmap_t *hash = 0;
	unmap_t *board_map = 0;
	un2ch_t *get = un2ch_init();
	unstr_t *bl = 0;
	unstr_t *line = 0;
	unstr_t *server = 0;
	unstr_t *board = 0;
	size_t index = 0;
	
	/* 板一覧の更新確認 */
	un2ch_get_server(get);

	bl = unstr_file_get_contents(get->board_list);
	if(bl == NULL){
		un2ch_free(get);
		perror("板一覧ファイルが無いよ。\n");
	}
	board_map = get_board_data(conf->favolist_path);
	hash = unmap_init(16);
	server = unstr_init_memory(32);
	board = unstr_init_memory(32);
	line = unstr_strtok(bl, "\n", &index);
	while(line != NULL){
		if(unstr_sscanf(line, "$/$<>", server, board) == 2){
			switch(conf->type){
			case READ2CH_NORMAL:
				push_board(hash, server, board);
				break;
			case READ2CH_FAVO:
				if(unmap_find(board_map, board->data, unstr_strlen(board)) != NULL){
					push_board(hash, server, board);
				}
				break;
			case READ2CH_NOFAVO:
				if(unmap_find(board_map, board->data, unstr_strlen(board)) == NULL){
					push_board(hash, server, board);
				}
				break;
			}
		}
		unstr_free(line);
		line = unstr_strtok(bl, "\n", &index);
	}
	unstr_delete(4, bl, line, server, board);
	unmap_free(board_map, (void (*)(void *))unstr_free_func);
	un2ch_free(get);
	return hash;
}

static void push_board(unmap_t *hash, const unstr_t *server, const unstr_t *board)
{
	unarray_t *list = 0;
	nich_t *nich = unmalloc(sizeof(nich_t));
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
		unstr_delete(2, moto, data);
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
	mysql_config_t *m = &(databox->conf->mysql_config);
	unarray_t *bl = databox->bl;
	unarray_t *tl = 0;
	nich_t *nich = 0;
	size_t i = 0;
	int board_no = 0;
	MYSQL *mysql = 0;
	/* スレッドを親から切り離す */
	pthread_detach(pthread_self());
	if(m->host && m->user && m->passwd && m->db){
		if((mysql = mysql_init(NULL)) != NULL){
			mysql_real_connect(
				mysql,
				m->host->data,
				m->user->data,
				m->passwd->data,
				m->db->data,
				m->port,
				NULL,
				0
			);
		}
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
	un2ch_t *get = 0;
	bool ok = false;
	/* ロック */
	pthread_mutex_lock(&g_mutex);
	get = un2ch_init();
	ok = un2ch_get_server(get);
	if((ok == false) && (g_stop_flag == false)){
		/* 板一覧に更新が無い場合スレッド作成 */
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
	}
	un2ch_free(get);
	/* ロック解除 */
	pthread_mutex_unlock(&g_mutex);
}

static void get_mysql_config(mysql_config_t *m, unstr_t *path)
{
	unstr_t *data = unstr_file_get_contents(path);
	unstr_t *line = 0;
	unstr_t *p1 = unstr_init_memory(32);
	unstr_t *p2 = unstr_init_memory(32);
	size_t index = 0;

	mysql_config_free(m);
	m->port = 0;
	m->client_flag = 0;

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
	unstr_delete(4, data, p1, p2, line);
}

static void mysql_config_free(mysql_config_t *m)
{
	unstr_delete(
		5,
		m->host,
		m->user,
		m->db,
		m->passwd,
		m->unix_socket
	);
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
		set_mysql_res_query(str, nich, 0, mysql);
	} else {
		moto_len = unstr_strlen(moto);
		if(unstr_strlen(data) > (moto_len + 1)){
			str = unstr_init(&(data->data[moto_len]));
			array = unstr_quick_search(moto, search, &res_no);
			set_mysql_res_query(str, nich, res_no, mysql);
		}
	}
	free(array);
	unstr_delete(2, str, search);
}

static void set_mysql_res_query(unstr_t *data, nich_t *nich, size_t res_no, MYSQL *mysql)
{
	unstr_t *tmp_query = 0;
	unstr_t *query = unstr_init("INSERT INTO restime (board_id,thread,number,date) VALUES");
	unstr_t *line = 0;
	unarray_t *list = unarray_init(128);
	size_t index = 0;
	size_t i = 0;

	line = unstr_strtok(data, "\n", &index);
	while(line != NULL){
		res_no++;
		tmp_query = create_date_query(nich, res_no, line);
		if(tmp_query != NULL){
			unarray_push(list, tmp_query);
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
	unstr_delete(2, query, line);
	unarray_free(list, (void (*)(void *))unstr_free_func);
}

static unstr_t* create_date_query(const nich_t *nich, size_t res_no, const unstr_t *data)
{
	OnigRegion *region = onig_region_new();
	unstr_t *strtmp = 0;
	unstr_t *query = 0;
	UChar *start;
	UChar *end;
	UChar *range;
	int ret = -1;
	int i = 0;

	end = (UChar *)(data->data + unstr_strlen(data));
	start = (UChar *)(data->data);
	range = end;

	ret = onig_search(nich->reg, (UChar *)data->data, end, start, range, region, ONIG_OPTION_NONE);
	if(ret >= 0){
		strtmp = unstr_init_memory(8);
		query = unstr_sprintf(NULL, "(%d,%$,%d,'", nich->board_no, nich->thread, res_no);
		for(i = 1; i < region->num_regs; i++){
			unstr_substr_char(strtmp, data->data + region->beg[i], region->end[i] - region->beg[i]);
			unstr_strcat(query, strtmp);
		}
		unstr_strcat_char(query, "')");
	}
	onig_region_clear(region);
	onig_region_free(region, 1);
	unstr_free(strtmp);
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
	}
	unstr_free(query);
	return board_no;
}

static void set_board_no_query(nich_t *nich, MYSQL *mysql)
{
	unstr_t *query = unstr_sprintf(NULL, "INSERT INTO boardlist (board) VALUES('%$')", nich->board);
	mysql_query(mysql, query->data);
}

