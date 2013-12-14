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

#include "otama_config.h"
#if OTAMA_WITH_MYSQL
#if OTAMA_MYSQL_H_INCLUDE
#  include "mysql.h"
#elif OTAMA_MYSQL_H_INCLUDE_MYSQL
#  include "mysql/mysql.h"
#else
#  error "mysql.h not detected"
#endif
#include "nv_core.h"
#include "otama_log.h"
#include "otama_dbi.h"
#include "otama_dbi_internal.h"

static void
otama_dbi_mysql_close(otama_dbi_t **dbi)
{
	if (dbi && *dbi) {
		if ((*dbi)->conn) {
			mysql_close((MYSQL *)(*dbi)->conn);
		}
		nv_free(*dbi);
		*dbi = NULL;
	}
}

static int
otama_dbi_mysql_table_exist(otama_dbi_t *dbi,
							int *exist,
							const char *table_name)
{
	int ret;
	MYSQL_RES *res = mysql_list_tables((MYSQL *)dbi->conn, table_name);

	if (res) {
		ret = 0;
		if (mysql_num_rows(res)	> 0) {
			*exist = 1;
		} else {
			*exist = 0;
		}
		mysql_free_result(res);
	} else {
		ret = -1;
	}
	
	return ret;
}

static const char *
otama_dbi_mysql_escape(otama_dbi_t *dbi,
						 char *esc, size_t len,
						 const char *s)
{
	if (len >= 3) {
		size_t slen = strlen(s);
		if (slen > 0) {
			char *to = nv_alloc_type(char, len * 3 + 1);
			mysql_real_escape_string((MYSQL *)dbi->conn, to, s, slen);
			strcpy(esc, "'");
			strncat(esc, to, len-3);
			strcat(esc, "'");
			nv_free(to);
		} else {
			strcpy(esc,"''");
		}
	} else {
		return NULL;
	}
	return esc;
}

static otama_dbi_result_t *
otama_dbi_mysql_query(otama_dbi_t *dbi, const char *query)
{
	otama_dbi_result_t *res;
	int  ret;
	MYSQL_RES *stmt = NULL;
	
	ret = mysql_query((MYSQL *)dbi->conn, query);
	if (ret != 0) {
		OTAMA_LOG_ERROR("%s: %s", query, mysql_error((MYSQL *)dbi->conn));
		return NULL;
	}
	stmt = mysql_store_result((MYSQL *)dbi->conn);
	res = nv_alloc_type(otama_dbi_result_t, 1);
	res->cursor = stmt;
	res->dbi = dbi;
	res->tuples = stmt == NULL ? 0 : mysql_num_rows(stmt);
	res->fields = stmt == NULL ? 0 : mysql_num_fields(stmt);
	res->row = NULL;
	res->index = 0;
	
	return res;
}

static int
otama_dbi_mysql_result_next(otama_dbi_result_t *res)
{
	if (res->cursor == NULL) {
		return 0;
	}
	res->row = mysql_fetch_row((MYSQL_RES *)res->cursor);
	res->lengths = mysql_fetch_lengths((MYSQL_RES *)res->cursor);
	if (res->row != NULL && res->lengths != NULL) {
		return 1;
	}
	return 0;
}

static int
otama_dbi_mysql_result_seek(otama_dbi_result_t *res, int64_t j)
{
	int ret = -1;
	
	if (res->cursor) {
		if (j < res->tuples) {
			mysql_data_seek((MYSQL_RES *)res->cursor, j);
			res->row = mysql_fetch_row((MYSQL_RES *)res->cursor);
			res->lengths = mysql_fetch_lengths((MYSQL_RES *)res->cursor);
			ret = 0;
		}
	}
	return ret;
}

static int
otama_dbi_mysql_begin(otama_dbi_t *dbi)
{
	return otama_dbi_exec(dbi, "START TRANSACTION;");
}

static int
otama_dbi_mysql_commit(otama_dbi_t *dbi)
{
	return otama_dbi_exec(dbi, "COMMIT;");
}

static int
otama_dbi_mysql_rollback(otama_dbi_t *dbi)
{
	return otama_dbi_exec(dbi, "ROLLBACK;");	
}

static const char *
otama_dbi_mysql_result_string(otama_dbi_result_t *res, int i)
{
	if (i < res->fields && res->row && res->lengths) {
		return (const char *)((MYSQL_ROW *)res->row)[i];
	}
	return NULL;
}

static void
otama_dbi_mysql_result_free(otama_dbi_result_t **res)
{
	if (res && *res) {
		if ((*res)->cursor) {
			mysql_free_result((MYSQL_RES *)(*res)->cursor);
		}
		nv_free(*res);
		*res = NULL;
	}
}

static int
otama_dbi_mysql_create_sequence(otama_dbi_t *dbi, const char *sequence_name)
{
	return otama_dbi_execf(dbi,
						   "CREATE TABLE %s_sequence_ (id bigint primary key auto_increment);",
						   sequence_name);
}

static int
otama_dbi_mysql_drop_sequence(otama_dbi_t *dbi, const char *sequence_name)
{
	return otama_dbi_execf(dbi, "DROP TABLE %s_sequence_;", sequence_name);	
}

static int
otama_dbi_mysql_sequence_next(otama_dbi_t *dbi, int64_t *seq,
							  const char *sequence_name)
{
	char sql[8192];
	int  ret;
	
	nv_snprintf(sql, sizeof(sql) - 1,
				"INSERT INTO %s_sequence_ (id) values(NULL);",
				sequence_name);
	OTAMA_LOG_DEBUG("%s", sql);
	
	ret = mysql_query((MYSQL *)dbi->conn, sql);
	if (ret != 0) {
		OTAMA_LOG_ERROR("%s: %s", sql, mysql_error((MYSQL *)dbi->conn));
		return -1;
	}
	*seq = mysql_insert_id((MYSQL *)dbi->conn);
	if (*seq == 0) {
		OTAMA_LOG_ERROR("%s: %s", sql, mysql_error((MYSQL *)dbi->conn));
		return -1;
	}

	return 0;
}

static int
otama_dbi_mysql_sequence_exist(otama_dbi_t *dbi, int *exist, const char *sequence_name)
{
	char table_name[8192];
	nv_snprintf(table_name, sizeof(table_name) - 1, "%s_sequence_", sequence_name);
	return otama_dbi_table_exist(dbi, exist, table_name);
}

int
otama_dbi_mysql_open(otama_dbi_t *dbi)
{
	MYSQL *conn;
	const char *unix_socket = dbi->config.host[0] == '/' ? dbi->config.host: NULL;
	const char *host = dbi->config.host[0] != '\0' ? dbi->config.host: NULL;
	
	conn = mysql_init(NULL);
	if (mysql_real_connect(conn,
						   host,
						   strlen(dbi->config.username) > 0 ? dbi->config.username : NULL,
						   strlen(dbi->config.password) > 0 ? dbi->config.password : NULL,
						   strlen(dbi->config.dbname) > 0 ? dbi->config.dbname : NULL,
						   strlen(dbi->config.port) > 0 ?
						   strtol(dbi->config.port, NULL, 10) : 0,
						   unix_socket,
						   0) == NULL)
	{
		OTAMA_LOG_ERROR("%s\n", mysql_error(conn));
		mysql_close(conn);
		return -1;
	}
	dbi->conn = conn;
	dbi->func.close = otama_dbi_mysql_close;
	dbi->func.query = otama_dbi_mysql_query;
	dbi->func.escape = otama_dbi_mysql_escape;
	dbi->func.table_exist = otama_dbi_mysql_table_exist;
	dbi->func.result_next = otama_dbi_mysql_result_next;
	dbi->func.result_seek = otama_dbi_mysql_result_seek;
	dbi->func.result_string = otama_dbi_mysql_result_string;
	dbi->func.result_free = otama_dbi_mysql_result_free;
	dbi->func.begin = otama_dbi_mysql_begin;
	dbi->func.commit = otama_dbi_mysql_commit;
	dbi->func.rollback = otama_dbi_mysql_rollback;
	dbi->func.create_sequence = otama_dbi_mysql_create_sequence;
	dbi->func.drop_sequence = otama_dbi_mysql_drop_sequence;
	dbi->func.sequence_next = otama_dbi_mysql_sequence_next;
	dbi->func.sequence_exist = otama_dbi_mysql_sequence_exist;
	
	return 0;
}

#endif
