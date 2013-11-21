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
#ifndef OTAMA_LEVELDB_HPP
#define OTAMA_LEVELDB_HPP
#include <sys/types.h>
#include <string>
#include <inttypes.h>
#include "leveldb/db.h"
#include "leveldb/cache.h"
#include "leveldb/comparator.h"
#include "leveldb/write_batch.h"
#include "otama_log.h"

namespace otama
{
	template<class KEY_TYPE, class VALUE_TYPE,
			 size_t READ_CACHE_SIZE,
			 size_t WRITE_CACHE_SIZE>
	class LevelDB
	{
	protected:
		leveldb::DB *m_db;
		leveldb::Options m_opt;
		std::string m_last_error;
		std::string m_path;
		
		void
		set_error(std::string error_message)
		{
#ifdef _OPENMP
#pragma omp critical
#endif
			{
				m_last_error = error_message;
			}
		}
		
	public:
		LevelDB(void)
		{
			m_db = NULL;
			m_path = "./leveldb";
			if (WRITE_CACHE_SIZE > 0) {
				m_opt.write_buffer_size = WRITE_CACHE_SIZE;
			}
			m_opt.create_if_missing = true;
			m_opt.block_cache = NULL;
		}

		bool is_active(void)
		{
			return m_db != NULL;
		}
		
		void
		path(std::string path)
		{
			m_path = path;
		}
		
		std::string
		path(void)
		{
			return m_path;
		}
		
		bool
		open()
		{
			if (!m_db) {
				leveldb::Status ret;
				if (READ_CACHE_SIZE > 0) {
					m_opt.block_cache = leveldb::NewLRUCache(READ_CACHE_SIZE);
				}
				ret = leveldb::DB::Open(m_opt, m_path, &m_db);
				if (!ret.ok()) {
					set_error(ret.ToString());
				}
				return ret.ok();
			}
			return true;
		}
		
		bool
		sync(void)
		{
			// not implemented..
			return true;
		}
		
		void
		clear()
		{
			bool reopen = false;
			leveldb::Status ret;
			
			if (m_db) {
				reopen = true;
				close();
			}
			ret = leveldb::DestroyDB(m_path, m_opt);
			if (!ret.ok()) {
				set_error(ret.ToString());
			}
			if (reopen) {
				open();
			}
		}
		
		bool
		vacuum()
		{
			assert(m_db != NULL);
			m_db->CompactRange(NULL, NULL);
			return true;
		}
		
		void
		close()
		{
			if (m_db) {
				delete m_db;
				m_db = NULL;
				if (m_opt.block_cache) {
					delete m_opt.block_cache;
					m_opt.block_cache = NULL;
				}
			}
		}
		
		~LevelDB()
		{
			close();
		}

		inline void *
		get(const void *key, size_t key_len, size_t *sp)
		{
			assert(m_db != NULL);
			std::string db_value;
			leveldb::Status ret;
			char *value;

			ret = m_db->Get(leveldb::ReadOptions(),
							leveldb::Slice((const char *)key,
										   key_len),
							&db_value);
			if (!ret.ok()) {
				set_error(ret.ToString());
				return NULL;
			}
			value = new char[db_value.size()];
			*sp = db_value.size();
			memcpy(value, db_value.data(), *sp);
			
			return value;
		}
		
		inline bool
		set(const void *key, size_t key_len,
			const void *value, size_t value_len)
		{
			assert(m_db != NULL);
			leveldb::Status ret;
			ret = m_db->Put(leveldb::WriteOptions(),
							leveldb::Slice((const char *)key, key_len),
							leveldb::Slice((const char *)value, value_len));
			if (!ret.ok()) {
				set_error(ret.ToString());
			}
			return ret.ok();
		}
		
		inline bool
		set_sync(const void *key, size_t key_len,
				 const void *value, size_t value_len)
		{
			assert(m_db != NULL);
			leveldb::Status ret;
			leveldb::WriteOptions opt;
			
			opt.sync = true;
			ret = m_db->Put(opt,
							leveldb::Slice((const char *)key, key_len),
							leveldb::Slice((const char *)value, value_len));
			if (!ret.ok()) {
				set_error(ret.ToString());
			}
			return ret.ok();
		}
		
		inline bool
		append(const KEY_TYPE *key, const VALUE_TYPE *value, size_t n)
		{
			assert(m_db != NULL);
			std::string db_value;
			leveldb::Slice db_key((const char *)key, sizeof(KEY_TYPE));
			leveldb::Status ret;
			leveldb::ReadOptions opt;
			
			assert(m_db != NULL);
			opt.fill_cache = false;
			ret = m_db->Get(opt, db_key, &db_value);
			if (!ret.ok() && !ret.IsNotFound()) {
				set_error(ret.ToString());
				return false;
			}
			db_value.append((const char *)value, sizeof(VALUE_TYPE) * n);
			ret = m_db->Put(leveldb::WriteOptions(), db_key, db_value);
			if (!ret.ok()) {
				set_error(ret.ToString());
			}
			return ret.ok();
		}
		
		inline bool
		set(const KEY_TYPE *key, const VALUE_TYPE *value)
		{
			return set(key, sizeof(KEY_TYPE), value, sizeof(VALUE_TYPE));
		}			
		
		inline bool
		append(const KEY_TYPE *key, const VALUE_TYPE *value)
		{
			return append(key, value, 1);
		}
		
		
		inline bool
		replace(const void *key, size_t key_len, const void *value, size_t value_len)
		{
			return set(key, key_len, value, value_len);
		}
		
		inline bool
		add(const KEY_TYPE *key, const VALUE_TYPE *value)
		{
			return set(key, value);
		}

		inline bool
		add(const void *key, size_t key_len, const void *value, size_t value_len)
		{
			return set(key, key_len,
					   value, value_len);
		}
		
		inline VALUE_TYPE *
		get(const KEY_TYPE *key)
		{
			assert(m_db != NULL);
			std::string db_value;
			leveldb::Status ret;
			char *value;

			ret = m_db->Get(leveldb::ReadOptions(),
							leveldb::Slice((const char *)key,
										   sizeof(KEY_TYPE)),
							&db_value);
			if (!ret.ok()) {
				set_error(ret.ToString());
				return NULL;
			}
			value = new char[db_value.size()];
			memcpy(value, db_value.data(), db_value.size());
			
			return (VALUE_TYPE *)value;
		}
		
		void free_value_raw(void *p)
		{
			delete [] (char *)p;
		}
		
		void free_value(VALUE_TYPE *p)
		{
			free_value_raw(p);
		}

		inline int64_t
		count(void)
		{
			assert(m_db != NULL);
			size_t sp = 0;
			int64_t *count_ptr = (int64_t *)get("_count", 6, &sp);
			int64_t val;
			
			if (!count_ptr || sp != sizeof(int64_t)) {
				return 0;
			}
			val = *count_ptr;
			free_value_raw(count_ptr);
			
			return val;
		}
		
		inline bool
		update_count(void)
		{
			assert(m_db != NULL);
			int64_t old_count = count();
			int64_t new_count = 0;
			
			leveldb::Iterator *i = m_db->NewIterator(leveldb::ReadOptions());
			for (i->SeekToFirst(); i->Valid(); i->Next()) {
				++new_count;
			}
			delete i;
			if (old_count > 0) {
				new_count -= 1;
			}
			return set("_count", 6, &new_count, sizeof(new_count));
		}
		
		std::string
		error_message(void)
		{
			std::string ret;
#ifdef _OPENMP
#pragma omp critical
#endif
			ret = m_last_error;
			
			return ret;
		}
	};
}

#endif
#endif

