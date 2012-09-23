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

#ifndef OTAMA_DBI_INTERNAL_H
#define OTAMA_DBI_INTERNAL_H

#include "otama_dbi.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*otama_dbi_close_t)(otama_dbi_t **dbi);
typedef otama_dbi_result_t *(*otama_dbi_query_t)(otama_dbi_t *dbi, const char *query);
typedef int (*otama_dbi_table_exist_t)(otama_dbi_t *dbi, int *exist, const char *table_name);

typedef int (*otama_dbi_result_next_t)(otama_dbi_result_t *res);
typedef int (*otama_dbi_result_seek_t)(otama_dbi_result_t *res, int64_t j);

typedef const char *(*otama_dbi_result_string_t)(otama_dbi_result_t *res, int i);
typedef void (*otama_dbi_result_free_t)(otama_dbi_result_t **res);

typedef const char *(*otama_dbi_escape_t)(otama_dbi_t *res,
										  char *escaped, size_t len, const char *s);

typedef int (*otama_dbi_begin_t)(otama_dbi_t *dbi);
typedef int (*otama_dbi_commit_t)(otama_dbi_t *dbi);
typedef int (*otama_dbi_rollback_t)(otama_dbi_t *dbi);

typedef int (*otama_dbi_create_sequence_t)(otama_dbi_t *dbi, const char *sequence_name);
typedef int (*otama_dbi_drop_sequence_t)(otama_dbi_t *dbi, const char *sequence_name);
typedef int (*otama_dbi_sequence_next_t)(otama_dbi_t *dbi, int64_t *seq, const char *sequence_name);
typedef int (*otama_dbi_sequence_exist_t)(otama_dbi_t *dbi, int *exist, const char *sequence_name);

typedef struct {
	otama_dbi_close_t close;
	otama_dbi_query_t query;
	otama_dbi_escape_t escape;
	otama_dbi_table_exist_t table_exist;
	otama_dbi_result_next_t result_next;
	otama_dbi_result_seek_t result_seek;
	otama_dbi_result_string_t result_string;
	otama_dbi_result_free_t result_free;
	otama_dbi_begin_t begin;
	otama_dbi_commit_t commit;
	otama_dbi_rollback_t rollback;
	otama_dbi_create_sequence_t create_sequence;
	otama_dbi_drop_sequence_t drop_sequence;
	otama_dbi_sequence_next_t sequence_next;
	otama_dbi_sequence_exist_t sequence_exist;
} otama_dbi_func_t;

#if OTAMA_WITH_PGSQL
int otama_dbi_pgsql_open(otama_dbi_t *dbi);
#endif
#if OTAMA_WITH_MYSQL
int otama_dbi_mysql_open(otama_dbi_t *dbi);
#endif
#if OTAMA_WITH_SQLITE3
int otama_dbi_sqlite3_open(otama_dbi_t *dbi);
#endif

struct otama_dbi {
	void *conn;
	otama_dbi_config_t config;
	otama_dbi_func_t func;
};

struct otama_dbi_result {
	void *cursor;
	int64_t index;
	int64_t tuples;
	int fields;
	void *row;              // for mysql
	unsigned long *lengths; // for mysql
	otama_dbi_t *dbi;
};

#ifdef __cplusplus
}
#endif

#endif
