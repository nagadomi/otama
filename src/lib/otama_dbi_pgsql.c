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
#if OTAMA_WITH_PGSQL
#include "postgresql/libpq-fe.h"
#include "nv_core.h"
#include "otama_log.h"
#include "otama_dbi.h"
#include "otama_dbi_internal.h"

static void
otama_dbi_pgsql_close(otama_dbi_t **dbi)
{
	if (dbi && *dbi) {
		if ((*dbi)->conn) {
			PQfinish((PGconn *)(*dbi)->conn);
		}
		nv_free(*dbi);
		*dbi = NULL;
	}
}

static int
otama_dbi_pgsql_table_exist(otama_dbi_t *dbi,
							int *exist,
							const char *table_name)
{
	char esc[1024];
	int ret;
	otama_dbi_result_t *res;

	res = otama_dbi_queryf(dbi,
						   "select tablename from pg_tables "
						   "where tablename = %s;",
						   otama_dbi_escape(dbi, esc, sizeof(esc), table_name));
	if (res) {
		ret = 0;
		*exist = otama_dbi_result_next(res) > 0 ? 1: 0;
		otama_dbi_result_free(&res);
	}else {
		ret = -1;
	}
	
	return ret;
}

static const char *
otama_dbi_pgsql_escape(otama_dbi_t *dbi,
					   char *esc, size_t len,
					   const char *s)
{
	if (len >= 3) {
		size_t slen = strlen(s);
		if (slen > 0) {
			char *to = nv_alloc_type(char, strlen(s) * 3 + 1);
			int e = 0;
			
			PQescapeStringConn((PGconn *)dbi->conn, to, s, slen, &e);
			if (e == 0) {
				strcpy(esc, "'");
				strncat(esc, to, len-3);
				strcat(esc, "'");
			} else {
				return NULL;
			}
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
otama_dbi_pgsql_query(otama_dbi_t *dbi, const char *query)
{
	otama_dbi_result_t *res = nv_alloc_type(otama_dbi_result_t, 1);
	ExecStatusType  ret;
	
	res->cursor = PQexec((PGconn *)dbi->conn, query);
	ret = PQresultStatus((PGresult *)res->cursor);
	if (!(ret == PGRES_COMMAND_OK || ret == PGRES_TUPLES_OK)) {
		OTAMA_LOG_ERROR("%s: %s", query, PQresultErrorMessage((PGresult *)res->cursor));
		PQclear((PGresult *)res->cursor);
		nv_free(res);
		return NULL;
	}
	res->dbi = dbi;
	res->tuples = PQntuples((PGresult *)res->cursor);
	res->fields = PQnfields((PGresult *)res->cursor);
	res->index = 0;
	
	return res;
}

static int
otama_dbi_pgsql_result_next(otama_dbi_result_t *res)
{
	if (res->index < res->tuples) {
		++res->index;
		return 1;
	}
	
	return 0;
}

static int
otama_dbi_pgsql_result_seek(otama_dbi_result_t *res, int64_t j)
{
	if (j < res->tuples) {
		res->index = j + 1;
		return 0;
	}
	return -1;
}

static int
otama_dbi_pgsql_begin(otama_dbi_t *dbi)
{
	return otama_dbi_exec(dbi, "BEGIN;");
}

static int
otama_dbi_pgsql_commit(otama_dbi_t *dbi)
{
	return otama_dbi_exec(dbi, "COMMIT;");
}

static int
otama_dbi_pgsql_rollback(otama_dbi_t *dbi)
{
	return otama_dbi_exec(dbi, "ROLLBACK;");
}

static const char *
otama_dbi_pgsql_result_string(otama_dbi_result_t *res, int i)
{
	return PQgetvalue((PGresult*)res->cursor, (int)res->index - 1, i);
}

static void
otama_dbi_pgsql_result_free(otama_dbi_result_t **res)
{
	if (res && *res) {
		PQclear((*res)->cursor);
		nv_free((*res));
		*res = NULL;
	}
}

static int
otama_dbi_pgsql_create_sequence(otama_dbi_t *dbi, const char *sequence_name)
{
	char sql[8192];

	nv_snprintf(sql, sizeof(sql) - 1,
				"CREATE SEQUENCE %s_sequence_;", sequence_name);
	return otama_dbi_exec(dbi, sql);
}

static int
otama_dbi_pgsql_drop_sequence(otama_dbi_t *dbi, const char *sequence_name)
{
	char sql[8192];

	nv_snprintf(sql, sizeof(sql) - 1,
				"DROP SEQUENCE %s_sequence_;", sequence_name);
	return otama_dbi_exec(dbi, sql);
}

static int
otama_dbi_pgsql_sequence_exist(otama_dbi_t *dbi,
							   int *exist,
							   const char *sequence_name)
{
	char name[8192];	
	char esc[8192];
	int ret;
	otama_dbi_result_t *res;

	nv_snprintf(name, sizeof(name) - 1,"%s_sequence_", sequence_name);
	res = otama_dbi_queryf(dbi,
						   "select relname from pg_class "
						   "where relkind = 'S' and relname=%s;",
						   otama_dbi_escape(dbi, esc, sizeof(esc), name));
	if (res) {
		ret = 0;
		*exist = otama_dbi_result_next(res) > 0 ? 1: 0;
		otama_dbi_result_free(&res);
	}else {
		ret = -1;
	}
	
	return ret;
}


static int
otama_dbi_pgsql_sequence_next(otama_dbi_t *dbi, int64_t *seq,
							  const char *sequence_name)
{
	char name[8192];	
	char esc[8192];	
	otama_dbi_result_t *res;
	
	nv_snprintf(name, sizeof(name) - 1,"%s_sequence_", sequence_name);
	res = otama_dbi_queryf(dbi, "SELECT nextval(%s);", otama_dbi_escape(dbi, esc, sizeof(esc), name));
	if (res == NULL) {
		return -1;
	}
	if (otama_dbi_result_next(res)) {
		*seq = otama_dbi_result_int64(res, 0);
		otama_dbi_result_free(&res);
	} else {
		otama_dbi_result_free(&res);
		return -1;
	}
	
	return 0;
}

int
otama_dbi_pgsql_open(otama_dbi_t *dbi)
{
	ConnStatusType ret;
	PGconn *conn;
	
	conn = PQsetdbLogin(
		strlen(dbi->config.host) > 0 ? dbi->config.host : NULL,
		strlen(dbi->config.port) > 0 ? dbi->config.port : NULL,
		NULL,
		NULL,
		strlen(dbi->config.dbname) > 0 ? dbi->config.dbname : NULL,
		strlen(dbi->config.username) > 0 ? dbi->config.username : NULL,
		strlen(dbi->config.password) > 0 ? dbi->config.password : NULL
		);
	ret = PQstatus(conn);
	if (ret != CONNECTION_OK) {
		OTAMA_LOG_ERROR("%s", PQerrorMessage(conn));
		PQfinish(conn);
		return -1;
	}
	dbi->conn = conn;
	dbi->func.close = otama_dbi_pgsql_close;
	dbi->func.query = otama_dbi_pgsql_query;
	dbi->func.escape = otama_dbi_pgsql_escape;
	dbi->func.table_exist = otama_dbi_pgsql_table_exist;
	dbi->func.result_next = otama_dbi_pgsql_result_next;
	dbi->func.result_seek = otama_dbi_pgsql_result_seek;
	dbi->func.result_string = otama_dbi_pgsql_result_string;
	dbi->func.result_free = otama_dbi_pgsql_result_free;
	dbi->func.begin = otama_dbi_pgsql_begin;
	dbi->func.commit = otama_dbi_pgsql_commit;
	dbi->func.rollback = otama_dbi_pgsql_rollback;
	dbi->func.create_sequence = otama_dbi_pgsql_create_sequence;
	dbi->func.drop_sequence = otama_dbi_pgsql_drop_sequence;
	dbi->func.sequence_next = otama_dbi_pgsql_sequence_next;
	dbi->func.sequence_exist = otama_dbi_pgsql_sequence_exist;

	return 0;
}

#endif

