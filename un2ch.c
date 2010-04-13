#include <curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <unistd.h>
#include <utime.h>
#include "un2ch.h"

static bool in_array(const char *str, const char **array, size_t size);
static bool set_thread(un2ch_t *init, unstr_t *unstr);
static bool server_check(unstr_t *str);
static un2ch_code_t mode_change(un2ch_t *init);
static unstr_t* normal_data(un2ch_t *init);
static unstr_t* request(un2ch_t *init, bool flag);
static size_t returned_data(void *ptr, size_t size, size_t nmemb, void *data);
static unstr_t* bourbon_data(un2ch_t *init);
static unstr_t* bourbon_request(un2ch_t *init);
static unstr_t* slice_board_name(unstr_t *data);
static unstr_t* unstr_get_http_file(unstr_t *url, time_t *mod);
static bool create_cache(un2ch_t *init, unstr_t *data, un2ch_cache_t flag);
static bool file_exists(unstr_t *filename, struct stat *data);
static void touch(unstr_t *filename, time_t atime, time_t mtime);
static bool make_dir_all(unstr_t *path);

#define UN2CH_G_SABAKILL_SIZE		(11)
static const char *g_sabakill[UN2CH_G_SABAKILL_SIZE] = {
	"www.2ch.net",
	"info.2ch.net",
	"find.2ch.net",
	"v.isp.2ch.net",
	"m.2ch.net",
	"test.up.bbspink.com",
	"stats.2ch.net",
	"c-au.2ch.net",
	"c-others1.2ch.net",
	"movie.2ch.net",
	"dubai.2ch.net"
};

#define UN2CH_G_SABAFILTER_SIZE		(4)
static char g_sabafilter[UN2CH_G_SABAFILTER_SIZE][25] = {
	/* 他のサイト */
	{ 0x91, 0xbc, 0x82, 0xcc, 0x83, 0x54, 0x83, 0x43, 0x83, 0x67, 0x00},
	/* まちＢＢＳ */
	{ 0x82, 0xdc, 0x82, 0xbf, 0x82, 0x61, 0x82, 0x61, 0x82, 0x72, 0x00},
	/* ツール類 */
	{ 0x83, 0x63, 0x81, 0x5b, 0x83, 0x8b, 0x97, 0xde, 0x00},
	/* チャット２ｃｈ＠ＩＲＣ */
	{ 0x83, 0x60, 0x83, 0x83, 0x83, 0x62, 0x83, 0x67, 0x82, 0x51, 0x82, 0x83, 0x82, 0x88, 0x81, 0x97, 0x82, 0x68, 0x82, 0x71, 0x82, 0x62, 0x00}
};

#define UN2CH_G_BOURBON_URL_SIZE	(5)
static const char *g_bourbon_url[UN2CH_G_BOURBON_URL_SIZE] = {
	"bg20.2ch.net",
	"bg21.2ch.net",
	"bg22.2ch.net",
	"bg23.2ch.net",
	"bg24.2ch.net"
};

/* 短パンマン ★ (Shift-JIS) */
static const char g_tanpan[] = { 0x92, 0x5A, 0x83, 0x70, 0x83, 0x93, 0x83, 0x7d, 0x83, 0x93, 0x20, 0x81, 0x9a, '\0'};
/* 名古屋はエ～エ～で (Shift-JIS) */
static const char g_nagoya[] = { 0x96, 0xBC, 0x8C, 0xC3, 0x89, 0xAE, 0x82, 0xCD, 0x83, 0x47, 0x81, 0x60, 0x83, 0x47, 0x81, 0x60, 0x82, 0xC5, '\0'};

un2ch_t* un2ch_init(void)
{
	un2ch_t *init = malloc(sizeof(un2ch_t));
	if(init == NULL){
		perror("un2ch_init:");
		return NULL;
	}
	memset(init, 0, sizeof(un2ch_t));
	init->byte = 0;						/* datのデータサイズ */
	init->mod = 0;						/* datの最終更新時間 */
	init->code = 0;						/* HTTPステータスコード */
	init->mode = UN2CH_MODE_NOTING;
	init->server = unstr_init_memory(UN2CH_SERVER_LENGTH);
	init->board = unstr_init_memory(UN2CH_BOARD_LENGTH);
	init->thread = unstr_init_memory(UN2CH_THREAD_LENGTH);
	init->thread_index = unstr_init_memory(UN2CH_THREAD_INDEX_LENGTH);
	init->thread_number = unstr_init_memory(UN2CH_THREAD_NUMBER_LENGTH);
	init->error = unstr_init_memory(UN2CH_CHAR_LENGTH);
	init->folder = unstr_init(UN2CH_DAT_SAVE_FOLDER);
	init->board_list = unstr_init(UN2CH_BBS_DATA_FILENAME);
	init->board_subject = unstr_init(UN2CH_BOARD_SUBJECT_FILENAME);
	init->board_setting = unstr_init(UN2CH_BOARD_SETTING_FILENAME);
	init->logfile = unstr_init_memory(UN2CH_CHAR_LENGTH);
	init->bourbon = false;
	init->bourbon_count = 0;
	return init;
}

void un2ch_free_func(un2ch_t *init)
{
	if(init == NULL) return;
	unstr_delete(
		11,
		init->server,
		init->board,
		init->thread,
		init->thread_index,
		init->thread_number,
		init->error,
		init->folder,
		init->board_list,
		init->board_subject,
		init->board_setting,
		init->logfile
	);
	free(init);
}

static bool set_thread(un2ch_t *init, unstr_t *unstr)
{
	char *tmp = 0;
	size_t count = 0;
	if((init == NULL) || unstr_empty(unstr)){
		return false;
	}
	tmp = unstr->data;
	while((tmp[count] >= '0') && (tmp[count] <= '9')){
		count++;
	}
	if((count < 9) || (count > 10)){
		return false;
	}
	unstr_substr(init->thread_number, unstr, count);
	unstr_sprintf(init->thread, "%$.dat", init->thread_number);
	unstr_substr(init->thread_index, init->thread_number, 4);
	return true;
}

static bool server_check(unstr_t *str)
{
	if(unstr_empty(str)){
		return false;
	}
	if(str->length > 8){
		if(strcmp(&(str->data[str->length - 8]), ".2ch.net") == 0){
			return true;
		}
		if(str->length > 12){
			if(strcmp(&(str->data[str->length - 12]), ".bbspink.com") == 0){
				return true;
			}
		}
	}
	return false;
}

static un2ch_code_t mode_change(un2ch_t *init)
{
	un2ch_code_t ret = UN2CH_OK;
	if((!unstr_empty(init->thread_number)) &&
	   (!unstr_empty(init->board)) &&
	   (!unstr_empty(init->server))){
		unstr_sprintf(init->logfile, "%$/%$/%$/%$/%$",
			init->folder, init->server, init->board, init->thread_index, init->thread);
		init->mode = UN2CH_MODE_THREAD;
		ret = UN2CH_OK;
	} else if((!unstr_empty(init->board)) &&
			  (!unstr_empty(init->server))){
		unstr_sprintf(init->logfile, "%$/%$/%$/%s",
			init->folder, init->server, init->board, UN2CH_BOARD_SUBJECT_FILENAME);
		init->mode = UN2CH_MODE_BOARD;
		ret = UN2CH_OK;
	} else {
		init->mode = UN2CH_MODE_NOTING;
		ret = UN2CH_NOACCESS;
	}
	return ret;
}

un2ch_code_t un2ch_set_info(un2ch_t *init, unstr_t *server, unstr_t *board, unstr_t *thread_number)
{
	un2ch_code_t ret = UN2CH_OK;
	init->mode = UN2CH_MODE_NOTING;
	unstr_zero(init->server);
	unstr_zero(init->board);
	unstr_zero(init->thread_number);
	unstr_zero(init->thread);
	unstr_zero(init->thread_index);
	unstr_zero(init->logfile);

	if(unstr_empty(server) || unstr_empty(board)){
		init->mode = UN2CH_MODE_SERVER;
		ret = UN2CH_OK;
		return ret;
	}
	unstr_strcat(init->server, server);
	unstr_strcat(init->board, board);
	if(!server_check(init->server)){
		ret = UN2CH_NOSERVER;
	} else if(in_array(init->server->data, g_sabakill, UN2CH_G_SABAKILL_SIZE)){
		ret = UN2CH_NOACCESS;
	} else {
		if(thread_number){
			set_thread(init, thread_number);
		}
		ret = mode_change(init);
	}
	return ret;
}

un2ch_code_t un2ch_setopt(un2ch_t *init, un2chopt_t opt, ...)
{
	va_list list;
	unstr_t *unstr = 0;
	char *str = 0;
	un2ch_code_t ret = UN2CH_OK;
	va_start(list, opt);
	switch(opt){
	case UN2CHOPT_SERVER:
		unstr = va_arg(list, unstr_t *);
		unstr_strcpy(init->server, unstr);
		break;
	case UN2CHOPT_SERVER_CHAR:
		str = va_arg(list, char *);
		unstr_strcpy_char(init->server, str);
		break;
	case UN2CHOPT_BOARD:
		unstr = va_arg(list, unstr_t *);
		unstr_strcpy(init->board, unstr);
		break;
	case UN2CHOPT_BOARD_CHAR:
		str = va_arg(list, char *);
		unstr_strcpy_char(init->board, str);
		break;
	case UN2CHOPT_THREAD:
		unstr = va_arg(list, unstr_t *);
		unstr_strcpy(init->thread_number, unstr);
		break;
	case UN2CHOPT_THREAD_CHAR:
		str = va_arg(list, char *);
		unstr_strcpy_char(init->thread_number, str);
		break;
	default:
		ret = UN2CH_NOACCESS;
		break;
	}
	va_end(list);
	return ret;
}

unstr_t* un2ch_get_data(un2ch_t *init)
{
	unstr_t *data = 0;
	if(init->bourbon){
		data = bourbon_data(init);
		init->bourbon_count++;
		if(init->bourbon_count > UN2CH_BOURBON_LIMIT){
			init->bourbon = false;
			init->bourbon_count = 0;
		}
	} else {
		data = normal_data(init);
	}
	return data;
}

static unstr_t* normal_data(un2ch_t *init)
{
	/* データ取得 */
	unstr_t *data = 0;
	unstr_t *logfile = init->logfile;
	time_t timestp = 0;
	time_t mod = 0;
	data = request(init, true);
	timestp = time(NULL);
	
	if(init->mode == UN2CH_MODE_THREAD){
		switch(init->code){
		case 200:
			create_cache(init, data, UN2CH_CACHE_FOLDER);
			break;
		case 206:
			create_cache(init, data, UN2CH_CACHE_EDIT);
			unstr_free(data);
			if((data = unstr_file_get_contents(logfile)) != NULL){
				init->byte = data->length;
			} else {
				fprintf(stderr, "206 error\n");
			}
			break;
		case 304:
			unstr_free(data);
			if((data = unstr_file_get_contents(logfile)) != NULL){
				init->byte = data->length;
			} else {
				fprintf(stderr, "304 error\n");
				/* 鯖情報取得 */
				un2ch_get_server(init);
			}
			break;
		case 416:
			unstr_free(data);
			if((data = request(init, false)) != NULL){
				create_cache(init, data, UN2CH_CACHE_OVERWRITE);
			} else {
				fprintf(stderr, "416 error\n");
			}
			break;
		case 301:
		case 302:
		case 404:
			unstr_free(data);
			if((data = unstr_file_get_contents(logfile)) != NULL){
				init->byte = data->length;
				if(init->bourbon == false){
					mod = timestp + (60 * 60 * 24 * 365 * 5);
					touch(logfile, mod, mod); /* 未来の時間をセットしておく */
				}
			}
			break;
		case 0:
			unstr_free(data);
			if((data = unstr_file_get_contents(logfile)) != NULL){
				init->byte = data->length;
			}
			break;
		default:
			unstr_free(data);
			if((data = unstr_file_get_contents(logfile)) != NULL){
				init->byte = data->length;
			} else {
				/* 鯖情報取得 */
				un2ch_get_server(init);
			}
			break;
		}
	} else if(init->mode == UN2CH_MODE_BOARD){
		switch(init->code){
		case 200:
		case 206:
			create_cache(init, data, UN2CH_CACHE_OVERWRITE);
			break;
		case 304:
			unstr_free(data);
			if((data = unstr_file_get_contents(logfile)) != NULL){
				init->byte = data->length;
			}
			break;
		case 301:
		case 302:
		case 404:
			unstr_free(data);
			un2ch_get_server(init);
			break;
		case 0:
			unstr_free(data);
			if((data = unstr_file_get_contents(logfile)) != NULL){
				init->byte = data->length;
			}
			break;
		default:
			unstr_free(data);
			/* 鯖情報取得 */
			un2ch_get_server(init);
			break;
		}
	}
	return data;
}

static unstr_t* bourbon_data(un2ch_t *init)
{
	struct stat st;
	unstr_t *logfile = init->logfile;
	unstr_t *data = 0;
	unstr_t *tmp = 0;
	time_t times = time(NULL);

	if((init->mode == UN2CH_MODE_THREAD) && file_exists(logfile, &st)){
		if(st.st_mtime > times){
			init->code = 302;
			return unstr_file_get_contents(logfile);
		}
	}

	data = bourbon_request(init);
	if(unstr_empty(data)){
		unstr_free(data);	
		return NULL;
	}

	tmp = unstr_substr_char(data->data, 256);
	if((strstr(tmp->data, g_tanpan) != NULL) ||
	   (strstr(tmp->data, g_nagoya) != NULL))
	{
		unstr_free(data);
		init->code = 302;
		if(file_exists(logfile, NULL)){
			time_t mod = times + (60 * 60 * 24 * 365 * 5);
			data = unstr_file_get_contents(logfile);
			touch(logfile, mod, mod);
		} else {
			data = NULL;
		}
	} else {
		create_cache(init, data, UN2CH_CACHE_FOLDER);
	}
	unstr_free(tmp);
	return data;
}

/* 板一覧取得 */
bool un2ch_get_server(un2ch_t *init)
{
	size_t index = 0;
	unstr_t *p1 = 0;
	unstr_t *p2 = 0;
	unstr_t *p3 = 0;
	unstr_t *writedata = 0;
	unstr_t *list = 0;
	unstr_t *tmp = unstr_init(UN2CH_BBS_DATA_URL);
	unstr_t *line = 0;
	time_t mod = 0;
	struct stat st;
	const char *filter[UN2CH_G_SABAFILTER_SIZE];

	line = unstr_get_http_file(tmp, &mod);
	if(unstr_empty(line)){
		unstr_delete(2, tmp, line);
		return false;
	}
	/* 更新時間の確認 */
	if(file_exists(init->board_list, &st)){
		if(st.st_mtime >= mod){
			unstr_delete(2, tmp, line);
			return false;
		}
	}
	
	filter[0] = g_sabafilter[0];
	filter[1] = g_sabafilter[1];
	filter[2] = g_sabafilter[2];
	filter[3] = g_sabafilter[3];
	writedata = unstr_init_memory(UN2CH_CHAR_LENGTH);
	p1 = unstr_init_memory(UN2CH_CHAR_LENGTH);
	p2 = unstr_init_memory(UN2CH_CHAR_LENGTH);
	p3 = unstr_init_memory(UN2CH_CHAR_LENGTH);
	unstr_strcpy(tmp, line);
	list = unstr_strtok(tmp, "\n", &index);
	unstr_zero(line);
	while(list != NULL){
		if(unstr_sscanf(list, "<B>$</B>", p1) == 1){
			if(!in_array(p1->data, filter, UN2CH_G_SABAFILTER_SIZE)){
				/* 書き込む */
				unstr_strcat(line, p1);
				unstr_strcat_char(line, "\n");
			}
		} else if(unstr_sscanf(list, "<A HREF=http://$/$/>$</A>", p1, p2, p3) == 3){
			if((strstr(p1->data, ".2ch.net") != NULL) || (strstr(p1->data, ".bbspink.com") != NULL)){
				if(!in_array(p1->data, g_sabakill, UN2CH_G_SABAKILL_SIZE)){
					/* 書き込む */
					unstr_sprintf(writedata, "%$/%$<>%$\n", p1, p2, p3);
					unstr_strcat(line, writedata);
				}
			}
		}
		unstr_free(list);
		list = unstr_strtok(tmp, "\n", &index);
	}
	unstr_file_put_contents(init->board_list, line, "w");
	unstr_delete(7, list, tmp, line, writedata, p1, p2, p3);
	return true;
}

static bool in_array(const char *str, const char **array, size_t size)
{
	int i = 0;
	bool ret = false;
	for(i = 0; i < size; i++){
		if(strcmp(str, array[i]) == 0){
			ret = true;
			break;
		}
	}
	return ret;
}

static bool file_exists(unstr_t *filename, struct stat *data)
{
	struct stat st;
	bool ret = false;
	if(stat(filename->data, &st) == 0){
		if(data) *data = st;
		if(S_ISREG(st.st_mode) || S_ISDIR(st.st_mode)){
			ret = true;
		}
	}
	return ret;
}

static size_t returned_data(void *ptr, size_t size, size_t nmemb, void *data)
{
	size_t length = size * nmemb;
	unstr_t *sfer = (unstr_t *)data;
	if((sfer->length + length + 1) >= sfer->heap){
		unstr_alloc(sfer, UN2CH_TCP_IP_FRAME_SIZE + length + 1);
	}
	memcpy(&(sfer->data[sfer->length]), ptr, length);
	sfer->length += length;
	sfer->data[sfer->length] = '\0';
	return length;
}

/* 板名取得 */
unstr_t* un2ch_get_board_name(un2ch_t *init)
{
	unstr_t *url = 0;
	unstr_t *title = 0;
	unstr_t *data = 0;
	unstr_t *set = unstr_sprintf(NULL, "%$/%$/%$/setting.txt",
		init->folder, init->server, init->board);
	time_t times = time(NULL);
	time_t mod = 0;
	struct stat st;
	
	if(file_exists(set, &st)){
		if(st.st_mtime > times){
			data = unstr_file_get_contents(set);
			title = slice_board_name(data);
			unstr_delete(2, set, data);
			return title;
		}
	}

	url = unstr_sprintf(NULL, "http://%$/%$/%s", init->server, init->board, UN2CH_BOARD_SETTING_FILENAME);
	/* HTTP接続で設定ファイル取得 */
	data = unstr_get_http_file(url, NULL);
	if(unstr_empty(data)){
		unstr_delete(3, set, url, data);
		return NULL;
	}

	make_dir_all(set);
	unstr_file_put_contents(set, data, "w");
	title = slice_board_name(data);
	if(!unstr_empty(title)){
		mod = times + (60 * 60 * 24 * 7);
		/* ファイル更新時刻の変更 */
		touch(set, mod, mod);
	}
	/* 領域解放 */
	unstr_delete(3, set, url, data);
	return title;
}

static unstr_t* unstr_get_http_file(unstr_t *url, time_t *mod)
{
	long code = 0;
	unstr_t *getdata = 0;
	CURLcode res;
	CURL* curl;
	/* curl */
	curl = curl_easy_init();
	if(curl == NULL) return NULL;
	/* データ格納用 */
	curl_easy_setopt(curl, CURLOPT_URL, url->data);
	curl_easy_setopt(curl, CURLOPT_ENCODING, "gzip");
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
	curl_easy_setopt(curl, CURLOPT_FILETIME, 1);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, returned_data);
	getdata = unstr_init_memory(UN2CH_TCP_IP_FRAME_SIZE);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, getdata);
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
	/* データを取得する */
	res = curl_easy_perform(curl);
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
	if(mod != NULL){
		curl_easy_getinfo(curl, CURLINFO_FILETIME, (long *)mod);
	}
	/* 接続を閉じる */
	curl_easy_cleanup(curl);
	if(unstr_empty(getdata) || (code != 200)){
		unstr_free(getdata);
		getdata = NULL;
	}
	return getdata;
}

static unstr_t* slice_board_name(unstr_t *data)
{
	unstr_t *title = unstr_init_memory(128);
	if(unstr_sscanf(data, "BBS_TITLE=$\n", title) != 1){
		unstr_free(title);
		title = NULL;
	}
	return title;
}

/* header送信 */
static unstr_t* request(un2ch_t *init, bool flag)
{
	CURLcode res;
	CURL* curl;
	struct curl_slist *header = 0;
	unstr_t *str = 0;
	/* レスポンス格納用 */
	unstr_t *getdata = 0;
	/* スレッド一覧取得用header生成 */
	unstr_t *tmp = 0;
	/* スレッドデータサイズ */
	size_t data_size = 0;
	/* レスポンスヘッダー格納用 */
	char *header_data = 0;
	/* レスポンスヘッダーサイズ */
	long header_size = 0;
	/* ファイル更新時間 */
	time_t timestp = 0;
	time_t times = 0;
	char strtime[UN2CH_CHAR_LENGTH] = {0};
	struct stat st;

	/* curl */
	curl = curl_easy_init();
	if(curl == NULL) return NULL;

	/* スレッド一覧取得用header生成 */
	tmp = unstr_init_memory(UN2CH_CHAR_LENGTH);

	init->bourbon = false;
	init->code = 0;
	init->mod = 0;
	init->byte = 0;

	if(init->mode == UN2CH_MODE_THREAD){
		/* dat取得用header生成 */
		unstr_sprintf(tmp, "GET /%$/dat/%$ HTTP/1.1", init->board, init->thread);
		header = curl_slist_append(header, tmp->data);
		unstr_sprintf(tmp, "Host: %$", init->server);
		header = curl_slist_append(header, tmp->data);
		unstr_sprintf(tmp, "User-Agent: %s", UN2CH_VERSION);
		header = curl_slist_append(header, tmp->data);
		
		if(file_exists(init->logfile, &st) && flag){
			times = time(NULL);
			if(st.st_mtime > times){
				init->code = 302;
				unstr_free(tmp);
				curl_slist_free_all(header);
				curl_easy_cleanup(curl);
				return NULL;
			} else {
				/* ファイル更新時刻を代入 */
				timestp = st.st_mtime;
				/* 1バイト引いて取得する */
				unstr_sprintf(tmp, "Range: bytes=%d-", st.st_size - 1);
				header = curl_slist_append(header, tmp->data);
				/* Sat, 29 Oct 1994 19:43:31 GMT */
				strftime(strtime, UN2CH_CHAR_LENGTH - 1, "%a, %d %b %Y %H:%M:%S %Z", gmtime(&timestp));
				unstr_sprintf(tmp, "If-Modified-Since: %s", strtime);
				header = curl_slist_append(header, tmp->data);
			}
		} else {
			/* 差分取得には使えないためここで設定 */
			curl_easy_setopt(curl, CURLOPT_ENCODING, "gzip");
		}
		header = curl_slist_append(header, "Connection: close");
		unstr_sprintf(tmp, "http://%$/%$/dat/%$", init->server, init->board, init->thread);
		curl_easy_setopt(curl, CURLOPT_URL, tmp->data);
	} else if(init->mode == UN2CH_MODE_BOARD){
		/* スレッド一覧取得用header生成 */
		unstr_sprintf(tmp, "GET /%$/%s HTTP/1.1", init->board, UN2CH_BOARD_SUBJECT_FILENAME);
		header = curl_slist_append(header, tmp->data);
		unstr_sprintf(tmp, "Host: %$", init->server);
		header = curl_slist_append(header, tmp->data);
		unstr_sprintf(tmp, "User-Agent: %s", UN2CH_VERSION);
		header = curl_slist_append(header, tmp->data);
		header = curl_slist_append(header, "Connection: close");
		curl_easy_setopt(curl, CURLOPT_ENCODING, "gzip");
		unstr_sprintf(tmp, "http://%$/%$/%s", init->server, init->board, UN2CH_BOARD_SUBJECT_FILENAME);
		curl_easy_setopt(curl, CURLOPT_URL, tmp->data);
	} else {
		init->code = 0;
		unstr_free(tmp);
		curl_slist_free_all(header); /* 現段階ではNULL */
		curl_easy_cleanup(curl);
		return NULL;
	}
	/* 領域解放 */
	unstr_free(tmp);

	/* headerをセットする */
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
	/* headerを出力させる */
	curl_easy_setopt(curl, CURLOPT_HEADER, 1);
	/* タイムアウトを指定 */
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);
	/* スレッドセーフに */
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
	/* 400以上のステータスが帰ってきたら本文は取得しない */
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
	/* ファイル更新時間を取得 */
	curl_easy_setopt(curl, CURLOPT_FILETIME, 1);

	/* コールバック関数を指定 */
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, returned_data);
	/* レスポンス格納用 */
	getdata = unstr_init_memory(UN2CH_TCP_IP_FRAME_SIZE);
	/* データの入れ物を設定 */
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, getdata);

	/* データを取得 */
	res = curl_easy_perform(curl);
	if(res != CURLE_OK){
		unstr_free(getdata);
		curl_slist_free_all(header);
		curl_easy_cleanup(curl);
		return NULL;
	}
	/* headerを解放 */
	curl_slist_free_all(header);

	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &(init->code));
	curl_easy_getinfo(curl, CURLINFO_FILETIME, (long *)&(init->mod));
	curl_easy_getinfo(curl, CURLINFO_HEADER_SIZE, &header_size);

	/* 接続を閉じる */
	curl_easy_cleanup(curl);

	/* Locationを監視、バーボン検知 */
	if(header_size > 0){
		/* ダウンロードしたサイズ */
		init->byte = getdata->length - header_size;
		/* あぼーん検知 */
		if(flag && (init->code == 206) && (init->byte > 1)){
			if(getdata->data[header_size] == '\n'){
				header_size++;
			} else {
				init->code = 416;
			}
		}
		data_size = getdata->length - header_size;
		/* ヘッダーと本文との境目を\0に */
		getdata->data[header_size - 1] = '\0';
		if((header_data = strstr(getdata->data, "Location:")) != NULL){
			if((header_data = strstr(header_data, "403")) != NULL){
				init->bourbon = true;
			}
		}
		if(data_size > 0){
			str = unstr_substr_char(getdata->data + header_size, data_size);
		}
	} else {
		/* ダミー領域 */
		init->code = 304;
	}
	unstr_free(getdata);
	return str;
}

static unstr_t* bourbon_request(un2ch_t *init)
{
	CURLcode res;
	CURL* curl;
	struct curl_slist *header = 0;
	/* レスポンス格納用 */
	unstr_t *getdata = 0;
	/* スレッド一覧取得用header生成 */
	unstr_t *tmp = 0;
	unstr_t *host = 0;

	/* curl */
	curl = curl_easy_init();
	if(curl == NULL) return NULL;

	init->code = 0;
	init->mod = time(NULL);
	init->byte = 0;

	/* TODO:スレッドセーフな乱数にしたい */
	host = unstr_init(g_bourbon_url[clock() % UN2CH_G_BOURBON_URL_SIZE]);

	tmp = unstr_init_memory(UN2CH_CHAR_LENGTH);

	if(init->mode == UN2CH_MODE_THREAD){
		/* dat取得用header生成 */
		unstr_sprintf(tmp, "GET /test/r.so/%$/%$/%$/ HTTP/1.1", init->server, init->board, init->thread_number);
		header = curl_slist_append(header, tmp->data);
		unstr_sprintf(tmp, "Host: %$", host);
		header = curl_slist_append(header, tmp->data);
		unstr_sprintf(tmp, "User-Agent: %s", UN2CH_VERSION);
		header = curl_slist_append(header, tmp->data);
		/* 差分取得には使えないためここで設定 */
		curl_easy_setopt(curl, CURLOPT_ENCODING, "gzip");
		header = curl_slist_append(header, "Connection: close");
		unstr_sprintf(tmp, "http://%$/test/r.so/%$/%$/%$/", host, init->server, init->board, init->thread_number);
		curl_easy_setopt(curl, CURLOPT_URL, tmp->data);
	} else if(init->mode == UN2CH_MODE_BOARD){
		/* スレッド一覧取得用header生成 */
		unstr_sprintf(tmp, "GET /test/p.so/%$/%$/ HTTP/1.1", init->server, init->board);
		header = curl_slist_append(header, tmp->data);
		unstr_sprintf(tmp, "Host: %$", host);
		header = curl_slist_append(header, tmp->data);
		unstr_sprintf(tmp, "User-Agent: %s", UN2CH_VERSION);
		header = curl_slist_append(header, tmp->data);
		curl_easy_setopt(curl, CURLOPT_ENCODING, "gzip");
		header = curl_slist_append(header, "Connection: close");
		unstr_sprintf(tmp, "http://%$/test/p.so/%$/%$/", host, init->server, init->board);
		curl_easy_setopt(curl, CURLOPT_URL, tmp->data);
	} else {
		init->code = 0;
		unstr_delete(2, host, tmp);
		curl_slist_free_all(header); /* 現段階ではNULL */
		curl_easy_cleanup(curl);
		return NULL;
	}
	/* 領域解放 */
	unstr_delete(2, host, tmp);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
	curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10);
	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
	/* 400以上のステータスが帰ってきたら本文は取得しない */
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, returned_data);
	getdata = unstr_init_memory(UN2CH_TCP_IP_FRAME_SIZE);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, getdata);
	res = curl_easy_perform(curl);
	curl_slist_free_all(header);
	if(res != CURLE_OK){
		unstr_free(getdata);
		getdata = NULL;
	} else {
		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &(init->code));
	}
	curl_easy_cleanup(curl);
	return getdata;
}

static void touch(unstr_t *filename, time_t atime, time_t mtime)
{
	struct utimbuf buf = {atime, mtime};
	utime(filename->data, &buf);
}

static bool create_cache(un2ch_t *init, unstr_t *data, un2ch_cache_t flag)
{
	if(unstr_empty(data)){
		return false;
	}

	switch(flag){
	case UN2CH_CACHE_FOLDER:
		make_dir_all(init->logfile);
		break;
	case UN2CH_CACHE_EDIT:
		if(data->data[0] == '\n'){
			unstr_zero(data);
		}
		break;
	case UN2CH_CACHE_OVERWRITE:
		if(!file_exists(init->logfile, NULL)){
			make_dir_all(init->logfile);
		}
		break;
	default:
		return false;
	}
	
	/* ファイルに書き込む */
	if(!unstr_empty(data)){
		if(flag == UN2CH_CACHE_EDIT){
			unstr_file_put_contents(init->logfile, data, "a"); /* 追記 */
		} else {
			unstr_file_put_contents(init->logfile, data, "w"); /* 上書き */
		}
		/* If-Modified-Sinceをセット */
		touch(init->logfile, init->mod, init->mod);
	} else {
		return false;
	}
	return true;
}

static bool make_dir_all(unstr_t *path)
{
	int mode = 0755;
	unstr_t *p1 = 0;
	unstr_t *p2 = 0;
	unstr_t *create = 0;
	unstr_t *tmp = 0;
	size_t count = 0;
	if(file_exists(path, NULL)){
		return false;
	}
	p1 = unstr_init_memory(16);
	p2 = unstr_init_memory(32);
	create = unstr_init_memory(32);
	tmp = unstr_copy(path);
	/* 下から順番に確認 */
	while((count = unstr_sscanf(tmp, "/$/$", p1, p2)) == 2){
		unstr_strcat_char(create, "/");
		unstr_strcat(create, p1);
		if(!file_exists(create, NULL)){
			if(mkdir(create->data, mode) != 0){
				perror("make_dir_all:");
				return false;
			}
		}
		unstr_sprintf(tmp, "/%$", p2);
	}
	unstr_delete(4, p1, p2, tmp, create);
	return true;
}
