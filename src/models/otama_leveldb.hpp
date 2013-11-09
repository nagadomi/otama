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
	template<class KEY_TYPE, class VALUE_TYPE>
	class LevelDB
	{
	protected:
		leveldb::DB *m_db;
		leveldb::Options m_opt;
		leveldb::ReadOptions m_ropt;
		leveldb::ReadOptions m_ropt_tmp;
		leveldb::WriteOptions m_wopt;
		std::string m_last_error;
		std::string m_path;
		
		void
		set_error(std::string error_message)
		{
#ifdef _OPENMP
#pragma omp critical
#endif
			m_last_error = error_message;
		}
		
	public:
		LevelDB(void)
		{
			m_db = NULL;
			m_path = "./leveldb";
			m_ropt_tmp.fill_cache = false;
			m_opt.write_buffer_size = 256 * 1048576;
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
		keep_alive_open()
		{
			return begin_writer();
		}
		
		bool
		begin_writer(void)
		{
			if (!m_db) {
				leveldb::Status ret;
				
				m_opt.block_cache = leveldb::NewLRUCache(64 * 1048576);
				ret = leveldb::DB::Open(m_opt, m_path, &m_db);
				if (!ret.ok()) {
					set_error(ret.ToString());
				}
				return ret.ok();
			}
			return true;
		}
		
		bool
		begin_reader(void)
		{
			return begin_writer();
		}
		
		bool
		sync(void)
		{
			return true;
		}
		
		void
		end(bool sync = false)
		{
		}
		
		void
		clear()
		{
			if (m_db) {
				close();
			}
			leveldb::DestroyDB(m_path, m_opt);
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

			ret = m_db->Get(m_ropt,
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
			ret = m_db->Put(m_wopt,
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
			
			assert(m_db != NULL);
			ret = m_db->Get(m_ropt_tmp, db_key, &db_value);
			if (!ret.ok() && !ret.IsNotFound()) {
				set_error(ret.ToString());
				return false;
			}
			db_value.append((const char *)value, sizeof(VALUE_TYPE) * n);
			ret = m_db->Put(m_wopt, db_key, db_value);
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
			size_t sp = 0;
			VALUE_TYPE *ret = (VALUE_TYPE*)get(key, sizeof(KEY_TYPE), &sp);
			if (ret) {
				if (sp != sizeof(VALUE_TYPE)) {
					OTAMA_LOG_ERROR("mismatch value size", 0);
					free_value(ret);
					return NULL;
				}
			}
			return ret;
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
			
			leveldb::Iterator *i = m_db->NewIterator(m_ropt);
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

