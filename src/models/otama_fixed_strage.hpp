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
#ifndef OTAMA_FIXED_STRAGE_HPP
#define OTAMA_FIXED_STRAGE_HPP

#include "nv_core.h"
#include "otama_mmap.h"
#include <string>

namespace otama
{
	template<class T>
	class FixedStrage
	{
	private:
		static const int DEFAULT_COUNT_MAX = 10000;
		
		typedef struct {
			otama_mmap_t *metadata;
			otama_mmap_t *index;
			otama_mmap_t *vec;
		} shm_t;
		typedef struct {
			int64_t count_max;
			int64_t last_no;
			int64_t last_commit_no;
			int64_t count;
		} metadata_t;
		typedef struct {
			int64_t index;
			int64_t seq;
			otama_id_t id;
			uint8_t flag;
		} index_t;
		typedef struct {
			int64_t count;
			metadata_t *metadata;
			index_t *index;
			T *vec;
		} memory_table_t;
		
		shm_t m_shm;
		memory_table_t m_memory_table;
		std::string m_metadata_name, m_index_name, m_vector_name, m_shm_dir, m_prefix;
		
		static inline int
		seq_cmp(const void *p1, const void *p2)
		{
			const index_t *h1 = (index_t *)p1;
			const index_t *h2 = (index_t *)p2;
			
			if (h1->seq > h2->seq) {
				return 1;
			} else if (h1->seq < h2->seq) {
				return -1;
			}
			return 0;
		}
		
	public:
		FixedStrage(const std::string &dir,
					const std::string &prefix = "m")
		{
			m_shm_dir = dir;
			m_prefix = prefix;
			m_metadata_name = m_prefix + "_metadata";
			m_index_name = m_prefix + "_index";
			m_vector_name = m_prefix + "_vector";
			
			memset(&m_shm, 0, sizeof(m_shm));
			memset(&m_memory_table, 0, sizeof(m_memory_table ));
		}
		
		virtual
		~FixedStrage()
		{
			close();
		}
		
		inline std::string
		metadata_name(void)
		{
			return m_metadata_name;
		}
		
		inline std::string
		index_name(void)
		{
			return m_index_name;
		}
		
		inline std::string
		vector_name(void)
		{
			return m_vector_name;
		}

		inline void
		metadata_name(const std::string &name)
		{
			m_metadata_name = name;
		}
		
		inline void
		index_name(const std::string &name)
		{
			m_index_name = name;
		}
		
		inline void
		vector_name(const std::string &name)
		{
			m_vector_name = name;
		}

		otama_status_t
		create(void)
		{
			/* TODO: global mutex */
	
			int ret;
			otama_mmap_t *metadata_shm;
			metadata_t *metadata;
			
			ret = otama_mmap_create(m_shm_dir.c_str(),
								   metadata_name().c_str(),
								   sizeof(*m_memory_table.metadata));
			if (ret != 0) {
				OTAMA_LOG_ERROR("shm_create: %s",
								metadata_name().c_str());
				return OTAMA_STATUS_SYSERROR;
			}
			ret = otama_mmap_create(m_shm_dir.c_str(),
								   index_name().c_str(),
								   sizeof(*m_memory_table.index) * DEFAULT_COUNT_MAX);
			if (ret != 0) {
				OTAMA_LOG_ERROR("shm_create: %s",
								index_name().c_str());
				return OTAMA_STATUS_SYSERROR;
			}
			ret = otama_mmap_create(m_shm_dir.c_str(),
								   vector_name().c_str(),
								   sizeof(*m_memory_table.vec) * DEFAULT_COUNT_MAX);
			if (ret != 0) {
				OTAMA_LOG_ERROR("shm_create: %s",
								vector_name().c_str());
				return OTAMA_STATUS_SYSERROR;
			}
			
			ret = otama_mmap_open(&metadata_shm,
								 m_shm_dir.c_str(), metadata_name().c_str(),
								 sizeof(metadata_t));
			if (ret != 0) {
				OTAMA_LOG_ERROR("shm_open failed: %s", metadata_name().c_str());
				return OTAMA_STATUS_SYSERROR;
			}
			metadata = (metadata_t *)otama_mmap_mem(metadata_shm);
			metadata->count_max = DEFAULT_COUNT_MAX;
			metadata->last_no = -1;
			metadata->last_commit_no = -1;
			metadata->count = 0;
	
			otama_mmap_sync(metadata_shm);
			otama_mmap_close(&metadata_shm);
	
			return OTAMA_STATUS_OK;
		}
		
		otama_status_t
		open(void)
		{
			int ret;
			
			ret = otama_mmap_open(&m_shm.metadata,
								 m_shm_dir.c_str(),
								 metadata_name().c_str(), sizeof(metadata_t));
			if (ret != 0) {
				close();
				// file not found?
				//OTAMA_LOG_NOTICE("shm_open failed: %s", metadata_name().c_str());
				return OTAMA_STATUS_SYSERROR;
			}
			m_memory_table.metadata = (metadata_t *)otama_mmap_mem(m_shm.metadata);

			if (m_memory_table.metadata->count_max == 0) {
				close();
				// windows
				//OTAMA_LOG_NOTICE("shm_open: count zero: %s", metadata_name().c_str());
				return OTAMA_STATUS_SYSERROR;
			}
	
			ret = otama_mmap_open(&m_shm.index,
								 m_shm_dir.c_str(),
								 index_name().c_str(),
								 sizeof(*m_memory_table.index) * m_memory_table.metadata->count_max);
			if (ret != 0) {
				close();				
				OTAMA_LOG_ERROR("shm_open failed: %s", index_name().c_str());
				return OTAMA_STATUS_SYSERROR;								
			}
			m_memory_table.index = (index_t *)otama_mmap_mem(m_shm.index);
	
			ret = otama_mmap_open(&m_shm.vec,
								 m_shm_dir.c_str(),
								 vector_name().c_str(),
								 sizeof(*m_memory_table.vec) * m_memory_table.metadata->count_max);
			if (ret != 0) {
				close();				
				OTAMA_LOG_ERROR("shm_open failed: %s", vector_name().c_str());
				return OTAMA_STATUS_SYSERROR;								
			}
			m_memory_table.vec = (T*)otama_mmap_mem(m_shm.vec);

			// sync count
			m_memory_table.count = m_memory_table.metadata->count;
			
			return OTAMA_STATUS_OK;
		}

		bool
		is_active(void)
		{
			return m_memory_table.metadata != NULL;
		}
		
		otama_status_t
		sync(void)
		{
			if (m_shm.metadata) {
				otama_mmap_sync(m_shm.metadata);
			}
			if (m_shm.index) {
				otama_mmap_sync(m_shm.index);
			}
			if (m_shm.vec) {
				otama_mmap_sync(m_shm.vec);
			}
			// update count
			m_memory_table.count = m_memory_table.metadata->count;
			
			if ((size_t)otama_mmap_len(m_shm.index) !=
				sizeof(*m_memory_table.index) * m_memory_table.metadata->count_max)
			{
				// extended
				OTAMA_LOG_DEBUG("detecting shm_extend: reopen: %"PRId64 ", %zd",
								otama_mmap_len(m_shm.index),
								sizeof(*m_memory_table.index) * m_memory_table.count
					);
				close();
				open();
			}
	
			return OTAMA_STATUS_OK;
		}
		
		otama_status_t
		extend(void)
		{
			int64_t count;
			int ret;

			count = m_memory_table.metadata->count_max + DEFAULT_COUNT_MAX;
			ret = otama_mmap_extend(&m_shm.index,
								   sizeof(*m_memory_table.index) * count);
			ret |= otama_mmap_extend(&m_shm.vec,
									sizeof(*m_memory_table.vec) * count);
			if (ret != 0) {
				return OTAMA_STATUS_SYSERROR;
			}
			m_memory_table.metadata->count_max = count;
			m_memory_table.index = (index_t *)otama_mmap_mem(m_shm.index);
			m_memory_table.vec = (T *)otama_mmap_mem(m_shm.vec);
			
			return sync();
		}
		
		otama_status_t
		close(void)
		{
			if (m_shm.metadata) {
				otama_mmap_close(&m_shm.metadata);
				m_memory_table.metadata = NULL;
			}
			if (m_shm.index) {
				otama_mmap_close(&m_shm.index);
				m_memory_table.index = NULL;
			}
			if (m_shm.vec) {
				otama_mmap_close(&m_shm.vec);
				m_memory_table.vec = NULL;
			}
			memset(&m_memory_table, 0, sizeof(m_memory_table));
			
			return OTAMA_STATUS_OK;
		}
		
		otama_status_t
		unlink(void)
		{
			otama_status_t ret;
			
			ret = close();
			if (ret != OTAMA_STATUS_OK) {
				return ret;
			}
			otama_mmap_unlink(m_shm_dir.c_str(), index_name().c_str());
			otama_mmap_unlink(m_shm_dir.c_str(), metadata_name().c_str());
			otama_mmap_unlink(m_shm_dir.c_str(), vector_name().c_str());

			ret = create();
			if (ret != OTAMA_STATUS_OK) {
				return ret;
			}
	
			return open();
		}

		void
		update_flag(int64_t seq, uint8_t flag)
		{
			index_t key;
			index_t *rec;

			key.seq = seq;
			rec = (index_t *)bsearch(&key, m_memory_table.index,
									 (size_t)m_memory_table.count, sizeof(index_t),
									 seq_cmp);
			if (rec != NULL) {
				rec->flag = flag;
				OTAMA_LOG_DEBUG("seq => %"PRId64" updated ", seq);				
			} else {
				OTAMA_LOG_DEBUG("seq => %"PRId64" not found", seq, flag);
			}
			
		}
		
		otama_status_t
		try_load(const otama_id_t *id,
				 uint64_t seq,
				 T *vec)
		{
			index_t key = {0};
			
			key.seq = seq;
			index_t *rec = (index_t *)
				bsearch(&key, m_memory_table.index,
						(size_t)m_memory_table.count, sizeof(index_t),
						seq_cmp);
			if (rec == NULL) {
				return OTAMA_STATUS_NODATA;
			} else {
				*vec = m_memory_table.vec[rec->index];
			}
			return OTAMA_STATUS_OK;
		}
		
		int64_t
		count(void)
		{
			return m_memory_table.count;
		}
		
		void
		set_count(int64_t count)
		{
			// not m_memory_table.count
			m_memory_table.metadata->count = count;
		}
		
		bool
		set(int64_t i,
			int64_t seq,
			const otama_id_t *id,
			uint8_t flag,
			const T *vec)
		{
			m_memory_table.index[i].index = i;
			m_memory_table.index[i].seq = seq;
			m_memory_table.index[i].flag = 0;
			m_memory_table.index[i].id = *id;
			m_memory_table.vec[i] = *vec;
			
			return true;
		}
		
		bool
		set(int64_t i,
			int64_t seq,
			const char *id,
			uint8_t flag,
			const T *vec)
		{
			m_memory_table.index[i].index = i;
			m_memory_table.index[i].seq = seq;
			m_memory_table.index[i].flag = 0;
			otama_id_hexstr2bin(&(m_memory_table.index[i].id), id);
			m_memory_table.vec[i] = *vec;
			
			return true;
		}

		int64_t
		get_last_no(void)
		{
			return m_memory_table.metadata->last_no;
		}
		
		void
		set_last_no(int64_t no)
		{
			m_memory_table.metadata->last_no = no;
		}
		
		int64_t
		get_last_commit_no(void)
		{
			return m_memory_table.metadata->last_commit_no;
		}
		
		void
		set_last_commit_no(int64_t no)
		{
			m_memory_table.metadata->last_commit_no = no;
		}

		int64_t
		count_max(void)
		{
			return m_memory_table.metadata->count_max;
		}

		otama_status_t
		extend(int64_t s)
		{
			otama_status_t ret;
			while (s >= count_max()) {
				OTAMA_LOG_DEBUG("count exceed. count_max=%"PRId64,
								count_max());
				ret = extend();
				if (ret != OTAMA_STATUS_OK) {
					return ret;
				}
			}
			return OTAMA_STATUS_OK;
		}

		inline const T
		*vec(void)
		{
			return m_memory_table.vec;
		}
		
		inline const T
		*vec_at(int64_t index)
		{
			return &m_memory_table.vec[index];
		}

		inline uint8_t
		flag_at(int64_t index)
		{
			return m_memory_table.index[index].flag;
		}
		
		inline const otama_id_t *
		id_at(int64_t index)
		{
			return &m_memory_table.index[index].id;
		}
	};
}

#endif

