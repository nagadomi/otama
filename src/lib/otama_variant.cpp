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

#include "otama_variant.h"
#include "nv_portable.h"
#include "otama_omp_lock.hpp"
#include <map>
#include <string>
#include <vector>

namespace otama {};

using namespace std;
using namespace otama;

struct otama_variant_pool {
	vector<otama_variant_t *> vars;
#ifdef _OPENMP
	omp_nest_lock_t lock;
#endif
};

typedef map<int64_t, otama_variant_t *> varray_array_t;
typedef struct {
	varray_array_t *array;
	int64_t count;
} varray_t;

typedef map<string, otama_variant_t *> vhash_t;

struct otama_variant {
	otama_variant_pool_t *pool;
	otama_variant_type_e type;
	union {
		float f;
		int64_t i;
		string *s;
		vhash_t *h;
		varray_t a;
		void *p;
		struct {
			uint8_t *ptr;
			size_t len;
			int copy;
		} b;
	} value;
	string to_s;
};

otama_variant_pool_t *
otama_variant_pool_alloc(void)
{
	otama_variant_pool_t *pool = new otama_variant_pool_t;
	
#ifdef _OPENMP
	omp_init_nest_lock(&pool->lock);
#endif

	return pool;
}

otama_variant_t *
otama_variant_pool_new(otama_variant_pool_t *pool)
{
#ifdef _OPENMP		
	OMPLock lock(&pool->lock);
#endif	
	otama_variant_t *var = new otama_variant_t;

	memset(&var->value, 0, sizeof(var->value));
	var->type = OTAMA_VARIANT_TYPE_NULL;
	var->pool = pool;

	pool->vars.push_back(var);

	return var;
}

static void
otama_variant_clear(otama_variant_t *var)
{
#ifdef _OPENMP	
	OMPLock lock(&var->pool->lock);
#endif
	switch (var->type) {
	case OTAMA_VARIANT_TYPE_INT:
		break;
	case OTAMA_VARIANT_TYPE_POINTER:
		break;
	case OTAMA_VARIANT_TYPE_FLOAT:
		break;
	case OTAMA_VARIANT_TYPE_STRING:
		delete var->value.s;
		break;
	case OTAMA_VARIANT_TYPE_ARRAY:
		delete var->value.a.array;
		break;
	case OTAMA_VARIANT_TYPE_HASH:
		delete var->value.h;
		break;
	case OTAMA_VARIANT_TYPE_BINARY:
		if (var->value.b.copy && var->value.b.ptr) {
			delete [] var->value.b.ptr;
		}
		break;
	case OTAMA_VARIANT_TYPE_NULL:
		break;
	default:
		break;
	}
	memset(&var->value, 0, sizeof(var->value));
	var->to_s = "";
}

static void
otama_variant_pool_clear(otama_variant_pool_t *pool)
{
#ifdef _OPENMP	
	OMPLock lock(&pool->lock);
#endif
	vector<otama_variant_t *>::iterator it;
	for (it = pool->vars.begin(); it != pool->vars.end(); ++it) {
		otama_variant_clear(*it);
		delete *it;
	}
	pool->vars.clear();
}

void
otama_variant_pool_free(otama_variant_pool_t **pool)
{
#ifdef _OPENMP	
#pragma omp critical (otama_variant_pool_free)
#endif
	{
		if (pool && *pool) {
			otama_variant_pool_clear(*pool);
#ifdef _OPENMP	
			omp_destroy_nest_lock(&(*pool)->lock);
#endif
			delete *pool;
			*pool = NULL;
		}
	}
}

otama_variant_t *
otama_variant_new(otama_variant_pool_t *pool)
{
	return otama_variant_pool_new(pool);
}

void
otama_variant_set(otama_variant_t *var, otama_variant_type_e type)
{
#ifdef _OPENMP	
	OMPLock lock(&var->pool->lock);
#endif
	otama_variant_clear(var);
	
	var->type = type;
	switch (type) {
	case OTAMA_VARIANT_TYPE_HASH:
		var->value.h = new vhash_t;
		break;	
	case OTAMA_VARIANT_TYPE_ARRAY:
		var->value.a.array = new varray_array_t;
		var->value.a.count = 0;
		break;
	case OTAMA_VARIANT_TYPE_STRING:
		var->value.s = new string;
		break;
	default:
		break;
	}
}

void
otama_variant_set_null(otama_variant_t *var)
{
#ifdef _OPENMP	
	OMPLock lock(&var->pool->lock);
#endif
	
	otama_variant_set(var, OTAMA_VARIANT_TYPE_NULL);
}

void
otama_variant_set_hash(otama_variant_t *var)
{
#ifdef _OPENMP	
	OMPLock lock(&var->pool->lock);
#endif
	otama_variant_set(var, OTAMA_VARIANT_TYPE_HASH);
}

void
otama_variant_set_array(otama_variant_t *var)
{
#ifdef _OPENMP	
	OMPLock lock(&var->pool->lock);
#endif
	otama_variant_set(var, OTAMA_VARIANT_TYPE_ARRAY);
}

void
otama_variant_set_int(otama_variant_t *var, int64_t iv)
{
#ifdef _OPENMP	
	OMPLock lock(&var->pool->lock);
#endif
	otama_variant_set(var, OTAMA_VARIANT_TYPE_INT);
	var->value.i = iv;
}

void
otama_variant_set_pointer(otama_variant_t *var, const void *p)
{
#ifdef _OPENMP	
	OMPLock lock(&var->pool->lock);
#endif
	otama_variant_set(var, OTAMA_VARIANT_TYPE_POINTER);
	var->value.p = const_cast<void *>(p);
}

void
otama_variant_set_float(otama_variant_t *var, float fv)
{
#ifdef _OPENMP	
	OMPLock lock(&var->pool->lock);
#endif
	otama_variant_set(var, OTAMA_VARIANT_TYPE_FLOAT);
	var->value.f = fv;
}

void
otama_variant_set_string(otama_variant_t *var, const char *sv)
{
#ifdef _OPENMP	
	OMPLock lock(&var->pool->lock);
#endif
	otama_variant_set(var, OTAMA_VARIANT_TYPE_STRING);
	*var->value.s = sv;
}

void
otama_variant_set_binary(otama_variant_t *var, const void *data, size_t data_len)
{
#ifdef _OPENMP	
	OMPLock lock(&var->pool->lock);
#endif
	otama_variant_set(var, OTAMA_VARIANT_TYPE_BINARY);
	var->value.b.ptr = (uint8_t *)data;
	var->value.b.len = data_len;
	var->value.b.copy = 0;
}

void
otama_variant_set_binary_copy(otama_variant_t *var, const void *data, size_t data_len)
{
#ifdef _OPENMP	
	OMPLock lock(&var->pool->lock);
#endif
	otama_variant_set(var, OTAMA_VARIANT_TYPE_BINARY);
	var->value.b.ptr = new uint8_t[data_len];
	var->value.b.copy = 1;
	
	memcpy(var->value.b.ptr, data, data_len);
}

otama_variant_t *
otama_variant_array_find_or_create(otama_variant_t *var, int64_t index)
{
#ifdef _OPENMP	
	OMPLock lock(&var->pool->lock);
#endif
	varray_array_t::iterator it = var->value.a.array->find(index);
	pair<varray_array_t::iterator, bool> ret;

	if (it == var->value.a.array->end()) {
		ret = var->value.a.array->insert(
			make_pair<int64_t, otama_variant_t *>(index,
												  otama_variant_pool_new(var->pool)));
		it = ret.first;
	}
	if (index > var->value.a.count) {
		var->value.a.count = index;
	}
	return it->second;
}

otama_variant_t *
otama_variant_array_at(otama_variant_t *array, int64_t index)
{
#ifdef _OPENMP	
	OMPLock lock(&array->pool->lock);
#endif
	assert(OTAMA_VARIANT_IS_ARRAY(array));	
	return otama_variant_array_find_or_create(array, index);
}

otama_variant_t *
otama_variant_array_at2(otama_variant_t *array,
						otama_variant_t *index)
{
#ifdef _OPENMP	
	OMPLock lock(&array->pool->lock);
#endif
	
	int64_t i;
	
	assert(OTAMA_VARIANT_IS_ARRAY(array));
	
	i = otama_variant_to_int(index);
	return otama_variant_array_find_or_create(array, i);
}

int64_t
otama_variant_array_count(otama_variant_t *array)
{
#ifdef _OPENMP
	OMPLock lock(&array->pool->lock);
#endif
	
	assert(OTAMA_VARIANT_IS_ARRAY(array));
	
	return array->value.a.count + 1;
}

otama_variant_t *
otama_variant_hash_find_or_create(otama_variant_t *var, const char *key)
{
#ifdef _OPENMP
	OMPLock lock(&var->pool->lock);
#endif
	vhash_t::iterator it = var->value.h->find(key);
	pair<vhash_t::iterator, bool> ret;
	
	if (it == var->value.h->end()) {
		ret = var->value.h->insert(
			make_pair<string, otama_variant_t *>(key,
												 otama_variant_pool_new(var->pool)));
		it = ret.first;
	}
	return it->second;
}

void
otama_variant_hash_remove(otama_variant_t *var, const char *key)
{
#ifdef _OPENMP
	OMPLock lock(&var->pool->lock);
#endif
	vhash_t::iterator it;
	assert(OTAMA_VARIANT_IS_HASH(var));

	it = var->value.h->find(key);
	if (it != var->value.h->end()) {
		var->value.h->erase(it);
	}
}


otama_variant_t *
otama_variant_hash_at(otama_variant_t *hash,
					  const char *key)
{
#ifdef _OPENMP
	OMPLock lock(&hash->pool->lock);
#endif
	assert(OTAMA_VARIANT_IS_HASH(hash));
	return otama_variant_hash_find_or_create(hash, key);
}

otama_variant_t *
otama_variant_hash_at2(otama_variant_t *hash,
					   otama_variant_t *key)
{
#ifdef _OPENMP
	OMPLock lock(&hash->pool->lock);
#endif
	assert(OTAMA_VARIANT_IS_HASH(hash));	
	return otama_variant_hash_find_or_create(hash,
											 otama_variant_to_string(key));
}

const char *
otama_variant_hash_at3(otama_variant_t *hash,
					   const char *key)
{
#ifdef _OPENMP
	OMPLock lock(&hash->pool->lock);
#endif
	assert(OTAMA_VARIANT_IS_HASH(hash));	
	if (otama_variant_hash_exist(hash, key)) {
		return otama_variant_to_string(otama_variant_hash_at(hash, key));
	}

	return NULL;
}

int
otama_variant_hash_exist(otama_variant_t *hash, const char *key)
{
#ifdef _OPENMP
	OMPLock lock(&hash->pool->lock);
#endif
	vhash_t::iterator it = hash->value.h->find(key);
	
	assert(OTAMA_VARIANT_IS_HASH(hash));
	
	if (it == hash->value.h->end()) {
		return 0;
	}
	return 1;
}

int
otama_variant_hash_exist2(otama_variant_t *hash, otama_variant_t *key)
{
#ifdef _OPENMP
	OMPLock lock(&hash->pool->lock);
#endif
	vhash_t::iterator it = hash->value.h->find(otama_variant_to_string(key));
	
	assert(OTAMA_VARIANT_IS_HASH(hash));
		   
	if (it == hash->value.h->end()) {
		return 0;
	}
	return 1;
}

otama_variant_t *
otama_variant_hash_keys(otama_variant_t *hash)
{
#ifdef _OPENMP
	OMPLock lock(&hash->pool->lock);
#endif
	otama_variant_t *keys;
	int64_t i;
	vhash_t::iterator it;

	assert(OTAMA_VARIANT_IS_HASH(hash));

	keys = otama_variant_pool_new(hash->pool);
	otama_variant_set_array(keys);
	
	for (i = 0, it = hash->value.h->begin();
		 it != hash->value.h->end();
		 ++it, ++i)
	{
		otama_variant_set_string(
			otama_variant_array_at(keys, i),
			it->first.c_str());
	}
	
	return keys;
}

int64_t
otama_variant_to_int(otama_variant_t *var)
{
#ifdef _OPENMP
	OMPLock lock(&var->pool->lock);
#endif
	int64_t iv = 0;
	
	switch (var->type) {
	case OTAMA_VARIANT_TYPE_INT:
		iv = var->value.i;
		break;
	case OTAMA_VARIANT_TYPE_FLOAT:
		iv = (int64_t)var->value.f;
		break;
	case OTAMA_VARIANT_TYPE_STRING:
		iv = nv_strtoll(var->value.s->c_str(), NULL, 10);
		break;
	case OTAMA_VARIANT_TYPE_POINTER:		
		iv = (int64_t)var->value.p;
		break;
	case OTAMA_VARIANT_TYPE_ARRAY:
	case OTAMA_VARIANT_TYPE_HASH:
	case OTAMA_VARIANT_TYPE_NULL:
		break;
	default:
		break;
	}
	return iv;
}

void *
otama_variant_to_pointer(otama_variant_t *var)
{
#ifdef _OPENMP
	OMPLock lock(&var->pool->lock);
#endif
	void *p = NULL;

	if (var->type == OTAMA_VARIANT_TYPE_POINTER) {
		p = var->value.p;
	}
	
	return p;
}

float
otama_variant_to_float(otama_variant_t *var)
{
#ifdef _OPENMP
	OMPLock lock(&var->pool->lock);
#endif
	float fv = 0.0f;
	
	switch (var->type) {
	case OTAMA_VARIANT_TYPE_INT:
		fv = (float)var->value.i;
		break;
	case OTAMA_VARIANT_TYPE_FLOAT:
		fv = var->value.f;
		break;
	case OTAMA_VARIANT_TYPE_STRING:
		fv = (float)strtod(var->value.s->c_str(), NULL);
		break;
	case OTAMA_VARIANT_TYPE_ARRAY:
	case OTAMA_VARIANT_TYPE_HASH:
	case OTAMA_VARIANT_TYPE_NULL:
	case OTAMA_VARIANT_TYPE_POINTER:		
		break;
	default:
		break;
	}
	
	return fv;
}

const char *
otama_variant_to_string(otama_variant_t *var)
{
#ifdef _OPENMP
	OMPLock lock(&var->pool->lock);
#endif
	const char *sv = "";
	char temp[1024];
	
	switch (var->type) {
	case OTAMA_VARIANT_TYPE_INT:
		sprintf(temp, "%" PRId64, var->value.i);
		var->to_s = temp;
		sv = var->to_s.c_str();
		break;
	case OTAMA_VARIANT_TYPE_POINTER:
		sprintf(temp, "%p", var->value.p);
		var->to_s = temp;
		sv = var->to_s.c_str();
		break;
	case OTAMA_VARIANT_TYPE_FLOAT:
		sprintf(temp, "%E", var->value.f);
		var->to_s = temp;
		sv = var->to_s.c_str();
		break;
	case OTAMA_VARIANT_TYPE_STRING:
		sv = var->value.s->c_str();
		break;
	case OTAMA_VARIANT_TYPE_ARRAY:
		sprintf(temp, "ARRAY(%p)", &var->value.a);
		var->to_s = temp;
		sv = var->to_s.c_str();
		break;
	case OTAMA_VARIANT_TYPE_HASH:
		sprintf(temp, "HASH(%p)", &var->value.h);		
		var->to_s = temp;
		sv = var->to_s.c_str();
		break;
	case OTAMA_VARIANT_TYPE_NULL:
		sv = "NULL";
		break;
	case OTAMA_VARIANT_TYPE_BINARY: {
		size_t i;
		for (i = 0; ((char *)var->value.b.ptr)[i] != '\0' &&
				 i < var->value.b.len; ++i)
			;
		if (i == var->value.b.len) {
			char *str = new char[i + 1];
			memcpy(str, var->value.b.ptr, i);
			str[i] = '\0';
			var->to_s = str;
			delete [] str;
			sv = var->to_s.c_str();
		} else {
			sprintf(temp, "BINARY(%p)", var->value.b.ptr);
			var->to_s = temp;
			sv = var->to_s.c_str();
		}
	}		
		break;
	default:
		sv = "UNKNOWN";
		break;
	}
	
	return sv;
}

const void *
otama_variant_to_binary_ptr(otama_variant_t *var)
{
#ifdef _OPENMP
	OMPLock lock(&var->pool->lock);
#endif
	const void *p = (const void *)"";
	switch (var->type) {
	case OTAMA_VARIANT_TYPE_INT:
		p = &var->value.i;
		break;
	case OTAMA_VARIANT_TYPE_FLOAT:
		p = &var->value.f;		
		break;
	case OTAMA_VARIANT_TYPE_STRING:
		p = var->value.s->c_str();
		break;
	case OTAMA_VARIANT_TYPE_POINTER:
		p = var->value.p;
		break;
	case OTAMA_VARIANT_TYPE_BINARY:
		p = var->value.b.ptr;
		break;
	case OTAMA_VARIANT_TYPE_ARRAY:
	case OTAMA_VARIANT_TYPE_HASH:
	case OTAMA_VARIANT_TYPE_NULL:
		break;
	default:
		break;
	}
	
	return p;
}

size_t
otama_variant_to_binary_len(otama_variant_t *var)
{
#ifdef _OPENMP
	OMPLock lock(&var->pool->lock);
#endif
	size_t len = 0;
	
	switch (var->type) {
	case OTAMA_VARIANT_TYPE_INT:
		len = sizeof(var->value.i);
		break;
	case OTAMA_VARIANT_TYPE_FLOAT:
		len = sizeof(var->value.f);
		break;
	case OTAMA_VARIANT_TYPE_STRING:
		len = strlen(var->value.s->c_str());
		break;
	case OTAMA_VARIANT_TYPE_BINARY:
		len = var->value.b.len;
		break;
	case OTAMA_VARIANT_TYPE_POINTER:
		len = sizeof(void *);
		break;
	case OTAMA_VARIANT_TYPE_ARRAY:
	case OTAMA_VARIANT_TYPE_HASH:
	case OTAMA_VARIANT_TYPE_NULL:
		break;
	default:
		break;
	}
	
	return len;
}

otama_variant_type_e
otama_variant_type(otama_variant_t *var)
{
#ifdef _OPENMP
	OMPLock lock(&var->pool->lock);
#endif
	return var->type;
}

void
otama_variant_print(FILE *out, otama_variant_t *var)
{
#ifdef _OPENMP
	OMPLock lock(&var->pool->lock);
#endif
	switch (otama_variant_type(var)) {
	case OTAMA_VARIANT_TYPE_NULL:
		fprintf(out, "NULL");
		break;
	case OTAMA_VARIANT_TYPE_POINTER:
		fprintf(out, "%p", otama_variant_to_pointer(var));
		break;
	case OTAMA_VARIANT_TYPE_INT:
		fprintf(out, "%"PRId64, otama_variant_to_int(var));
		break;
	case OTAMA_VARIANT_TYPE_FLOAT:
		fprintf(out, "%E", otama_variant_to_float(var));
		break;
	case OTAMA_VARIANT_TYPE_STRING: {
		string escape_string;
		const char *s = otama_variant_to_string(var);
		
		while (*s != '\0') {
			if (*s == '"') {
				escape_string += '\\';
				escape_string += *s;
			} else {
				escape_string += *s;
			}
			++s;
		}
		fprintf(out, "\"%s\"", escape_string.c_str());
	}
		break;
	case OTAMA_VARIANT_TYPE_ARRAY: {
		int64_t count = otama_variant_array_count(var);
		int64_t i;
		fprintf(out, "[");
		for (i = 0; i < count; ++i) {
			if (i != 0) {
				fprintf(out, ",");
			}
			otama_variant_print(out, otama_variant_array_at(var, i));
		}
		fprintf(out, "]");
	}
		break;
	case OTAMA_VARIANT_TYPE_HASH: {
		otama_variant_t *keys = otama_variant_hash_keys(var);
		int64_t count = otama_variant_array_count(keys);
		int64_t i;

		fprintf(out, "{");
		for (i = 0; i < count; ++i) {
			if (i != 0) {
				fprintf(out, ",");
			}
			otama_variant_print(out, otama_variant_array_at(keys, i));
			fprintf(out, ":");
			otama_variant_print(out,
								otama_variant_hash_at2(var,
													   otama_variant_array_at(keys, i)));
		}
		fprintf(out, "}");		
	}
		break;
	case OTAMA_VARIANT_TYPE_BINARY: {
		size_t i;
		fprintf(out, "\"");
		fprintf(out, "hex:");
		for (i = 0; i < var->value.b.len; ++i) {
			fprintf(out, "%02x", ((uint8_t *)var->value.b.ptr)[i] & 0xff);
		}
		fprintf(out, "\"");		
	}
		break;
	default:
		break;
	}
}

int
otama_variant_to_bool(otama_variant_t *var)
{
#ifdef _OPENMP
	OMPLock lock(&var->pool->lock);
#endif
	int ret = OTAMA_VARIANT_FALSE;
	
	switch (var->type) {
	case OTAMA_VARIANT_TYPE_INT:
		ret = var->value.i == 0 ? OTAMA_VARIANT_FALSE: OTAMA_VARIANT_TRUE;
		break;
	case OTAMA_VARIANT_TYPE_FLOAT:
		ret = OTAMA_VARIANT_TRUE;
		break;
	case OTAMA_VARIANT_TYPE_STRING:
		if (nv_strcasecmp(var->value.s->c_str(), "true") == 0) {
			ret = OTAMA_VARIANT_TRUE;
		} else if (nv_strcasecmp(var->value.s->c_str(), "on") == 0) {
			ret = OTAMA_VARIANT_TRUE;
		} else if (otama_variant_to_int(var) != 0) {
			ret = OTAMA_VARIANT_TRUE;
		}
		break;
	case OTAMA_VARIANT_TYPE_ARRAY:
		ret = OTAMA_VARIANT_TRUE;
	   break;
	case OTAMA_VARIANT_TYPE_HASH:
		ret = OTAMA_VARIANT_TRUE;
		break;
	case OTAMA_VARIANT_TYPE_NULL:
		ret = OTAMA_VARIANT_FALSE;
		break;
	case OTAMA_VARIANT_TYPE_POINTER:
		if (var->value.p != 0) {
			ret = OTAMA_VARIANT_TRUE;
		}
		break;
	case OTAMA_VARIANT_TYPE_BINARY:
		if (var->value.b.len > 0) {
			ret = OTAMA_VARIANT_TRUE;
		}		
		break;
	default:
		break;
	}
	
	return ret;
}


otama_variant_t *
otama_variant_copy(otama_variant_t *dest, otama_variant_t *src, int deep)
{
#ifdef _OPENMP
	OMPLock lock1(&dest->pool->lock);
	OMPLock lock2(&src->pool->lock);	
#endif
	
	switch (src->type) {
	case OTAMA_VARIANT_TYPE_INT:
		otama_variant_set_int(dest, otama_variant_to_int(src));
		break;
	case OTAMA_VARIANT_TYPE_FLOAT:
		otama_variant_set_float(dest, otama_variant_to_float(src));
		break;
	case OTAMA_VARIANT_TYPE_STRING:
		otama_variant_set_string(dest, otama_variant_to_string(src));
		break;
	case OTAMA_VARIANT_TYPE_ARRAY:
		otama_variant_set_array(dest);
		{
			int i;
			for (i = 0; i < otama_variant_array_count(src); ++i) {
				otama_variant_copy(otama_variant_array_at(dest, i),
								   otama_variant_array_at(src, i),
								   deep);
			}
		}
	   break;
	case OTAMA_VARIANT_TYPE_HASH:
		otama_variant_set_hash(dest);
		{
			int i;
			otama_variant_t *keys = otama_variant_hash_keys(src);
			
			for (i = 0; i < otama_variant_array_count(keys); ++i) {
				otama_variant_copy(otama_variant_hash_at2(dest, otama_variant_array_at(keys, i)),
								   otama_variant_hash_at2(src, otama_variant_array_at(keys, i)),
								   deep);
			}
		}
		break;
	case OTAMA_VARIANT_TYPE_NULL:
		otama_variant_set_null(dest);
		break;
	case OTAMA_VARIANT_TYPE_POINTER:
		otama_variant_set_pointer(dest, otama_variant_to_pointer(src));
		break;
	case OTAMA_VARIANT_TYPE_BINARY:
		if (deep) {
			otama_variant_set_binary_copy(dest,
										  otama_variant_to_binary_ptr(src),
										  otama_variant_to_binary_len(src));
		} else {
			otama_variant_set_binary(dest,
									 otama_variant_to_binary_ptr(src),
									 otama_variant_to_binary_len(src));
		}
		break;
	default:
		break;
	}

	return dest;
}

otama_variant_t *
otama_variant_dup(otama_variant_t *var, otama_variant_pool_t *pool)
{
	otama_variant_t *dup = otama_variant_new(pool);
	
	otama_variant_copy(dup, var, 1);
	
	return dup;
}
