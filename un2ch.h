#ifndef UN2CH_H_INCLUDE
#define UN2CH_H_INCLUDE

#include <stdio.h>
#include <time.h>
#include <stdbool.h>
#include "unstring.h"

/* 板データ保存パス */
#define UN2CH_BBS_DATA_FILENAME			"/2ch/dat/ita.data"
/* 板データマスターURL */
#define UN2CH_BBS_DATA_URL				"http://menu.70.kg/bbsmenu.html"
//#define UN2CH_BBS_DATA_URL				"http://menu.2ch.net/bbsmenu.html"
/* バージョン */
#define UN2CH_VERSION					"Monazilla/1.00 (un2ch/0.0.1)"
/* dat保管フォルダ名 */
#define UN2CH_DAT_SAVE_FOLDER			"/2ch/dat"
#define UN2CH_BOARD_SUBJECT_FILENAME	"subject.txt"
/* SETTING.txtキャッシュ補完 */
#define UN2CH_BOARD_SETTING_FILENAME	"SETTING.TXT"

#define UN2CH_CHAR_LENGTH				(256)
#define UN2CH_SERVER_LENGTH				(64)
#define UN2CH_BOARD_LENGTH				(32)
#define UN2CH_THREAD_LENGTH				(15)
#define UN2CH_THREAD_INDEX_LENGTH		(4)
#define UN2CH_THREAD_NUMBER_LENGTH		(11)
#define UN2CH_TCP_IP_FRAME_SIZE			(40960)
#define UN2CH_BOURBON_LIMIT				(255)

#define un2ch_free(data)				\
	do { un2ch_free_func(data); (data) = NULL; } while(0)

/* 動作モード */
typedef enum {
	UN2CH_MODE_NOTING = 0,
	UN2CH_MODE_SERVER,
	UN2CH_MODE_BOARD,
	UN2CH_MODE_THREAD
} un2ch_mode_t;

/* キャッシュモード */
typedef enum {
	UN2CH_CACHE_FOLDER = 1,
	UN2CH_CACHE_EDIT,
	UN2CH_CACHE_OVERWRITE
} un2ch_cache_t;

/* メッセージ */
typedef enum {
	UN2CH_OK = 0,
	UN2CH_NOSERVER,
	UN2CH_NOACCESS,
	UN2CH_BOURBON
} un2ch_code_t;

typedef enum {
	UN2CHOPT_NONE = 0,
	UN2CHOPT_SERVER,
	UN2CHOPT_SERVER_CHAR,
	UN2CHOPT_BOARD,
	UN2CHOPT_BOARD_CHAR,
	UN2CHOPT_THREAD,
	UN2CHOPT_THREAD_CHAR
} un2chopt_t;

typedef enum {
	UN2CHINFO_NONE = 0,
	UN2CHINFO_RESPONSE_CODE,
	UN2CHINFO_FILETIME
} un2chinfo_t;

typedef struct un2ch_st {
	time_t mod;							/* datの最終更新時間 */
	long code;							/* HTTPステータスコード */
	unstr_t *folder;
	unstr_t *board_list;
	unstr_t *board_subject;
	unstr_t *board_setting;
	unstr_t *logfile;
	unstr_t *server;
	unstr_t *board;
	unstr_t *thread;
	unstr_t *thread_index;
	unstr_t *thread_number;
	unstr_t *error;
	un2ch_mode_t mode;
	bool bourbon;
	size_t bourbon_count;
} un2ch_t;

un2ch_t* un2ch_init(void);
void un2ch_free_func(un2ch_t *init);
un2ch_code_t un2ch_set_info(un2ch_t *init, unstr_t *server, unstr_t *board, unstr_t *thread);
un2ch_code_t un2ch_set_info_path(un2ch_t *init, char *path);
un2ch_code_t un2ch_setopt(un2ch_t *init, un2chopt_t opt, ...);
unstr_t* un2ch_get_data(un2ch_t *init);
bool un2ch_get_server(un2ch_t *init);
unstr_t* un2ch_get_board_name(un2ch_t *init);

#endif /* UN2CH_H_INCLUDE */
