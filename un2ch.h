#ifndef GET2CH_H_INCLUDE
#define GET2CH_H_INCLUDE

#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include "unstring.h"

/* �f�[�^�ۑ��p�X */
#define UN_BBS_DATA_FILENAME		"/virtual/unkar/ita.data"
/* �f�[�^�}�X�^�[URL */
#define UN_BBS_DATA_URL				"http://menu.2ch.net/bbsmenu.html"
/* �o�[�W���� */
#define UN_VERSION					"Monazilla/1.00 (un2ch/0.0.1)"
/* dat�ۊǃt�H���_�� */
#define UN_DAT_SAVE_FOLDER			"/virtual/unkar/public_html/2ch/dat"
/* SETTING.txt�L���b�V���⊮ */
#define UN_BOARD_SETTING_FILENAME	"setting.txt"

#define UN_CHAR_LENGTH				256
#define UN_SERVER_LENGTH			64
#define UN_BOARD_LENGTH				32
#define UN_THREAD_LENGTH			15
#define UN_THREAD_INDEX_LENGTH		4
#define UN_THREAD_NUMBER_LENGTH		11
#define UN_TCP_IP_FRAME_SIZE		40960

/* ���샂�[�h */
typedef enum {
	UN_MODE_SERVER = 1,
	UN_MODE_BOARD,
	UN_MODE_THREAD
} un_mode_t;

/* ���b�Z�[�W */
typedef enum {
	UN_MESSAGE_SACCESS = 0,
	UN_MESSAGE_NOSERVER,
	UN_MESSAGE_NOACCESS,
	UN_MESSAGE_BOURBON
} un_message_t;

/* �L���b�V�����[�h */
typedef enum {
	UN_CACHE_FOLDER = 1,
	UN_CACHE_EDIT,
	UN_CACHE_OVERWRITE
} un_cache_t;

typedef struct un2ch_st {
	size_t byte;						/* dat�̃f�[�^�T�C�Y */
	time_t mod;							/* dat�̍ŏI�X�V���� */
	long code;							/* HTTP�X�e�[�^�X�R�[�h */
	unstr_t *server;
	unstr_t *board;
	unstr_t *thread;
	unstr_t *thread_index;
	unstr_t *thread_number;
	unstr_t *error;
	unstr_t *folder;
	unstr_t *bbspath;
	unstr_t *logfile;
	un_mode_t mode;
	bool bourbon;
} un2ch_t;

un2ch_t* un2ch_init(void);
void un2ch_free(un2ch_t *init);
un_message_t un2ch_set_info(un2ch_t *init, unstr_t *server, unstr_t *board, unstr_t *thread);
un_message_t un2ch_set_info_path(un2ch_t *init, char *path);
unstr_t* un2ch_get_data(un2ch_t *init);
bool un2ch_get_server(un2ch_t *init);
unstr_t* un2ch_get_board_name(un2ch_t *init);

#endif /* un2ch_H_INCLUDE */
