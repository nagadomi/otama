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
test_convert(void)
{
	otama_variant_pool_t *pool = otama_variant_pool_alloc();
	otama_variant_t *var = otama_variant_new(pool);

	NV_ASSERT(otama_variant_type(var) == OTAMA_VARIANT_TYPE_NULL);
	NV_ASSERT(otama_variant_to_int(var) == 0);
	NV_ASSERT(otama_variant_to_float(var) == 0.0f);
	NV_ASSERT(strcmp(otama_variant_to_string(var), "(null)") == 0);
	
	otama_variant_set_string(var, "123hoge");
	NV_ASSERT(otama_variant_to_int(var) == 123);
	NV_ASSERT(otama_variant_to_float(var) == 123.0f);
	NV_ASSERT(strcmp(otama_variant_to_string(var), "123hoge") == 0);
	
	otama_variant_set_string(var, "-100.5");
	NV_ASSERT(otama_variant_to_int(var) == -100);
	NV_ASSERT(otama_variant_to_float(var) == -100.5f);
	NV_ASSERT(strcmp(otama_variant_to_string(var), "-100.5") == 0);
	
	otama_variant_pool_free(&pool);
}

static void
test_to_string(void)
{
	otama_variant_pool_t *pool = otama_variant_pool_alloc();
	otama_variant_t *root = otama_variant_new(pool);
	otama_variant_t *var, *value, *value2;
	int i;
	
	otama_variant_set_hash(root);
	var = otama_variant_hash_at(root, "string value");
	otama_variant_set_string(var, "123hogehoge");
	var = otama_variant_hash_at(root, "integer value");
	otama_variant_set_int(var, 0xffff);
	var = otama_variant_hash_at(root, "floating value");
	otama_variant_set_float(var, -1.2345e-6f);
	var = otama_variant_hash_at(root, "pointer value");
	otama_variant_set_pointer(var, (void *)0x7FFF1234);
	var = otama_variant_hash_at(root, "binary value");
	otama_variant_set_binary(var, "\x00\x01\x02\xfe\xdc", 5);
	var = otama_variant_hash_at(root, "array value");
	
	otama_variant_set_array(var);
	for (i = 0; i < 2; ++i) {
		value = otama_variant_array_at(var, i);
		otama_variant_set_int(value, i);
	}
	for (i = 2; i < 4; ++i) {
		value = otama_variant_array_at(var, i);
		otama_variant_set_float(value, i * 0.1234f);
	}
	value = otama_variant_array_at(var, 5);
	otama_variant_set_hash(value);
	value2 = otama_variant_hash_at(value, "key1");
	otama_variant_set_string(value2, "value1");
	value2 = otama_variant_hash_at(value, "key2");
	otama_variant_set_string(value2, "value2");
	value = otama_variant_array_at(var, 6);
	otama_variant_set_array(value);
	value2 = otama_variant_array_at(value, 0);
	otama_variant_set_string(value2, "value1");
	value2 = otama_variant_array_at(value, 1);
	otama_variant_set_string(value2, "value2");

	otama_variant_print(stderr, root);
	otama_variant_pool_free(&pool);
}

static void
test_array(void)
{
	otama_variant_pool_t *pool = otama_variant_pool_alloc();
	otama_variant_t *array = otama_variant_new(pool);
	int i;
	otama_variant_t *var;
	int64_t count;
	
	otama_variant_set_array(array);
	var = otama_variant_array_at(array, 0);
	otama_variant_set_string(var, "123hoge");
	var = otama_variant_array_at(array, 1);
	otama_variant_set_string(var, "-100.5");
	count = otama_variant_array_count(array);
	NV_ASSERT(count == 2);
	
	var = otama_variant_array_at(array, 0);
	NV_ASSERT(otama_variant_to_int(var) == 123);
	NV_ASSERT(otama_variant_to_float(var) == 123.0f);
	NV_ASSERT(strcmp(otama_variant_to_string(var), "123hoge") == 0);
	
	var = otama_variant_array_at(array, 1);
	NV_ASSERT(otama_variant_to_int(var) == -100);
	NV_ASSERT(otama_variant_to_float(var) == -100.5f);
	NV_ASSERT(strcmp(otama_variant_to_string(var), "-100.5") == 0);
#ifdef _OPENMP
#pragma omp parallel for
#endif	
	for (i = 0; i < 100; ++i) {
		otama_variant_t *v = otama_variant_array_at(array, i);
		otama_variant_set_int(v, i);
	}
	NV_ASSERT(otama_variant_array_count(array) == 100);
	for (i = 0; i < 100; ++i) {
		var = otama_variant_array_at(array, i);
		NV_ASSERT(otama_variant_to_int(var) == i);
	}
	
	otama_variant_pool_free(&pool);
}

static void
test_hash(void)
{
	otama_variant_pool_t *pool = otama_variant_pool_alloc();
	otama_variant_t *hash = otama_variant_new(pool);
	int i;
	otama_variant_t *var, *keys;
	
	otama_variant_set_hash(hash);
	var = otama_variant_hash_at(hash, "hoge");
	otama_variant_set_string(var, "123hoge");
	var = otama_variant_hash_at(hash, "piyo");
	otama_variant_set_string(var, "-100.5");
	NV_ASSERT(otama_variant_hash_exist(hash, "hoge") == 1);
	NV_ASSERT(otama_variant_hash_exist(hash, "hage") == 0);
	NV_ASSERT(otama_variant_hash_exist(hash, "piyo") == 1);
	
	keys = otama_variant_hash_keys(hash);
	NV_ASSERT(otama_variant_type(keys) == OTAMA_VARIANT_TYPE_ARRAY);
	NV_ASSERT(otama_variant_array_count(keys) == 2);
	if (strcmp(otama_variant_to_string(otama_variant_array_at(keys, 0)),
			   "hoge") == 0)
	{
		NV_ASSERT(strcmp(otama_variant_to_string(otama_variant_array_at(keys, 0)),
						 "hoge") == 0);
		NV_ASSERT(strcmp(otama_variant_to_string(otama_variant_array_at(keys, 1)),
						 "piyo") == 0);
	} else {
		NV_ASSERT(strcmp(otama_variant_to_string(otama_variant_array_at(keys, 1)),
						 "hoge") == 0);
		NV_ASSERT(strcmp(otama_variant_to_string(otama_variant_array_at(keys, 0)),
						 "piyo") == 0);
	}
	otama_variant_hash_remove(hash, "hoge");
	otama_variant_hash_remove(hash, "piyo");
	NV_ASSERT(otama_variant_hash_exist(hash, "hoge") == 0);
	NV_ASSERT(otama_variant_hash_exist(hash, "piyo") == 0);
	
#ifdef _OPENMP
#pragma omp parallel for
#endif
	for (i = 0; i < 100; ++i) {
		otama_variant_t *key = otama_variant_new(pool);
		otama_variant_t *v;
		otama_variant_set_int(key, i);
		v = otama_variant_hash_at2(hash, key);
		otama_variant_set_int(v, i);
	}
	NV_ASSERT(otama_variant_array_count(otama_variant_hash_keys(hash)) == 100);
	for (i = 0; i < 100; ++i) {
		otama_variant_t *key = otama_variant_new(pool);
		otama_variant_set_int(key, i);
		var = otama_variant_hash_at2(hash, key);
		NV_ASSERT(otama_variant_to_int(var) == i);
	}
	
	otama_variant_pool_free(&pool);
}

void
otama_test_variant(void)
{
	OTAMA_TEST_NAME;

	test_convert();
	test_to_string();
	test_array();
	test_hash();
}
