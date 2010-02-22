#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "unstring.h"

static void *unstr_malloc(size_t size);
static void *unstr_realloc(void *p, size_t size);

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
/* �֐���  �Funstr_alloc												*/
/* �߂�l  �Fvoid														*/
/* ����1   �Funstr_t * 			(�g���Ώ�)								*/
/* ����2   �Fsize_t				(�����������)							*/
/* �A�N�Z�X�Fpublic														*/
/* ��  ��  �F������̃o�b�t�@���g������B								*/
/************************************************************************/
void unstr_alloc(unstr_t *str, size_t size)
{
	/* �p�ɂɊm�ۂ���Ɨǂ��Ȃ��炵���̂ő�܂��Ɋm�ۂ��� */
	str->heap += size + UNSTRING_HEAP_SIZE;
	str->data = unstr_realloc(str->data, str->heap);
}

/************************************************************************/
/* �֐���  �Funstr_init													*/
/* �߂�l  �Funstr_t * 													*/
/* ����1   �Fconst char *		(������������)							*/
/* �A�N�Z�X�Fpublic														*/
/* ��  ��  �F�����œn���ꂽ�������unstr_t�ŏ���������B				*/
/************************************************************************/
unstr_t *unstr_init(const char *str)
{
	unstr_t *data = unstr_malloc(sizeof(unstr_t));
	size_t size = strlen(str);
	/* \0������̂ŃT�C�Y�{1�Ŏ擾�B�����͐M�p���Ȃ� */
	data->heap = size + 1;
	data->data = unstr_malloc(data->heap);
	memcpy(data->data, str, size);
	data->data[size] = '\0';
	data->length = size;
	return data;
}

/************************************************************************/
/* �֐���  �Funstr_init_memory											*/
/* �߂�l  �Funstr_t * 													*/
/* ����1   �Fsize_t				(�m�ۂ���̈�̃T�C�Y)					*/
/* �A�N�Z�X�Fpublic														*/
/* ��  ��  �F������̗̈悾���m�ۂ��ď���������B						*/
/************************************************************************/
unstr_t *unstr_init_memory(size_t size)
{
	unstr_t *data = unstr_malloc(sizeof(unstr_t));
	data->heap = size + 1;
	data->data = unstr_malloc(data->heap);
	/* ����0�̕����񈵂� */
	data->data[0] = '\0';
	data->length = 0;
	return data;
}

/************************************************************************/
/* �֐���  �Funstr_check_heap_size										*/
/* �߂�l  �Fint														*/
/* ����1   �Funstr_t * 			(�v�Z���s��������)						*/
/* ����2   �Fsize_t				(�g�����镶����̒���)					*/
/* �A�N�Z�X�Fpublic														*/
/* ��  ��  �F��������g������ۂɗ̈�̊m�ۂ��K�v���v�Z����B			*/
/************************************************************************/
int unstr_check_heap_size(unstr_t *str, size_t size)
{
	return (((str->length + size) >= str->heap) ? 1 : 0);
}

/************************************************************************/
/* �֐���  �Funstr_free_func											*/
/* �߂�l  �Fvoid														*/
/* ����1   �Funstr_t * 			(�J�����镶����)						*/
/* �A�N�Z�X�Fpublic														*/
/* ��  ��  �F��������J������B											*/
/************************************************************************/
void unstr_free_func(unstr_t *str)
{
	if(str != NULL){
		free(str->data);
		free(str);
	}
}

/************************************************************************/
/* �֐���  �Funstr_delete												*/
/* �߂�l  �Fvoid														*/
/* ����1   �Fsize_t				(�J�����镶����̐�)					*/
/* ����2   �F...				(�J�����镶����)						*/
/* �A�N�Z�X�Fpublic														*/
/* ��  ��  �F�����œn���ꂽ��������J������B							*/
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
/* �֐���  �Funstr_zero													*/
/* �߂�l  �Fvoid														*/
/* ����1   �Funstr_t * 			(���������镶����)						*/
/* �A�N�Z�X�Fpublic														*/
/* ��  ��  �F�󕶎���ŏ���������B�̈�͊J�����Ȃ��B					*/
/************************************************************************/
void unstr_zero(unstr_t *str)
{
	if(str->data != NULL){
		str->data[0] = '\0';
	}
	str->length = 0;
}

/************************************************************************/
/* �֐���  �Funstr_copy													*/
/* �߂�l  �Funstr_t * 													*/
/* ����1   �Funstr_t * 			(�Ώە�����)							*/
/* �A�N�Z�X�Fpublic														*/
/* ��  ��  �F��������R�s�[����B�̈�̃T�C�Y���R�s�[����B				*/
/************************************************************************/
unstr_t *unstr_copy(unstr_t *str)
{
	unstr_t *data = unstr_malloc(sizeof(unstr_t));
	data->heap = str->heap;
	data->data = unstr_malloc(str->heap);
	memcpy(data->data, str->data, str->length);
	data->length = str->length;
	data->data[str->length] = '\0';
	return data;
}

/************************************************************************/
/* �֐���  �Funstr_substr												*/
/* �߂�l  �Funstr_t * 													*/
/* ����1   �Funstr_t * 			(�R�s�[��)								*/
/* ����2   �Funstr_t * 			(�R�s�[��)								*/
/* ����3   �Fsize_t				(�R�s�[���钷��)						*/
/* �A�N�Z�X�Fpublic														*/
/* ��  ��  �F����1�Ɉ���2������3�̃T�C�Y�����R�s�[����B				*/
/*			 ������̐擪����擪�փR�s�[����B							*/
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
/* �֐���  �Funstr_substr_char											*/
/* �߂�l  �Funstr_t * 													*/
/* ����1   �Fconst char *		(�Ώە�����)							*/
/* ����2   �Fsize_t				(�R�s�[���钷��)						*/
/* �A�N�Z�X�Fpublic														*/
/* ��  ��  �F��������w�肵�������Ő؂�o���Aunstr_t�^�ŏ���������B	*/
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
/* �֐���  �Funstr_strcat												*/
/* �߂�l  �Funstr_t *													*/
/* ����1   �Funstr_t *			(�����敶����)							*/
/* ����2   �Funstr_t *			(����������)							*/
/* �A�N�Z�X�Fpublic														*/
/* ��  ��  �F��������������A�Ԃ��B										*/
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
/* �֐���  �Funstr_strcat_char											*/
/* �߂�l  �Funstr_t *													*/
/* ����1   �Funstr_t *			(�����敶����)							*/
/* ����2   �Fconst char *		(����������)							*/
/* �A�N�Z�X�Fpublic														*/
/* ��  ��  �Funstr_t�^��char�^���������������B						*/
/************************************************************************/
unstr_t *unstr_strcat_char(unstr_t *str, const char *c)
{
	size_t size = strlen(c);
	if(unstr_check_heap_size(str, size + 1)){
		unstr_alloc(str, size);
	}
	/* \0�܂ŃR�s�[���� */
	memcpy(&(str->data[str->length]), c, size);
	str->length += size;
	str->data[str->length] = '\0';
	return str;
}

/************************************************************************/
/* �֐���  �Funstr_strcmp												*/
/* �߂�l  �Funstr_t *													*/
/* ����1   �Funstr_t *			(��r������1)							*/
/* ����2   �Funstr_t *			(��r������2)							*/
/* �A�N�Z�X�Fpublic														*/
/* ��  ��  �F�������r���s���B											*/
/************************************************************************/
int unstr_strcmp(unstr_t *str1, unstr_t *str2)
{
	return strcmp(str1->data, str2->data);
}

/************************************************************************/
/* �֐���  �Funstr_split												*/
/* �߂�l  �Funstr_t *													*/
/* ����1   �Funstr_t *			(�i�[��)								*/
/* ����2   �Fconst char *		(�Ώە�����)							*/
/* ����3   �Fchar 				(��؂蕶��)							*/
/* �A�N�Z�X�Fpublic														*/
/* ��  ��  �F�Ώە��������؂蕶���Ő؂�A�i�[��Ɋi�[����B			*/
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
/* �֐���  �Funstr_sprintf												*/
/* �߂�l  �Funstr_t *													*/
/* ����1   �Funstr_t *			(�i�[��)								*/
/* ����2   �Fconst char *		(�t�H�[�}�b�g)							*/
/* ����3   �F... 				(�ψ���)								*/
/* �A�N�Z�X�Fpublic														*/
/* ��  ��  �F�����g���@�\�t��sprintf�B�ׂ����t�H�[�}�b�g�ɂ͖��Ή��B	*/
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
/* �֐���  �Funstr_reverse												*/
/* �߂�l  �Funstr_t *													*/
/* ����1   �Funstr_t *			(�Ώە�����)							*/
/* �A�N�Z�X�Fpublic														*/
/* ��  ��  �F������𔽓]������B										*/
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
/* �֐���  �Funstr_itoa													*/
/* �߂�l  �Funstr_t *													*/
/* ����1   �Fint				(�Ώې��l)								*/
/* ����2   �Fint				(�)									*/
/* ����3   �Fint				(�2)									*/
/* �A�N�Z�X�Fpublic														*/
/* ��  ��  �F���l���當������쐬����B									*/
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

/* data����format�ʂ�ɕ������؂�o���Aunstr_t�^�Ɋi�[����B
 * �Ή��t�H�[�}�b�g�́u$�v�̂�
 * "abcdefg"����"cd"��"fg"�𔲂��o�������ꍇ�A"ab$e$"�Ə���
 * �u$$�v�Ə����΁u$�v����؂蕶���ɏo����
 * �擾����������̐���Ԃ��B��L�̏ꍇ�́u2�v��Ԃ�
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
	/* �擪��T������ */
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
		/* ������������\�z */
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
			/* ���� */
			index = strstr(tmp, search->data);
			if(index == NULL){
				steady = strlen(tmp);
				format += strlen(format);
			} else {
				/* �����𐔒l�� */
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
		/* �|�C���^��i�߂� */
		tmp += steady + search->length;
	}
	va_end(list);
	unstr_free(search);
	return count;
}

/* �t�@�C���擾 */
unstr_t *unstr_file_get_contents(unstr_t *filename)
{
	FILE *fp = fopen(filename->data, "r");
	unstr_t *str = 0;
	long size = 0;
	size_t getsize = 0;

	if(!fp) return NULL;
	/* �t�@�C���T�C�Y�����߂� */
	/* �t�@�C���|�C���^���Ō�܂ňړ� */
	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	/* �t�@�C���|�C���^��擪�ɖ߂� */
	rewind(fp);

	/* �t�@�C���T�C�Y���̗̈���m�� */
	str = unstr_init_memory((size_t)size + 1);

	/* �ǂݍ��� */
	getsize = fread(str->data, 1, (size_t)size, fp);
	str->data[getsize] = '\0';
	str->length = getsize;

	/* �t�@�C���|�C���^���N���[�Y�I */
	fclose(fp);
	return str;
}

/* �t�@�C���������� */
void unstr_file_put_contents(unstr_t *filename, unstr_t *data, const char *mode)
{
	FILE *fp = fopen(filename->data, mode);
	size_t size = 0;
	ftell(fp);
	size = fwrite(data->data, 1, data->length, fp);
	fclose(fp);
}

/* �u������ */
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

/* �N�C�b�N�T�[�` */
size_t* unstr_quick_search(unstr_t *text, unstr_t *search, size_t *size)
{
	unsigned char *x = (unsigned char *)search->data;
	size_t m = search->length;
	unsigned char *y = (unsigned char *)text->data;
	size_t n = text->length;
	size_t j = 0;
	size_t count = 0;
	size_t table[256] = {0};

	// ������
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

