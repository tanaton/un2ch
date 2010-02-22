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
static bool set_thread(unstr_t *thread);
static bool server_check(unstr_t *str);
static unstr_t* normal_data(un2ch_t *init);
static unstr_t* request(un2ch_t *init, bool flag);
static size_t returned_data(void *ptr, size_t size, size_t nmemb, void *data);
static unstr_t* bourbon_data(un2ch_t *init);
static unstr_t* bourbon_request(un2ch_t *init);
static unstr_t* slice_board_name(unstr_t *data);
static unstr_t* unstr_get_http_file(unstr_t *url);
static bool create_cache(un2ch_t *init, unstr_t *data, un_cache_t flag);
static bool file_exists(unstr_t *filename, struct stat *data);
static void touch(unstr_t *filename, time_t atime, time_t mtime);
static void make_folder(un2ch_t *init);

static char *sabakill[10] = {
	"www.2ch.net",
	"info.2ch.net",
	"find.2ch.net",
	"v.isp.2ch.net",
	"m.2ch.net",
	"test.up.bbspink.com",
	"stats.2ch.net",
	"c-au.2ch.net",
	"c-others1.2ch.net",
	"movie.2ch.net"
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
	init->server = unstr_init_memory(UN_SERVER_LENGTH);
	init->board = unstr_init_memory(UN_BOARD_LENGTH);
	init->thread = unstr_init_memory(UN_THREAD_LENGTH);
	init->thread_index = unstr_init_memory(UN_THREAD_INDEX_LENGTH);
	init->thread_number = unstr_init_memory(UN_THREAD_NUMBER_LENGTH);
	init->error = unstr_init_memory(UN_CHAR_LENGTH);
	init->folder = unstr_init(UN_DAT_SAVE_FOLDER);
	init->bbspath = unstr_init(UN_BBS_DATA_FILENAME);
	init->logfile = unstr_init_memory(UN_CHAR_LENGTH);
	init->bourbon = false;
	return init;
}

void un2ch_free(un2ch_t *init)
{
	unstr_delete(9, init->server, init->board, init->thread_number,
		init->thread, init->thread_index, init->logfile, init->bbspath,
		init->error, init->folder);
	free(init);
}

static bool set_thread(unstr_t *thread_number)
{
	char *tmp = thread_number->data;
	char str[UN_THREAD_NUMBER_LENGTH] = {0};
	size_t count = 0;
	while((*tmp >= '0') && (*tmp <= '9')){
		str[count++] = *tmp++;
	}
	if((count < 9) || (count > 10)){
		return false;
	}
	str[count] = '\0';
	unstr_zero(thread_number);
	unstr_strcat_char(thread_number, str);
	return true;
}

static bool server_check(unstr_t *str)
{
	size_t len = str->length;
	int *c = 0;
	int *i = 0;
	if(len < 8) return false;
	c = (int *)(str->data + (len - 8));
	i = (int *)".2ch.net"; /* 8���� */
	if((c[0] == i[0]) && (c[1] == i[1])){
		return true;
	}
	if(len < 12) return false;
	c = (int *)(str->data + (len - 12));
	i = (int *)".bbspink.com"; /* 12���� */
	if((c[0] == i[0]) && (c[1] == i[1]) && (c[2] == i[2])){
		return true;
	}
	return false;
}

un_message_t un2ch_set_info(un2ch_t *init, unstr_t *server, unstr_t *board, unstr_t *thread_number)
{
	unstr_zero(init->server);
	unstr_zero(init->board);
	unstr_zero(init->thread_number);
	unstr_zero(init->thread);
	unstr_zero(init->thread_index);
	unstr_zero(init->logfile);

	if(!(server && board)){
		init->mode = UN_MODE_SERVER;
		return UN_MESSAGE_SACCESS;
	}
	unstr_strcat(init->server, server);
	unstr_strcat(init->board, board);
	if(!server_check(server)){
		return UN_MESSAGE_NOSERVER;
	} else if(in_array(server->data, sabakill, 10)){
		return UN_MESSAGE_NOACCESS;
	} else if(thread_number){
		if(!set_thread(thread_number)){
			return UN_MESSAGE_NOACCESS;
		}
		unstr_strcat(init->thread_number, thread_number);
		unstr_sprintf(init->thread, "%$.dat", init->thread_number);
		unstr_substr(init->thread_index, init->thread_number, 4);
		unstr_sprintf(init->logfile, "%$/%$/%$/%$/%$",
			init->folder, init->server, init->board, init->thread_index, init->thread);
		init->mode = UN_MODE_THREAD;
	} else {
		unstr_sprintf(init->logfile, "%$/%$/%$/%s",
			init->folder, init->server, init->board, UN_BOARD_SETTING_FILENAME);
		init->mode = UN_MODE_BOARD;
	}
	return UN_MESSAGE_SACCESS;
}

un_message_t un2ch_set_info_path(un2ch_t *init, char *path)
{
	unstr_t *path_info = 0;
	size_t num = 0;
	unstr_zero(init->server);
	unstr_zero(init->board);
	unstr_zero(init->thread_number);
	unstr_zero(init->thread);
	unstr_zero(init->thread_index);
	unstr_zero(init->logfile);

	path_info = unstr_substr_char(path, 96); /* 96�����܂Ŏ擾 */
	num = unstr_sscanf(path_info, "/$/$/$/", init->server, init->board, init->thread_number);
	unstr_free(path_info);

	if(num == 0){
		init->mode = UN_MODE_SERVER;
		return UN_MESSAGE_SACCESS;
	} else if(num == 1){
		return UN_MESSAGE_NOACCESS;
	} else if(!server_check(init->server)){
		return UN_MESSAGE_NOSERVER;
	} else if(in_array(init->server->data, sabakill, 10)){
		return UN_MESSAGE_NOACCESS;
	} else if(num == 2){
		unstr_sprintf(init->logfile, "%$/%$/%$/%s",
			init->folder, init->server, init->board, UN_BOARD_SETTING_FILENAME);
		init->mode = UN_MODE_BOARD;
	} else {
		if(!set_thread(init->thread_number)){
			return UN_MESSAGE_NOACCESS;
		}
		unstr_sprintf(init->thread, "%$.dat", init->thread_number);
		unstr_substr(init->thread_index, init->thread_number, 4);
		unstr_sprintf(init->logfile, "%$/%$/%$/%$/%$",
			init->folder, init->server, init->board, init->thread_index, init->thread);
		init->mode = UN_MODE_THREAD;
	}
	return UN_MESSAGE_SACCESS;
}

unstr_t* un2ch_get_data(un2ch_t *init)
{
	unstr_zero(init->server);
	unstr_zero(init->board);
	unstr_zero(init->thread_number);
	unstr_zero(init->thread);
	unstr_zero(init->thread_index);
	unstr_zero(init->logfile);

	if(init->bourbon){
		return bourbon_data(init);
	}
	return normal_data(init);
}

static unstr_t* normal_data(un2ch_t *init)
{
	/* �f�[�^�擾 */
	unstr_t *data = 0;
	unstr_t *logfile = init->logfile;
	time_t timestp = 0;
	time_t mod = 0;
	data = request(init, true);
	timestp = time(NULL);
	
	if(init->mode == UN_MODE_THREAD){
		switch(init->code){
		case 200:
			create_cache(init, data, UN_CACHE_FOLDER);
			break;
		case 206:
			create_cache(init, data, UN_CACHE_EDIT);
			unstr_free(data);
			data = unstr_file_get_contents(logfile);
			init->byte = data->length;
			break;
		case 304:
			unstr_free(data);
			if((data = unstr_file_get_contents(logfile)) != NULL){
				init->byte = data->length;
			} else {
				// �I���擾
				un2ch_get_server(init);
			}
			break;
		case 416:
			unstr_free(data);
			data = request(init, false);
			create_cache(init, data, UN_CACHE_OVERWRITE);
			break;
		case 301:
		case 302:
		case 404:
			unstr_free(data);
			if((data = unstr_file_get_contents(logfile)) != NULL){
				init->byte = data->length;
				if(init->bourbon == false){
					mod = timestp + (3600 * 24 * 365 * 5);
					touch(logfile, mod, mod); /* �����̎��Ԃ��Z�b�g���Ă��� */
				}
			}
			break;
		default:
			unstr_free(data);
			if((data = unstr_file_get_contents(logfile)) != NULL){
				init->byte = data->length;
			} else {
				/* �I���擾 */
				un2ch_get_server(init);
			}
			break;
		}
	} else if(init->mode == UN_MODE_BOARD){
		switch(init->code){
		case 200:
		case 206:
			create_cache(init, data, UN_CACHE_OVERWRITE);
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
			/* �I���擾 */
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
	time_t times = time(NULL);

	if((init->mode == UN_MODE_THREAD) && file_exists(logfile, &st)){
		if(st.st_mtime > times){
			init->code = 302;
			data = unstr_file_get_contents(logfile);
			return data;
		}
	}

	data = bourbon_request(init);
	if(!data) return NULL;

	if((strstr(data->data, "�Z�p���}�� ��") != NULL) ||
	   (strstr(data->data, "���É��̓G�`�G�`��") != NULL))
	{
		unstr_free(data);
		init->code = 302;
		if(file_exists(logfile, NULL)){
			time_t mod = times + (60 * 60 * 24 * 365 * 5);
			data = unstr_file_get_contents(logfile);
			touch(logfile, mod, mod);
		} else {
			return NULL;
		}
	} else {
		create_cache(init, data, UN_CACHE_FOLDER);
	}
	return data;
}

/* �ꗗ�擾 */
bool un2ch_get_server(un2ch_t *init)
{
	char *index = 0;
	unstr_t *p1 = 0;
	unstr_t *p2 = 0;
	unstr_t *p3 = 0;
	unstr_t *writedata = unstr_init_memory(UN_CHAR_LENGTH);
	unstr_t *list = 0;
	unstr_t *tmp = unstr_init(UN_BBS_DATA_URL);
	unstr_t *line = unstr_get_http_file(tmp);
	if(!line){
		unstr_delete(2, tmp, line);
		return false;
	}
	
	p1 = unstr_init_memory(UN_CHAR_LENGTH);
	p2 = unstr_init_memory(UN_CHAR_LENGTH);
	p3 = unstr_init_memory(UN_CHAR_LENGTH);
	unstr_free(tmp);
	tmp = unstr_copy(line);
	index = strtok(tmp->data, "\n");
	unstr_zero(line);
	while(index != NULL){
		list = unstr_init(index);
		if(unstr_sscanf(list, "<B>$</B>", p1) == 1){
			/* �������� */
			unstr_strcat(line, p1);
			unstr_strcat_char(line, "\n");
		} else if(unstr_sscanf(list, "<A HREF=http://$/$/>$</A>", p1, p2, p3) == 3){
			if((strstr(p1->data, ".2ch.net") != NULL) || (strstr(p1->data, ".bbspink.com") != NULL)){
				if(!in_array(p1->data, sabakill, 10)){
					/* �������� */
					unstr_sprintf(writedata, "%$/%$<>%$\n", p1, p2, p3);
					unstr_strcat(line, writedata);
				}
			}
		}
		unstr_free(list);
		index = strtok(NULL, "\n");
	}
	unstr_file_put_contents(init->bbspath, line, "w");
	unstr_delete(7, list, tmp, line, writedata, p1, p2, p3);
	return true;
}

static bool in_array(const char *str, char **array, size_t size)
{
	int i = 0;
	for(i = 0; i < size; i++){
		if(strcmp(str, array[i]) == 0){
			return true;
		}
	}
	return false;
}

static bool file_exists(unstr_t *filename, struct stat *data)
{
	struct stat st;
	if(stat(filename->data, &st) != 0){
		return false;
	}
	if(data) *data = st;
	if(S_ISREG(st.st_mode) || S_ISDIR(st.st_mode)){
		/* �X�V���Ԃ�Ԃ� */
		return true;
	}
	return false;
}

static size_t returned_data(void *ptr, size_t size, size_t nmemb, void *data)
{
	size_t length = size * nmemb;
	unstr_t *sfer = (unstr_t *)data;
	if((sfer->length + length + 1) >= sfer->heap){
		unstr_alloc(sfer, UN_TCP_IP_FRAME_SIZE);
	}
	memcpy(&(sfer->data[sfer->length]), ptr, length);
	sfer->length += length;
	sfer->data[sfer->length] = '\0';
	return length;
}

/* ���擾 */
unstr_t* un2ch_get_board_name(un2ch_t *init)
{
	unstr_t *url = 0;
	unstr_t *title = 0;
	unstr_t *data = 0;
	unstr_t *set = unstr_sprintf(NULL, "%$/%$/%$/%s",
		init->folder, init->server, init->board, UN_BOARD_SETTING_FILENAME);
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

	url = unstr_sprintf(NULL, "http://%$/%$/SETTING.TXT", init->server, init->board);
	/* HTTP�ڑ��Őݒ�t�@�C���擾 */
	data = unstr_get_http_file(url);
	if(!data){
		unstr_delete(2, set, url);
		return NULL;
	}

	make_folder(init);
	unstr_file_put_contents(set, data, "w");
	title = slice_board_name(data);
	if(title){
		mod = (times + (3600 * 24 * 7));
		/* �t�@�C���X�V�����̕ύX */
		touch(set, mod, mod);
	}
	/* �̈��� */
	unstr_delete(3, set, url, data);
	return title;
}

static unstr_t* unstr_get_http_file(unstr_t *url)
{
	unstr_t *getdata = 0;
	CURLcode res;
	CURL* curl;
	/* curl */
	curl = curl_easy_init();
	if(!curl) return NULL;
	/* �f�[�^�i�[�p */
	getdata = unstr_init_memory(UN_TCP_IP_FRAME_SIZE);
	curl_easy_setopt(curl, CURLOPT_URL, url->data);
	curl_easy_setopt(curl, CURLOPT_ENCODING, "gzip");
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, returned_data);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, getdata);
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
	res = curl_easy_perform(curl);
	/* �ڑ������ */
	curl_easy_cleanup(curl);
	if(getdata->length <= 0){
		unstr_free(getdata);
		return NULL;
	}
	return getdata;
}

static unstr_t* slice_board_name(unstr_t *data)
{
	unstr_t *title = unstr_init_memory(128);
	if(unstr_sscanf(data, "BBS_TITLE=$\n", title) != 1){
		unstr_free(title);
	}
	return title;
}

/* header���M */
static unstr_t* request(un2ch_t *init, bool flag)
{
	CURLcode res;
	CURL* curl;
	struct curl_slist *header = 0;
	unstr_t *str = 0;
	/* ���X�|���X�i�[�p */
	unstr_t *getdata = 0;
	/* �X���b�h�ꗗ�擾�pheader���� */
	unstr_t *tmp = 0;
	/* �X���b�h�f�[�^�T�C�Y */
	size_t data_size = 0;
	/* ���X�|���X�w�b�_�[�i�[�p */
	char *header_data = 0;
	/* ���X�|���X�w�b�_�[�T�C�Y */
	long header_size = 0;
	/* �t�@�C���X�V���� */
	time_t timestp = 0;
	time_t times = 0;
	char strtime[UN_CHAR_LENGTH];
	struct stat st;

	/* curl */
	curl = curl_easy_init();
	if(!curl) return NULL;

	/* ���X�|���X�i�[�p */
	getdata = unstr_init_memory(UN_TCP_IP_FRAME_SIZE);
	/* �X���b�h�ꗗ�擾�pheader���� */
	tmp = unstr_init_memory(UN_CHAR_LENGTH);

	init->bourbon = false;
	init->code = 0;
	init->mod = 0;
	init->byte = 0;

	if(init->mode == UN_MODE_THREAD){
		/* dat�擾�pheader���� */
		unstr_sprintf(tmp, "GET /%$/dat/%$ HTTP/1.1", init->board, init->thread);
		header = curl_slist_append(header, tmp->data);
		unstr_sprintf(tmp, "Host: %$", init->server);
		header = curl_slist_append(header, tmp->data);
		unstr_sprintf(tmp, "User-Agent: %s", UN_VERSION);
		header = curl_slist_append(header, tmp->data);
		
		if(file_exists(init->logfile, &st) && flag){
			times = time(NULL);
			if(st.st_mtime > times){
				init->code = 302;
				unstr_delete(2, tmp, getdata);
				return NULL;
			} else {
				/* 1�o�C�g�����Ď擾���� */
				unstr_sprintf(tmp, "Range: bytes=%d-", st.st_size - 1);
				header = curl_slist_append(header, tmp->data);
				/* Sat, 29 Oct 1994 19:43:31 GMT */
				strftime(strtime, UN_CHAR_LENGTH, "%a, %d %b %Y %H:%M:%S %Z", gmtime(&timestp));
				unstr_sprintf(tmp, "If-Modified-Since: %s", strtime);
				header = curl_slist_append(header, tmp->data);
			}
		} else {
			/* �����擾�ɂ͎g���Ȃ����߂����Őݒ� */
			curl_easy_setopt(curl, CURLOPT_ENCODING, "gzip");
		}
		header = curl_slist_append(header, "Connection: close");
		unstr_sprintf(tmp, "http://%$/%$/dat/%$", init->server, init->board, init->thread);
		curl_easy_setopt(curl, CURLOPT_URL, tmp->data);
	} else if(init->mode == UN_MODE_BOARD){
		/* �X���b�h�ꗗ�擾�pheader���� */
		unstr_sprintf(tmp, "GET /%$/subject.txt HTTP/1.1", init->board);
		header = curl_slist_append(header, tmp->data);
		unstr_sprintf(tmp, "Host: %$", init->server);
		header = curl_slist_append(header, tmp->data);
		unstr_sprintf(tmp, "User-Agent: %s", UN_VERSION);
		header = curl_slist_append(header, tmp->data);
		header = curl_slist_append(header, "Connection: close");

		curl_easy_setopt(curl, CURLOPT_ENCODING, "gzip");
		unstr_sprintf(tmp, "http://%$/%$/subject.txt", init->server, init->board);
		curl_easy_setopt(curl, CURLOPT_URL, tmp->data);
	} else {
		init->code = 0;
		unstr_delete(2, tmp, getdata);
		return NULL;
	}
	/* �̈��� */
	unstr_free(tmp);

	/* header���Z�b�g���� */
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
	/* header���o�͂����� */
	curl_easy_setopt(curl, CURLOPT_HEADER, 1);
	/* �^�C���A�E�g���w�� */
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
	/* 400�ȏ�̃X�e�[�^�X���A���Ă�����{���͎擾���Ȃ� */
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
	/* �t�@�C���X�V���Ԃ��擾 */
	curl_easy_setopt(curl, CURLOPT_FILETIME, 1);

	/* �R�[���o�b�N�֐����w�� */
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, returned_data);
	/* �f�[�^�̓��ꕨ��ݒ� */
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, getdata);

	/* �f�[�^���擾 */
	res = curl_easy_perform(curl);
	if(res != CURLE_OK){
		unstr_free(getdata);
		return str; /* NULL */
	}
	/* header����� */
	curl_slist_free_all(header);

	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &(init->code));
	curl_easy_getinfo(curl, CURLINFO_FILETIME, (long *)&(init->mod));
	curl_easy_getinfo(curl, CURLINFO_HEADER_SIZE, &header_size);

	/* �ڑ������ */
	curl_easy_cleanup(curl);

	/* Location���Ď��A�o�[�{�����m */
	if(header_size > 0){
		/* �_�E�����[�h�����T�C�Y */
		init->byte = getdata->length - header_size;
		/* ���ځ[�񌟒m */
		if(flag && (init->code == 206) && (init->byte > 1)){
			if(getdata->data[header_size] == '\n'){
				header_size++;
			} else {
				init->code = 416;
			}
		}
		data_size = getdata->length - header_size;
		if(data_size > 0){
			/* �w�b�_�[�Ɩ{���Ƃ̋��ڂ�\0�� */
			getdata->data[header_size - 1] = '\0';
			if((header_data = strstr(getdata->data, "Location:")) != NULL){
				if((header_data = strstr(header_data, "403/")) != NULL){
					init->bourbon = true;
				}
			}
			str = unstr_substr_char(getdata->data + header_size, data_size);
		}
	} else {
		/* �_�~�[�̈� */
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
	/* ���X�|���X�i�[�p */
	unstr_t *getdata = 0;
	/* �X���b�h�ꗗ�擾�pheader���� */
	unstr_t *tmp = 0;
	unstr_t *host = 0;

	/* curl */
	curl = curl_easy_init();
	if(!curl) return NULL;

	init->code = 0;
	init->mod = time(NULL);
	init->byte = 0;

	srand(init->mod);
	host = unstr_init(bourbon_url[rand() % 5]);

	getdata = unstr_init_memory(UN_TCP_IP_FRAME_SIZE);
	tmp = unstr_init_memory(UN_CHAR_LENGTH);

	if(init->mode == UN_MODE_THREAD){
		/* dat�擾�pheader���� */
		unstr_sprintf(tmp, "GET /test/r.so/%$/%$/%$/ HTTP/1.1", init->server, init->board, init->thread_number);
		header = curl_slist_append(header, tmp->data);
		unstr_sprintf(tmp, "Host: %$", init->server);
		header = curl_slist_append(header, tmp->data);
		unstr_sprintf(tmp, "User-Agent: %s", UN_VERSION);
		header = curl_slist_append(header, tmp->data);
		/* �����擾�ɂ͎g���Ȃ����߂����Őݒ� */
		curl_easy_setopt(curl, CURLOPT_ENCODING, "gzip");
		header = curl_slist_append(header, "Connection: close");
		unstr_sprintf(tmp, "http://%$/test/r.so/%$/%$/%$/", host, init->server, init->board, init->thread_number);
		curl_easy_setopt(curl, CURLOPT_URL, tmp->data);
	} else if(init->mode == UN_MODE_BOARD){
		/* �X���b�h�ꗗ�擾�pheader���� */
		unstr_sprintf(tmp, "GET /test/p.so/%$/%$/ HTTP/1.1", init->server, init->board);
		header = curl_slist_append(header, tmp->data);
		unstr_sprintf(tmp, "Host: %$", init->server);
		header = curl_slist_append(header, tmp->data);
		unstr_sprintf(tmp, "User-Agent: %s", UN_VERSION);
		header = curl_slist_append(header, tmp->data);
		header = curl_slist_append(header, "Connection: close");
		curl_easy_setopt(curl, CURLOPT_ENCODING, "gzip");
		unstr_sprintf(tmp, "http://%$/test/p.so/%$/%$/", host, init->server, init->board);
		curl_easy_setopt(curl, CURLOPT_URL, tmp->data);
	} else {
		init->code = 0;
		unstr_delete(2, tmp, getdata);
		return NULL;
	}
	/* �̈��� */
	unstr_free(tmp);
	unstr_free(host);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
	curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
	/* 400�ȏ�̃X�e�[�^�X���A���Ă�����{���͎擾���Ȃ� */
	curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, returned_data);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, getdata);
	res = curl_easy_perform(curl);
	curl_slist_free_all(header);
	curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &(init->code));
	curl_easy_cleanup(curl);
	printf("%s\n", getdata->data);
	return getdata;
}

static void touch(unstr_t *filename, time_t atime, time_t mtime)
{
	struct utimbuf buf = {atime, mtime};
	utime(filename->data, &buf);
}

static bool create_cache(un2ch_t *init, unstr_t *data, un_cache_t flag)
{
	if(!data) return false;
	
	switch(flag){
	case UN_CACHE_FOLDER:
		make_folder(init);
		break;
	case UN_CACHE_EDIT:
		if(*(data->data) == '\n'){
			*(data->data) = '\0';
		}
		break;
	case UN_CACHE_OVERWRITE:
		if(!file_exists(init->logfile, NULL)){
			make_folder(init);
		}
		break;
	default:
		return false;
	}
	
	/* �t�@�C���ɏ������� */
	if(*(data->data) != '\0'){
		if(flag == UN_CACHE_EDIT){
			unstr_file_put_contents(init->logfile, data, "a"); /* �ǋL */
		} else {
			unstr_file_put_contents(init->logfile, data, "w"); /* �㏑�� */
		}
		/* If-Modified-Since���Z�b�g */
		touch(init->logfile, init->mod, init->mod);
	} else {
		return false;
	}
	return true;
}

static void make_folder(un2ch_t *init)
{
	/* �t�H���_�̊m�F���쐬 */
	int mode = 0755;
	unstr_t *path1 = 0;
	unstr_t *path2 = 0;
	unstr_t *path3 = 0;
	path1 = unstr_sprintf(unstr_init_memory(64), "%$/%$",
		init->folder, init->server);
	path2 = unstr_sprintf(unstr_init_memory(96), "%$/%$/%$",
		init->folder, init->server, init->board);
	if(init->mode == UN_MODE_THREAD){
		path3 = unstr_sprintf(unstr_init_memory(128), "%$/%$/%$/%$",
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

