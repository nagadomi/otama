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
#include "otama_status.h"
#include "otama_log.h"
#include "otama_kvs.h"
#include "leveldb/c.h"
#include "nv_core.h"

struct otama_kvs {
	leveldb_t *db;
	leveldb_options_t *opt;
	leveldb_readoptions_t *ropt;
	leveldb_writeoptions_t *wopt;
	char path[8192];
};

otama_status_t
otama_kvs_open(otama_kvs_t **kvs, const char *filename)
{
	char *errptr = NULL;
	
	*kvs = nv_alloc_type(otama_kvs_t, 1);
	strncpy((*kvs)->path, filename, sizeof((*kvs)->path));
	(*kvs)->opt = leveldb_options_create();
	leveldb_options_set_create_if_missing((*kvs)->opt, 1);
	(*kvs)->ropt = leveldb_readoptions_create();
	(*kvs)->wopt = leveldb_writeoptions_create();
	
	(*kvs)->db = leveldb_open((*kvs)->opt, (*kvs)->path, &errptr);
	if ((*kvs)->db == NULL) {
		OTAMA_LOG_ERROR("%s: %s", (*kvs)->path, errptr);
		nv_free(*kvs);
		leveldb_free(errptr);
		*kvs = NULL;
		return OTAMA_STATUS_SYSERROR;
	}
	return OTAMA_STATUS_OK;
}

void
otama_kvs_close(otama_kvs_t **kvs)
{
	if (kvs && *kvs) {
		leveldb_close((*kvs)->db);
		leveldb_options_destroy((*kvs)->opt);
		leveldb_readoptions_destroy((*kvs)->ropt);
		leveldb_writeoptions_destroy((*kvs)->wopt);
		(*kvs)->db = NULL;
		(*kvs)->opt = NULL;
		(*kvs)->ropt = NULL;
		(*kvs)->wopt = NULL;
		nv_free(*kvs);
		*kvs = NULL;
	}
}

otama_status_t
otama_kvs_get(otama_kvs_t *kvs,
			  const void *key_ptr, size_t key_len,
			  otama_kvs_value_t *value)
{
	char *errptr = NULL;
	
	otama_kvs_value_clear(value);
	value->data = leveldb_get(kvs->db, kvs->ropt,
							  (const char *)key_ptr, key_len,
							  &value->len, &errptr);
	if (value->data == NULL) {
		if (errptr == NULL) {
			return OTAMA_STATUS_NODATA;
		}
		OTAMA_LOG_ERROR("%s", errptr);
		leveldb_free(errptr);
		return OTAMA_STATUS_SYSERROR;
	}
	return OTAMA_STATUS_OK;
}

otama_status_t
otama_kvs_set(otama_kvs_t *kvs,
			  const void *key_ptr, size_t key_len,
			  const void *value_ptr, size_t value_len)
{
	char *errptr = NULL;
	
	leveldb_put(kvs->db, kvs->wopt,
				(const char *)key_ptr, key_len,
				(const char *)value_ptr, value_len,
				&errptr);
	if (errptr != NULL) {
		OTAMA_LOG_ERROR("%s", errptr);
		leveldb_free(errptr);
		return OTAMA_STATUS_SYSERROR;
	}
	return OTAMA_STATUS_OK;
}

otama_status_t
otama_kvs_delete(otama_kvs_t *kvs,
				 const void *key_ptr, size_t key_len)
{
	char *errptr = NULL;

	leveldb_delete(kvs->db, kvs->wopt,
				   (const char *)key_ptr, key_len, &errptr);
	if (errptr != NULL) {
		OTAMA_LOG_ERROR("%s", errptr);
		leveldb_free(errptr);
		return OTAMA_STATUS_SYSERROR;
	}
	return OTAMA_STATUS_OK;
}

otama_status_t
otama_kvs_clear(otama_kvs_t *kvs)
{
	char *errptr = NULL;
	if (kvs->db) {
		leveldb_close(kvs->db);
		kvs->db = NULL;
	}
	leveldb_destroy_db(kvs->opt, kvs->path, &errptr);
	
	if (errptr != NULL) {
		OTAMA_LOG_ERROR("%s", errptr);
		leveldb_free(errptr);
		kvs->db = leveldb_open(kvs->opt, kvs->path, &errptr);
		return OTAMA_STATUS_SYSERROR;
	}
	kvs->db = leveldb_open(kvs->opt, kvs->path, &errptr);
	if (kvs->db == NULL) {
		OTAMA_LOG_ERROR("%s", errptr);
		leveldb_free(errptr);
		return OTAMA_STATUS_SYSERROR;
	}
	
	return OTAMA_STATUS_OK;
}

otama_status_t
otama_kvs_vacuum(otama_kvs_t *kvs)
{
	leveldb_compact_range(kvs->db, NULL, 0, NULL, 0);
	return OTAMA_STATUS_OK;
}

otama_status_t
otama_kvs_each_pair(otama_kvs_t *kvs,
					otama_kvs_each_pair_f func,
					void *user_data)
{
	const leveldb_snapshot_t *snap = leveldb_create_snapshot(kvs->db);
	leveldb_iterator_t *iter = leveldb_create_iterator(kvs->db, kvs->ropt);
	
	for (leveldb_iter_seek_to_first(iter);
		 leveldb_iter_valid(iter);
		 leveldb_iter_next(iter))
	{
		size_t key_len, value_len;
		const char *key_ptr = leveldb_iter_key(iter, &key_len);
		const char *value_ptr = leveldb_iter_value(iter, &value_len);
		
		if ((*func)(user_data,
					key_ptr, key_len,
					value_ptr, value_len) != 0)
		{
			break;
		}
	}
	leveldb_iter_destroy(iter);
	leveldb_release_snapshot(kvs->db, snap);
	
	return OTAMA_STATUS_OK;
}

otama_status_t
otama_kvs_each_key(otama_kvs_t *kvs,
					otama_kvs_each_key_f func,
					void *user_data)
{
	const leveldb_snapshot_t *snap = leveldb_create_snapshot(kvs->db);
	leveldb_iterator_t *iter = leveldb_create_iterator(kvs->db, kvs->ropt);
	
	for (leveldb_iter_seek_to_first(iter);
		 leveldb_iter_valid(iter);
		 leveldb_iter_next(iter))
	{
		size_t key_len;
		const char *key_ptr = leveldb_iter_key(iter, &key_len);
		
		if ((*func)(user_data, key_ptr, key_len) != 0) {
			break;
		}
	}
	leveldb_iter_destroy(iter);
	leveldb_release_snapshot(kvs->db, snap);
	
	return OTAMA_STATUS_OK;
}

otama_status_t
otama_kvs_each_value(otama_kvs_t *kvs,
					otama_kvs_each_value_f func,
					void *user_data)
{
	const leveldb_snapshot_t *snap = leveldb_create_snapshot(kvs->db);
	leveldb_iterator_t *iter = leveldb_create_iterator(kvs->db, kvs->ropt);
	
	for (leveldb_iter_seek_to_first(iter);
		 leveldb_iter_valid(iter);
		 leveldb_iter_next(iter))
	{
		size_t value_len;
		const char *value_ptr = leveldb_iter_value(iter, &value_len);
		
		if ((*func)(user_data, value_ptr, value_len) != 0) {
			break;
		}
	}
	leveldb_iter_destroy(iter);
	leveldb_release_snapshot(kvs->db, snap);
	
	return OTAMA_STATUS_OK;
}

otama_kvs_value_t *otama_kvs_value_alloc(void)
{
	otama_kvs_value_t *value = nv_alloc_type(otama_kvs_value_t, 1);
	value->data = NULL;
	value->len = 0;
	return value;
}

void
otama_kvs_value_clear(otama_kvs_value_t *value)
{
	if (value->data) {
		leveldb_free(value->data);
		value->data = NULL;
		value->len = 0;
	}
}

void
otama_kvs_value_free(otama_kvs_value_t **value)
{
	if (value && *value) {
		otama_kvs_value_clear(*value);
		nv_free(*value);
		*value = 0;
	}
}

#endif

