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

#include "nv_core.h"
#include "otama_config.h"
#include "otama_log.h"
#include "otama_dbi.h"
#include "otama_dbi_internal.h"
#include <string>

#define OTAMA_DBI_SQL_LEN (32 * 1024)

void
otama_dbi_config_driver(otama_dbi_config_t *config, const char *driver)
{
	strncpy(config->driver, driver, sizeof(config->driver) -1);
}

void
otama_dbi_config_host(otama_dbi_config_t *config, const char *host)
{
	strncpy(config->host, host, sizeof(config->host) -1);
}

void
otama_dbi_config_port(otama_dbi_config_t *config, int port)
{
	sprintf(config->port, "%d", port);
}

void
otama_dbi_config_username(otama_dbi_config_t *config, const char *username)
{
	strncpy(config->username, username, sizeof(config->username) -1);
}

void
otama_dbi_config_password(otama_dbi_config_t *config, const char *password)
{
	strncpy(config->password, password, sizeof(config->password) -1);
}

void
otama_dbi_config_dbname(otama_dbi_config_t *config, const char *dbname)
{
	strncpy(config->dbname, dbname, sizeof(config->dbname) -1);
}

int
otama_dbi_table_exist(otama_dbi_t *dbi, int *exist, const char *table_name)
{
	return dbi->func.table_exist(dbi, exist, table_name);
}

int
otama_dbi_execf(otama_dbi_t *dbi, const char *fmt, ...)
{
	char *buff = nv_alloc_type(char, OTAMA_DBI_SQL_LEN);
	va_list args;
	int s;
	int ret;
	
	va_start(args, fmt);
	s = nv_vsnprintf(buff, OTAMA_DBI_SQL_LEN - 1, fmt, args);
	va_end(args);
	
	if (s > 0) {
		nv_free(buff);
		buff = nv_alloc_type(char, (s + 2));
		va_start(args, fmt);
		s = nv_vsnprintf(buff, s + 1, fmt, args);
		va_end(args);
	}
	ret = otama_dbi_exec(dbi, buff);
	nv_free(buff);

	return ret;
}

int
otama_dbi_exec(otama_dbi_t *dbi, const char *sql)
{
	otama_dbi_result_t *res = otama_dbi_query(dbi, sql);
	if (res) {
		otama_dbi_result_free(&res);
		return 0;
	}
	return -1;
}

otama_dbi_result_t *
otama_dbi_query(otama_dbi_t *dbi, const char *query)
{
	OTAMA_LOG_DEBUG("%s", query);
	return dbi->func.query(dbi, query);
}

otama_dbi_result_t *
otama_dbi_queryf(otama_dbi_t *dbi, const char *fmt, ...)
{
	char *buff = nv_alloc_type(char, OTAMA_DBI_SQL_LEN);
	va_list args;
	int s;
	otama_dbi_result_t *res;
	
	va_start(args, fmt);
	s = nv_vsnprintf(buff, OTAMA_DBI_SQL_LEN - 1, fmt, args);
	va_end(args);
	
	if (s > 0) {
		nv_free(buff);
		buff = nv_alloc_type(char, (s + 2));
		
		va_start(args, fmt);
		s = nv_vsnprintf(buff, s + 1, fmt, args);
		va_end(args);
	}
	
	res = otama_dbi_query(dbi, buff);
	nv_free(buff);

	return res;
}

int
otama_dbi_result_next(otama_dbi_result_t *res)
{
	return res->dbi->func.result_next(res);
}

int
otama_dbi_result_seek(otama_dbi_result_t *res, int64_t j)
{
	return res->dbi->func.result_seek(res, j);
}

static float
otama_dbi_to_float(const char *s)
{
	return (float)strtod(s, NULL);
}

static int64_t
otama_dbi_to_int64(const char *s)
{
	return nv_strtoll(s, NULL, 10);
}

static int
otama_dbi_to_int(const char *s)
{
	return (int)nv_strtoll(s, NULL, 10);
}

int
otama_dbi_result_is_null(otama_dbi_result_t *res, int i)
{
	const char *s = res->dbi->func.result_string(res, i);
	if (s) {
		// TODO: "NULL"
		if (strlen(s) == 0) {
			return 1;
		}
		return 0;
	}
	return 1;
}

int64_t
otama_dbi_result_int64(otama_dbi_result_t *res, int i)
{
	const char *s = res->dbi->func.result_string(res, i);
	if (s) {
		return otama_dbi_to_int64(s);
	}
	return 0;
}

int32_t
otama_dbi_result_int(otama_dbi_result_t *res, int i)
{
	const char *s = res->dbi->func.result_string(res, i);
	if (s) {
		return otama_dbi_to_int(s);
	}
	return 0;
}

time_t
otama_dbi_result_time(otama_dbi_result_t *res, int i)
{
	const char *s = otama_dbi_result_string(res, i);
	int n;
	struct tm tm;
	
    // 2012-05-06 04:16:05
	memset(&tm, 0, sizeof(tm));
	tm.tm_yday = -1;
	tm.tm_wday = -1;
	tm.tm_isdst = -1;
	
	n = sscanf(s,
			   "%04d-%02d-%02d %02d:%02d:%02d",
			   &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
			   &tm.tm_hour, &tm.tm_min, &tm.tm_sec
		);
	if (n == 6) {
		tm.tm_year -= 1900;
		tm.tm_mon -= 1;
		return nv_mkgmtime(&tm);
	}
    // 2012-05-06
	memset(&tm, 0, sizeof(tm));
	n = sscanf(s, "%04d-%02d-%02d",
			   &tm.tm_year, &tm.tm_mon, &tm.tm_mday);
	if (n == 3) {
		tm.tm_year -= 1900;
		tm.tm_mon -= 1;
		return nv_mkgmtime(&tm);
	}
	
	return (time_t)0;
}

int
otama_dbi_to_bool(const char *s)
{
	int ret = 0;
	
	if (nv_strcasecmp(s, "true") == 0) {
		ret = 1;
	} else if (nv_strcasecmp(s, "t") == 0) {
		ret = 1;
	} else if (nv_strcasecmp(s, "false") == 0) {
		ret = 0;
	} else if (nv_strcasecmp(s, "f") == 0) {
		ret = 0;
	} else {
		ret = (int)strtol(s, NULL, 10);
	}
	return ret ? 1:0;
}

float
otama_dbi_result_float(otama_dbi_result_t *res, int i)
{
	const char *s = res->dbi->func.result_string(res, i);
	if (s) {
		return otama_dbi_to_float(s);
	}
	return 0.0f;
}
int
otama_dbi_result_bool(otama_dbi_result_t *res, int i)
{
	int ret = 0;
	const char *s = res->dbi->func.result_string(res, i);
	
	if (s) {
		return otama_dbi_to_bool(s);
	}
	
	return ret;
}

const char *
otama_dbi_result_string(otama_dbi_result_t *res, int i)
{
	return res->dbi->func.result_string(res, i);
}

void
otama_dbi_result_free(otama_dbi_result_t **res)
{
	(*res)->dbi->func.result_free(res);
}

void
otama_dbi_close(otama_dbi_t **dbi)
{
	if (dbi && *dbi) {
		if ((*dbi)->conn) {
			(*dbi)->func.close(dbi);
		} else {
			nv_free(*dbi);
			*dbi = NULL;
		}
	}
}

const char *
otama_dbi_escape(otama_dbi_t *dbi, char *esc, size_t len, const char *s)
{
	return dbi->func.escape(dbi, esc, len, s);
}

/* table declare */

static char OTAMA_DBI_TRUE = 't';
static char OTAMA_DBI_FALSE = 'f';
static const struct {
	const char *name;
	int index;
} s_driver_index[] = {
	{ "pgsql", 0 },
	{ "sqlite3", 1 },
	{ "mysql", 2 },
	{ NULL, -1 },
};

typedef struct {
	const char *type[3];
} driver_column_type_t;

static const driver_column_type_t
s_pgsql_column[] =
{
	{{ "varchar(255)", NULL, NULL }},
	{{ "char", "char(%d)", NULL }},
	{{ "text", NULL, NULL }},
	{{ "integer", NULL, NULL }},
	{{ "bigint", NULL, NULL }},
	{{ "bigserial primary key", NULL, NULL }},
	{{ "real", NULL, NULL }},
	{{ "timestamp", NULL, NULL }},
	{{ "bytea", "bytea(%d)", NULL }},
	{{ "boolean", NULL, NULL }},
	{{ "integer[]", NULL, NULL }},	
	{{ NULL, NULL, NULL }}
};

static const driver_column_type_t
s_sqlite3_column[] =
{
	{{ "varchar(255)", "varchar(%d)", NULL }},
	{{ "char", "char(%d)", NULL }},
	{{ "text", NULL, NULL }},
	{{ "integer", NULL, NULL }},
	{{ "integer", NULL, NULL }},
	{{ "integer primary key autoincrement", NULL, NULL }},
	{{ "real", NULL, NULL }},
	{{ "datetime", NULL, NULL }},
	{{ "blob", "blob(%d)", NULL }},
	{{ "boolean", NULL, NULL }},
	{{ NULL, NULL, NULL }},
	{{ NULL, NULL, NULL }}
};

static const driver_column_type_t
s_mysql_column[] =
{
	{{ "varchar(255)", "varchar(%d)", NULL }},
	{{ "text", "text(%d)", NULL }},
	{{ "text", NULL, NULL }},
	{{ "integer", NULL, NULL }},
	{{ "bigint", NULL, NULL }},
	{{ "bigint primary key auto_increment", NULL, NULL }},
	{{ "real", NULL, NULL }},
	{{ "datetime", NULL, NULL }},
	{{ "longblob", "longblob(%d)", NULL }},
	{{ "boolean", NULL, NULL }},
	{{ NULL, NULL, NULL }},
	{{ NULL, NULL, NULL }}
};

static const
driver_column_type_t *s_driver_column_def[] = {
	s_pgsql_column,
	s_sqlite3_column,
	s_mysql_column
};

static int
otama_dbi_driver_index(const char *driver_name)
{
	int i;
	for (i = 0; s_driver_index[i].name != NULL; ++i) {
		if (nv_strcasecmp(driver_name, s_driver_index[i].name) == 0) {
			return s_driver_index[i].index;
		}
	}
	return -1;
}

int
otama_dbi_create_table(otama_dbi_t *dbi,
					   const char *table_name,
					   const otama_dbi_column_t *columns,
					   int n)
{
	const char *driver_name = dbi->config.driver;
	int driver_index = otama_dbi_driver_index(driver_name);
	int i;
	std::string sql = std::string("create table ") + table_name + " (";
	char esc[1024];
	int ret;
	
	if (driver_index == -1) {
		OTAMA_LOG_ERROR("`%s' are not supported.", driver_name);
		//return OTAMA_STATUS_INVALID_ARGUMENTS;
		return -1;
	}
	
	for (i = 0; i < n; ++i) {
		const otama_dbi_column_t *it = &columns[i];
		const driver_column_type_t *column =
			&s_driver_column_def[driver_index][it->type];
		char buff[2048];

		if (column->type[0] == NULL) {
			OTAMA_LOG_ERROR("%s does not supported specified column type.", driver_name);
			return -1;
			//return OTAMA_STATUS_INVALID_ARGUMENTS;
		}
		
		if (i != 0) {
			sql += ",\n";
		}
		sql += it->name;
		sql += " ";
		if (it->length1 > 0 && it->length2 > 0 && column->type[2] != NULL) {
			sprintf(buff, column->type[2], it->length1, it->length2);
			sql += buff;
		} else if (it->length1 > 0 && column->type[1] != NULL) {
			sprintf(buff, column->type[1], it->length1);
			sql += buff;
		} else {
			sql += column->type[0];
		}
		sql += " ";
		if (it->not_null) {
			sql += "not null ";
		}
		if (it->default_value) {
			switch (it->type) {
			case OTAMA_DBI_COLUMN_STRING:
			case OTAMA_DBI_COLUMN_TEXT:
				sprintf(buff, "default %s",
						otama_dbi_escape(dbi, esc, sizeof(esc) - 1, it->default_value));
				sql += buff;
				break;
			case OTAMA_DBI_COLUMN_INT:
				nv_snprintf(buff, sizeof(buff) - 1, "default %d",
							otama_dbi_to_int(it->default_value));
				sql += buff;
				break;
			case OTAMA_DBI_COLUMN_INT64:
			case OTAMA_DBI_COLUMN_TIMESTAMP:
				nv_snprintf(buff, sizeof(buff) - 1, "default %" PRId64,
							otama_dbi_to_int64(it->default_value));
				sql += buff;
				break;
			case OTAMA_DBI_COLUMN_FLOAT:
				nv_snprintf(buff, sizeof(buff) - 1, "default %E",
							otama_dbi_to_float(it->default_value));
				sql += buff;
				break;
			case OTAMA_DBI_COLUMN_BOOLEAN:
				nv_snprintf(buff, sizeof(buff) - 1, "default '%c'",
							otama_dbi_to_bool(it->default_value) ?
							OTAMA_DBI_TRUE : OTAMA_DBI_FALSE);
				sql += buff;
				break;
			default:
				OTAMA_LOG_ERROR("%s", "%s default value not supported", it->name);
				break;
			}
		}
	}
	sql += ")";
	if (nv_strcasecmp(driver_name, "mysql") == 0) {
		sql += " ENGINE=INNODB";
	}
	sql += ";";
	
	ret = otama_dbi_exec(dbi, sql.c_str());
	
	return ret;
}

int
otama_dbi_create_index(otama_dbi_t *dbi,
					   const char *table_name,
					   otama_dbi_unique_e unique,
					   const otama_dbi_column_t *columns,
					   int n)
{
	std::string sql;
	std::string index_name = std::string(table_name) + "_";
	int i = 0;
	const char *driver_name = dbi->config.driver;
	
	for (i = 0; i < n; ++i) {
		if (i != 0) {
			index_name += "_";
		}
		index_name += columns[i].name;
	}
	if (unique == OTAMA_DBI_UNIQUE_TRUE) {
		sql = "create unique index ";
	} else {
		sql = "create index ";
	}
	sql += index_name + " on ";
	sql += table_name;
	sql += " (";
	for (i = 0; i < n; ++i) {
		if (i != 0) {
			sql += ", ";
		}
		sql += columns[i].name;
		if (nv_strcasecmp(driver_name, "mysql") == 0) {
			if (columns[i].type == OTAMA_DBI_COLUMN_CHAR
				|| columns[i].type == OTAMA_DBI_COLUMN_BINARY)
			{
				char buff[128];
				nv_snprintf(buff, sizeof(buff) - 1, "(%d)", columns[i].length1);
				sql += buff;
			}
		}
	}
	sql += ");";
	
	return otama_dbi_exec(dbi, sql.c_str());
}

const char *
otama_dbi_time_string(char *s, const time_t *t)
{
	struct tm tm;
	
	nv_gmtime_r(t, &tm);
	sprintf(s, "%04d-%02d-%02d %02d:%02d:%02d",
			1900 + tm.tm_year, tm.tm_mon + 1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec);
	
	return s;
}

int
otama_dbi_drop_table(otama_dbi_t *dbi,
					 const char *table_name)
{
	return otama_dbi_execf(dbi, "DROP TABLE %s;", table_name);
}

int
otama_dbi_begin(otama_dbi_t *dbi)
{
	return dbi->func.begin(dbi);
}

int
otama_dbi_commit(otama_dbi_t *dbi)
{
	return dbi->func.commit(dbi);
}

int
otama_dbi_rollback(otama_dbi_t *dbi)
{
	return dbi->func.rollback(dbi);
}

otama_dbi_t *
otama_dbi_new(const otama_dbi_config_t *config)
{
	otama_dbi_t *dbi = nv_alloc_type(otama_dbi_t, 1);
	
	memset(dbi, 0, sizeof(*dbi));
	
	memcpy(&dbi->config, config, sizeof(dbi->config));
	return dbi;
}

int
otama_dbi_open(otama_dbi_t *dbi)
{
#if OTAMA_WITH_PGSQL
	if (nv_strcasecmp(dbi->config.driver, "pgsql") == 0 ||
		nv_strcasecmp(dbi->config.driver, "postgres") == 0 ||
		nv_strcasecmp(dbi->config.driver, "postgresql") == 0)
	{
		return otama_dbi_pgsql_open(dbi);
	}
#endif
#if OTAMA_WITH_MYSQL
	if (nv_strcasecmp(dbi->config.driver, "mysql") == 0) {
		return otama_dbi_mysql_open(dbi);
	}
#endif
#if OTAMA_WITH_SQLITE3
	if (nv_strcasecmp(dbi->config.driver, "sqlite3") == 0) {
		return otama_dbi_sqlite3_open(dbi);
	}
#endif
	OTAMA_LOG_ERROR("%s driver not found", dbi->config.driver);
	
	return -1;
}

int
otama_dbi_active(otama_dbi_t *dbi)
{
	return dbi->conn == NULL ? 0:1;
}

int
otama_dbi_create_sequence(otama_dbi_t *dbi,
						  const char *sequence_name)
{
	return dbi->func.create_sequence(dbi, sequence_name);
}

int
otama_dbi_drop_sequence(otama_dbi_t *dbi,
						const char *sequence_name)
{
	return dbi->func.drop_sequence(dbi, sequence_name);
}

int
otama_dbi_sequence_next(otama_dbi_t *dbi,
						int64_t *seq,
						const char *sequence_name)
{
	return dbi->func.sequence_next(dbi, seq, sequence_name);
}

int
otama_dbi_sequence_exist(otama_dbi_t *dbi,
						 int *exist,
						 const char *sequence_name)
{
	return dbi->func.sequence_exist(dbi, exist, sequence_name);
}

