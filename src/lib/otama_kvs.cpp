/*
 * This file is part of otama.
 *
 * Copyright (C) 201e nagadomi@nurs.or.jp
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
#include "leveldb/db.h"
#include <string>

struct otama_kvs {
	leveldb::DB *db;
	leveldb::Options opt;
	std::string path;
};

struct otama_kvs_value {
	std::string *data;
};

otama_status_t
otama_kvs_open(otama_kvs_t **kvs, const char *filename)
{
	leveldb::Status ret;
	
	*kvs = new otama_kvs_t;
	(*kvs)->path = filename;
	(*kvs)->db = 0;
	(*kvs)->opt.create_if_missing = true;
	ret = leveldb::DB::Open((*kvs)->opt, (*kvs)->path, &(*kvs)->db);
	if (!ret.ok()) {
		OTAMA_LOG_ERROR("%s: %s", (*kvs)->path.c_str(), ret.ToString().c_str());
		delete *kvs;
		*kvs = 0;
		return OTAMA_STATUS_SYSERROR;
	}
	return OTAMA_STATUS_OK;
}

void
otama_kvs_close(otama_kvs_t **kvs)
{
	if (kvs && *kvs) {
		delete (*kvs)->db;
		(*kvs)->db = 0;
		delete *kvs;
		*kvs = 0;
	}
}

otama_status_t
otama_kvs_get(otama_kvs_t *kvs,
			  const void *key_ptr, size_t key_len,
			  otama_kvs_value_t *value)
{
	leveldb::Status ret;
	ret = kvs->db->Get(leveldb::ReadOptions(),
					   leveldb::Slice((const char *)key_ptr,
									  key_len),
					   value->data);
	if (!ret.ok()) {
		value->data->clear();
		if (ret.IsNotFound()) {
			return OTAMA_STATUS_NODATA;
		}
		OTAMA_LOG_ERROR("%s", ret.ToString().c_str());
		return OTAMA_STATUS_SYSERROR;
	}
	return OTAMA_STATUS_OK;
}

otama_status_t
otama_kvs_set(otama_kvs_t *kvs,
			  const void *key_ptr, size_t key_len,
			  const void *value_ptr, size_t value_len)
{
	leveldb::Status ret;
	ret = kvs->db->Put(leveldb::WriteOptions(),
					   leveldb::Slice((const char *)key_ptr,
									  key_len),
					   leveldb::Slice((const char *)value_ptr,
									  value_len));
	if (!ret.ok()) {
		OTAMA_LOG_ERROR("%s", ret.ToString().c_str());
		return OTAMA_STATUS_SYSERROR;
	}
	return OTAMA_STATUS_OK;
}

otama_status_t
otama_kvs_delete(otama_kvs_t *kvs,
				 const void *key_ptr, size_t key_len)
{
	leveldb::Status ret;
	ret = kvs->db->Delete(leveldb::WriteOptions(),
						  leveldb::Slice((const char *)key_ptr,
										 key_len));
	if (!ret.ok()) {
		if (ret.IsNotFound()) {
			return OTAMA_STATUS_NODATA;
		}
		OTAMA_LOG_ERROR("%s", ret.ToString().c_str());
		return OTAMA_STATUS_SYSERROR;
	}
	return OTAMA_STATUS_OK;
}

otama_status_t
otama_kvs_clear(otama_kvs_t *kvs)
{
	leveldb::Status ret;
	
	delete kvs->db;
	kvs->db = 0;
	ret = leveldb::DestroyDB(kvs->path, kvs->opt);
	if (!ret.ok()) {
		OTAMA_LOG_ERROR("%s", ret.ToString().c_str());
		leveldb::DB::Open(kvs->opt, kvs->path, &kvs->db);
		return OTAMA_STATUS_SYSERROR;
	}
	ret = leveldb::DB::Open(kvs->opt, kvs->path, &kvs->db);
	if (!ret.ok()) {
		OTAMA_LOG_ERROR("%s", ret.ToString().c_str());
		return OTAMA_STATUS_SYSERROR;
	}
	
	return OTAMA_STATUS_OK;
}

otama_status_t
otama_kvs_vacuum(otama_kvs_t *kvs)
{
	kvs->db->CompactRange(NULL, NULL);
	return OTAMA_STATUS_OK;
}

otama_status_t
otama_kvs_each_pair(otama_kvs_t *kvs,
					otama_kvs_each_pair_f func,
					void *user_data)
{
	leveldb::ReadOptions ropt;
	leveldb::Iterator *i;
	
	ropt.snapshot = kvs->db->GetSnapshot();
	i = kvs->db->NewIterator(ropt);
	for (i->SeekToFirst(); i->Valid(); i->Next()) {
		leveldb::Slice key = i->key();
		leveldb::Slice value = i->value();
		if ((*func)(user_data,
					key.data(), key.size(),
					value.data(), value.size()) != 0)
		{
			break;
		}
	}
	delete i;
	kvs->db->ReleaseSnapshot(ropt.snapshot);
	
	return OTAMA_STATUS_OK;
}

otama_status_t
otama_kvs_each_key(otama_kvs_t *kvs,
					otama_kvs_each_key_f func,
					void *user_data)
{
	leveldb::ReadOptions ropt;
	leveldb::Iterator *i;
	
	ropt.snapshot = kvs->db->GetSnapshot();
	i = kvs->db->NewIterator(ropt);
	for (i->SeekToFirst(); i->Valid(); i->Next()) {
		leveldb::Slice key = i->key();
		if ((*func)(user_data, key.data(), key.size())) {
			break;
		}
	}
	delete i;
	kvs->db->ReleaseSnapshot(ropt.snapshot);
	
	return OTAMA_STATUS_OK;
}

otama_status_t
otama_kvs_each_value(otama_kvs_t *kvs,
					otama_kvs_each_value_f func,
					void *user_data)
{
	leveldb::ReadOptions ropt;
	leveldb::Iterator *i;
	
	ropt.snapshot = kvs->db->GetSnapshot();
	i = kvs->db->NewIterator(ropt);
	for (i->SeekToFirst(); i->Valid(); i->Next()) {
		leveldb::Slice value = i->value();
		if ((*func)(user_data, value.data(), value.size())) {
			break;
		}
	}
	delete i;
	kvs->db->ReleaseSnapshot(ropt.snapshot);
	
	return OTAMA_STATUS_OK;
}

otama_kvs_value_t *otama_kvs_value_alloc(void)
{
	otama_kvs_value_t *value = new otama_kvs_value_t;
	value->data = new std::string;
	return value;
}

const void *
otama_kvs_value_ptr(otama_kvs_value_t *value)
{
	return value->data->data();
}

size_t
otama_kvs_value_size(otama_kvs_value_t *value)
{
	return value->data->size();
}

void
otama_kvs_value_free(otama_kvs_value_t **value)
{
	if (value && *value) {
		delete (*value)->data;
		(*value)->data = 0;
		delete (*value);
		*value = 0;
	}
}

#endif

