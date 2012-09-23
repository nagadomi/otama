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
#if OTAMA_WITH_KC
#ifndef OTAMA_KYOTO_CABINET_HPP
#define OTAMA_KYOTO_CABINET_HPP
#include <sys/types.h>
#include <kcpolydb.h>
#include <string>
#include <inttypes.h>
#include "otama_log.h"

namespace otama
{
	template<class DB_TYPE, class KEY_TYPE, class VALUE_TYPE>
	class KyotoCabinet
	{
	protected:
		DB_TYPE *m_db;
		std::string m_path;
		uint32_t m_mode;
		bool m_keep_alive;
		
		bool
		begin(uint32_t mode)
		{
			bool ret = true;
			if (!m_keep_alive) {
				assert(m_db == NULL);
				m_db = new DB_TYPE;
				ret = m_db->open(m_path, mode);
			} else {
			}
			return ret;
		}
		
	public:
		KyotoCabinet(void)
		{
			m_db = NULL;
			m_path = ":";
			m_keep_alive = false;
			m_mode = 0;
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
		begin_reader(void)
		{
			return begin(kyotocabinet::BasicDB::OREADER);
		}
		
		bool
		begin_writer(void)
		{
			return begin(kyotocabinet::BasicDB::OWRITER
						 | kyotocabinet::BasicDB::OCREATE);
		}

		bool
		sync(void)
		{
			return m_db->synchronize();
		}
		
		void
		end(bool sync = false)
		{
			if (!m_keep_alive) {
				assert(m_db != NULL);
				close();
			} else {
				if (sync) {
					m_db->synchronize();
				}
			}
		}
		
		bool
		keep_alive_open(uint32_t mode)
		{
			bool ret = true;

			if (!m_keep_alive) {
				m_keep_alive = true;
				m_db = new DB_TYPE;
				ret = m_db->open(m_path, mode);
				m_mode = mode;
			}
			return ret;
		}

		void clear()
		{
			assert(m_db != NULL);
			m_db->clear();
		}

		void close()
		{
			if (m_db) {
				m_db->close();
				delete m_db;
				m_db = NULL;
			}
		}
		
		~KyotoCabinet()
		{
			close();
		}

		inline bool
		append(const KEY_TYPE *key, const VALUE_TYPE *value)
		{
			assert(m_db != NULL);			
			return m_db->append((const char *)key, sizeof(KEY_TYPE),
								(const char *)value, sizeof(VALUE_TYPE));
		}
		
		inline bool
		replace(const void *key, size_t key_len, const void *value, size_t value_len)
		{
			assert(m_db != NULL);
			m_db->remove((const char *)key, key_len);
			return m_db->set((const char *)key, key_len, (const char *)value, value_len);
		}
		
		inline bool
		set(const KEY_TYPE *key, const VALUE_TYPE *value)
		{
			assert(m_db != NULL);
			return m_db->set((const char *)key, sizeof(KEY_TYPE),
							 (const char *)value, sizeof(VALUE_TYPE));
		}

		inline bool
		set(const void *key, size_t key_len, const void *value, size_t value_len)
		{
			assert(m_db != NULL);			
			return m_db->set((const char *)key, key_len,
							 (const char *)value, value_len);
		}
		
		inline bool
		add(const KEY_TYPE *key, const VALUE_TYPE *value)
		{
			assert(m_db != NULL);			
			return m_db->add((const char *)key, sizeof(KEY_TYPE),
							 (const char *)value, sizeof(VALUE_TYPE));
		}

		inline bool
		add(const void *key, size_t key_len, const void *value, size_t value_len)
		{
			assert(m_db != NULL);			
			return m_db->add((const char *)key, key_len,
							 (const char *)value, value_len);
		}

		inline VALUE_TYPE *
		get(const KEY_TYPE *key)
		{
			assert(m_db != NULL);			
			size_t sp = 0;
			VALUE_TYPE *v = (VALUE_TYPE *)m_db->get((const char *)key,
													sizeof(KEY_TYPE), &sp);
			if (v != NULL && sp != sizeof(VALUE_TYPE)) {
				assert("missmatch size" == NULL);
				delete v;
				return NULL;
			}
			return v;
		}

		inline void *
		get(const void *key, size_t key_len, size_t *sp)
		{
			assert(m_db != NULL);			
			return (void *)m_db->get((const char *)key, key_len, sp);
		}

		template<typename T>
		void free_value(T *p)
		{
			delete [] (char *)p;
		}

		void free_value(VALUE_TYPE *p)
		{
			delete [] p;
		}

		inline int64_t
		count(void)
		{
			assert(m_db != NULL);			
			return m_db->count();
		}
		
		std::string
		error_message(void)
		{
			if (!m_db) {
				return "null";
			}
			return m_db->error().name();
		}
	};
}

#endif
#endif

