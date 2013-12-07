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
#include "otama.h"
#include "otama_util.h"
#include "otama_test.h"
#include "nv_core.h"

static void
drop_create(const char *config)
{
	otama_t *otama;

	OTAMA_TEST_NAME;
	
	NV_ASSERT(otama_open(&otama, config) == OTAMA_STATUS_OK);
	otama_drop_database(otama);
	NV_ASSERT(otama_create_database(otama) == OTAMA_STATUS_OK);
	
	otama_close(&otama);
}

static void
test_error(const char *config)
{
	otama_id_t id;
	otama_t *otama;
	otama_result_t *results;
	char *s;
	otama_feature_raw_t *p;
	int64_t count;
	
	OTAMA_TEST_NAME;
	
	drop_create(config);
	NV_ASSERT(otama_open(&otama, "ok_success") != OTAMA_STATUS_OK);
	NV_ASSERT(otama_open(&otama, config) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_insert_file(otama, &id, "ok_success") != OTAMA_STATUS_OK);
	NV_ASSERT(otama_insert_data(otama, &id, "ok_success", 8) != OTAMA_STATUS_OK);
	NV_ASSERT(otama_insert_data(otama, &id, "ok_success", 8) != OTAMA_STATUS_OK);
	NV_ASSERT(otama_search_file(otama, &results, 10, "ok_success") != OTAMA_STATUS_OK);
	NV_ASSERT(otama_search_data(otama, &results, 10, "ok_success", 8) != OTAMA_STATUS_OK);
	NV_ASSERT(otama_feature_string_file(otama, &s, "ok_success") != OTAMA_STATUS_OK);
	NV_ASSERT(otama_feature_string_data(otama, &s, "ok_success", 8) != OTAMA_STATUS_OK);
	NV_ASSERT(otama_feature_raw_file(otama, &p, "ok_success") != OTAMA_STATUS_OK);
	NV_ASSERT(otama_feature_raw_data(otama, &p, "ok_success", 8) != OTAMA_STATUS_OK);
	
	NV_ASSERT(otama_active(otama));
	NV_ASSERT(otama_insert_file(otama, &id, OTAMA_TEST_IMG) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_insert_file(otama, &id, OTAMA_TEST_IMG_NEGA) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_pull(otama) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_count(otama, &count) == OTAMA_STATUS_OK);
	NV_ASSERT(count == 2);
	
	otama_close(&otama);
}

static void
test_open(const char *config)
{
	otama_t *otama;
	otama_variant_pool_t *pool;
	otama_variant_t *opt;
	
	OTAMA_TEST_NAME;
	drop_create(config);
	
	NV_ASSERT(otama_open(&otama, config) == OTAMA_STATUS_OK);
	otama_close(&otama);

	pool = otama_variant_pool_alloc();
	opt = otama_yaml_read_file(config, pool);
	NV_ASSERT(opt != NULL);
	
	NV_ASSERT(otama_open_opt(&otama, opt) == OTAMA_STATUS_OK);
	otama_close(&otama);

	otama_variant_pool_free(&pool);
}

static void
test_drop_create(const char *config)
{
	OTAMA_TEST_NAME;
	drop_create(config);
}

static void
test_drop_index(const char *config)
{
	int64_t count;
	otama_id_t id;
	otama_t *otama;
	
	OTAMA_TEST_NAME;
	drop_create(config);
	
	NV_ASSERT(otama_open(&otama, config) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_insert_file(otama, &id, OTAMA_TEST_IMG) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_insert_file(otama, &id, OTAMA_TEST_IMG_NEGA) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_pull(otama) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_count(otama, &count) == OTAMA_STATUS_OK);
	NV_ASSERT(count == 2);
	
	NV_ASSERT(otama_drop_index(otama) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_count(otama, &count) == OTAMA_STATUS_OK);
	NV_ASSERT(count == 0);
	NV_ASSERT(otama_pull(otama) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_count(otama, &count) == OTAMA_STATUS_OK);
	NV_ASSERT(count == 2);
	
	otama_close(&otama);
}

static void
test_vacuum_index(const char *config)
{
	int64_t count;
	otama_id_t id;
	otama_t *otama;
	otama_status_t ret;
	
	OTAMA_TEST_NAME;
	drop_create(config);
	
	NV_ASSERT(otama_open(&otama, config) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_insert_file(otama, &id, OTAMA_TEST_IMG) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_insert_file(otama, &id, OTAMA_TEST_IMG_NEGA) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_pull(otama) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_count(otama, &count) == OTAMA_STATUS_OK);
	NV_ASSERT(count == 2);
	ret = otama_vacuum_index(otama);
	NV_ASSERT(ret == OTAMA_STATUS_OK || ret == OTAMA_STATUS_NOT_IMPLEMENTED);
	NV_ASSERT(otama_pull(otama) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_count(otama, &count) == OTAMA_STATUS_OK);
	NV_ASSERT(count == 2);
	
	otama_close(&otama);
}


static void
test_white(const char *config)
{
	otama_t *otama;
	otama_id_t id;
	char *s;
	otama_feature_raw_t *p;
	
	OTAMA_TEST_NAME;
	drop_create(config);
	
	NV_ASSERT(otama_open(&otama, config) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_insert_file(otama, &id, OTAMA_TEST_IMG_WHITE) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_id_file(&id, OTAMA_TEST_IMG_WHITE) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_feature_string_file(otama, &s, OTAMA_TEST_IMG_WHITE) == OTAMA_STATUS_OK);
	otama_feature_string_free(&s);
	NV_ASSERT(otama_feature_raw_file(otama, &p, OTAMA_TEST_IMG_WHITE) == OTAMA_STATUS_OK);
	otama_feature_raw_free(&p);

	otama_close(&otama);
}

static void
test_id(const char *config)
{
	otama_t *otama;
	otama_id_t id1, id2, id3, id4;
	
	OTAMA_TEST_NAME;
	drop_create(config);
	
	NV_ASSERT(otama_open(&otama, config) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_insert_file(otama, &id1, OTAMA_TEST_IMG) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_insert_file(otama, &id2, OTAMA_TEST_IMG_NEGA) == OTAMA_STATUS_OK);
	
	otama_id_file(&id3, OTAMA_TEST_IMG);
	otama_id_file(&id4, OTAMA_TEST_IMG_NEGA);
	
	NV_ASSERT(memcmp(&id1, &id3, sizeof(id1)) == 0);
	NV_ASSERT(memcmp(&id2, &id4, sizeof(id1)) == 0);
	
	otama_close(&otama);
}

static void
test_insert_pull(const char *config)
{
	int64_t count;
	otama_id_t id;
	otama_t *otama;
	
	OTAMA_TEST_NAME;
	drop_create(config);
	
	NV_ASSERT(otama_open(&otama, config) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_insert_file(otama, &id, OTAMA_TEST_IMG) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_insert_file(otama, &id, OTAMA_TEST_IMG_NEGA) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_pull(otama) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_count(otama, &count) == OTAMA_STATUS_OK);
	NV_ASSERT(count == 2);
	NV_ASSERT(otama_insert_file(otama, &id, OTAMA_TEST_IMG) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_insert_file(otama, &id, OTAMA_TEST_IMG_NEGA) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_pull(otama) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_count(otama, &count) == OTAMA_STATUS_OK);
	NV_ASSERT(count == 2);

	otama_close(&otama);
}

static void
test_file_insert_search_remove_search(const char *config)
{
	otama_id_t id1, id2;
	otama_t *otama;
	otama_result_t *results;

	OTAMA_TEST_NAME;
	drop_create(config);
	
	NV_ASSERT(otama_open(&otama, config) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_insert_file(otama, &id1, OTAMA_TEST_IMG) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_insert_file(otama, &id2, OTAMA_TEST_IMG_NEGA) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_remove(otama, &id1) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_pull(otama) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_search_file(otama, &results, 10, OTAMA_TEST_IMG_NEGA) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_result_count(results) == 1);
	NV_ASSERT(memcmp(otama_result_id(results, 0), &id2, sizeof(id2)) == 0);
	otama_result_free(&results);
	
	NV_ASSERT(otama_remove(otama, &id2) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_pull(otama) == OTAMA_STATUS_OK);
	
	NV_ASSERT(otama_search_file(otama, &results, 10, OTAMA_TEST_IMG) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_result_count(results) == 0);
	otama_result_free(&results);
	
	NV_ASSERT(otama_search_file(otama, &results, 10, OTAMA_TEST_IMG_NEGA) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_result_count(results) == 0);
	otama_result_free(&results);
	
	otama_close(&otama);
}

static void
test_data_insert_search_remove_search(const char *config)
{
	otama_id_t id1, id2;
	otama_t *otama;
	otama_result_t *results;
	void *data1, *data2;
	size_t data1_len, data2_len;
	
	OTAMA_TEST_NAME;
	drop_create(config);
	
	NV_ASSERT(otama_open(&otama, config) == OTAMA_STATUS_OK);

	otama_test_read_file(OTAMA_TEST_IMG, &data1, &data1_len);
	otama_test_read_file(OTAMA_TEST_IMG_NEGA, &data2, &data2_len);
	
	NV_ASSERT(otama_insert_data(otama, &id1, data1, data1_len) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_insert_data(otama, &id2, data2, data2_len) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_remove(otama, &id1) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_pull(otama) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_search_data(otama, &results, 10, data2, data2_len) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_result_count(results) == 1);
	NV_ASSERT(memcmp(otama_result_id(results, 0), &id2, sizeof(id2)) == 0);
	otama_result_free(&results);
	
	NV_ASSERT(otama_remove(otama, &id2) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_pull(otama) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_search_data(otama, &results, 10, data1, data1_len) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_result_count(results) == 0);
	otama_result_free(&results);
	
	NV_ASSERT(otama_search_data(otama, &results, 10, data2, data2_len) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_result_count(results) == 0);
	otama_result_free(&results);
	
	nv_free(data1);
	nv_free(data2);
	
	otama_close(&otama);
}

static void
test_string_insert_search_remove_search(const char *config)
{
	otama_id_t id1, id2;
	otama_t *otama;
	otama_result_t *results;
	char *s1, *s2;
	void *data2;
	size_t data2_len;
	
	OTAMA_TEST_NAME;
	drop_create(config);
	
	NV_ASSERT(otama_open(&otama, config) == OTAMA_STATUS_OK);

	NV_ASSERT(otama_feature_string_file(otama, &s1, OTAMA_TEST_IMG) == OTAMA_STATUS_OK);
	otama_test_read_file(OTAMA_TEST_IMG_NEGA, &data2, &data2_len);
	NV_ASSERT(otama_feature_string_data(otama, &s2, data2, data2_len) == OTAMA_STATUS_OK);
	
	NV_ASSERT(otama_insert_file(otama, &id1, OTAMA_TEST_IMG) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_insert_data(otama, &id2, data2, data2_len) == OTAMA_STATUS_OK);
	
	NV_ASSERT(otama_remove(otama, &id1) == OTAMA_STATUS_OK);

	NV_ASSERT(otama_pull(otama) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_search_string(otama, &results, 10, s2)
			  == OTAMA_STATUS_OK);
	NV_ASSERT(otama_result_count(results) == 1);
	NV_ASSERT(memcmp(otama_result_id(results, 0), &id2, sizeof(id2)) == 0);
	otama_result_free(&results);
	
	NV_ASSERT(otama_remove(otama, &id2) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_pull(otama) == OTAMA_STATUS_OK);
	
	NV_ASSERT(otama_search_string(otama, &results, 10, s1)
			  == OTAMA_STATUS_OK);
	NV_ASSERT(otama_result_count(results) == 0);
	otama_result_free(&results);
	
	NV_ASSERT(otama_search_string(otama, &results, 10, s2)
			  == OTAMA_STATUS_OK);
	NV_ASSERT(otama_result_count(results) == 0);
	otama_result_free(&results);
	
	nv_free(data2);
	otama_feature_string_free(&s1);
	otama_feature_string_free(&s2);
	
	otama_close(&otama);
}

static void
test_raw_insert_search_remove_search(const char *config)
{
	otama_id_t id1, id2;
	otama_t *otama;
	otama_result_t *results;
	otama_feature_raw_t *s1, *s2;
	void *data2;
	size_t data2_len;
	
	OTAMA_TEST_NAME;
	drop_create(config);
	
	NV_ASSERT(otama_open(&otama, config) == OTAMA_STATUS_OK);

	NV_ASSERT(otama_feature_raw_file(otama, &s1, OTAMA_TEST_IMG) == OTAMA_STATUS_OK);
	otama_test_read_file(OTAMA_TEST_IMG_NEGA, &data2, &data2_len);
	NV_ASSERT(otama_feature_raw_data(otama, &s2, data2, data2_len) == OTAMA_STATUS_OK);
	
	NV_ASSERT(otama_insert_file(otama, &id1, OTAMA_TEST_IMG) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_insert_data(otama, &id2, data2, data2_len) == OTAMA_STATUS_OK);
	
	NV_ASSERT(otama_remove(otama, &id1) == OTAMA_STATUS_OK);

	NV_ASSERT(otama_pull(otama) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_search_raw(otama, &results, 10, s2)
			  == OTAMA_STATUS_OK);
	NV_ASSERT(otama_result_count(results) == 1);
	NV_ASSERT(memcmp(otama_result_id(results, 0), &id2, sizeof(id2)) == 0);
	otama_result_free(&results);
	
	NV_ASSERT(otama_remove(otama, &id2) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_pull(otama) == OTAMA_STATUS_OK);
	
	NV_ASSERT(otama_search_raw(otama, &results, 10, s1)
			  == OTAMA_STATUS_OK);
	NV_ASSERT(otama_result_count(results) == 0);
	otama_result_free(&results);
	
	NV_ASSERT(otama_search_raw(otama, &results, 10, s2)
			  == OTAMA_STATUS_OK);
	NV_ASSERT(otama_result_count(results) == 0);
	otama_result_free(&results);
	
	nv_free(data2);
	otama_feature_raw_free(&s1);
	otama_feature_raw_free(&s2);
	
	otama_close(&otama);
}


static void
test_id_insert_search_remove_search(const char *config)
{
	otama_id_t id1, id2;
	otama_t *otama;
	otama_result_t *results;
	
	OTAMA_TEST_NAME;
	drop_create(config);
	
	NV_ASSERT(otama_open(&otama, config) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_insert_file(otama, &id1, OTAMA_TEST_IMG) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_insert_file(otama, &id2, OTAMA_TEST_IMG_NEGA) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_remove(otama, &id1) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_pull(otama) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_search_id(otama, &results, 10, &id2)
			  == OTAMA_STATUS_OK);
	NV_ASSERT(otama_result_count(results) == 1);
	NV_ASSERT(memcmp(otama_result_id(results, 0), &id2, sizeof(id2)) == 0);
	otama_result_free(&results);
	
	NV_ASSERT(otama_remove(otama, &id2) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_pull(otama) == OTAMA_STATUS_OK);
	
	NV_ASSERT(otama_search_id(otama, &results, 10, &id1) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_result_count(results) == 0);
	otama_result_free(&results);
	
	NV_ASSERT(otama_search_id(otama, &results, 10, &id2) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_result_count(results) == 0);
	otama_result_free(&results);
	
	otama_close(&otama);
}


void
otama_test_api(const char *config)
{
	OTAMA_TEST_NAME;
	printf("config: %s\n", config);
	test_open(config);
	test_drop_create(config);
	test_white(config);	
	test_id(config);
	test_insert_pull(config);
	test_drop_index(config);
	test_vacuum_index(config);
	test_file_insert_search_remove_search(config);
	test_error(config);
	test_data_insert_search_remove_search(config);
	test_string_insert_search_remove_search(config);
	test_id_insert_search_remove_search(config);
	test_raw_insert_search_remove_search(config);
}
