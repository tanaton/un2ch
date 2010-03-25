#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>

#include "unstring.h"

static void *unstr_malloc(size_t size);
static void *unstr_realloc(void *p, size_t size, size_t len);
static unstr_bool_t unstr_check_heap_size(unstr_t *str, size_t size);

//! メモリを確保し領域をしるしで埋める。
/*!
 * \param[in] size 確保する領域のサイズ
 * \return 確保した領域へのポインタ
 */
static void *unstr_malloc(size_t size)
{
	void *p = malloc(size);
	if(p == NULL){
		/*! 領域の確保に失敗した場合、perrorを呼び出し終了する。 */
		perror("unstr_malloc");
	} else {
		memset(p, UNSTRING_MEMORY_STAMP, size);
	}
	return p;
}

//! 領域を拡張し、新しい領域をしるしで埋める。
/*!
 * \param[in] p 領域へのポインタ
 * \param[in] size 新しい領域のサイズ
 * \param[in] len 元々使用していた領域のサイズ
 * \return 拡張した領域へのポインタ
 */
static void *unstr_realloc(void *p, size_t size, size_t len)
{
	p = realloc(p, size);
	if(p == NULL){
		/*! 領域の確保に失敗した場合、perrorを呼び出し終了する。 */
		perror("unstr_realloc");
	} else {
		len++;
		if(size > len){
			memset(((char *)p) + len, UNSTRING_MEMORY_STAMP, size - len);
		}
	}
	return p;
}

//! 文字列を拡張する際に領域の確保が必要か計算する
/*!
 * \param[in] str 計算を行う文字列
 * \param[in] size 拡張する文字列の長さ
 * \return UNSTRING_TRUE:必要、UNSTRING_FALSE:不要
 */
static unstr_bool_t unstr_check_heap_size(unstr_t *str, size_t size)
{
	return (((str->length + size) >= str->heap) ? UNSTRING_TRUE : UNSTRING_FALSE);
}

//! 文字列のバッファを拡張する
/*!
 * \param[in] str 拡張対象
 * \param[in] size 増加させる量
 * \return 無し
 * \public
 */
unstr_t *unstr_alloc(unstr_t *str, size_t size)
{
	if(str == NULL){
		str = unstr_malloc(sizeof(unstr_t));
		str->length = 0;
		str->heap = 0;
		str->data = NULL;
	}
	/*! 頻繁に確保すると良くないらしいので大まかに確保して \n
	 * 確保する回数を減らす。
	 */
	str->heap += ((size / UNSTRING_HEAP_SIZE) + 1) * UNSTRING_HEAP_SIZE;
	str->data = unstr_realloc(str->data, str->heap, str->length);
	return str;
}

//! 引数で渡された文字列をunstr_tで初期化する
/*!
 * \param[in] str 文字列
 * \return unstr_tに変換された文字列
 * \public
 */
unstr_t *unstr_init(const char *str)
{
	size_t size = strlen(str);
	unstr_t *data = unstr_alloc(NULL, size + 1);
	memcpy(data->data, str, size);
	data->data[size] = '\0';
	data->length = size;
	return data;
}

//! 文字列の領域だけ確保して初期化する。
/*!
 * \param[in] size 確保するunstr_t型のバッファサイズ
 * \return 空のunstr_t型
 * \public
 */
unstr_t *unstr_init_memory(size_t size)
{
	unstr_t *data = unstr_alloc(NULL, size);
	/* 長さ0の文字列扱い */
	data->data[0] = '\0';
	data->length = 0;
	return data;
}

//! 文字列(unstr_t)を開放する
/*!
 * \param[in] str 開放するunstr_t型
 * \return 無し
 * \public
 */
void unstr_free_func(unstr_t *str)
{
	if(str != NULL){
		free(str->data);
		str->data = NULL;
	}
	free(str);
}

//! 引数で渡された文字列を開放する
/*!
 * \param[in] size 開放する文字列(unstr_t)の数
 * \param[in] ... 開放する文字列(unstr_t)
 * \return 無し
 * \public
 */
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

//! 空文字列で初期化する。領域は開放しない。
/*!
 * \param[in,out] str 初期化する文字列
 * \return 無し
 * \public
 */
void unstr_zero(unstr_t *str)
{
	if(str != NULL){
		if(str->data != NULL){
			str->data[0] = '\0';
		}
		str->length = 0;
	}
}

//! 文字列用の領域が確保されていることを確認する。
/*!
 * \param[in] str 対象文字列
 * \return 確保済み:UNSTRING_TRUE, 確保なし:UNSTRING_FALSE
 * \public
 */
unstr_bool_t unstr_isset(unstr_t *str)
{
	unstr_bool_t ret = UNSTRING_FALSE;
	if((str != NULL) && (str->data != NULL)){
		ret = UNSTRING_TRUE;
	} else {
		ret = UNSTRING_FALSE;
	}
	return ret;
}

//! 文字列が空またはNULLであるか確認する。
/*!
 * \param[in] str 対象文字列
 * \return 空:UNSTRING_TRUE, 非空:UNSTRING_FALSE
 * \public
 */
unstr_bool_t unstr_empty(unstr_t *str)
{
	unstr_bool_t ret = UNSTRING_FALSE;
	if((str == NULL) || (str->data == NULL) || (str->length <= 0)){
		ret = UNSTRING_TRUE;
	} else {
		ret = UNSTRING_FALSE;
	}
	return ret;
}

//! 文字列の長さを返す
/*!
 * \param[in] str 対象文字列
 * \return 文字列の長さ
 * \public
 */
size_t unstr_strlen(unstr_t *str)
{
	if(unstr_isset(str)){
		return str->length;
	}
	return 0;
}

//! 文字列のコピーを返す。領域の大きさもある程度揃える
/*!
 * \param[in] str コピー元
 * \return コピーした文字列
 * \public
 */
unstr_t *unstr_copy(unstr_t *str)
{
	unstr_t *data = 0;
	if(unstr_isset(str)){
		data = unstr_init_memory(str->heap);
		unstr_strcpy(data, str);
	}
	return data;
}

//! 文字列をコピーする。
/*!
 * \param[in,out] s1 コピー先
 * \param[in] s2 コピー元
 * \return 成功:UNSTRING_TRUE, 失敗:UNSTRING_FALSE
 * \public
 */
unstr_bool_t unstr_strcpy(unstr_t *s1, unstr_t *s2)
{
	if(unstr_isset(s1)){
		unstr_zero(s1);
		return unstr_strcat(s1, s2);
	}
	return UNSTRING_FALSE;
}

//! unstr_t文字列にchar文字列をコピーする。
/*!
 * \param[in,out] s1 コピー先
 * \param[in] s2 コピー元
 * \return 成功:UNSTRING_TRUE, 失敗:UNSTRING_FALSE
 * \public
 */
unstr_bool_t unstr_strcpy_char(unstr_t *s1, const char *s2)
{
	unstr_t *str = 0;
	if((!unstr_isset(s1)) || (s2 == NULL)){
		return UNSTRING_FALSE;
	}
	str = unstr_init(s2);
	unstr_strcpy(s1, str);
	unstr_free(str);
	return UNSTRING_TRUE;
}

//! 文字列を切り出してコピーする
/*!
 * \param[in,out] s1 コピー先
 * \param[in] s2 コピー元
 * \param[in] size コピーする長さ
 * \return 成功:UNSTRING_TRUE, 失敗:UNSTRING_FALSE
 * \public
 */
unstr_bool_t unstr_substr(unstr_t *s1, unstr_t *s2, size_t len)
{
	if((!unstr_isset(s1)) || unstr_empty(s2)) return UNSTRING_FALSE;
	if(s2->length < len){
		len = s2->length;
	}
	if(unstr_check_heap_size(s1, len + 1)){
		unstr_alloc(s1, ((s1->length + len) - s1->heap) + 1);
	}
	unstr_zero(s1);
	memcpy(s1->data, s2->data, len);
	s1->data[len] = '\0';
	s1->length = len;
	return UNSTRING_TRUE;
}

//! char文字列を切り出してunstr_t文字列に変換する
/*!
 * \param[in] str 対象文字列
 * \param[in] size コピーする長さ
 * \return unstr_t文字列
 * \public
 */
unstr_t *unstr_substr_char(const char *str, size_t len)
{
	unstr_t *data = 0;
	unstr_t *copy = 0;
	if(str == NULL) return NULL;
	data = unstr_init_memory(len);
	copy = unstr_init(str);
	unstr_substr(data, copy, len);
	unstr_free(copy);
	return data;
}

//! 文字列を結合する
/*!
 * \param[in,out] s1 結合先文字列
 * \param[in] s2 結合文字列
 * \return 成功:UNSTRING_TRUE, 失敗:UNSTRING_FALSE
 * \public
 */
unstr_bool_t unstr_strcat(unstr_t *s1, unstr_t *s2)
{
	if((!unstr_isset(s1)) || unstr_empty(s2)) return UNSTRING_FALSE;
	if(unstr_check_heap_size(s1, s2->length + 1)){
		unstr_alloc(s1, s2->length);
	}
	memcpy(&(s1->data[s1->length]), s2->data, s2->length);
	s1->length += s2->length;
	s1->data[s1->length] = '\0';
	return UNSTRING_TRUE;
}

//! unstr_t文字列にchar文字列を結合する
/*!
 * \param[in,out] str 結合先文字列
 * \param[in] c 結合文字列
 * \return 成功:UNSTRING_TRUE, 失敗:UNSTRING_FALSE
 * \public
 */
unstr_bool_t unstr_strcat_char(unstr_t *str, const char *c)
{
	unstr_t *data = 0;
	unstr_bool_t ret = UNSTRING_FALSE;
	if((!unstr_isset(str)) || (c == NULL)) return UNSTRING_FALSE;
	data = unstr_init(c);
	ret = unstr_strcat(str, data);
	unstr_free(data);
	return ret;
}

//! 文字列を比較する
/*!
 * \param[in] s1 比較文字列1
 * \param[in] s2 比較文字列2
 * \return エラー時に0x100を返す。他はstrcmp関数と同じ
 * \public
 */
int unstr_strcmp(unstr_t *s1, unstr_t *s2)
{
	if(unstr_isset(s1) && unstr_isset(s2)){
		return strcmp(s1->data, s2->data);
	}
	return 0x100;
}

//! 対象文字列を区切り文字で切り、格納先に格納する。
/*!
 * \param[in] str 対象文字列
 * \param[in] tmp 区切り文字列
 * \param[out] len 配列の長さ
 * \return unstr_tの配列
 * \public
 */
unstr_t **unstr_split(unstr_t *str, const char *tmp, size_t *len)
{
	unstr_t *data = 0;
	unstr_t *s = 0;
	unstr_t **ret = 0;
	size_t size = 0;
	size_t heap = 8;
	if(unstr_empty(str) || tmp == NULL || len == NULL){
		return NULL;
	}
	data = unstr_copy(str);

	s = unstr_strtok(data, tmp, &size);
	if(s != NULL){
		ret = unstr_malloc(sizeof(unstr_t *) * heap);
		do {
			ret[size] = s;
			size++;
			if(size >= heap){
				heap += size;
				unstr_realloc(ret, heap, size);
			}
			s = unstr_strtok(data, tmp, &size);
		} while(s != NULL);
	}
	unstr_free(data);
	*len = size;
	return ret;
}

//! 自動拡張機能付きsprintf。細かいフォーマットには未対応。
/*!
 * \param[in,out] str 格納先
 * \param[in] format フォーマット
 * \param[in] ... 可変引数
 * \return unstr_t文字列
 * \public
 */
unstr_t *unstr_sprintf(unstr_t *str, const char *format, ...)
{
	va_list list;
	unstr_t *unsp = 0;
	char *sp = 0;
	int ip = 0;
	va_start(list, format);
	if(unstr_isset(str)){
		unstr_zero(str);
	} else {
		str = unstr_init_memory(UNSTRING_HEAP_SIZE);
	}
	while(*format != '\0'){
		switch(*format){
		case '%':
			if((*(format + 1)) == '\0') break;
			format++;
			switch(*format){
			case 's':
				sp = va_arg(list, char *);
				unstr_strcat_char(str, sp);
				break;
			case '$':
				unsp = va_arg(list, unstr_t*);
				unstr_strcat(str, unsp);
				break;
			case 'd':
				ip = va_arg(list, int);
				unsp = unstr_itoa(ip, 10);
				unstr_strcat(str, unsp);
				unstr_free(unsp);
				break;
			case 'x':
				ip = va_arg(list, int);
				unsp = unstr_itoa(ip, 16);
				unstr_strcat(str, unsp);
				unstr_free(unsp);
				break;
			case 'X': {
				size_t i = 0;
				ip = va_arg(list, int);
				unsp = unstr_itoa(ip, 16);
				for(i = 0; i < unsp->length; i++){
					unsp->data[i] = toupper(unsp->data[i]);
				}
				unstr_strcat(str, unsp);
				unstr_free(unsp);
				break;
			}
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

//! 文字列を反転させる。破壊的。
/*!
 * \param[in,out] str 対象文字列
 * \return 成功:UNSTRING_TRUE, 失敗:UNSTRING_FALSE
 * \public
 */
unstr_bool_t unstr_reverse(unstr_t *str)
{
	size_t length = 0;
	size_t size = 0;
	size_t count = 0;
	int c = 0;
	if(unstr_empty(str)) return UNSTRING_FALSE;
	length = str->length - 1;
	size = str->length / 2;
	while(count < size){
		c = str->data[count];
		str->data[count++] = str->data[length];
		str->data[length--] = c;
	}
	return UNSTRING_TRUE;
}

//! 数値から文字列を作成する。
/*!
 * \param[in] num 対象数値
 * \param[in] physics 基数
 * \return unstr_t文字列
 * \public
 */
unstr_t *unstr_itoa(int num, size_t physics)
{
	int value = 0;
	int c = 0;
	unstr_bool_t flag = UNSTRING_FALSE;
	size_t number = 0;
	unstr_t *str = 0;
	if((physics < 2) || (physics > 36)){
		return NULL;
	}
	str = unstr_init_memory(UNSTRING_HEAP_SIZE);
	if(num == 0){
		str->data[str->length++] = '0';
	} else if(num < 0){
		if(physics == 10){
			num *= -1;
		}
		flag = UNSTRING_TRUE;
	}
	number = num;
	while(number > 0){
		value = number % physics;
		number = number / physics;
		if(value >= 10){
			c = (value - 10) + 'a';
		} else {
			c = value + '0';
		}
		if(unstr_check_heap_size(str, 1)){
			unstr_alloc(str, 1);
		}
		str->data[str->length++] = c;
	}
	if(physics == 10 && flag){
		if(unstr_check_heap_size(str, 1)){
			unstr_alloc(str, 1);
		}
		str->data[str->length++] = '-';
	}
	str->data[str->length] = '\0';
	unstr_reverse(str);
	return str;
}

//! 頭の良いsubstr
/*!
 * \param[in] data 対象文字列
 * \param[in] format 切り出すフォーマット
 * \param[out] ... 領域が確保されたunstr_t型
 * \return 切り出した文字数を返す
 * \public
 * dataからformat通りに文字列を切り出し、unstr_t型に格納する。\n
 * 対応フォーマットは「$」のみ\n
 * "abcdefg"から"cd"と"fg"を抜き出したい場合、"ab$e$"と書く\n
 * 「$$」と書けば「$」を区切り文字に出来る\n
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

	if(!unstr_isset(data)) return 0;
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
		if(unstr_isset(str)){
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

//! ファイルを丸ごと読み込む
/*!
 * \param[in] filename ファイルパス
 * \return 読み込んだファイルの中身
 * \public
 */
unstr_t *unstr_file_get_contents(unstr_t *filename)
{
	FILE *fp = fopen(filename->data, "r");
	unstr_t *str = 0;
	long size = 0;
	size_t getsize = 0;

	if(fp == NULL) return NULL;
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

	/* ファイルポインタをクローズ */
	fclose(fp);
	return str;
}

//! ファイルに書き出す
/*!
 * \param[in] filename ファイルパス
 * \param[in] data 書き込む内容
 * \param[in] mode 書き込みモード
 * \return 成功:UNSTRING_TRUE, 失敗:UNSTRING_FALSE
 * \public
 */
unstr_bool_t unstr_file_put_contents(unstr_t *filename, unstr_t *data, const char *mode)
{
	size_t size = 0;
	FILE *fp = fopen(filename->data, mode);
	if((fp == NULL) || unstr_empty(data)){
		return UNSTRING_FALSE;
	}
	ftell(fp);
	size = fwrite(data->data, 1, data->length, fp);
	fclose(fp);
	return UNSTRING_TRUE;
}

//! 文字列を置換する。非破壊。
/*!
 * \param[in] data 対象文字列
 * \param[in] search 置換対象文字列
 * \param[in] replace 置換文字列
 * \return 対象文字列から置換対象文字列を置換文字列に置換した文字列
 * \public
 */
unstr_t *unstr_replace(unstr_t *data, unstr_t *search, unstr_t *replace)
{
	unstr_t *string = 0;
	size_t size = 0;
	char *pt = data->data;
	char *index = 0;
	
	if(unstr_empty(data) || unstr_empty(search) || unstr_empty(replace)){
		return NULL;
	}
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
	if(*pt != '\0'){
		size = strlen(pt);
		if(unstr_check_heap_size(string, size)){
			unstr_alloc(string, size);
		}
		memcpy(&(string->data[string->length]), pt, size);
		string->length += size;
	}
	string->data[string->length] = '\0';
	return string;
}

//! クイックサーチのお試し実装
/*!
 * \param[in] text 対象文字列
 * \param[in] search 検索文字列
 * \param[out] size 戻り値配列の要素数
 * \return 検索文字列に一致した文字列の先頭要素をまとめた配列
 * \public
 */
size_t *unstr_quick_search(unstr_t *text, unstr_t *search, size_t *size)
{
	unsigned char *x = (unsigned char *)search->data;
	size_t m = search->length;
	unsigned char *y = (unsigned char *)text->data;
	size_t n = text->length;
	size_t j = 0;
	size_t count = 0;
	size_t table[256] = {0};

	// 初期化
	size_t *array = unstr_malloc(sizeof(size_t) * 16);

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
				array = unstr_realloc(array, (count + 16) * sizeof(size_t), count * sizeof(size_t));
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

//! 文字列をトークンで切り分ける。マルチスレッド対応？
/*!
 * \param[in,out] str 対象文字列。改変されます。
 * \param[in] delim トークン(文字列可)
 * \param[in,out] index インデックス値。次回呼び出し時に必要
 * \return 切り出した文字列
 * \public
 * delimで指定された「文字列」でstrを切り分けます。
 */
unstr_t *unstr_strtok(unstr_t *str, const char *delim, size_t *index)
{
	unstr_t *data = 0;
	char *p = 0;
	char *ptr = 0;
	size_t len = 0;
	size_t i = 0;
	if(index == NULL) return NULL;
	if(unstr_empty(str)) return NULL;
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

