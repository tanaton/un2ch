#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdbool.h>

#include "unstring.h"

static void *unstr_malloc(size_t size);
static void *unstr_realloc(void *p, size_t size);
static bool unstr_check_heap_size(unstr_t *str, size_t size);

static void *unstr_malloc(size_t size)
{
	void *p = malloc(size);
	if(p == NULL){
		perror("unstr_malloc");
	}
	return p;
}

static void *unstr_realloc(void *p, size_t size)
{
	p = realloc(p, size);
	if(p == NULL){
		perror("unstr_realloc");
	}
	return p;
}

/************************************************************************/
/* 関数名  ：unstr_alloc												*/
/* 戻り値  ：void														*/
/* 引数1   ：unstr_t * 			(拡張対象)								*/
/* 引数2   ：size_t				(増加させる量)							*/
/* アクセス：public														*/
/* 動  作  ：文字列のバッファを拡張する。								*/
/************************************************************************/
void unstr_alloc(unstr_t *str, size_t size)
{
	/* 頻繁に確保すると良くないらしいので大まかに確保する */
	str->heap += size + UNSTRING_HEAP_SIZE;
	str->data = unstr_realloc(str->data, str->heap);
}

/************************************************************************/
/* 関数名  ：unstr_init													*/
/* 戻り値  ：unstr_t * 													*/
/* 引数1   ：const char *		(初期化文字列)							*/
/* アクセス：public														*/
/* 動  作  ：引数で渡された文字列をunstr_tで初期化する。				*/
/************************************************************************/
unstr_t *unstr_init(const char *str)
{
	unstr_t *data = unstr_malloc(sizeof(unstr_t));
	size_t size = strlen(str);
	/* \0があるのでサイズ＋1で取得。引数は信用しない */
	data->heap = size + 1;
	data->data = unstr_malloc(data->heap);
	memcpy(data->data, str, size);
	data->data[size] = '\0';
	data->length = size;
	return data;
}

/************************************************************************/
/* 関数名  ：unstr_init_memory											*/
/* 戻り値  ：unstr_t * 													*/
/* 引数1   ：size_t				(確保する領域のサイズ)					*/
/* アクセス：public														*/
/* 動  作  ：文字列の領域だけ確保して初期化する。						*/
/************************************************************************/
unstr_t *unstr_init_memory(size_t size)
{
	unstr_t *data = unstr_malloc(sizeof(unstr_t));
	data->heap = size + 1;
	data->data = unstr_malloc(data->heap);
	/* 長さ0の文字列扱い */
	data->data[0] = '\0';
	data->length = 0;
	return data;
}

/************************************************************************/
/* 関数名  ：unstr_check_heap_size										*/
/* 戻り値  ：bool														*/
/* 引数1   ：unstr_t * 			(計算を行う文字列)						*/
/* 引数2   ：size_t				(拡張する文字列の長さ)					*/
/* アクセス：public														*/
/* 動  作  ：文字列を拡張する際に領域の確保が必要か計算する。			*/
/************************************************************************/
static bool unstr_check_heap_size(unstr_t *str, size_t size)
{
	return (((str->length + size) >= str->heap) ? true : false);
}

/************************************************************************/
/* 関数名  ：unstr_free_func											*/
/* 戻り値  ：void														*/
/* 引数1   ：unstr_t * 			(開放する文字列)						*/
/* アクセス：public														*/
/* 動  作  ：文字列を開放する。											*/
/************************************************************************/
void unstr_free_func(unstr_t *str)
{
	if(str != NULL){
		free(str->data);
		free(str);
	}
}

/************************************************************************/
/* 関数名  ：unstr_delete												*/
/* 戻り値  ：void														*/
/* 引数1   ：size_t				(開放する文字列の数)					*/
/* 引数2   ：...				(開放する文字列)						*/
/* アクセス：public														*/
/* 動  作  ：引数で渡された文字列を開放する。							*/
/************************************************************************/
void unstr_delete(size_t size, ...)
{
	unstr_t *str = 0;
	va_list list;
	va_start(list, size);
	while(size--){
		str = va_arg(list, unstr_t *);
		unstr_free(str);
	}
	va_end(list);
}

/************************************************************************/
/* 関数名  ：unstr_zero													*/
/* 戻り値  ：void														*/
/* 引数1   ：unstr_t * 			(初期化する文字列)						*/
/* アクセス：public														*/
/* 動  作  ：空文字列で初期化する。領域は開放しない。					*/
/************************************************************************/
void unstr_zero(unstr_t *str)
{
	if(str->data != NULL){
		str->data[0] = '\0';
	}
	str->length = 0;
}

/************************************************************************/
/* 関数名  ：unstr_copy													*/
/* 戻り値  ：unstr_t * 													*/
/* 引数1   ：unstr_t * 			(コピー先)								*/
/* 引数2   ：unstr_t * 			(コピー元)								*/
/* アクセス：public														*/
/* 動  作  ：文字列をコピーする。										*/
/************************************************************************/
unstr_t *unstr_copy(unstr_t *s1, unstr_t *s2)
{
	if(!s2) return NULL;
	if(s1){
		unstr_zero(s1);
		if(unstr_check_heap_size(s1, s2->length + 1)){
			unstr_alloc(s1, ((s1->length + s2->length) - s1->heap) + 2);
		}
	} else {
		s1 = unstr_malloc(sizeof(unstr_t));
		s1->heap = s2->heap;
		s1->data = unstr_malloc(s2->heap);
	}
	memcpy(s1->data, s2->data, s2->length);
	s1->length = s2->length;
	s1->data[s2->length] = '\0';
	return s1;
}

/************************************************************************/
/* 関数名  ：unstr_copy_char											*/
/* 戻り値  ：unstr_t * 													*/
/* 引数1   ：unstr_t * 			(コピー先)								*/
/* 引数2   ：const char * 		(コピー元)								*/
/* アクセス：public														*/
/* 動  作  ：文字列をコピーする。										*/
/************************************************************************/
unstr_t *unstr_copy_char(unstr_t *s1, const char *s2)
{
	unstr_t *str = 0;
	if(!s2) return NULL;
	str = unstr_init(s2);
	unstr_copy(s1, str);
	unstr_free(str);
	return s1;
}

/************************************************************************/
/* 関数名  ：unstr_substr												*/
/* 戻り値  ：unstr_t * 													*/
/* 引数1   ：unstr_t * 			(コピー先)								*/
/* 引数2   ：unstr_t * 			(コピー元)								*/
/* 引数3   ：size_t				(コピーする長さ)						*/
/* アクセス：public														*/
/* 動  作  ：引数1に引数2を引数3のサイズだけコピーする。				*/
/*			 文字列の先頭から先頭へコピーする。							*/
/************************************************************************/
unstr_t *unstr_substr(unstr_t *s1, unstr_t *s2, size_t len)
{
	if(!s2) return NULL;
	if(s2->length < len){
		len = s2->length;
	}
	if(s1 == NULL){
		s1 = unstr_substr_char(s2->data, len);
	} else {
		if(unstr_check_heap_size(s1, len + 1)){
			unstr_alloc(s1, ((s1->length + len) - s1->heap) + 2);
		}
		unstr_zero(s1);
		memcpy(s1->data, s2->data, len);
		s1->data[len] = '\0';
		s1->length = len;
	}
	return s1;
}

/************************************************************************/
/* 関数名  ：unstr_substr_char											*/
/* 戻り値  ：unstr_t * 													*/
/* 引数1   ：const char *		(対象文字列)							*/
/* 引数2   ：size_t				(コピーする長さ)						*/
/* アクセス：public														*/
/* 動  作  ：文字列を指定した長さで切り出し、unstr_t型で初期化する。	*/
/************************************************************************/
unstr_t *unstr_substr_char(const char *str, size_t len)
{
	size_t size = strlen(str);
	unstr_t *data = unstr_malloc(sizeof(unstr_t));
	if(size < len){
		len = size;
	}
	data->heap = len + 1;
	data->data = unstr_malloc(len + 1);
	memcpy(data->data, str, len);
	data->data[len] = '\0';
	data->length = len;
	return data;
}

/************************************************************************/
/* 関数名  ：unstr_strcat												*/
/* 戻り値  ：unstr_t *													*/
/* 引数1   ：unstr_t *			(結合先文字列)							*/
/* 引数2   ：unstr_t *			(結合文字列)							*/
/* アクセス：public														*/
/* 動  作  ：文字列を結合し、返す。										*/
/************************************************************************/
unstr_t *unstr_strcat(unstr_t *s1, unstr_t *s2)
{
	if((!s1) || (!s2)) return NULL;
	if(unstr_check_heap_size(s1, s2->length + 1)){
		unstr_alloc(s1, s2->length);
	}
	memcpy(&(s1->data[s1->length]), s2->data, s2->length);
	s1->length += s2->length;
	s1->data[s1->length] = '\0';
	return s1;
}

/************************************************************************/
/* 関数名  ：unstr_strcat_char											*/
/* 戻り値  ：unstr_t *													*/
/* 引数1   ：unstr_t *			(結合先文字列)							*/
/* 引数2   ：const char *		(結合文字列)							*/
/* アクセス：public														*/
/* 動  作  ：unstr_t型にchar型文字列を結合する。						*/
/************************************************************************/
unstr_t *unstr_strcat_char(unstr_t *str, const char *c)
{
	size_t size = strlen(c);
	if(unstr_check_heap_size(str, size + 1)){
		unstr_alloc(str, size);
	}
	/* \0までコピーする */
	memcpy(&(str->data[str->length]), c, size);
	str->length += size;
	str->data[str->length] = '\0';
	return str;
}

/************************************************************************/
/* 関数名  ：unstr_strcmp												*/
/* 戻り値  ：unstr_t *													*/
/* 引数1   ：unstr_t *			(比較文字列1)							*/
/* 引数2   ：unstr_t *			(比較文字列2)							*/
/* アクセス：public														*/
/* 動  作  ：文字列比較を行う。											*/
/************************************************************************/
int unstr_strcmp(unstr_t *str1, unstr_t *str2)
{
	return strcmp(str1->data, str2->data);
}

/************************************************************************/
/* 関数名  ：unstr_split												*/
/* 戻り値  ：unstr_t *													*/
/* 引数1   ：unstr_t *			(格納先)								*/
/* 引数2   ：const char *		(対象文字列)							*/
/* 引数3   ：char 				(区切り文字)							*/
/* アクセス：public														*/
/* 動  作  ：対象文字列を区切り文字で切り、格納先に格納する。			*/
/************************************************************************/
unstr_t *unstr_split(unstr_t *str, const char *tmp, char c)
{
	if(!str) return NULL;
	unstr_zero(str);
	while(c != *tmp){
		if(*tmp == '\0') break;
		if(unstr_check_heap_size(str, 1)){
			unstr_alloc(str, 1);
		}
		str->data[str->length++] = *tmp++;
	}
	str->data[str->length] = '\0';
	return str;
}

/************************************************************************/
/* 関数名  ：unstr_sprintf												*/
/* 戻り値  ：unstr_t *													*/
/* 引数1   ：unstr_t *			(格納先)								*/
/* 引数2   ：const char *		(フォーマット)							*/
/* 引数3   ：... 				(可変引数)								*/
/* アクセス：public														*/
/* 動  作  ：自動拡張機能付きsprintf。細かいフォーマットには未対応。	*/
/************************************************************************/
unstr_t *unstr_sprintf(unstr_t *str, const char *format, ...)
{
	va_list list;
	unstr_t *unsp = 0;
	char *sp = 0;
	int ip = 0;
	va_start(list, format);
	if(str == NULL){
		str = unstr_init_memory(UNSTRING_HEAP_SIZE);
	} else {
		unstr_zero(str);
	}
	while(*format != '\0'){
		switch(*format){
		case '%':
			if((*(format + 1)) == '\0') break;
			format++;
			switch(*format){
			case 's':
				sp = va_arg(list, char *);
				str = unstr_strcat_char(str, sp);
				break;
			case '$':
				unsp = va_arg(list, unstr_t*);
				str = unstr_strcat(str, unsp);
				break;
			case 'd':
				ip = va_arg(list, int);
				unsp = unstr_itoa(ip, 10, 0);
				str = unstr_strcat(str, unsp);
				unstr_free(unsp);
				break;
			case 'x':
				ip = va_arg(list, int);
				unsp = unstr_itoa(ip, 16, 'a');
				str = unstr_strcat(str, unsp);
				unstr_free(unsp);
				break;
			case 'X':
				ip = va_arg(list, int);
				unsp = unstr_itoa(ip, 16, 'A');
				str = unstr_strcat(str, unsp);
				unstr_free(unsp);
				break;
			default:
				format--;
				break;
			}
			break;
		default:
			if(unstr_check_heap_size(str, 1)){
				unstr_alloc(str, 1);
			}
			str->data[str->length++] = *format;
			break;
		}
		format++;
	}
	str->data[str->length] = '\0';
	va_end(list);
	return str;
}

/************************************************************************/
/* 関数名  ：unstr_reverse												*/
/* 戻り値  ：unstr_t *													*/
/* 引数1   ：unstr_t *			(対象文字列)							*/
/* アクセス：public														*/
/* 動  作  ：文字列を反転させる。										*/
/************************************************************************/
unstr_t *unstr_reverse(unstr_t *str)
{
	size_t length = str->length - 1;
	size_t size = str->length / 2;
	size_t count = 0;
	int c = 0;
	while(count < size){
		c = str->data[count];
		str->data[count++] = str->data[length];
		str->data[length--] = c;
	}
	return str;
}

/************************************************************************/
/* 関数名  ：unstr_itoa													*/
/* 戻り値  ：unstr_t *													*/
/* 引数1   ：int				(対象数値)								*/
/* 引数2   ：int				(基数)									*/
/* 引数3   ：int				(基数2)									*/
/* アクセス：public														*/
/* 動  作  ：数値から文字列を作成する。									*/
/************************************************************************/
unstr_t *unstr_itoa(int num, int kisu, int moji)
{
	int value = 0;
	int c = 0;
	unstr_t *str = 0;
	if((kisu < 1) || (kisu > 36)) return NULL;
	str = unstr_init_memory(16);
	while(num > 0){
		value = num % kisu;
		num = num / kisu;
		if(value >= 10){
			c = (value - 10) + moji;
		} else {
			c = value + '0';
		}
		if(unstr_check_heap_size(str, 1)){
			unstr_alloc(str, 1);
		}
		str->data[str->length++] = c;
	}
	str->data[str->length] = '\0';
	return unstr_reverse(str);
}

/* dataからformat通りに文字列を切り出し、unstr_t型に格納する。
 * 対応フォーマットは「$」のみ
 * "abcdefg"から"cd"と"fg"を抜き出したい場合、"ab$e$"と書く
 * 「$$」と書けば「$」を区切り文字に出来る
 * 取得した文字列の数を返す。上記の場合は「2」を返す
 */
size_t unstr_sscanf(unstr_t *data, const char *format, ...)
{
	va_list list;
	size_t count = 0;
	size_t steady = 0;
	unstr_t *str = 0;
	unstr_t *search = 0;
	char *tmp = data->data;
	char *index = 0;

	if(data == NULL || data->data == NULL) return 0;
	search = unstr_init_memory(UNSTRING_HEAP_SIZE);
	va_start(list, format);
	/* 先頭を探索する */
	if((*format != '$') && (*format != '\0')){
		do {
			if(unstr_check_heap_size(search, 1)){
				unstr_alloc(search, 1);
			}
			search->data[search->length++] = *format++;
		} while((*format != '$') && (*format != '\0'));
		search->data[search->length] = '\0';
		index = strstr(tmp, search->data);
		if(index == NULL){
			tmp += strlen(tmp);
			format += strlen(format);
		} else {
			tmp = index + search->length;
		}
	}
	while(*format != '\0'){
		if(*format++ != '$') continue;
		str = va_arg(list, unstr_t *);
		unstr_zero(search);
		/* 検索文字列を構築 */
		if(*format == '\0'){
			steady = strlen(tmp);
		} else {
			if(*format == '$'){
				search->data[search->length++] = *format++;
			} else do {
				if(unstr_check_heap_size(search, 1)){
					unstr_alloc(search, 1);
				}
				search->data[search->length++] = *format++;
			} while((*format != '$') && (*format != '\0'));
			search->data[search->length] = '\0';
			/* 検索 */
			index = strstr(tmp, search->data);
			if(index == NULL){
				steady = strlen(tmp);
				format += strlen(format);
			} else {
				/* 差分を数値化 */
				steady = (size_t)(index - tmp);
			}
		}
		if(str != NULL){
			unstr_zero(str);
			if(unstr_check_heap_size(str, steady + 1)){
				unstr_alloc(str, steady + 1);
			}
			if(steady > 0){
				memcpy(str->data, tmp, steady);
				str->data[steady] = '\0';
				str->length = steady;
				count++;
			}
		}
		/* ポインタを進める */
		tmp += steady + search->length;
	}
	va_end(list);
	unstr_free(search);
	return count;
}

/* ファイル取得 */
unstr_t *unstr_file_get_contents(unstr_t *filename)
{
	FILE *fp = fopen(filename->data, "r");
	unstr_t *str = 0;
	long size = 0;
	size_t getsize = 0;

	if(!fp) return NULL;
	/* ファイルサイズを求める */
	/* ファイルポインタを最後まで移動 */
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	/* ファイルポインタを先頭に戻す */
	rewind(fp);

	/* ファイルサイズ分の領域を確保 */
	str = unstr_init_memory((size_t)size + 1);

	/* 読み込み */
	getsize = fread(str->data, 1, (size_t)size, fp);
	str->data[getsize] = '\0';
	str->length = getsize;

	/* ファイルポインタをクローズ！ */
	fclose(fp);
	return str;
}

/* ファイル書き込み */
void unstr_file_put_contents(unstr_t *filename, unstr_t *data, const char *mode)
{
	FILE *fp = fopen(filename->data, mode);
	size_t size = 0;
	ftell(fp);
	size = fwrite(data->data, 1, data->length, fp);
	fclose(fp);
}

/* 置換する */
unstr_t *unstr_replace(unstr_t *data, unstr_t *search, unstr_t *replace)
{
	unstr_t *string = 0;
	size_t size = 0;
	char *pt = data->data;
	char *index = 0;
	
	if((!data) || (!search) || (!replace)) return NULL;
	string = unstr_init_memory(data->length);
	while((index = strstr(pt, search->data)) != NULL){
		size = (size_t)(index - pt);
		if(unstr_check_heap_size(string, size + replace->length)){
			unstr_alloc(string, size + replace->length);
		}
		memcpy(&(string->data[string->length]), pt, size);
		string->length += size;
		memcpy(&(string->data[string->length]), replace->data, replace->length);
		string->length += replace->length;
		pt = index + search->length;
	}
	string->data[string->length] = '\0';
	return string;
}

/* クイックサーチ */
size_t* unstr_quick_search(unstr_t *text, unstr_t *search, size_t *size)
{
	unsigned char *x = (unsigned char *)search->data;
	size_t m = search->length;
	unsigned char *y = (unsigned char *)text->data;
	size_t n = text->length;
	size_t j = 0;
	size_t count = 0;
	size_t table[256] = {0};

	// 初期化
	size_t *array = unstr_malloc(16 * sizeof(size_t));

	for(j = 0; j < 256; ++j){
		table[j] = m + 1;
	}
	for(j = 0; j < m; ++j){
		table[x[j]] = m - j;
	}

	j = 0;
	while(j <= n - m){
		if(memcmp(x, y + j, m) == 0){
			array[count++] = j;
			if((count % 16) == 0){
				array = unstr_realloc(array, (count + 16) * sizeof(size_t));
			}
		}
		j += table[y[j + m]];
	}
	if(count == 0){
		free(array);
		array = NULL;
	}
	*size = count;
	return array;
}

/* strtokマルチスレッド対応 */
/* strtokと微妙に仕様が違う */
unstr_t* unstr_strtok(unstr_t *str, const char *delim, size_t *index)
{
	unstr_t *data = 0;
	char *p = 0;
	char *ptr = 0;
	size_t len = 0;
	size_t i = 0;
	if(index == NULL) return NULL;
	if((*index) > str->length) return NULL;
	ptr = str->data + (*index);
	p = strstr(ptr, delim);
	if(p){
		len = strlen(delim);
		for(i = 0; i < len; i++){
			p[i] = '\0';
		}
		*index += (size_t)(p - ptr) + len;
	} else {
		*index = str->length + 1;
	}
	data = unstr_init(ptr);
	return data;
}

