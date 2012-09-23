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

#undef NDEBUG
#include "otama_test.h"
#include "otama.h"
#include "otama_dbi.h"

#define OTAMA_TEST_TABLE "test99"

static void
otama_test_dbi_sql(const otama_dbi_config_t *config)
{
	otama_dbi_t *dbi;
	otama_dbi_result_t *res;
	int i;
	char s[256], esc[256];
	int exist;
	int64_t seq1, seq2;
	
	static const otama_dbi_column_t table_def[] = {
		{ "id", OTAMA_DBI_COLUMN_INT64, 0, 0, 0, NULL },
		{ "value", OTAMA_DBI_COLUMN_STRING, 0, 0, 0, NULL }
	};
	static const otama_dbi_column_t index_def[] = {
		{ "id", OTAMA_DBI_COLUMN_INT64, 0, 0, 0, NULL }
	};
	dbi = otama_dbi_new(config);
	NV_ASSERT(dbi != NULL);
	NV_ASSERT(otama_dbi_open(dbi) == 0);
	
	NV_ASSERT(otama_dbi_table_exist(dbi, &exist, OTAMA_TEST_TABLE) == 0);
	if (exist) {
		otama_dbi_drop_table(dbi, OTAMA_TEST_TABLE);
	}
	NV_ASSERT(otama_dbi_create_table(dbi, OTAMA_TEST_TABLE, table_def,
									 sizeof(table_def) / sizeof(otama_dbi_column_t)) == 0);
	NV_ASSERT(otama_dbi_create_index(dbi, OTAMA_TEST_TABLE,
									 OTAMA_DBI_UNIQUE_TRUE,
									 index_def,
									 sizeof(index_def) / sizeof(otama_dbi_column_t)) == 0);

	NV_ASSERT(otama_dbi_table_exist(dbi, &exist, OTAMA_TEST_TABLE) == 0);
	NV_ASSERT(exist != 0);
	NV_ASSERT(otama_dbi_begin(dbi) == 0);
	for (i = 0; i < 100; ++i) {
		sprintf(s, "%d's value", i);
		NV_ASSERT(otama_dbi_execf(dbi, "INSERT INTO "OTAMA_TEST_TABLE" (id, value) values(%d, %s);",
								 i, otama_dbi_escape(dbi, esc, sizeof(esc), s)) == 0);
	}
	NV_ASSERT(otama_dbi_rollback(dbi) == 0);
	res = otama_dbi_query(dbi, "SELECT count(*) FROM "OTAMA_TEST_TABLE";");
	NV_ASSERT(res != NULL);
	NV_ASSERT(otama_dbi_result_next(res) == 1);
	NV_ASSERT(otama_dbi_result_int(res, 0) == 0);
	otama_dbi_result_free(&res);
	NV_ASSERT(otama_dbi_begin(dbi) == 0);
	for (i = 0; i < 100; ++i) {
		sprintf(s, "%d's value", i);
		NV_ASSERT(otama_dbi_execf(dbi, "INSERT INTO "OTAMA_TEST_TABLE" (id, value) values(%d, %s);",
								 i, otama_dbi_escape(dbi, esc, sizeof(esc), s)) == 0);
	}
	NV_ASSERT(otama_dbi_commit(dbi) == 0);
	
	res = otama_dbi_query(dbi, "SELECT id,value FROM "OTAMA_TEST_TABLE" order by id;");
	NV_ASSERT(res != NULL);
	
	for (i = 0; i < 100; ++i) {
		int ret = otama_dbi_result_next(res);
		sprintf(s, "%d's value", i);
		NV_ASSERT(ret);
		NV_ASSERT(otama_dbi_result_int(res, 0) == i);
		NV_ASSERT(strcmp(otama_dbi_result_string(res, 1), s) == 0);
	}
	NV_ASSERT(otama_dbi_result_next(res) == 0);
	NV_ASSERT(otama_dbi_result_seek(res, 1) == 0);
	NV_ASSERT(otama_dbi_result_int(res, 0) == 1);
	NV_ASSERT(otama_dbi_result_seek(res, 20) == 0);
	NV_ASSERT(otama_dbi_result_int(res, 0) == 20);
	NV_ASSERT(otama_dbi_result_seek(res, 4) == 0);
	NV_ASSERT(otama_dbi_result_int(res, 0) == 4);
	NV_ASSERT(otama_dbi_result_seek(res, 73) == 0);
	NV_ASSERT(otama_dbi_result_seek(res, 73) == 0);	
	NV_ASSERT(otama_dbi_result_int(res, 0) == 73);
	NV_ASSERT(otama_dbi_result_seek(res, 0) == 0);
	NV_ASSERT(otama_dbi_result_int(res, 0) == 0);
	NV_ASSERT(otama_dbi_result_seek(res, 99) == 0);
	NV_ASSERT(otama_dbi_result_int(res, 0) == 99);
	NV_ASSERT(otama_dbi_result_seek(res, 100) != 0);
	
	otama_dbi_result_free(&res);
	
	NV_ASSERT(otama_dbi_drop_table(dbi, OTAMA_TEST_TABLE) == 0);
	NV_ASSERT(otama_dbi_table_exist(dbi, &exist, OTAMA_TEST_TABLE) == 0);
	NV_ASSERT(exist == 0);

	NV_ASSERT(otama_dbi_sequence_exist(dbi, &exist, "seq1") == 0);
	if (exist) {
		NV_ASSERT(otama_dbi_drop_sequence(dbi, "seq1") == 0);
	}
	NV_ASSERT(otama_dbi_create_sequence(dbi, "seq1") == 0);
	NV_ASSERT(otama_dbi_drop_sequence(dbi, "seq1") == 0);
	NV_ASSERT(otama_dbi_create_sequence(dbi, "seq1") == 0);
	
	NV_ASSERT(otama_dbi_sequence_next(dbi, &seq1, "seq1") == 0);
	for (i = 0; i < 10; ++i) {
		NV_ASSERT(otama_dbi_sequence_next(dbi, &seq2, "seq1") == 0);
		NV_ASSERT(seq1 < seq2);
		NV_ASSERT(otama_dbi_sequence_next(dbi, &seq1, "seq1") == 0);
	}
	NV_ASSERT(otama_dbi_drop_sequence(dbi, "seq1") == 0);
	
	otama_dbi_close(&dbi);
}

static void
otama_test_dbi_pgsql(void)
{
#if (OTAMA_WITH_PGSQL && defined(OTAMA_DBI_TEST))
	otama_dbi_config_t config;
	
	OTAMA_TEST_NAME;
	memset(&config, 0, sizeof(config));
	
	otama_dbi_config_driver(&config, "pgsql");
	otama_dbi_config_dbname(&config, "otama_test");
	otama_dbi_config_username(&config, "nagadomi");
	otama_dbi_config_password(&config, "1234");	
	
	otama_test_dbi_sql(&config);
#else
	printf("skip pgsql..\n");
#endif
}

static void
otama_test_dbi_sqlite3(void)
{
#if OTAMA_WITH_SQLITE3
	otama_dbi_config_t config;
	
	OTAMA_TEST_NAME;
	memset(&config, 0, sizeof(config));
	
	otama_dbi_config_driver(&config, "sqlite3");
	otama_dbi_config_dbname(&config, "./data/dbi-test.db");
	otama_test_dbi_sql(&config);
#else
	printf("skip sqlite3..\n");
#endif
}

static void
otama_test_dbi_mysql(void)
{
#if (OTAMA_WITH_MYSQL && defined(OTAMA_DBI_TEST))
	otama_dbi_config_t config;
	
	OTAMA_TEST_NAME;
	memset(&config, 0, sizeof(config));
	
	otama_dbi_config_driver(&config, "mysql");
	otama_dbi_config_dbname(&config, "otama_test");
	otama_dbi_config_username(&config, "nagadomi");
	otama_dbi_config_password(&config, "1234");	
	otama_test_dbi_sql(&config);
#else
	printf("skip mysql..\n");
#endif
}

void
otama_test_dbi(void)
{
	int i;
	for (i = 0; i < 2; ++i) {
		otama_test_dbi_pgsql();
		otama_test_dbi_sqlite3();
		otama_test_dbi_mysql();
	}
}
