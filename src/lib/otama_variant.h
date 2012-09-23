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
#ifndef OTAMA_VARIANT_H
#define OTAMA_VARIANT_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	OTAMA_VARIANT_TYPE_NULL,
	OTAMA_VARIANT_TYPE_INT,
	OTAMA_VARIANT_TYPE_FLOAT,
	OTAMA_VARIANT_TYPE_STRING,
	OTAMA_VARIANT_TYPE_HASH,
	OTAMA_VARIANT_TYPE_ARRAY,
	OTAMA_VARIANT_TYPE_BINARY,
	OTAMA_VARIANT_TYPE_POINTER
} otama_variant_type_e;

typedef struct otama_variant otama_variant_t;
typedef struct otama_variant_pool otama_variant_pool_t;
	
otama_variant_pool_t *otama_variant_pool_alloc(void);
void otama_variant_pool_free(otama_variant_pool_t **pool);
otama_variant_t *otama_variant_new(otama_variant_pool_t *pool);
otama_variant_t *otama_variant_copy(otama_variant_t *dest, otama_variant_t *src, int deep);
otama_variant_t *otama_variant_dup(otama_variant_t *var, otama_variant_pool_t *pool);

void otama_variant_set_null(otama_variant_t *var);
void otama_variant_set_hash(otama_variant_t *var);
void otama_variant_set_array(otama_variant_t *var);
void otama_variant_set_int(otama_variant_t *var, int64_t iv);
void otama_variant_set_float(otama_variant_t *var, float fv);
void otama_variant_set_string(otama_variant_t *var, const char *sv);
void otama_variant_set_binary(otama_variant_t *var, const void *data, size_t data_len);
void otama_variant_set_binary_copy(otama_variant_t *var, const void *data, size_t data_len);
	
void otama_variant_set_pointer(otama_variant_t *var, const void *p);

otama_variant_t *otama_variant_array_at(otama_variant_t *array, int64_t index);
otama_variant_t *otama_variant_array_at2(otama_variant_t *array,
										 otama_variant_t *index);
int64_t otama_variant_array_count(otama_variant_t *array);

otama_variant_t *otama_variant_hash_at(otama_variant_t *hash,
									   const char  *key);
otama_variant_t *otama_variant_hash_at2(otama_variant_t *hash, otama_variant_t *key);
const char *otama_variant_hash_at3(otama_variant_t *hash, const char *key);
int otama_variant_hash_exist(otama_variant_t *hash, const char *key);
int otama_variant_hash_exist2(otama_variant_t *hash, otama_variant_t *key);
otama_variant_t *otama_variant_hash_keys(otama_variant_t *hash);
void  otama_variant_hash_remove(otama_variant_t *hash, const char *key);


#define OTAMA_VARIANT_TRUE  1
#define OTAMA_VARIANT_FALSE 0
	
int64_t otama_variant_to_int(otama_variant_t *var);
int otama_variant_to_bool(otama_variant_t *var);
float otama_variant_to_float(otama_variant_t *var);
const char *otama_variant_to_string(otama_variant_t *var);
const void *otama_variant_to_binary_ptr(otama_variant_t *var);
size_t otama_variant_to_binary_len(otama_variant_t *var);
void *otama_variant_to_pointer(otama_variant_t *var);
void otama_variant_print(FILE *out, otama_variant_t *var);

otama_variant_type_e otama_variant_type(otama_variant_t *var);
#define OTAMA_VARIANT_IS_NULL(var) (otama_variant_type(var) == OTAMA_VARIANT_TYPE_NULL)
#define OTAMA_VARIANT_IS_INT(var) (otama_variant_type(var) == OTAMA_VARIANT_TYPE_INT)
#define OTAMA_VARIANT_IS_FLOAT(var) (otama_variant_type(var) == OTAMA_VARIANT_TYPE_FLOAT)
#define OTAMA_VARIANT_IS_STRING(var) (otama_variant_type(var) == OTAMA_VARIANT_TYPE_STRING)
#define OTAMA_VARIANT_IS_ARRAY(var) (otama_variant_type(var) == OTAMA_VARIANT_TYPE_ARRAY)
#define OTAMA_VARIANT_IS_HASH(var) (otama_variant_type(var) == OTAMA_VARIANT_TYPE_HASH)
#define OTAMA_VARIANT_IS_BINARY(var) (otama_variant_type(var) == OTAMA_VARIANT_TYPE_BINARY)
#define OTAMA_VARIANT_IS_POINTER(var) (otama_variant_type(var) == OTAMA_VARIANT_TYPE_POINTER)	

#ifdef __cplusplus
}
#endif


#endif
