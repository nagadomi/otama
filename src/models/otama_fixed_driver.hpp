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
#ifndef OTAMA_FIXED_DRIVER_HPP
#define OTAMA_FIXED_DRIVER_HPP

#include "nv_ml.h"
#include "otama_id.h"
#include "otama_log.h"
#include "otama_dbi.h"
#include "otama_omp_lock.hpp"
#include "otama_fixed_strage.hpp"
#include "otama_dbi_driver.hpp"

#include <inttypes.h>
#include <vector>
#include <string>

namespace otama
{
	template<typename T>
	class FixedDriver: public DBIDriver<T>
	{
	protected:
		static const int SYNC_INTERVAL = 60;
		
		bool m_sync_before_search;
		FixedStrage<T> *m_mmap;
		
		inline std::string	header_name(void) {	return m_mmap->header_name(); }
		inline std::string index_name(void) { return m_mmap->index_name(); }
		inline std::string vector_name(void) { return m_mmap->vector_name(); }

		virtual void
		feature_copy(T *to, const T *from)
		{
			// T = struct
			*to = *from;
		}
		
		virtual otama_status_t
		try_load_local(otama_id_t *id,
					   uint64_t seq,
					   T *fixed)
		{
			if (m_mmap->try_load(id, seq, fixed) != OTAMA_STATUS_OK) {
				otama_status_t ret = this->load(id, fixed);
				if (ret != OTAMA_STATUS_OK) {
					return ret;
				}
			}
			return OTAMA_STATUS_OK;
		}

		otama_status_t
		pull_records(bool &redo, int64_t max_id)
		{
			otama_status_t ret = OTAMA_STATUS_OK;
			otama_dbi_result_t *res = NULL;
			int64_t last_no = -1;
			int64_t count;
			int64_t ntuples = 0;
			
			redo = false;
			
			sync();
			count = m_mmap->count();
			last_no = count == 0 ? -1 : m_mmap->get_last_no();
			
			// select id, otama_id, vector
			res = this->select_new_records(last_no, max_id);
			if (res == NULL) {
				return OTAMA_STATUS_SYSERROR;
			}
			
			OTAMA_LOG_DEBUG("count: %d", count);
			while (otama_dbi_result_next(res)) {
				int ng;
				int64_t seq = otama_dbi_result_int64(res, 0);
				const char *id = otama_dbi_result_string(res, 1);
				const char *vec = otama_dbi_result_string(res, 2);
				T fixed;
					
				m_mmap->extend(count + ntuples + 1);
				last_no = seq;
				ng = this->feature_deserialize(&fixed, vec);
				if (ng) {
					ret = OTAMA_STATUS_ASSERTION_FAILURE;
					OTAMA_LOG_ERROR("invalid vector size. id(%s), size(%zd), vec(%s)",
									id, strlen(vec), vec);
					break;
				}
				m_mmap->set(count + ntuples, seq, id, 0, &fixed);
				++ntuples;
			}
			if (ret == OTAMA_STATUS_OK && ntuples > 0) {
				m_mmap->set_last_no(last_no);
				m_mmap->set_count(count + ntuples);
				OTAMA_LOG_DEBUG("count2: %d", m_mmap->count());
				sync();
				OTAMA_LOG_DEBUG("count3: %d", m_mmap->count());
			}
			
			otama_dbi_result_free(&res);
			if (ntuples == DBIDriver<T>::PULL_LIMIT) {
				redo = true;
			}
			
			return ret;
		}
		
		otama_status_t
		pull_flags(int64_t max_commit_id)
		{
			int64_t last_commit_no;
			
			sync();
			last_commit_no = m_mmap->get_last_commit_no();
			
			if (m_mmap->count() != 0) {
				otama_dbi_result_t *res;
				int64_t ntuples = 0;

				// select id, flag, commit_id
				res = this->select_updated_records(last_commit_no, max_commit_id);
				if (res == NULL) {
					return OTAMA_STATUS_SYSERROR;
				}
				while (otama_dbi_result_next(res)) {
					int64_t seq = otama_dbi_result_int64(res, 0);
					uint8_t flag = (uint8_t)(otama_dbi_result_int(res, 1)) & 0xff;
					
					last_commit_no = otama_dbi_result_int64(res, 2);
					m_mmap->update_flag(seq, flag);
					++ntuples;
				}
				if (ntuples > 0) {
					m_mmap->set_last_commit_no(last_commit_no);
				}
				otama_dbi_result_free(&res);
				sync();
			}
			
			return OTAMA_STATUS_OK;
		}
		
	public:
		FixedDriver(otama_variant_t *options)
			: DBIDriver<T>(options)
		{
			otama_variant_t *driver, *value;
			m_mmap = NULL;
			m_sync_before_search = false;
			
			driver = otama_variant_hash_at(options, "driver");
			if (OTAMA_VARIANT_IS_HASH(driver)) {
				if (!OTAMA_VARIANT_IS_NULL(value = otama_variant_hash_at(driver, "sync_before_search"))) {
					m_sync_before_search = otama_variant_to_bool(value) ? true : false;
				}
			}
			OTAMA_LOG_DEBUG("driver[sync_before_search] => %d",
							m_sync_before_search ? 1:0);
		}

		virtual
		~FixedDriver()
		{
			close();
		}
		
		virtual otama_status_t
		open(void)
		{
			otama_status_t ret;
#ifdef _OPENMP
			OMPLock lock(this->m_lock);
#endif
			ret = DBIDriver<T>::open();
			if (ret != OTAMA_STATUS_OK) {
				return ret;
			}
			m_mmap = new FixedStrage<T>(this->data_dir(), this->table_name());
			if (m_mmap->open() != OTAMA_STATUS_OK) {
				if (m_mmap->create() != OTAMA_STATUS_OK) {
					return OTAMA_STATUS_SYSERROR;
				}
				if (m_mmap->open() != OTAMA_STATUS_OK) {
					return OTAMA_STATUS_SYSERROR;
				}
			}
			
			return OTAMA_STATUS_OK;
		}
		
		virtual otama_status_t
		close(void)
		{
			if (m_mmap) {
				m_mmap->close();
				delete m_mmap;
				m_mmap = NULL;
			}
			return DBIDriver<T>::close();
		}
		
		virtual bool
		is_active(void)
		{
#ifdef _OPENMP
			OMPLock lock(this->m_lock);
#endif
			if (DBIDriver<T>::is_active() && m_mmap->is_active()) {
				return true;
			}
			return false;
		}

		virtual otama_status_t
		search(otama_result_t **results, int n,
			   otama_variant_t *query)
		{
			if (m_sync_before_search) {
				otama_status_t ret = sync();
				if (ret != OTAMA_STATUS_OK) {
					return ret;
				}
			}
			return DBIDriver<T>::search(results, n, query);
		}
		
		virtual otama_status_t
		count(int64_t *count)
		{
			*count = m_mmap->count();
			return OTAMA_STATUS_OK;
		}

		virtual otama_status_t
		sync(void)
		{
			return m_mmap->sync();
		}

		virtual otama_status_t
		pull(void)
		{
			otama_status_t ret;
			bool redo = true;
			int64_t max_id, max_commit_id;
			
#ifdef _OPENMP
			OMPLock lock(this->m_lock);
#endif
			ret = this->select_max_ids(&max_id, &max_commit_id);
			if (ret != OTAMA_STATUS_OK) {
				return ret;
			}
			do {
				ret = pull_records(redo, max_id);
				if (ret != OTAMA_STATUS_OK) {
					return ret;
				}
			} while (redo);
			ret = pull_flags(max_commit_id);
			if (ret != OTAMA_STATUS_OK) {
				return ret;
			}
			
			return ret;
		}
		virtual otama_status_t
		drop_database(void)
		{
			otama_status_t ret;

#ifdef _OPENMP
			OMPLock lock(this->m_lock);
#endif
			ret = DBIDriver<T>::drop_database();
	
			if (ret == OTAMA_STATUS_OK) {
				ret = m_mmap->unlink();
			}
			return ret;
		}
		virtual otama_status_t
		drop_index(void)
		{
			otama_status_t ret;

#ifdef _OPENMP
			OMPLock lock(this->m_lock);
#endif
			ret = m_mmap->unlink();
			return ret;
		}
		virtual otama_status_t
		vacuum_index(void)
		{
			return OTAMA_STATUS_OK;
		}
	};
}

#endif

