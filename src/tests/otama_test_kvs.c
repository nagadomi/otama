/*
 * This file is part of otama.
 *
 * Copyright (C) 2013 nagadomi@nurs.or.jp
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
#if OTAMA_HAS_KVS
#include "otama_kvs.h"
#include "otama_test.h"
#include "nv_core.h"

#define DB_PATH "./data/test.kvs"

static void delete_db(void)
{
	otama_kvs_t *kvs = NULL;
	NV_ASSERT(otama_kvs_open(&kvs, DB_PATH) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_kvs_clear(kvs) == OTAMA_STATUS_OK);
	otama_kvs_close(&kvs);
}

static void
test_setget(void)
{
	otama_kvs_t *kvs = NULL;
	int64_t key;
	int64_t value;
	otama_kvs_value_t *db_value = otama_kvs_value_alloc();
	const char *key_s;
	const char *value_s;
	
	delete_db();
	NV_ASSERT(otama_kvs_open(&kvs, DB_PATH) == OTAMA_STATUS_OK);
	NV_ASSERT(kvs != NULL);
	key = -123456;
	value = -654321;
	NV_ASSERT(otama_kvs_set(kvs, &key, sizeof(key),
							&value, sizeof(value)) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_kvs_get(kvs, &key, sizeof(key), db_value) == OTAMA_STATUS_OK);
	NV_ASSERT(OTAMA_KVS_VALUE_LEN(db_value) == sizeof(value));
	NV_ASSERT(*(const int64_t *)OTAMA_KVS_VALUE_PTR(db_value) == value);
	
	key = 0xFFFFFFFFFFFFFFFFULL;
	value = 0x1234567812345678ULL;
	NV_ASSERT(otama_kvs_set(kvs, &key, sizeof(key),
							&value, sizeof(value)) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_kvs_get(kvs, &key, sizeof(key), db_value) == OTAMA_STATUS_OK);
	NV_ASSERT(OTAMA_KVS_VALUE_LEN(db_value) == sizeof(value));
	NV_ASSERT(*(const int64_t *)OTAMA_KVS_VALUE_PTR(db_value) == value);

	key_s = "thekey";
	value_s = "thevalue";
	NV_ASSERT(otama_kvs_set(kvs, key_s, strlen(key_s),
							value_s, strlen(value_s) + 1) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_kvs_get(kvs, key_s, strlen(key_s), db_value) == OTAMA_STATUS_OK);
	NV_ASSERT(OTAMA_KVS_VALUE_LEN(db_value) == strlen(value_s) + 1);
	NV_ASSERT(memcmp(OTAMA_KVS_VALUE_PTR(db_value),
					 value_s,
					 OTAMA_KVS_VALUE_LEN(db_value)) == 0);
	otama_kvs_close(&kvs);

	key = -123456;
	value = -654321;
	NV_ASSERT(otama_kvs_open(&kvs, DB_PATH) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_kvs_get(kvs, &key, sizeof(key), db_value) == OTAMA_STATUS_OK);
	NV_ASSERT(OTAMA_KVS_VALUE_LEN(db_value) == sizeof(value));
	NV_ASSERT(*(const int64_t *)OTAMA_KVS_VALUE_PTR(db_value) == value);

	key = 0xFFFFFFFFFFFFFFFFULL;
	value = 0x1234567812345678ULL;
	NV_ASSERT(otama_kvs_get(kvs, &key, sizeof(key), db_value) == OTAMA_STATUS_OK);
	NV_ASSERT(OTAMA_KVS_VALUE_LEN(db_value) == sizeof(value));
	NV_ASSERT(*(const int64_t *)OTAMA_KVS_VALUE_PTR(db_value) == value);
	
	key_s = "thekey";
	value_s = "thevalue";
	NV_ASSERT(otama_kvs_get(kvs, key_s, strlen(key_s), db_value) == OTAMA_STATUS_OK);
	NV_ASSERT(OTAMA_KVS_VALUE_LEN(db_value) == strlen(value_s) + 1);
	NV_ASSERT(memcmp(OTAMA_KVS_VALUE_PTR(db_value),
					 value_s,
					 OTAMA_KVS_VALUE_LEN(db_value)) == 0);
	otama_kvs_close(&kvs);
	
	delete_db();
	NV_ASSERT(otama_kvs_open(&kvs, DB_PATH) == OTAMA_STATUS_OK);
	
	key = -123456;
	value = -654321;

	NV_ASSERT(otama_kvs_get(kvs, &key, sizeof(key),
							db_value) == OTAMA_STATUS_NODATA);
	NV_ASSERT(otama_kvs_set(kvs, &key, sizeof(key),
							&value, sizeof(value)) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_kvs_set(kvs, &key, sizeof(key),
							&value, sizeof(value)) == OTAMA_STATUS_OK);
	
	key = 0xFFFFFFFFFFFFFFFFULL;
	value = 0x1234567812345678ULL;
	NV_ASSERT(otama_kvs_get(kvs, &key, sizeof(key),
							db_value) == OTAMA_STATUS_NODATA);
	NV_ASSERT(otama_kvs_set(kvs, &key, sizeof(key),
							&value, sizeof(value)) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_kvs_set(kvs, &key, sizeof(key),
							&value, sizeof(value)) == OTAMA_STATUS_OK);
	
	key_s = "thekey";
	NV_ASSERT(otama_kvs_get(kvs, key_s, strlen(key_s),
							db_value) == OTAMA_STATUS_NODATA);
	NV_ASSERT(otama_kvs_set(kvs, key_s, strlen(key_s),
							value_s, strlen(value_s) + 1) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_kvs_set(kvs, key_s, strlen(key_s),
							value_s, strlen(value_s) + 1) == OTAMA_STATUS_OK);

	
	otama_kvs_close(&kvs);
	otama_kvs_value_free(&db_value);
}

typedef struct {
	int ksum;
	int vsum;
} kv_sum_t;

int
test_each_pair_func(void *user_data,
					const void *key, size_t key_len,
					const void *value, size_t value_len)
{
	const int *kp, *vp;
	kv_sum_t *sum = (kv_sum_t *)user_data;
	
	NV_ASSERT(sum != NULL);
	NV_ASSERT(key_len == sizeof(int));
	NV_ASSERT(value_len == sizeof(int));
	kp = (const int *)key;
	vp = (const int *)value;
	NV_ASSERT(*kp * 11 == *vp);
	sum->ksum += *kp;
	sum->vsum += *vp;
	
	return 0;
}

int
test_each_single_func(void *user_data,
					  const void *p, size_t len)
{
	const int *vp;
	int *sum = (int *)user_data;
	
	NV_ASSERT(sum != NULL);
	NV_ASSERT(len == sizeof(int));
	vp = (const int *)p;
	*sum += *vp;
	
	return 0;
}

void
test_each(void)
{
	otama_kvs_t *kvs;
	int i, ksum, vsum;
	kv_sum_t kv_sum;
	int sum;
	
	delete_db();
	NV_ASSERT(otama_kvs_open(&kvs, DB_PATH) == OTAMA_STATUS_OK);

	ksum = vsum = 0;
#ifdef _OPENMP
#pragma omp parallel for reduction(+: ksum, vsum)
#endif
	for (i = 0; i < 100; ++i) {
		int value = i * 11;
		NV_ASSERT(otama_kvs_set(kvs, &i, sizeof(i),
								&value, sizeof(value)) == OTAMA_STATUS_OK);
		ksum += i;
		vsum += value;
	}
	kv_sum.ksum = kv_sum.vsum = 0;
	NV_ASSERT(otama_kvs_each_pair(kvs, test_each_pair_func, &kv_sum) == OTAMA_STATUS_OK);
	NV_ASSERT(kv_sum.ksum == ksum);
	NV_ASSERT(kv_sum.vsum == vsum);
	
	sum = 0;
	NV_ASSERT(otama_kvs_each_key(kvs, test_each_single_func, &sum) == OTAMA_STATUS_OK);
	NV_ASSERT(sum == ksum);

	sum = 0;
	NV_ASSERT(otama_kvs_each_value(kvs, test_each_single_func, &sum) == OTAMA_STATUS_OK);
	NV_ASSERT(sum == vsum);
	
	otama_kvs_close(&kvs);
}

void
test_vacuum(void)
{
	otama_kvs_t *kvs;
	otama_kvs_value_t *db_value = otama_kvs_value_alloc();
	char key, value;
	delete_db();
	NV_ASSERT(otama_kvs_open(&kvs, DB_PATH) == OTAMA_STATUS_OK);

	key = 123;
	value = 21;
	NV_ASSERT(otama_kvs_set(kvs, &key, sizeof(key),
							&value, sizeof(value)) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_kvs_get(kvs, &key, sizeof(key), db_value) == OTAMA_STATUS_OK);
	NV_ASSERT(OTAMA_KVS_VALUE_LEN(db_value) == sizeof(value));
	NV_ASSERT(*(const char *)OTAMA_KVS_VALUE_PTR(db_value) == value);

	NV_ASSERT(otama_kvs_vacuum(kvs) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_kvs_get(kvs, &key, sizeof(key), db_value) == OTAMA_STATUS_OK);
	NV_ASSERT(OTAMA_KVS_VALUE_LEN(db_value) == sizeof(value));
	NV_ASSERT(*(const char *)OTAMA_KVS_VALUE_PTR(db_value) == value);
	
	otama_kvs_close(&kvs);
	otama_kvs_value_free(&db_value);
}

void
test_clear(void)
{
	otama_kvs_t *kvs;
	otama_kvs_value_t *db_value = otama_kvs_value_alloc();
	short key, value;
	delete_db();
	NV_ASSERT(otama_kvs_open(&kvs, DB_PATH) == OTAMA_STATUS_OK);

	key = 1234;
	value = 4321;
	NV_ASSERT(otama_kvs_set(kvs, &key, sizeof(key),
							&value, sizeof(value)) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_kvs_get(kvs, &key, sizeof(key), db_value) == OTAMA_STATUS_OK);
	NV_ASSERT(OTAMA_KVS_VALUE_LEN(db_value) == sizeof(value));
	NV_ASSERT(*(const short *)OTAMA_KVS_VALUE_PTR(db_value) == value);

	NV_ASSERT(otama_kvs_clear(kvs) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_kvs_get(kvs, &key, sizeof(key), db_value) == OTAMA_STATUS_NODATA);
	
	otama_kvs_close(&kvs);
	otama_kvs_value_free(&db_value);
}

void
otama_test_kvs(void)
{
	OTAMA_TEST_NAME;

	test_setget();
	test_each();
	test_vacuum();
	test_clear();
}

#endif

