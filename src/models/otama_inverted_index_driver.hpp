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
#ifndef OTAMA_INVERTED_INDEX_DRIVER_HPP
#define OTAMA_INVERTED_INDEX_DRIVER_HPP

#include "otama_dbi_driver.hpp"
#include "otama_inverted_index.hpp"
#include <inttypes.h>
#include <vector>
#include <string>
#include <algorithm>

namespace otama
{
	template<typename T, typename IV>
	class InvertedIndexDriver: public DBIDriver<T>
	{
	protected:
		IV *m_inverted_index;
		
		virtual void
		feature_to_sparse_vec(InvertedIndex::sparse_vec_t &svec,// must be sort
									const T *fixed) = 0;
		
		virtual otama_status_t
		feature_search(otama_result_t **results, int n,
					   const T *query,
					   otama_variant_t *options)
		{
			InvertedIndex::sparse_vec_t svec;
			
			feature_to_sparse_vec(svec, query);
			
			return m_inverted_index->search_cosine(results, n, svec);
			//return m_inverted_index->search_count(results, n, svec);
		}

		virtual void
		feature_copy(InvertedIndex::sparse_vec_t *to,
					 const InvertedIndex::sparse_vec_t *from)
		{
			*to = *from;
		}
		
		virtual InvertedIndex::ScoreFunction *feature_similarity_func(void) = 0;

		virtual float
		feature_similarity(const T *fixed1,
					  const T *fixed2,
					  otama_variant_t *options)
		{
			InvertedIndex::ScoreFunction *similarity_func = this->feature_similarity_func();
			InvertedIndex::sparse_vec_t svec1, svec2, intersection;
			InvertedIndex::sparse_vec_t::const_iterator it;
			float norm1 = 0.0f, norm2 = 0.0f, dot = 0.0f;
			
			feature_to_sparse_vec(svec1, fixed1);
			feature_to_sparse_vec(svec2, fixed2);
			std::set_intersection(svec1.begin(), svec1.end(),
								  svec2.begin(), svec2.end(),
								  std::back_inserter(intersection));
			
			for (it = svec1.begin(); it != svec1.end(); ++it) {
				float w = (*similarity_func)(*it);
				norm1 += w * w;
			}
			for (it = svec2.begin(); it != svec2.end(); ++it) {
				float w = (*similarity_func)(*it);				
				norm2 += w * w;
			}
			for (it = intersection.begin(); it != intersection.end(); ++it) {
				float w = (*similarity_func)(*it);
				dot += w * w;
			}
			
			return dot / (sqrtf(norm1) * sqrtf(norm2));
		}
		
		virtual otama_status_t
		try_load_local(otama_id_t *id,
					   uint64_t seq,
					   T *fixed)
		{
			return OTAMA_STATUS_NODATA;
		}
		
		otama_status_t
		pull_records(bool &redo, int64_t max_id)
		{
			otama_status_t ret = OTAMA_STATUS_OK;
			otama_dbi_result_t *res;
			int64_t last_no = -1;
			long t = nv_clock();
			long t0 = t;
			int64_t ntuples = 0;
			int i;
			InvertedIndex::batch_records_t records;
			typedef struct {
				int64_t no;
				std::string id_str;
				std::string vec_str;
			} tmp_t;
			tmp_t *tmp;
			
			redo = false;
			
			sync();
			
			last_no = m_inverted_index->get_last_no();
			
			// select id, otama_id, vector
			res = this->select_new_records(last_no, max_id);
			if (res == NULL) {
				return OTAMA_STATUS_SYSERROR;
			}
			t = nv_clock();
			
			tmp = new tmp_t[DBIDriver<T>::PULL_LIMIT];
			while (otama_dbi_result_next(res)) {
				tmp[ntuples].no = otama_dbi_result_int64(res, 0);
				tmp[ntuples].id_str = otama_dbi_result_string(res, 1);
				tmp[ntuples].vec_str = otama_dbi_result_string(res, 2);
				last_no = tmp[ntuples].no;
				++ntuples;
			}
			otama_dbi_result_free(&res);
			OTAMA_LOG_DEBUG("-- read: %dms\n", nv_clock() - t);
			t = nv_clock();
			records.resize(ntuples);
			
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic, 4)
#endif
			for (i = 0; i < (int)ntuples; ++i) {
				if (ret == OTAMA_STATUS_OK) {
					int ng;
					const char *id = tmp[i].id_str.c_str();
					const char *vec = tmp[i].vec_str.c_str();
					T fixed;
					records[i].no = tmp[i].no;
					
					otama_id_hexstr2bin(&records[i].id, id);
					ng = this->feature_deserialize(&fixed, vec);
					if (ng) {
						ret = OTAMA_STATUS_ASSERTION_FAILURE;
						OTAMA_LOG_ERROR("invalid vector string. id(%s)", id);
					} else {
						feature_to_sparse_vec(records[i].vec, &fixed);
					}
				}
			}
			delete [] tmp;
			OTAMA_LOG_DEBUG("-- parse: %dms\n", nv_clock() - t);
			t = nv_clock();
			
			if (ret == OTAMA_STATUS_OK && ntuples > 0) {
				// sequential access
				m_inverted_index->batch_set(records);
				m_inverted_index->set_last_no(last_no);
				sync();
			}
			OTAMA_LOG_DEBUG("-- append: %dms\n", nv_clock() - t);
			
			if (ntuples == DBIDriver<T>::PULL_LIMIT) {
				redo = true;
			}
			OTAMA_LOG_DEBUG("pull_records: %ldms\n",  nv_clock() - t0);
			
			return ret;
		}
		
		otama_status_t
		pull_flags(int64_t max_commit_id)
		{
			int64_t last_commit_no;
			int64_t count;
			
			sync();
			
			last_commit_no = m_inverted_index->get_last_commit_no();
			
			this->count(&count);
			if (count != 0) {
				otama_dbi_result_t *res;
				int64_t ntuples = 0;
				
				// select id, flag, commit_id
				res = this->select_updated_records(last_commit_no,
												   max_commit_id);
				if (res == NULL) {
					return OTAMA_STATUS_SYSERROR;
				}
				while (otama_dbi_result_next(res)) {
					int64_t seq = otama_dbi_result_int64(res, 0);
					uint8_t flag = (uint8_t)(otama_dbi_result_int(res, 1)) & 0xff;
					
					last_commit_no = otama_dbi_result_int64(res, 2);
					m_inverted_index->set_flag(seq, flag);
					++ntuples;
				}
				if (ntuples > 0) {
					m_inverted_index->set_last_commit_no(last_commit_no);
				}
				otama_dbi_result_free(&res);
				sync();
			}
			
			return OTAMA_STATUS_OK;
		}
		
	public:
		using DBIDriver<T>::search;
		
		InvertedIndexDriver(otama_variant_t *options)
			: DBIDriver<T>(options)
		{
			m_inverted_index = new IV(options);
		}
		
		virtual
		~InvertedIndexDriver()
		{
			close();
			delete m_inverted_index;
		}
		
		virtual otama_status_t
		open(void)
		{
#ifdef _OPENMP
			OMPLock lock(this->m_lock);
#endif
			otama_status_t ret = DBIDriver<T>::open();
			if (ret != OTAMA_STATUS_OK) {
				return ret;
			}
			m_inverted_index->similarity_func(this->feature_similarity_func());
			m_inverted_index->prefix(this->name());
			ret = m_inverted_index->open();
			
			return ret;
		}
		
		virtual otama_status_t
		close(void)
		{
#ifdef _OPENMP
			OMPLock lock(this->m_lock);
#endif
			if (m_inverted_index) {
				m_inverted_index->close();
				delete m_inverted_index;
				m_inverted_index = NULL;
			}
			return DBIDriver<T>::close();
		}
		
		virtual bool
		is_active(void)
		{
			return m_inverted_index && DBIDriver<T>::is_active();
		}
		
		virtual otama_status_t
		count(int64_t *count)
		{
			*count = m_inverted_index->count();
			return OTAMA_STATUS_OK;
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
				ret = m_inverted_index->begin_writer();
				if (ret != OTAMA_STATUS_OK) {
					m_inverted_index->end();
					return ret;
				}
				ret = pull_records(redo, max_id);
				if (ret != OTAMA_STATUS_OK) {
					m_inverted_index->end();
					return ret;
				}
				m_inverted_index->end();
			} while (redo);

			ret = m_inverted_index->begin_writer();
			if (ret != OTAMA_STATUS_OK) {
				m_inverted_index->end();
				return ret;
			}
			ret = pull_flags(max_commit_id);
			if (ret != OTAMA_STATUS_OK) {
				m_inverted_index->end();
				return ret;
			}
			m_inverted_index->update_count();
			m_inverted_index->end();
			
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
				ret = m_inverted_index->clear();
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
			ret = m_inverted_index->clear();
			return ret;
		}

		virtual otama_status_t
		vacuum_index(void)
		{
			otama_status_t ret;
#ifdef _OPENMP
			OMPLock lock(this->m_lock);
#endif
			ret = m_inverted_index->vacuum();
			return ret;
		}
		
		virtual otama_status_t
		sync(void)
		{
			otama_status_t ret = DBIDriver<T>::sync();
			if (ret != OTAMA_STATUS_OK) {
				return ret;
			}
			if (!m_inverted_index->sync()) {
				ret = OTAMA_STATUS_SYSERROR;
			}
			return ret;
		}
	};
}

#endif
