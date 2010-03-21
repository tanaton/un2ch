#include "un2ch.h"

#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <unistd.h>
#include <utime.h>
#include <curl/curl.h>

static bool in_array(const char *str, char **array, size_t size);
static bool set_thread(un2ch_t *init, unstr_t *unstr);
static bool server_check(unstr_t *str);
static bool mode_change(un2ch_t *init);
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
static void make_folder(un2ch_t *init);

static char *sabakill[11] = {
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

static char *bourbon_url[5] = {
	"bg20.2ch.net",
	"bg21.2ch.net",
	"bg22.2ch.net",
	"bg23.2ch.net",
	"bg24.2ch.net"
};

un2ch_t* un2ch_init(void)
{
	un2ch_t *init = malloc(sizeof(un2ch_t));
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
	if((init == NULL) || (unstr == NULL) || (unstr->data == NULL)){
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
	if((str == NULL) || (str->data == NULL))
		return false;
	if(strstr(str->data, ".2ch.net"))
		return true;
	if(strstr(str->data, ".bbspink.com"))
		return true;
	return false;
}

static bool mode_change(un2ch_t *init)
{
	un2ch_code_t ret = UN2CH_OK;
	if(	init->thread_number &&
			(init->thread_number->length > 0) &&
		init->board &&
			(init->board->length > 0) &&
		init->server &&
			(init->server->length > 0)){
		unstr_sprintf(init->thread, "%$.dat", init->thread_number);
		unstr_substr(init->thread_index, init->thread_number, 4);
		unstr_sprintf(init->logfile, "%$/%$/%$/%$/%$",
			init->folder, init->server, init->board, init->thread_index, init->thread);
		init->mode = UN2CH_MODE_THREAD;
		ret = UN2CH_OK;
	} else if(	init->board &&
					(init->board->length > 0) &&
				init->server &&
					(init->server->length > 0)){
		unstr_sprintf(init->logfile, "%$/%$/%$/%s",
			init->folder, init->server, init->board, UN2CH_BOARD_SUBJECT_FILENAME);
		init->mode = UN2CH_MODE_BOARD;
		ret = UN2CH_OK;
	} else {
		ret = UN2CH_NOACCESS;
	}
	return ret;
}

un2ch_code_t un2ch_set_info(un2ch_t *init, unstr_t *server, unstr_t *board, unstr_t *thread_number)
{
	un2ch_code_t ret = UN2CH_OK;
	unstr_zero(init->server);
	unstr_zero(init->board);
	unstr_zero(init->thread_number);
	unstr_zero(init->thread);
	unstr_zero(init->thread_index);
	unstr_zero(init->logfile);

	if((server == NULL) || (board == NULL)){
		init->mode = UN2CH_MODE_SERVER;
		ret = UN2CH_OK;
		return ret;
	}
	unstr_strcat(init->server, server);
	unstr_strcat(init->board, board);
	if(!server_check(init->server)){
		ret = UN2CH_NOSERVER;
	} else if(in_array(init->server->data, sabakill, 11)){
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
		unstr_free(init->server);
		unstr = va_arg(list, unstr_t *);
		break;
	case UN2CHOPT_BOARD:
		unstr_free(init->board);
		unstr = va_arg(list, unstr_t *);
		break;
	case UN2CHOPT_THREAD:
		unstr_free(init->thread_number);
		unstr = va_arg(list, unstr_t *);
		break;
	case UN2CHOPT_SERVER_CHAR:
		unstr_free(init->server);
		str = va_arg(list, char *);
		break;
	case UN2CHOPT_BOARD_CHAR:
		unstr_free(init->board);
		str = va_arg(list, char *);
		break;
	case UN2CHOPT_THREAD_CHAR:
		unstr_free(init->thread_number);
		str = va_arg(list, char *);
		break;
	default:
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
	/* 短パンマン ★ (Shift-JIS) */
	char tanpan[] = { 0x92, 0x5A, 0x83, 0x70, 0x83, 0x93, 0x83, 0x7d, 0x83, 0x93, 0x20, 0x81, 0x9a, '\0'};
	/* 名古屋はエ～エ～で (Shift-JIS) */
	char nagoya[] = { 0x96, 0xBC, 0x8C, 0xC3, 0x89, 0xAE, 0x82, 0xCD, 0x83, 0x47, 0x81, 0x60, 0x83, 0x47, 0x81, 0x60, 0x82, 0xC5, '\0'};

	if((init->mode == UN2CH_MODE_THREAD) && file_exists(logfile, &st)){
		if(st.st_mtime > times){
			init->code = 302;
			data = unstr_file_get_contents(logfile);
			return data;
		}
	}

	data = bourbon_request(init);
	if(!data) return NULL;

	tmp = unstr_substr_char(data->data, 256);
	if((strstr(tmp->data, tanpan) != NULL) ||
	   (strstr(tmp->data, nagoya) != NULL))
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
	unstr_t *writedata = unstr_init_memory(UN2CH_CHAR_LENGTH);
	unstr_t *list = 0;
	unstr_t *tmp = unstr_init(UN2CH_BBS_DATA_URL);
	unstr_t *line = 0;
	time_t mod = 0;
	struct stat st;

	line = unstr_get_http_file(tmp, &mod);
	if(!line){
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
	
	p1 = unstr_init_memory(UN2CH_CHAR_LENGTH);
	p2 = unstr_init_memory(UN2CH_CHAR_LENGTH);
	p3 = unstr_init_memory(UN2CH_CHAR_LENGTH);
	unstr_strcpy(tmp, line);
	list = unstr_strtok(tmp, "\n", &index);
	unstr_zero(line);
	while(list != NULL){
		if(unstr_sscanf(list, "<B>$</B>", p1) == 1){
			/* 書き込む */
			unstr_strcat(line, p1);
			unstr_strcat_char(line, "\n");
		} else if(unstr_sscanf(list, "<A HREF=http://$/$/>$</A>", p1, p2, p3) == 3){
			if((strstr(p1->data, ".2ch.net") != NULL) || (strstr(p1->data, ".bbspink.com") != NULL)){
				if(!in_array(p1->data, sabakill, 11)){
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

static bool in_array(const char *str, char **array, size_t size)
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
	if(stat(filename->data, &st) != 0){
		return ret;
	}
	if(data) *data = st;
	if(S_ISREG(st.st_mode) || S_ISDIR(st.st_mode)){
		ret = true;
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
	if(!data){
		unstr_delete(2, set, url);
		return NULL;
	}

	make_folder(init);
	unstr_file_put_contents(set, data, "w");
	title = slice_board_name(data);
	if(title){
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
	if(!curl) return NULL;
	/* データ格納用 */
	getdata = unstr_init_memory(UN2CH_TCP_IP_FRAME_SIZE);
	curl_easy_setopt(curl, CURLOPT_URL, url->data);
	curl_easy_setopt(curl, CURLOPT_ENCODING, "gzip");
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
	curl_easy_setopt(curl, CURLOPT_FILETIME, 1);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, returned_data);
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
	if((getdata->length <= 0) || (code != 200)){
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
	if(!curl) return NULL;

	/* レスポンス格納用 */
	getdata = unstr_init_memory(UN2CH_TCP_IP_FRAME_SIZE);
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
				unstr_delete(2, tmp, getdata);
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
		unstr_delete(2, tmp, getdata);
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
	/* 400以上のステータスが帰ってきたら本文は取得しない */
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
	/* ファイル更新時間を取得 */
	curl_easy_setopt(curl, CURLOPT_FILETIME, 1);

	/* コールバック関数を指定 */
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, returned_data);
	/* データの入れ物を設定 */
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, getdata);

	/* データを取得 */
	res = curl_easy_perform(curl);
	if(res != CURLE_OK){
		if(init->thread_number->length > 0){
			printf("ERROR %s/%s/%s\n",
				init->server->data, init->board->data, init->thread->data);
		} else {
			printf("ERROR %s/%s\n",
				init->server->data, init->board->data);
		}
		printf("%s\n", curl_easy_strerror(res));
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
	if(!curl) return NULL;

	init->code = 0;
	init->mod = time(NULL);
	init->byte = 0;

	/* TODO:スレッドセーフな乱数にしたい */
	host = unstr_init(bourbon_url[clock() % 5]);
	/* 停止すると調子が良くなる？ */
	sleep(1);

	getdata = unstr_init_memory(UN2CH_TCP_IP_FRAME_SIZE);
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
		unstr_delete(2, tmp, getdata);
		curl_easy_cleanup(curl);
		return NULL;
	}
	/* 領域解放 */
	unstr_free(tmp);
	unstr_free(host);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
	/* 400以上のステータスが帰ってきたら本文は取得しない */
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, returned_data);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, getdata);
	res = curl_easy_perform(curl);
	curl_slist_free_all(header);
	if(res != CURLE_OK){
		unstr_free(getdata);
		getdata = NULL;
		printf("%s\n", curl_easy_strerror(res));
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
	if(!data) return false;
	
	switch(flag){
	case UN2CH_CACHE_FOLDER:
		make_folder(init);
		break;
	case UN2CH_CACHE_EDIT:
		if(*(data->data) == '\n'){
			*(data->data) = '\0';
		}
		break;
	case UN2CH_CACHE_OVERWRITE:
		if(!file_exists(init->logfile, NULL)){
			make_folder(init);
		}
		break;
	default:
		return false;
	}
	
	/* ファイルに書き込む */
	if(*(data->data) != '\0'){
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

static void make_folder(un2ch_t *init)
{
	/* フォルダの確認＆作成 */
	int mode = 0755;
	unstr_t *path1 = 0;
	unstr_t *path2 = 0;
	unstr_t *path3 = 0;
	path1 = unstr_sprintf(NULL, "%$/%$",
		init->folder, init->server);
	path2 = unstr_sprintf(NULL, "%$/%$/%$",
		init->folder, init->server, init->board);
	if(init->mode == UN2CH_MODE_THREAD){
		path3 = unstr_sprintf(NULL, "%$/%$/%$/%$",
			init->folder, init->server, init->board, init->thread_index);
	}

	if(file_exists(path1, NULL)){
		if(file_exists(path2, NULL)){
			if(path3){
				if(!file_exists(path3, NULL)){
					mkdir(path3->data, mode);
				}
			}
		} else {
			mkdir(path2->data, mode);
			if(path3) mkdir(path3->data, mode);
		}
	} else {
		mkdir(path1->data, mode);
		mkdir(path2->data, mode);
		if(path3) mkdir(path3->data, mode);
	}
	unstr_delete(3, path1, path2, path3);
}

