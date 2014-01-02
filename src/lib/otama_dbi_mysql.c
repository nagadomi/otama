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

#define OTAMA_DBI_MYSQL_TEXT_LEN 65535

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

static MYSQL_BIND *
bind_alloc(int fields)
{
	MYSQL_BIND *bind = nv_alloc_type(MYSQL_BIND, fields);
	int i;
	
	memset(bind, 0, sizeof(MYSQL_BIND) * fields);
	for (i = 0; i < fields; ++i) {
		bind[i].buffer_type = MYSQL_TYPE_STRING;
		bind[i].buffer = nv_alloc_type(char, OTAMA_DBI_MYSQL_TEXT_LEN);
		bind[i].buffer_length = OTAMA_DBI_MYSQL_TEXT_LEN;
		bind[i].is_null = nv_alloc_type(my_bool, 1);
		bind[i].error = nv_alloc_type(my_bool, 1);
		bind[i].length = nv_alloc_type(unsigned long, 1);
	}
	return bind;
}

static void
bind_free(MYSQL_BIND **bind, int fields)
{
	if (bind && *bind) {
		int i;
		for (i = 0; i < fields; ++i) {
			nv_free((*bind)[i].buffer);
			nv_free((*bind)[i].is_null);
			nv_free((*bind)[i].error);
			nv_free((*bind)[i].length);
		}
		nv_free(*bind);
		*bind = NULL;
	}
}

static otama_dbi_result_t *
otama_dbi_mysql_query(otama_dbi_t *dbi, const char *query)
{
	otama_dbi_result_t *res;
	MYSQL_STMT *stmt;
	MYSQL_RES *metadata;
	
	stmt = mysql_stmt_init((MYSQL *)dbi->conn);
	if (stmt == NULL) {
		OTAMA_LOG_ERROR("%s", mysql_error((MYSQL *)dbi->conn));
		return NULL;
	}
	if (mysql_stmt_prepare(stmt, query, strlen(query)) != 0
		|| mysql_stmt_execute(stmt) != 0)
	{
		OTAMA_LOG_ERROR("%s: %s", query, mysql_stmt_error(stmt));
		mysql_stmt_close(stmt);
		return NULL;
	}
	metadata = mysql_stmt_result_metadata(stmt);
	res = nv_alloc_type(otama_dbi_result_t, 1);
	memset(res, 0, sizeof(*res));
	res->cursor = stmt;
	res->dbi = dbi;
	res->fields = metadata == NULL ? 0 : mysql_num_fields(metadata);
	res->index = 0;
	if (res->fields > 0) {
		MYSQL_BIND *bind = bind_alloc(res->fields);
		if (mysql_stmt_bind_result(stmt, bind) != 0) {
			OTAMA_LOG_ERROR("%s: %s", query, mysql_stmt_error(stmt));
			bind_free(&bind, res->fields);
			mysql_stmt_close(stmt);
			nv_free(res);
			return NULL;
		}
		if (mysql_stmt_store_result(stmt) != 0) {
			OTAMA_LOG_ERROR("%s: %s", query, mysql_stmt_error(stmt));
			mysql_stmt_close(stmt);
			return NULL;
		}
		res->tuples = mysql_stmt_num_rows(stmt);
		res->row = bind;
	} else {
		res->row = NULL;
	}
	if (metadata) {
		mysql_free_result(metadata);
	}
	
	return res;
}

static int
otama_dbi_mysql_result_next(otama_dbi_result_t *res)
{
	if (res->cursor == NULL) {
		return 0;
	}
	return mysql_stmt_fetch((MYSQL_STMT *)res->cursor) != 0 ? 0 : 1;
}

static int
otama_dbi_mysql_result_seek(otama_dbi_result_t *res, int64_t j)
{
	int ret = -1;
	MYSQL_STMT *stmt = (MYSQL_STMT *)res->cursor;
	if (res->cursor) {
		if (j < res->tuples) {
			mysql_stmt_data_seek(stmt, j);
			if (mysql_stmt_fetch(stmt) != 0) {
				OTAMA_LOG_ERROR("%s", mysql_stmt_error(stmt));
			} else {
				ret = 0;
			}
		}
	}
	return ret;
}

static int
otama_dbi_mysql_exec_sql(otama_dbi_t *dbi, const char *sql)
{
	MYSQL_RES *res;
	int ret;
	
	ret = mysql_query((MYSQL *)dbi->conn, sql);
	if (ret != 0) {
		OTAMA_LOG_ERROR("%s", mysql_error((MYSQL *)dbi->conn));
		return -1;
	}
	res = mysql_store_result((MYSQL *)dbi->conn);
	if (res) {
		mysql_free_result(res);
	}
	
	return 0;
}

static int
otama_dbi_mysql_begin(otama_dbi_t *dbi)
{
	return otama_dbi_mysql_exec_sql(dbi, "START TRANSACTION");
}

static int
otama_dbi_mysql_commit(otama_dbi_t *dbi)
{
	return otama_dbi_mysql_exec_sql(dbi, "COMMIT");
}

static int
otama_dbi_mysql_rollback(otama_dbi_t *dbi)
{
	return otama_dbi_mysql_exec_sql(dbi, "ROLLBACK");
}

static const char *
otama_dbi_mysql_result_string(otama_dbi_result_t *res, int i)
{
	if (i < res->fields && res->row) {
		MYSQL_BIND *bind = &((MYSQL_BIND*)res->row)[i];
		if (*bind->is_null) {
			return "";
		} else {
			return (const char *)bind->buffer;
		}
	}
	return NULL;
}

static void
otama_dbi_mysql_result_free(otama_dbi_result_t **res)
{
	if (res && *res) {
		if ((*res)->row) {
			bind_free((MYSQL_BIND **)&(*res)->row, (*res)->fields);
		}
		if (!(*res)->prepared && (*res)->cursor) {
			mysql_stmt_close((MYSQL_STMT *)(*res)->cursor);
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

otama_dbi_stmt_t *
otama_dbi_mysql_stmt_new(otama_dbi_t *dbi,
						 const char *query)
{
	otama_dbi_stmt_t *stmt = nv_alloc_type(otama_dbi_stmt_t, 1);
	MYSQL_STMT *stmt_native = NULL;
	int ret;
	
	memset(stmt, 0, sizeof(*stmt));
	stmt_native = mysql_stmt_init((MYSQL *)dbi->conn);
	ret = mysql_stmt_prepare(stmt_native, query, strlen(query));
	if (ret != 0) {
		OTAMA_LOG_ERROR("%s: %s", query, mysql_stmt_error(stmt_native));
		nv_free(stmt);
		mysql_stmt_close(stmt_native);
		return NULL;
	}
	stmt->dbi = dbi;
	stmt->params = mysql_stmt_param_count(stmt_native);
	stmt->stmt = stmt_native;
	
	return stmt;
}

otama_dbi_result_t *
otama_dbi_mysql_stmt_query(otama_dbi_stmt_t *stmt)
{
	otama_dbi_result_t *res;
	MYSQL_STMT *stmt_native = (MYSQL_STMT *)stmt->stmt;
	MYSQL_BIND *bind_param = nv_alloc_type(MYSQL_BIND, stmt->params);
	unsigned long *lengths = nv_alloc_type(unsigned long, stmt->params);
	MYSQL_RES *metadata;
	int i;
	int err = 0;
	
	memset(bind_param, 0, sizeof(MYSQL_BIND) * stmt->params);
	memset(lengths, 0, sizeof(unsigned long) * stmt->params);
	
	for (i = 0; i < stmt->params; ++i) {
		switch (stmt->param_types[i]) {
		case OTAMA_DBI_COLUMN_INT:
			bind_param[i].buffer_type= MYSQL_TYPE_LONG;
			bind_param[i].buffer = &stmt->param_values[i].i;
			break;
		case OTAMA_DBI_COLUMN_INT64:
			bind_param[i].buffer_type = MYSQL_TYPE_LONGLONG;
			bind_param[i].buffer = &stmt->param_values[i].i64;
			break;
		case OTAMA_DBI_COLUMN_FLOAT:
			bind_param[i].buffer_type = MYSQL_TYPE_FLOAT;
			bind_param[i].buffer = &stmt->param_values[i].f;
			break;
		case OTAMA_DBI_COLUMN_STRING:
			lengths[i] = strlen(stmt->param_values[i].s);
			bind_param[i].buffer_type = MYSQL_TYPE_STRING;
			bind_param[i].buffer = (char *)stmt->param_values[i].s;
			bind_param[i].buffer_length = lengths[i] + 1;
			bind_param[i].length = &lengths[i];
			break;
		default:
			NV_ASSERT(0);
			err = 1;
			break;
		}
	}
	if (err) {
		nv_free(lengths);
		nv_free(bind_param);
		mysql_stmt_close(stmt_native);
		return NULL;
	}
	mysql_stmt_reset(stmt_native);
	if (mysql_stmt_bind_param(stmt_native, bind_param) != 0) {
		OTAMA_LOG_ERROR("%s", mysql_stmt_error(stmt_native));
		nv_free(lengths);
		nv_free(bind_param);
		mysql_stmt_reset(stmt_native);
		return NULL;
	}
	if (mysql_stmt_execute(stmt_native) != 0) {
		OTAMA_LOG_ERROR("%s", mysql_stmt_error(stmt_native));
		nv_free(lengths);
		nv_free(bind_param);
		mysql_stmt_reset(stmt_native);
		return NULL;
	}
	metadata = mysql_stmt_result_metadata(stmt_native);
	res = nv_alloc_type(otama_dbi_result_t, 1);
	memset(res, 0, sizeof(*res));
	res->cursor = stmt_native;
	res->dbi = stmt->dbi;
	res->fields = metadata == NULL ? 0 : mysql_num_fields(metadata);
	res->index = 0;
	res->prepared = 1;
	if (res->fields > 0) {
		MYSQL_BIND *bind = bind_alloc(res->fields);
		if (mysql_stmt_bind_result(stmt_native, bind) != 0
			|| mysql_stmt_store_result(stmt_native) != 0)
		{
			OTAMA_LOG_ERROR("%s", mysql_stmt_error(stmt_native));
			bind_free(&bind, res->fields);
			nv_free(res);
			nv_free(lengths);
			nv_free(bind_param);
			if (metadata) {
				mysql_free_result(metadata);
			}
			mysql_stmt_reset(stmt_native);
			return NULL;
		}
		res->tuples = mysql_stmt_num_rows(stmt_native);
		res->row = bind;
	} else {
		res->row = NULL;
	}
	if (metadata) {
		mysql_free_result(metadata);
	}
	nv_free(lengths);
	nv_free(bind_param);
	
	return res;
}

void
otama_dbi_mysql_stmt_reset(otama_dbi_stmt_t *stmt)
{
	mysql_stmt_reset((MYSQL_STMT *)stmt->stmt);
}

void
otama_dbi_mysql_stmt_free(otama_dbi_stmt_t **stmt)
{
	if (stmt && *stmt) {
		mysql_stmt_close((MYSQL_STMT *)(*stmt)->stmt);
		nv_free(*stmt);
		*stmt = NULL;
	}
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
	dbi->func.stmt_new = otama_dbi_mysql_stmt_new;
	dbi->func.stmt_query = otama_dbi_mysql_stmt_query;
	dbi->func.stmt_reset = otama_dbi_mysql_stmt_reset;
	dbi->func.stmt_free = otama_dbi_mysql_stmt_free;
	
	return 0;
}

#endif
