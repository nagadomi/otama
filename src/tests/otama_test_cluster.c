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
	otama_drop_table(otama);
	NV_ASSERT(otama_create_table(otama) == OTAMA_STATUS_OK);
	otama_close(&otama);
}

#define OTAMA_TEST_IMG_0_7_1 OTAMA_TEST_IMG
#define OTAMA_TEST_IMG_0_7_2 OTAMA_TEST_IMG_AFFINE
#define OTAMA_TEST_IMG_8_f_1 OTAMA_TEST_IMG_NEGA

void
otama_test_cluster(const char *node1_config,
				   const char *node2_config)
{
	int64_t count;
	otama_id_t id1, id2, id3;
	otama_t *otama1, *otama2;
	otama_result_t *result1, *result2, *result3;
	int i, hit;
	
	OTAMA_TEST_NAME;

	printf("%s, %s\n", node1_config, node2_config);

	drop_create(node1_config);
	drop_create(node2_config);

	NV_ASSERT(otama_open(&otama1, node1_config) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_open(&otama2, node2_config) == OTAMA_STATUS_OK);
	
	// insert
	NV_ASSERT(otama_insert_file(otama1, &id1, OTAMA_TEST_IMG_0_7_1) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_insert_file(otama1, &id2, OTAMA_TEST_IMG_8_f_1) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_pull(otama1) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_pull(otama2) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_search_file(otama1, &result1, 10, OTAMA_TEST_IMG_0_7_1) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_search_file(otama2, &result2, 10, OTAMA_TEST_IMG_8_f_1) == OTAMA_STATUS_OK);

	NV_ASSERT(otama_result_count(result1) == 1);
	NV_ASSERT(otama_result_count(result2) == 1);
	NV_ASSERT(memcmp(otama_result_id(result1, 0), &id1, sizeof(otama_id_t)) == 0);
	NV_ASSERT(memcmp(otama_result_id(result2, 0), &id2, sizeof(otama_id_t)) == 0);
	
	otama_result_free(&result1);
	otama_result_free(&result2);
	
	NV_ASSERT(otama_insert_file(otama1, &id3, OTAMA_TEST_IMG_0_7_2) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_pull(otama1) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_pull(otama2) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_search_file(otama1, &result1, 10, OTAMA_TEST_IMG_0_7_2) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_search_file(otama2, &result2, 10, OTAMA_TEST_IMG_0_7_2) == OTAMA_STATUS_OK);

	NV_ASSERT(otama_result_count(result1) > 1);
	NV_ASSERT(memcmp(otama_result_id(result1, 0), &id3, sizeof(otama_id_t)) == 0);

	count = otama_result_count(result1);
	hit = 0;
	for (i = 0; i < count; ++i) {
		if (memcmp(otama_result_id(result1, i), &id3, sizeof(otama_id_t)) == 0) {
			hit = 1;
		}
	}
	NV_ASSERT(hit == 1);
	count = otama_result_count(result2);
	hit = 0;
	for (i = 0; i < count; ++i) {
		if (memcmp(otama_result_id(result2, i), &id3, sizeof(otama_id_t)) == 0) {
			hit = 1;
		}
	}
	NV_ASSERT(hit == 0);
	
	otama_result_free(&result1);
	otama_result_free(&result2);

	// remove
	otama_id_file(&id1, OTAMA_TEST_IMG_0_7_1);
	otama_id_file(&id2, OTAMA_TEST_IMG_8_f_1);
	otama_id_file(&id3, OTAMA_TEST_IMG_0_7_2);
	
	NV_ASSERT(otama_remove(otama1, &id2) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_remove(otama2, &id1) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_pull(otama1) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_pull(otama2) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_search_file(otama1, &result1, 10, OTAMA_TEST_IMG_0_7_1) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_search_file(otama2, &result2, 10, OTAMA_TEST_IMG_8_f_1) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_search_file(otama1, &result3, 10, OTAMA_TEST_IMG_0_7_2) == OTAMA_STATUS_OK);

	count = otama_result_count(result1);
	hit = 0;
	for (i = 0; i < count; ++i) {
		if (memcmp(otama_result_id(result1, i), &id1, sizeof(otama_id_t)) == 0) {
			hit = 1;
		}
	}
	NV_ASSERT(hit == 0);
	
	count = otama_result_count(result2);
	hit = 0;
	for (i = 0; i < count; ++i) {
		if (memcmp(otama_result_id(result2, i), &id2, sizeof(otama_id_t)) == 0) {
			hit = 1;
		}
	}
	NV_ASSERT(hit == 0);

	count = otama_result_count(result3);
	hit = 0;
	for (i = 0; i < count; ++i) {
		if (memcmp(otama_result_id(result3, i), &id3, sizeof(otama_id_t)) == 0) {
			hit = 1;
		}
	}
	NV_ASSERT(hit == 1);
	
	otama_result_free(&result1);
	otama_result_free(&result2);
	otama_result_free(&result3);
	
	NV_ASSERT(otama_remove(otama2, &id3) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_pull(otama1) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_pull(otama2) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_search_file(otama1, &result1, 10, OTAMA_TEST_IMG_0_7_2) == OTAMA_STATUS_OK);

	count = otama_result_count(result1);
	hit = 0;
	for (i = 0; i < count; ++i) {
		if (memcmp(otama_result_id(result1, i), &id3, sizeof(otama_id_t)) == 0) {
			hit = 1;
		}
	}
	NV_ASSERT(hit == 0);
	
	otama_result_free(&result1);
	
	otama_close(&otama1);
	otama_close(&otama2);
}
