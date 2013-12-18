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
#include "leveldb/c.h"
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
		leveldb_t *m_db;
		leveldb_options_t *m_opt;
		leveldb_readoptions_t *m_ropt;
		leveldb_writeoptions_t *m_wopt;
		leveldb_cache_t *m_cache;
		std::string m_last_error;
		std::string m_path;
		
		void
		set_error(const char *error_message)
		{
#ifdef _OPENMP
#pragma omp critical (otama_leveldb_set_error)
#endif
			{
				m_last_error = error_message;
			}
		}
		
	public:
		LevelDB(void)
		{
			m_db = NULL;
			m_opt = NULL;
			m_path = "./leveldb.ldb";
			m_ropt = leveldb_readoptions_create();
			m_wopt = leveldb_writeoptions_create();
			m_cache = NULL;

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
				char *errptr = NULL;

				if (m_opt) {
					leveldb_options_destroy(m_opt);
				}
				m_opt = leveldb_options_create();
				if (WRITE_CACHE_SIZE > 0) {
					leveldb_options_set_write_buffer_size(m_opt, WRITE_CACHE_SIZE);
				}
				leveldb_options_set_create_if_missing(m_opt, 1);
				
				if (READ_CACHE_SIZE > 0) {
					if (m_cache) {
						leveldb_cache_destroy(m_cache);
					}
					m_cache = leveldb_cache_create_lru(READ_CACHE_SIZE);
					leveldb_options_set_cache(m_opt, m_cache);
				}
				m_db = leveldb_open(m_opt, m_path.c_str(), &errptr);
				if (m_db == NULL) {
					set_error(errptr);
					leveldb_free(errptr);
					return false;
				}
				return true;
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
			char *errptr = NULL;
			leveldb_options_t *opt = leveldb_options_create();
			leveldb_options_set_create_if_missing(opt, 1);
			
			if (m_db) {
				reopen = true;
				close();
			}
			leveldb_destroy_db(opt, m_path.c_str(), &errptr);
			if (errptr != NULL) {
				set_error(errptr);
				leveldb_free(errptr);
			}
			leveldb_options_destroy(opt);
			if (reopen) {
				open();
			}
		}
		
		bool
		vacuum()
		{
			assert(m_db != NULL);
			leveldb_compact_range(m_db, NULL, 0, NULL, 0);
			return true;
		}
		
		void
		close()
		{
			if (m_db) {
				leveldb_close(m_db);
				m_db = NULL;
			}
			if (m_cache) {
				leveldb_cache_destroy(m_cache);
				m_cache = NULL;
			}
			if (m_opt) {
				leveldb_options_destroy(m_opt);
				m_opt = NULL;
			}
		}
		
		~LevelDB()
		{
			close();
			leveldb_readoptions_destroy(m_ropt);
			leveldb_writeoptions_destroy(m_wopt);
		}
		
		inline void *
		get(const void *key, size_t key_len, size_t *sp)
		{
			assert(m_db != NULL);
			char *errptr = NULL;
			char *value = leveldb_get(m_db, m_ropt, 
									  (const char *)key,
									  key_len, sp, &errptr);
			if (value == NULL) {
				if (errptr != NULL) {
					set_error(errptr);
					leveldb_free(errptr);
				}
				return NULL;
			}
			return value;
		}
		
		inline bool
		set(const void *key, size_t key_len,
			const void *value, size_t value_len)
		{
			assert(m_db != NULL);
			char *errptr = NULL;
			
			leveldb_put(m_db, m_wopt,
						(const char *)key, key_len,
						(const char *)value, value_len,
						&errptr);
			if (errptr != NULL) {
				set_error(errptr);
				leveldb_free(errptr);
				return false;
			}
			return true;
		}
		
		inline bool
		set_sync(const void *key, size_t key_len,
				 const void *value, size_t value_len)
		{
			assert(m_db != NULL);
			char *errptr = NULL;
			leveldb_writeoptions_t *wopt = leveldb_writeoptions_create();
			
			leveldb_writeoptions_set_sync(wopt, 1);
			leveldb_put(m_db, m_wopt,
						(const char *)key, key_len,
						(const char *)value, value_len,
						&errptr);
			leveldb_writeoptions_destroy(wopt);
			if (errptr != NULL) {
				set_error(errptr);
				leveldb_free(errptr);
				return false;
			}
			return true;
		}
		
		inline bool
		append(const KEY_TYPE *key, const VALUE_TYPE *value, size_t n)
		{
			assert(m_db != NULL);
			char *errptr = NULL;
			char *new_value;
			char *db_value;
			size_t len = 0;
			size_t new_len;
			bool ret;
			
			leveldb_readoptions_t *ropt = leveldb_readoptions_create();

			leveldb_readoptions_set_fill_cache(ropt, 0);
			db_value = leveldb_get(m_db, m_ropt, 
								   (const char *)key,
								   sizeof(KEY_TYPE), &len, &errptr);
			if (value == NULL) {
				if (errptr != NULL) {
					set_error(errptr);
					leveldb_free(errptr);
					leveldb_readoptions_destroy(ropt);
					return false;
				}
			}
			new_len = len + sizeof(VALUE_TYPE) * n;
			new_value = nv_alloc_type(char, new_len);
			if (len > 0) {
				memcpy(new_value, db_value, len);
			}
			memcpy(new_value + len, value, sizeof(VALUE_TYPE) * n);

			if (db_value != NULL) {
				leveldb_free(db_value);
			}
			leveldb_readoptions_destroy(ropt);
			ret = set(key, sizeof(KEY_TYPE), new_value, new_len);
			nv_free(new_value);
			
			return ret;
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
			return set(key, key_len, value, value_len);
		}
		
		inline VALUE_TYPE *
		get(const KEY_TYPE *key)
		{
			size_t sp;
			return (VALUE_TYPE *)get(key, sizeof(KEY_TYPE), &sp);
		}
		
		void free_value(void *p)
		{
			leveldb_free(p);
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
			free_value(count_ptr);
			
			return val;
		}
		
		inline bool
		update_count(void)
		{
			assert(m_db != NULL);
			int64_t old_count = count();
			int64_t new_count = 0;
			const leveldb_snapshot_t *snap = leveldb_create_snapshot(m_db);
			leveldb_iterator_t *iter = leveldb_create_iterator(m_db, m_ropt);
			
			for (leveldb_iter_seek_to_first(iter);
				 leveldb_iter_valid(iter);
				 leveldb_iter_next(iter))
			{
				++new_count;
			}
			leveldb_iter_destroy(iter);
			leveldb_release_snapshot(m_db, snap);
			
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
#pragma omp critical (otama_leveldb_set_error)
#endif
			ret = m_last_error;
			
			return ret;
		}
	};
}

#endif
#endif

