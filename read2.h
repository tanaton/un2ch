#ifndef INCLUDE_READ_H
#define INCLUDE_READ_H

#include <oniguruma.h>
#include <unstring.h>
#include "unarray.h"

#define READ2CH_GET_BOARD_PATH	("/2ch/getboard.data")
#define READ2CH_MYSQL_CONF_PATH	("./un2ch.conf")

typedef enum {
	READ2CH_NORMAL = 0,
	READ2CH_FAVO,
	READ2CH_NOFAVO
} read2ch_exe_t;

typedef struct mysql_config_st {
	unstr_t *host;
	unstr_t *user;
	unstr_t *db;
	unstr_t *passwd;
	unsigned int port;
	unstr_t *unix_socket;
	unsigned long client_flag;
} mysql_config_t;

typedef struct read2ch_config_st {
	mysql_config_t mysql_config;
	unstr_t *favolist_path;
	read2ch_exe_t type;
} read2ch_config_t;

typedef struct nich_st {
	unstr_t *server;
	unstr_t *board;
	unstr_t *thread;
	int board_no;
	regex_t *reg;
} nich_t;

typedef struct databox_st {
	unarray_t	*bl;
	unstr_t		*key;
	read2ch_config_t *conf;
} databox_t;

#endif
