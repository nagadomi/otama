/*
 * This file is part of otama.
 *
 * Copyright (C) 2012 nagadomi@nurs.or.jp
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License,
 * or any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef OTAMA_DBI_H
#define OTAMA_DBI_H

#include "otama_config.h"
#include <stdarg.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef struct otama_dbi otama_dbi_t;
typedef struct otama_dbi_result otama_dbi_result_t;

#define OTAMA_DBI_PGSQL   "pgsql"
#define OTAMA_DBI_MYSQL   "mysql"
#define OTAMA_DBI_SQLITE3 "sqlite3"

typedef struct otama_dbi_config {
	char driver[32];
	char host[256];
	char port[32];
	char dbname[8192];
	char username[256];
	char password[256];
} otama_dbi_config_t;

void otama_dbi_config_driver(otama_dbi_config_t *config, const char *driver);
void otama_dbi_config_host(otama_dbi_config_t *config, const char *host);
void otama_dbi_config_port(otama_dbi_config_t *config, int port);
void otama_dbi_config_username(otama_dbi_config_t *config, const char *username);
void otama_dbi_config_password(otama_dbi_config_t *config, const char *password);
void otama_dbi_config_dbname(otama_dbi_config_t *config, const char *dbname);

otama_dbi_t *otama_dbi_new(const otama_dbi_config_t *config);
int otama_dbi_open(otama_dbi_t *dbi);
int otama_dbi_active(otama_dbi_t *dbi);
void otama_dbi_close(otama_dbi_t **dbi);

const char *otama_dbi_escape(otama_dbi_t *dbi, char *esc, size_t len, const char *s);
int otama_dbi_table_exist(otama_dbi_t *dbi, int *exist, const char *table_name);

int otama_dbi_exec(otama_dbi_t *dbi, const char *sql);
int otama_dbi_execf(otama_dbi_t *dbi, const char *fmt, ...);
otama_dbi_result_t *otama_dbi_query(otama_dbi_t *dbi, const char *query);
otama_dbi_result_t *otama_dbi_queryf(otama_dbi_t *dbi, const char *fmt, ...);
int otama_dbi_begin(otama_dbi_t *dbi);
int otama_dbi_commit(otama_dbi_t *dbi);
int otama_dbi_rollback(otama_dbi_t *dbi);

int otama_dbi_result_next(otama_dbi_result_t *res);
int otama_dbi_result_seek(otama_dbi_result_t *res, int64_t j);

int64_t otama_dbi_result_int64(otama_dbi_result_t *res, int i);
int otama_dbi_result_is_null(otama_dbi_result_t *res, int i);	
int32_t otama_dbi_result_int(otama_dbi_result_t *res, int i);
float otama_dbi_result_float(otama_dbi_result_t *res, int i);
time_t otama_dbi_result_time(otama_dbi_result_t *res, int i);
int otama_dbi_result_bool(otama_dbi_result_t *res, int i);
const char *otama_dbi_result_string(otama_dbi_result_t *res, int i);
void otama_dbi_result_free(otama_dbi_result_t **res);

const char *otama_dbi_time_string(char *s, const time_t *t);

int otama_dbi_create_sequence(otama_dbi_t *dbi,
							  const char *sequence_name);
int otama_dbi_sequence_next(otama_dbi_t *dbi,
							int64_t *seq,
							const char *sequence_name);
int otama_dbi_sequence_exist(otama_dbi_t *dbi, int *exist, const char *sequence_name);
int otama_dbi_drop_sequence(otama_dbi_t *dbi,
							const char *sequence_name);

/* table declare */
	
typedef enum {
	OTAMA_DBI_COLUMN_STRING
	,OTAMA_DBI_COLUMN_CHAR
	,OTAMA_DBI_COLUMN_TEXT
	,OTAMA_DBI_COLUMN_INT
	,OTAMA_DBI_COLUMN_INT64
	,OTAMA_DBI_COLUMN_INT64_PKEY_AUTO
	,OTAMA_DBI_COLUMN_FLOAT
	,OTAMA_DBI_COLUMN_TIMESTAMP
	,OTAMA_DBI_COLUMN_BINARY
	,OTAMA_DBI_COLUMN_BOOLEAN
	,OTAMA_DBI_COLUMN_INT_ARRAY
} otama_dbi_column_e;

typedef struct {
	const char *name;
	otama_dbi_column_e type;
	int length1;
	int length2;
	int not_null;
	const char *default_value;
} otama_dbi_column_t;

typedef enum {
	OTAMA_DBI_UNIQUE_FALSE,
	OTAMA_DBI_UNIQUE_TRUE
} otama_dbi_unique_e;

int otama_dbi_create_table(otama_dbi_t *dbi,
						   const char *table_name,
						   const otama_dbi_column_t *columns, int n);

int otama_dbi_drop_table(otama_dbi_t *dbi,
						 const char *table_name);
	

int otama_dbi_create_index(otama_dbi_t *dbi,
						   const char *table_name,
						   otama_dbi_unique_e unique,
						   const otama_dbi_column_t *columns, int n);

#ifdef __cplusplus
}
#endif

#endif
