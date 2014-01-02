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
#if OTAMA_WITH_SQLITE3
#include "sqlite3.h"
#include "nv_core.h"
#include "otama_log.h"
#include "otama_dbi.h"
#include "otama_dbi_internal.h"
#if OTAMA_WINDOWS
#  include <winnls.h>
#  include <windows.h>
#endif

#define DEFAULT_TIMEOUT 10000

static void
otama_dbi_sqlite3_close(otama_dbi_t **dbi)
{
	if (dbi && *dbi) {
		sqlite3_close((sqlite3 *)(*dbi)->conn);
		nv_free(*dbi);
		*dbi = NULL;
	}
}

static int
otama_dbi_sqlite3_table_exist(otama_dbi_t *dbi,
							  int *exist,
							  const char *table_name)
{
	char esc[1024];
	int ret;
	otama_dbi_result_t *res;

	res = otama_dbi_queryf(dbi,
						   "select name from sqlite_master "
						   "where type = 'table' and name = %s;",
						   otama_dbi_escape(dbi, esc, sizeof(esc), table_name));
	if (res) {
		ret = 0;
		*exist = otama_dbi_result_next(res) ? 1 : 0;
		otama_dbi_result_free(&res);
	} else {
		ret = -1;
	}
	
	return ret;
}

static const char *
otama_dbi_sqlite3_escape(otama_dbi_t *dbi,
						 char *esc, size_t len,
						 const char *s)
{
	if (len >= 3) {
		size_t slen = strlen(s);
		
		if (slen > 0) {
			char *to = sqlite3_mprintf("%Q", s);
			if (to) {
				strncpy(esc, to, len-1);
				sqlite3_free(to);
			} else {
				return NULL;
			}
		} else {
			strcpy(esc,"''");
		}
	} else {
		return NULL;
	}
	return esc;
}

static otama_dbi_result_t *
otama_dbi_sqlite3_query(otama_dbi_t *dbi, const char *query)
{
	otama_dbi_result_t *res;
	int  ret;
	sqlite3_stmt *stmt = NULL;
	
	ret = sqlite3_prepare_v2((sqlite3 *)dbi->conn, query, -1, &stmt, NULL);
	if (ret != SQLITE_OK) {
		OTAMA_LOG_ERROR("%s: %s", query, sqlite3_errmsg((sqlite3 *)dbi->conn));
		return NULL;
	}
	ret = sqlite3_step(stmt);
	if (ret != SQLITE_DONE && ret != SQLITE_ROW) {
		OTAMA_LOG_ERROR("%s: %s", query, sqlite3_errmsg((sqlite3 *)dbi->conn));
		return NULL;
	}
	
	res = nv_alloc_type(otama_dbi_result_t, 1);
	res->cursor = stmt;
	res->dbi = dbi;
	res->prepared = 0;
	if (ret == SQLITE_ROW) {
		res->tuples = 1;
	} else {
		res->tuples = 0;
	}
	res->fields = sqlite3_column_count((sqlite3_stmt *)res->cursor);
	res->index = 1;
	
	return res;
}

static int
otama_dbi_sqlite3_result_next(otama_dbi_result_t *res)
{
	if (res->index == 1) {
		if (res->tuples > 0) {
			++res->index;
			return 1;
		}
	} else {
		int ret = sqlite3_step((sqlite3_stmt *)res->cursor);
		if (ret == SQLITE_ROW) {
			++res->index;
			return 1;
		}
	}
	return 0;
}

static int
otama_dbi_sqlite3_result_seek(otama_dbi_result_t *res, int64_t j)
{
	if (res->index == j + 1) {
		return 0;
	} else if (res->index < j + 1) {
		while (otama_dbi_result_next(res)) {
			if (res->index == j + 1) {
				break;
			}
		}
	} else {
		int ret;
		sqlite3_reset((sqlite3_stmt *)res->cursor);
		ret = sqlite3_step((sqlite3_stmt *)res->cursor);
		if (ret == SQLITE_ROW) {
			res->tuples = 1;
		} else {
			res->tuples = 0;
		}
		res->index = 0;
		if (j == 0) {
			return 0;
		}
		while (otama_dbi_result_next(res)) {
			if (res->index == j + 1) {
				break;
			}
		}
	}
	return res->index == j + 1 ? 0 : -1;
}

static int
otama_dbi_sqlite3_begin(otama_dbi_t *dbi)
{
	return otama_dbi_exec(dbi, "BEGIN;");
}

static int
otama_dbi_sqlite3_commit(otama_dbi_t *dbi)
{
	return otama_dbi_exec(dbi, "COMMIT;");
}

static int
otama_dbi_sqlite3_rollback(otama_dbi_t *dbi)
{
	return otama_dbi_exec(dbi, "ROLLBACK;");
}

static const char *
otama_dbi_sqlite3_result_string(otama_dbi_result_t *res, int i)
{
	return (const char *)sqlite3_column_text((sqlite3_stmt *)res->cursor, i);
}

static void
otama_dbi_sqlite3_result_free(otama_dbi_result_t **res)
{
	if (res && *res) {
		if (!(*res)->prepared) {
			sqlite3_finalize((sqlite3_stmt *)(*res)->cursor);
		}
		nv_free(*res);
		*res = NULL;
	}
}

static int
otama_dbi_sqlite3_create_sequence(otama_dbi_t *dbi, const char *sequence_name)
{
	return otama_dbi_execf(dbi,
						   "CREATE TABLE %s_sequence_ (id integer primary key autoincrement);",
						   sequence_name);
}

static int
otama_dbi_sqlite3_drop_sequence(otama_dbi_t *dbi, const char *sequence_name)
{
	return otama_dbi_execf(dbi, "DROP TABLE %s_sequence_;", sequence_name);
}

static int
otama_dbi_sqlite3_sequence_next(otama_dbi_t *dbi, int64_t *seq,
								const char *sequence_name)
{
	char sql[8192];
	int  ret;
	sqlite3_stmt *stmt = NULL;
	
	nv_snprintf(sql, sizeof(sql) - 1,
				"INSERT INTO %s_sequence_ (id) values(NULL);",
				sequence_name);
	OTAMA_LOG_DEBUG("%s", sql);
	
	ret = sqlite3_prepare_v2((sqlite3 *)dbi->conn, sql, -1, &stmt, NULL);
	if (ret != SQLITE_OK) {
		OTAMA_LOG_ERROR("%s: %s", sql, sqlite3_errmsg((sqlite3 *)dbi->conn));
		return -1;
	}
	ret = sqlite3_step(stmt);
	if (ret == SQLITE_DONE) {
		*seq = (int64_t)sqlite3_last_insert_rowid((sqlite3 *)dbi->conn);
		sqlite3_finalize(stmt);
	} else {
		sqlite3_finalize(stmt);
		OTAMA_LOG_ERROR("%s: %s", sql, sqlite3_errmsg((sqlite3 *)dbi->conn));
		return -1;
	}

	return 0;
}

static int
otama_dbi_sqlite3_sequence_exist(otama_dbi_t *dbi,
							  int *exist,
							  const char *sequence_name)
{
	char table_name[8192];
	nv_snprintf(table_name, sizeof(table_name) - 1, "%s_sequence_", sequence_name);
	return otama_dbi_sqlite3_table_exist(dbi, exist, table_name);
}

otama_dbi_stmt_t *
otama_dbi_sqlite3_stmt_new(otama_dbi_t *dbi,
						   const char *sql)
{
	otama_dbi_stmt_t *stmt = nv_alloc_type(otama_dbi_stmt_t, 1);
	sqlite3_stmt *stmt_native = NULL;
	int ret;
	
	memset(stmt, 0, sizeof(*stmt));
	ret = sqlite3_prepare_v2((sqlite3 *)dbi->conn, sql, -1, &stmt_native, NULL);
	if (ret != SQLITE_OK) {
		OTAMA_LOG_ERROR("%s: %s", sql, sqlite3_errmsg((sqlite3 *)dbi->conn));
		nv_free(stmt);
		return NULL;
	}
	stmt->dbi = dbi;
	stmt->params = sqlite3_bind_parameter_count(stmt_native);
	stmt->stmt = stmt_native;

	return stmt;
}

otama_dbi_result_t *
otama_dbi_sqlite3_stmt_query(otama_dbi_stmt_t *stmt)
{
	otama_dbi_result_t *res;
	sqlite3_stmt *stmt_native = (sqlite3_stmt *)stmt->stmt;
	int  ret;
	int i;
	int err = 0;
	for (i = 0; i < stmt->params; ++i) {
		switch (stmt->param_types[i]) {
		case OTAMA_DBI_COLUMN_INT:
			ret = sqlite3_bind_int(stmt_native, i + 1,
								   stmt->param_values[i].i);
			if (ret != SQLITE_OK) {
				err = 0;
			}
			break;
		case OTAMA_DBI_COLUMN_INT64:
			ret = sqlite3_bind_int64(stmt_native, i + 1,
									 stmt->param_values[i].i64);
			if (ret != SQLITE_OK) {
				err = 0;
			}
			break;
		case OTAMA_DBI_COLUMN_FLOAT:
			ret = sqlite3_bind_double(stmt_native, i + 1,
									  (double)stmt->param_values[i].f);
			if (ret != SQLITE_OK) {
				err = 0;
			}
			break;
		case OTAMA_DBI_COLUMN_STRING:
			ret = sqlite3_bind_text(stmt_native, i + 1,
									stmt->param_values[i].s, -1, SQLITE_STATIC);
			if (ret != SQLITE_OK) {
				err = 0;
			}
			break;
		default:
			NV_ASSERT(0);
			err = 1;
			break;
		}
	}
	if (err) {
		sqlite3_reset(stmt_native);
		return NULL;
	}
	ret = sqlite3_step(stmt_native);
	if (ret != SQLITE_DONE && ret != SQLITE_ROW) {
		OTAMA_LOG_ERROR("%s", sqlite3_errmsg((sqlite3 *)stmt->dbi->conn));
		sqlite3_reset(stmt_native);
		return NULL;
	}
	res = nv_alloc_type(otama_dbi_result_t, 1);
	res->cursor = stmt_native;
	res->dbi = stmt->dbi;
	res->prepared = 1;
	if (ret == SQLITE_ROW) {
		res->tuples = 1;
	} else {
		res->tuples = 0;
	}
	res->fields = sqlite3_column_count((sqlite3_stmt *)res->cursor);
	res->index = 1;
	
	return res;
}

void
otama_dbi_sqlite3_stmt_reset(otama_dbi_stmt_t *stmt)
{
	sqlite3_reset((sqlite3_stmt *)stmt->stmt);
}

void
otama_dbi_sqlite3_stmt_free(otama_dbi_stmt_t **stmt)
{
	if (stmt && *stmt) {
		sqlite3_finalize((*stmt)->stmt);
		nv_free(*stmt);
		*stmt = NULL;
	}
}

int
otama_dbi_sqlite3_open(otama_dbi_t *dbi)
{
	int ret;
	sqlite3 *conn;
	char sqlite3_name[8192 * 3] = {0};
	
	strncpy(sqlite3_name, dbi->config.dbname, sizeof(sqlite3_name));
	
#if OTAMA_WINDOWS
	{
		// to utf8
		char sqlite3_name_tmp[8192 * 3];
		strcpy(sqlite3_name_tmp, sqlite3_name);
		MultiByteToWideChar(CP_ACP, 0,
							  (LPCSTR)sqlite3_name_tmp, -1,
							  (LPWSTR)sqlite3_name, sizeof(sqlite3_name));
		WideCharToMultiByte(CP_UTF8, 0,
							  (LPCWSTR)sqlite3_name, -1,
							  (LPSTR)sqlite3_name_tmp, sizeof(sqlite3_name_tmp),
							  NULL, NULL);
		strcpy(sqlite3_name, sqlite3_name_tmp);
	}
#endif
	ret = sqlite3_open(sqlite3_name, &conn);
	if (ret != SQLITE_OK) {
		OTAMA_LOG_ERROR("sqlite3_open: %d\n", ret);
		return -1;
	}
	dbi->conn = conn;
	dbi->func.close = otama_dbi_sqlite3_close;
	dbi->func.query = otama_dbi_sqlite3_query;
	dbi->func.escape = otama_dbi_sqlite3_escape;
	dbi->func.table_exist = otama_dbi_sqlite3_table_exist;
	dbi->func.result_next = otama_dbi_sqlite3_result_next;
	dbi->func.result_seek = otama_dbi_sqlite3_result_seek;
	dbi->func.result_string = otama_dbi_sqlite3_result_string;
	dbi->func.result_free = otama_dbi_sqlite3_result_free;
	dbi->func.begin = otama_dbi_sqlite3_begin;
	dbi->func.commit = otama_dbi_sqlite3_commit;
	dbi->func.rollback = otama_dbi_sqlite3_rollback;
	dbi->func.create_sequence = otama_dbi_sqlite3_create_sequence;
	dbi->func.drop_sequence = otama_dbi_sqlite3_drop_sequence;
	dbi->func.sequence_next = otama_dbi_sqlite3_sequence_next;
	dbi->func.sequence_exist = otama_dbi_sqlite3_sequence_exist;
	dbi->func.stmt_new = otama_dbi_sqlite3_stmt_new;
	dbi->func.stmt_query = otama_dbi_sqlite3_stmt_query;
	dbi->func.stmt_reset = otama_dbi_sqlite3_stmt_reset;
	dbi->func.stmt_free = otama_dbi_sqlite3_stmt_free;
	
	// default memory config
	sqlite3_busy_timeout(conn, DEFAULT_TIMEOUT);
	otama_dbi_exec(dbi, "PRAGMA journal_mode=MEMORY;");
	otama_dbi_exec(dbi, "PRAGMA temp_store=MEMORY;");
	otama_dbi_exec(dbi, "PRAGMA cache_size=32000;");
	//otama_dbi_exec(dbi, "PRAGMA synchronous = off;");
	
	return 0;
}

#endif
