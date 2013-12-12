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
#include "otama_config.h"
#if OTAMA_WITH_LEVELDB
#ifndef OTAMA_KVS_H
#define OTAMA_KVS_H
#include "otama_config.h"
#include "otama_status.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct otama_kvs otama_kvs_t;
typedef struct otama_kvs_value otama_kvs_value_t;
typedef int (*otama_kvs_each_pair_f)(void *user_data,
									 const void *key_ptr, size_t key_len,
									 const void *value_ptr, size_t value_len);
typedef int (*otama_kvs_each_key_f)(void *user_data,
									const void *key_ptr, size_t key_len);
typedef int (*otama_kvs_each_value_f)(void *user_data,
									  const void *value_ptr, size_t value_len);

otama_status_t otama_kvs_open(otama_kvs_t **kvs, const char *filename);
void otama_kvs_close(otama_kvs_t **kvs);
otama_status_t otama_kvs_get(otama_kvs_t *kvs,
							 const void *key_ptr, size_t key_len,
							 otama_kvs_value_t *value);
otama_status_t otama_kvs_set(otama_kvs_t *kvs,
							 const void *key_ptr, size_t key_len,
							 const void *value_ptr, size_t value_len);
otama_status_t otama_kvs_delete(otama_kvs_t *kvs,
								const void *key_ptr, size_t key_len);
otama_status_t otama_kvs_clear(otama_kvs_t *kvs);
otama_status_t otama_kvs_vacuum(otama_kvs_t *kvs);

otama_status_t otama_kvs_each_pair(otama_kvs_t *kvs,
								   otama_kvs_each_pair_f func,
								   void *user_data);
otama_status_t otama_kvs_each_key(otama_kvs_t *kvs,
								  otama_kvs_each_key_f func,
								  void *user_data);
otama_status_t otama_kvs_each_value(otama_kvs_t *kvs,
									otama_kvs_each_value_f func,
									void *user_data);

const void *otama_kvs_value_ptr(otama_kvs_value_t *value);
size_t otama_kvs_value_size(otama_kvs_value_t *value);
otama_kvs_value_t *otama_kvs_value_alloc(void);
void otama_kvs_value_free(otama_kvs_value_t **value);

#ifdef __cplusplus
}
#endif

#endif
#endif
