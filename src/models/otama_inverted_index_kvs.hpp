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
#ifndef OTAMA_INVERTED_INDEX_KVS_HPP
#define OTAMA_INVERTED_INDEX_KVS_HPP

#include "nv_core.h"
#include "nv_num.h"
#include "otama_log.h"
#include "otama_dbi.h"
#include "otama_omp_lock.hpp"
#include "otama_inverted_index.hpp"
#include <string>
#include <queue>
#include <iterator>
#include <vector>
#include <map>
#include <algorithm>
#include <inttypes.h>

namespace otama
{
	template <typename MT, typename IT, typename VT>
	class InvertedIndexKVS :public InvertedIndex
	{
	private:
	protected:
		static const int COUNT_TOPN_MIN = 128;
		
		MT m_metadata;
		IT m_ids;
		VT m_inverted_index;
		bool m_keep_alive;
		bool m_auto_repair;
		
		typedef struct similarity_result {
			int64_t no;
			float similarity;
			int count;

			inline bool
			operator<(const struct similarity_result &rhs) const
			{
				return similarity > rhs.similarity ? true : false;
			}
		} similarity_result_t;
		typedef struct similarity_temp {
			int64_t no;
			float w;
			inline bool
			operator <(const struct similarity_temp &rhs) const
			{
				return no < rhs.no;
			}
		} similarity_temp_t;
		
		typedef std::map<uint32_t, std::vector<uint8_t> > index_buffer_t;
		typedef std::map<uint32_t, int64_t > last_no_buffer_t;
		typedef std::priority_queue<similarity_result_t, std::vector<similarity_result_t> > topn_t;
		
		virtual std::string id_file_name(void) = 0;
		virtual std::string metadata_file_name(void) = 0;
		virtual std::string inverted_index_file_name(void) = 0;
		
		otama_status_t
		set_vbc(int64_t no, const sparse_vec_t &vec)
		{
			sparse_vec_t::const_iterator i;
			otama_status_t ret = OTAMA_STATUS_OK;

			for (i = vec.begin(); i != vec.end(); ++i) {
				uint32_t hash = *i;
				const uint64_t last_no_key = (uint64_t)hash << 32;
				size_t sp = 0;
				int64_t *last_no_v = (int64_t *)m_inverted_index.get(&last_no_key,
																	 sizeof(last_no_key), &sp);
				bool bret;
				int64_t last_no = 0;
				uint64_t a;
				std::vector<uint8_t> append_value;
				
				if (last_no_v) {
					last_no = *last_no_v;
					m_inverted_index.free_value_raw(last_no_v);
				}
		
				NV_ASSERT(last_no < no);
		
				a = no - last_no;
				while (a) {
					uint8_t v = (a & 0x7f);
					a >>= 7;
					if (a) {
						v |= 0x80U;
					}
					append_value.push_back(v);
				}
				bret = m_inverted_index.append(&hash, append_value.data(), append_value.size());
				if (!bret) {
					OTAMA_LOG_ERROR("%s", m_inverted_index.error_message().c_str());
					ret = OTAMA_STATUS_SYSERROR;
					break;
				}
				bret = m_inverted_index.replace(&last_no_key, sizeof(last_no_key),
												&no, sizeof(no));
				if (!bret) {
					OTAMA_LOG_ERROR("%s", m_inverted_index.error_message().c_str());
					ret = OTAMA_STATUS_SYSERROR;
					break;
				}
			}
	
			return ret;

		}
		void decode_vbc(uint32_t hash, std::vector<int64_t> &vec)
		{
			static const int s_t[8] = { 0, 7, 14, 21, 28, 35, 42, 49 };
			size_t sp;
			uint8_t *vs = (uint8_t *)m_inverted_index.get(&hash, sizeof(hash), &sp);
	
			vec.clear();
	
			if (vs != NULL) {
				int64_t a = 0;
				int64_t last_no = 0;
				int j = 0;
				size_t i;
				const size_t n = sp / sizeof(uint8_t);
		
				for (i = 0; i < n; ++i) {
					const uint8_t v = vs[i];
					if ((v & 0x80) != 0) {
						a |= ((int64_t)(v & 0x7f) << s_t[j]);
						++j;
					} else {
						int64_t no = last_no + (((int64_t)v << s_t[j]) | a);
						vec.push_back(no);
						last_no = no;
						j = 0;
						a = 0;
					}
				}
			}
			m_inverted_index.free_value(vs);
		}
		
		void
		init_index_buffer(index_buffer_t &index_buffer,
						  last_no_buffer_t &last_no_buffer,
						  const batch_records_t &records)
		{
			batch_records_t::const_iterator i;
			
			index_buffer.clear();
			last_no_buffer.clear();
			
			for (i = records.begin(); i != records.end(); ++i) {
				sparse_vec_t::const_iterator j;
				for (j = i->vec.begin(); j != i->vec.end(); ++j) {
					uint32_t hash = *j;
					last_no_buffer_t::const_iterator no = last_no_buffer.find(hash);
					if (no == last_no_buffer.end()) {
						const uint64_t last_no_key = (uint64_t)hash << 32;
						size_t sp = 0;
						int64_t *last_no_v = (int64_t *)m_inverted_index.get(&last_no_key,
																			 sizeof(last_no_key), &sp);
						int64_t last_no;
						
						if (last_no_v) {
							last_no = *last_no_v;
							m_inverted_index.free_value_raw(last_no_v);
						} else {
							last_no = 0;
						}
						
						std::vector<uint8_t> empty_rec;
						last_no_buffer.insert(
							std::make_pair<uint32_t, int64_t>(
								hash,
								last_no));
						index_buffer.insert(
							std::make_pair<uint32_t, std::vector<uint8_t> >(
								hash,
								empty_rec));
					}
				}
			}
		}
		
		void
		set_index_buffer(index_buffer_t &index_buffer,
						 last_no_buffer_t &last_no_buffer,
						 const batch_records_t &records)
		{
			batch_records_t::const_iterator j;

			for (j = records.begin(); j != records.end(); ++j) {
				sparse_vec_t::const_iterator i;
				int64_t no = j->no;
				const sparse_vec_t &vec = j->vec;
				
				for (i = vec.begin(); i != vec.end(); ++i) {
					uint32_t hash = *i;
					uint64_t a;
					index_buffer_t::iterator rec = index_buffer.find(hash);
					last_no_buffer_t::iterator last_no = last_no_buffer.find(hash);

					NV_ASSERT(rec != index_buffer.end());
					NV_ASSERT(last_no != last_no_buffer.end());
					NV_ASSERT(last_no->second < no);
					
					a = no - last_no->second;
					while (a) {
						uint8_t v = (a & 0x7f);
						a >>= 7;
						if (a) {
							v |= 0x80U;
						}
						rec->second.push_back(v);
					}
					last_no->second = no;
				}
			}
		}

		otama_status_t
		write_index_buffer(const index_buffer_t &index_buffer,
						   const last_no_buffer_t &last_no_buffer,
						   const batch_records_t &records)
		{
			otama_status_t ret = OTAMA_STATUS_OK;
			batch_records_t::const_iterator j;
			index_buffer_t::const_iterator i;
			bool bret;
			int8_t verify_index_value = 0;
			
			if (!m_metadata.set_sync("_VERIFY_INDEX", 13,
									 &verify_index_value,
									 sizeof(verify_index_value))) 
			{
				return OTAMA_STATUS_SYSERROR;
			}
			OTAMA_LOG_DEBUG("begin index write", 0);
			
			for (i = index_buffer.begin(); i != index_buffer.end(); ++i) {
				uint32_t hash = i->first;
				const uint64_t last_no_key = (uint64_t)hash << 32;
				
				last_no_buffer_t::const_iterator last_no = last_no_buffer.find(hash);
				
				NV_ASSERT(last_no != last_no_buffer.end());
				
				bret = m_inverted_index.append(&hash,
											   i->second.data(),
											   i->second.size());
				if (!bret) {
					OTAMA_LOG_ERROR("%s", m_inverted_index.error_message().c_str());
					ret = OTAMA_STATUS_SYSERROR;
					break;
				}
				bret = m_inverted_index.replace(&last_no_key,
												sizeof(last_no_key),
												&last_no->second,
												sizeof(&last_no->second));
				if (!bret) {
					OTAMA_LOG_ERROR("%s", m_inverted_index.error_message().c_str());
					ret = OTAMA_STATUS_SYSERROR;
					break;
				}
			}
			for (j = records.begin(); j != records.end(); ++j) {
				bool bret;
				metadata_record_t rec;
				
				memset(&rec, 0, sizeof(rec));
				rec.norm = norm(j->vec);
				rec.flag = 0;
				
				bret = m_ids.add(&j->no, &j->id);
				if (!bret) {
					OTAMA_LOG_ERROR("%s", m_ids.error_message().c_str());
					ret = OTAMA_STATUS_SYSERROR;
					break;
				}
				m_metadata.add(&j->no, &rec);
			}
			if (ret == OTAMA_STATUS_OK) {
				verify_index_value = 1;
				if (!m_metadata.set_sync("_VERIFY_INDEX", 13,
										 &verify_index_value,
										 sizeof(verify_index_value))) 
				{
					ret = OTAMA_STATUS_SYSERROR;
				}
			}
			m_inverted_index.sync();
			m_ids.sync();
			m_metadata.sync();
			
			OTAMA_LOG_DEBUG("end index write", 0);
			
			return ret;
		}

		bool
		verify_index(void)
		{
			size_t sp = 0;
			int8_t *flag = (int8_t *)m_metadata.get("_VERIFY_INDEX", 13, &sp);
			if (flag) {
				bool ret = false;
				if (sp == sizeof(int8_t) && *flag == 1) {
					ret = true;
					OTAMA_LOG_DEBUG("verify_index 1", 0);
				} else {
					OTAMA_LOG_DEBUG("verify_index 0", 0);
				}
				m_metadata.free_value_raw(flag);
				return ret;
			} else {
				OTAMA_LOG_DEBUG("verify_index null", 0);
				return true;
			}
		}
		
	public:
		InvertedIndexKVS(otama_variant_t *options)
		: InvertedIndex(options)
		{
			otama_variant_t *driver, *value;
			
			m_keep_alive = true;
			m_auto_repair = true;
			
			driver = otama_variant_hash_at(options, "driver");
			if (OTAMA_VARIANT_IS_HASH(driver)) {
				if (!OTAMA_VARIANT_IS_NULL(value = otama_variant_hash_at(driver,
																		 "keep_alive")))
				{
					m_keep_alive = otama_variant_to_bool(value);
				}
			}
		}
		
		virtual otama_status_t
		open(void)
		{
			otama_status_t ret = OTAMA_STATUS_OK;
			
			m_metadata.path(metadata_file_name());
			m_inverted_index.path(inverted_index_file_name());
			m_ids.path(id_file_name());

			if (m_keep_alive) {
				if (!m_inverted_index.keep_alive_open()) {
					OTAMA_LOG_ERROR("%s: %s\n", m_inverted_index.path().c_str(),
									m_inverted_index.error_message().c_str());
					return OTAMA_STATUS_SYSERROR;
				}
				if (!m_metadata.keep_alive_open()) {
					OTAMA_LOG_ERROR("%s: %s\n", m_metadata.path().c_str(),
									m_metadata.error_message().c_str());
					return OTAMA_STATUS_SYSERROR;
				}
				if (!m_ids.keep_alive_open()) {
					OTAMA_LOG_ERROR("%s: %s\n", m_ids.path().c_str(),
									m_ids.error_message().c_str());
					return OTAMA_STATUS_SYSERROR;
				}
				if (!verify_index()) {
					OTAMA_LOG_NOTICE("indexes are corrupted. try to clear index..", 0);
					m_inverted_index.clear();
					m_metadata.clear();
					m_ids.clear();
				}
				if (!setup()) {
					ret = OTAMA_STATUS_SYSERROR;
				}
			} else {
				ret = begin_writer();
				if (ret != OTAMA_STATUS_OK) {
					return ret;
				}
				if (!verify_index()) {
					OTAMA_LOG_NOTICE("indexes are corrupted. try to clear index..", 0);
					m_inverted_index.clear();
					m_metadata.clear();
					m_ids.clear();
				}
				if (!setup()) {
					ret = OTAMA_STATUS_SYSERROR;
				}
				end();
			}
	
			return ret;

		}
		
		virtual otama_status_t
		close(void)
		{
			m_inverted_index.close();
			m_metadata.close();
			m_ids.close();
			
			return OTAMA_STATUS_OK;
		}
		
		virtual otama_status_t
		clear(void)
		{
			otama_status_t ret;
			
			ret = begin_writer();
			if (ret != OTAMA_STATUS_OK) {
				return ret;
			}
			m_inverted_index.clear();
			m_metadata.clear();
			m_ids.clear();
			
			end();
			
			return ret;
		}

		virtual otama_status_t
		vacuum(void)
		{
			otama_status_t ret;
			
			ret = begin_writer();
			if (ret != OTAMA_STATUS_OK) {
				return ret;
			}
			if (!m_ids.vacuum()) {
				end();
				return OTAMA_STATUS_SYSERROR;
			}
			if (!m_metadata.vacuum()) {
				end();
				return OTAMA_STATUS_SYSERROR;
			}
			if (!m_inverted_index.vacuum()) {
				end();
				return OTAMA_STATUS_SYSERROR;
			}
			end();
			
			return OTAMA_STATUS_OK;
		}

		virtual otama_status_t
		search_cosine(otama_result_t **results, int n,
					  const sparse_vec_t &vec)
		{
			int l, result_max, i;
			long t;
			int num_threads  = nv_omp_procs();
			std::vector<similarity_temp_t> *hits;
			size_t c;
			otama_status_t ret;
			topn_t topn;

			if (num_threads > 2) {
				num_threads -= 1; // reservation for other threads
			}
			
			if (n < 1) {
				return OTAMA_STATUS_INVALID_ARGUMENTS;
			}
	
			ret = begin_reader();
			if (ret != OTAMA_STATUS_OK) {
				return ret;
			}
	
			hits = new std::vector<similarity_temp_t>[num_threads];
			t = nv_clock();
			c = count();
			hits[0].reserve(1 + c * 3);
			for (i = 1; i < num_threads; ++i) {
				hits[i].reserve(1 + (c * 3 / num_threads));
			}
	
#ifdef _OPENMP
#pragma omp parallel for num_threads(num_threads) schedule(dynamic, 4)
#endif
			for (i = 0; i < (int)vec.size(); ++i) {
				int thread_id = nv_omp_thread_id();
				std::vector<similarity_temp_t> &hit = hits[thread_id];
				uint32_t h = vec[i];
				std::vector<int64_t> v;
				int j;
		
				decode_vbc(h, v);
				for (j = 0; j < (int)v.size(); ++j) {
					similarity_temp_t hi;
					hi.no = v[j];
					hi.w = (*m_similarity_func)(h);
					hi.w *= hi.w;
					hit.push_back(hi);
				}
			}
			for (i = 1; i < num_threads; ++i) {
				std::copy(hits[i].begin(), hits[i].end(), std::back_inserter(hits[0]));
			}
			
			OTAMA_LOG_DEBUG("search: inverted index search: %zd, %ldms",
							hits[0].size(), nv_clock() - t);
			t = nv_clock();
	
			std::sort(hits[0].begin(), hits[0].end());
			OTAMA_LOG_DEBUG("search: sort:  %ldms", nv_clock() - t);
			t = nv_clock();
	
			if (hits[0].size() > 0) {
				int64_t no = hits[0][0].no;
				float w = 0.0f;
				float query_norm = norm(vec);
				int count = 0;
		
				for (typename std::vector<similarity_temp_t>::iterator j = hits[0].begin();
					 j != hits[0].end(); ++j)
				{
					if (j->no == no) {
						w += j->w;
						++count;
					} else {
						if (count > m_hit_threshold) {
							metadata_record_t *rec = m_metadata.get(&no);
							if (rec == NULL) {
								OTAMA_LOG_ERROR("indexes are corrupted. try to clear index.. please rebuild index using otama_pull.", 0);
								delete [] hits;
								end();
								clear();
								return OTAMA_STATUS_SYSERROR;
							}
							if ((rec->flag & FLAG_DELETE) == 0) {
								float similarity = w / (query_norm * rec->norm);
								if (n > (int)topn.size()) {
									similarity_result_t t;
									t.no = no;
									t.similarity = similarity;
									topn.push(t);
								} else if (topn.top().similarity < similarity) {
									similarity_result_t t;
									t.no = no;
									t.similarity = similarity;
									topn.push(t);
									topn.pop();
								}
							}
							m_metadata.free_value(rec);
						}
						no = j->no;
						w = j->w;
						count = 1;
					}
				}
				if (count > m_hit_threshold) {
					metadata_record_t *rec = m_metadata.get(&no);
					if (rec == NULL) {
						OTAMA_LOG_ERROR("indexes are corrupted. try to clear index.. please rebuild index using otama_pull.", 0);
						delete [] hits;
						end();
						clear();
						return OTAMA_STATUS_SYSERROR;
					}
					
					if ((rec->flag & FLAG_DELETE) == 0) {
						float similarity = w / (query_norm * rec->norm);
						if (n > (int)topn.size()) {
							similarity_result_t t;
							t.no = no;
							t.similarity = similarity;
							topn.push(t);
						} else if (topn.top().similarity < similarity) {
							similarity_result_t t;
							t.no = no;
							t.similarity = similarity;
							topn.push(t);
							topn.pop();
						}
					}
					m_metadata.free_value(rec);
				}
			}
	
			*results = otama_result_alloc(n);
			result_max = NV_MIN(n, (int)topn.size());
	
			for (l = result_max - 1; l >= 0; --l) {
				const similarity_result_t &p = topn.top();
				otama_id_t *id = m_ids.get(&p.no);
		
				set_result(*results, l, id, p.similarity);
				m_ids.free_value(id);
				topn.pop();
			}
			otama_result_set_count(*results, result_max);
	
			end();
	
			OTAMA_LOG_DEBUG("search: ranking: %ldms", nv_clock() - t);

			delete [] hits;	
	
			return OTAMA_STATUS_OK;

		}
		
		virtual int64_t count(void)
		{
			int64_t count;
			bool b = false;
	
			if (!m_ids.is_active()) {
				b = true;
				if (!m_ids.begin_reader()) {
					OTAMA_LOG_ERROR("%s: %s\n", m_ids.path().c_str(),
									m_ids.error_message().c_str());
					m_ids.end();
					return -1;
				}
			}
			count = m_ids.count();
			if (b) {
				m_ids.end();
			}
	
			if (count < 0) {
				count = 0;
			}
	
			return count;
		}
		virtual bool sync(void)
		{
			if (m_metadata.is_active()) {
				m_metadata.sync();
			}
			if (m_inverted_index.is_active()) {
				m_inverted_index.sync();
			}
			if (m_ids.is_active()) {
				m_ids.sync();
			}
			return true;
		}
		virtual bool update_count(void)
		{
			bool b = false;
			bool ret;
			
			if (!m_ids.is_active()) {
				b = true;
				if (!m_ids.begin_writer()) {
					OTAMA_LOG_ERROR("%s: %s\n", m_ids.path().c_str(),
									m_ids.error_message().c_str());
					m_ids.end();
					return 0;
				}
			}
			ret = m_ids.update_count();
			if (!ret) {
				OTAMA_LOG_ERROR("%s: %s\n", m_ids.path().c_str(),
								m_ids.error_message().c_str());
				m_ids.end();
				return 0;
			}
			if (b) {
				m_ids.end();
			}
			return true;
		}
		virtual int64_t hash_count(uint32_t hash)
		{
			bool b = false;
	
			if (!m_inverted_index.is_active()) {
				b = true;
				if (!m_inverted_index.begin_reader()) {
					OTAMA_LOG_ERROR("%s: %s\n", m_inverted_index.path().c_str(),
									m_inverted_index.error_message().c_str());
					m_inverted_index.end();
					return 0;
				}
			}
			std::vector<int64_t> nos;
			decode_vbc(hash, nos);
	
			if (b) {
				m_inverted_index.end();
			}
	
			return (int64_t)nos.size();
		}
		
		virtual otama_status_t begin_writer(void)
		{
			if (!m_ids.begin_writer()) {
				OTAMA_LOG_ERROR("%s: %s\n", m_ids.path().c_str(),
								m_ids.error_message().c_str());
				return OTAMA_STATUS_SYSERROR;
			}
			if (!m_metadata.begin_writer()) {
				OTAMA_LOG_ERROR("%s: %s\n", m_metadata.path().c_str(),
								m_metadata.error_message().c_str());
				m_ids.end();
				return OTAMA_STATUS_SYSERROR;
			}
			if (!m_inverted_index.begin_writer()) {
				OTAMA_LOG_ERROR("%s: %s\n", m_inverted_index.path().c_str(),
								m_inverted_index.error_message().c_str());
				m_ids.end();
				m_metadata.end();
		
				return OTAMA_STATUS_SYSERROR;
			}
	
			return OTAMA_STATUS_OK;

		}
		virtual otama_status_t begin_reader(void)
		{
			if (!m_ids.begin_reader()) {
				OTAMA_LOG_ERROR("%s: %s\n", m_ids.path().c_str(),
								m_ids.error_message().c_str());
				return OTAMA_STATUS_SYSERROR;
			}
			if (!m_metadata.begin_reader()) {
				OTAMA_LOG_ERROR("%s: %s\n", m_metadata.path().c_str(),
								m_metadata.error_message().c_str());
				m_ids.end();
				return OTAMA_STATUS_SYSERROR;
			}
			if (!m_inverted_index.begin_reader()) {
				OTAMA_LOG_ERROR("%s: %s\n", m_inverted_index.path().c_str(),
								m_inverted_index.error_message().c_str());
				m_ids.end();
				m_metadata.end();
		
				return OTAMA_STATUS_SYSERROR;
			}
	
			return OTAMA_STATUS_OK;

		}
		virtual otama_status_t end(void)
		{
			m_ids.end();
			m_metadata.end();
			m_inverted_index.end();
			
			return OTAMA_STATUS_OK;
		}
		
		/* begin_writer required */
		virtual otama_status_t set(int64_t no, const otama_id_t *id,
								   const sparse_vec_t &vec)
		{
			otama_status_t ret = OTAMA_STATUS_OK;
			bool bret;
			metadata_record_t rec;
	
			memset(&rec, 0, sizeof(rec));
			rec.norm = norm(vec);
			rec.flag = 0;
	
			bret = m_ids.add(&no, id);
			if (bret) {
				m_metadata.add(&no, &rec);
				set_vbc(no, vec);
			} else {
				OTAMA_LOG_ERROR("%s", m_ids.error_message().c_str());
				ret = OTAMA_STATUS_SYSERROR;
			}
	
			return ret;
		}
		
		virtual otama_status_t
		batch_set(const batch_records_t records)
		{
			batch_records_t::const_iterator i;
			otama_status_t ret = OTAMA_STATUS_OK;
			index_buffer_t index_buffer;
			last_no_buffer_t last_no_buffer;

			init_index_buffer(index_buffer, last_no_buffer, records);
			set_index_buffer(index_buffer, last_no_buffer, records);
			ret = write_index_buffer(index_buffer, last_no_buffer, records);
			
			return ret;
		}
		
		virtual otama_status_t set_flag(int64_t no, uint8_t flag)
		{
			metadata_record_t *rec;
			otama_status_t ret = OTAMA_STATUS_OK;
			bool bret;
			
			rec = m_metadata.get(&no);
			if (rec) {
				rec->flag = flag;
				bret = m_metadata.set(&no, rec);
				if (!bret) {
					OTAMA_LOG_ERROR("%s", m_metadata.error_message().c_str());
					ret = OTAMA_STATUS_SYSERROR;
				}
				m_metadata.free_value(rec);
			} else {
				OTAMA_LOG_ERROR("record not found(%"PRId64")", no);
				ret = OTAMA_STATUS_NODATA;
			}
	
			return ret;

		}
		virtual int64_t get_last_commit_no(void)
		{
			size_t sp;
			int64_t last_modified = 0;
			int64_t *p = (int64_t *)m_metadata.get("_LAST_COMMIT_NO", 15, &sp);
			if (p) {
				if (sp == sizeof(last_modified)) {
					last_modified = *p;
				}
				m_metadata.free_value_raw(p);
			} else {
				last_modified = -1;
			}
	
			return last_modified;
		}
		virtual bool set_last_commit_no(int64_t no)
		{
			bool ret = m_metadata.set("_LAST_COMMIT_NO", 15, &no, sizeof(no));
			if (!ret) {
				OTAMA_LOG_ERROR("%s: %s\n", m_metadata.path().c_str(),
								m_metadata.error_message().c_str());
			}
			return ret;
		}
		virtual int64_t get_last_no(void)
		{
			size_t sp;
			int64_t no;
			int64_t *p = (int64_t *)m_metadata.get("_LAST_NO", 8, &sp);
			if (p) {
				no = *p;
				m_metadata.free_value_raw(p);
			} else {
				no = -1;
			}
	
			return no;
		}
		virtual bool set_last_no(int64_t no)
		{
			bool ret;

			ret = m_metadata.set("_LAST_NO", 8, &no, sizeof(no));
			if (!ret) {
				OTAMA_LOG_ERROR("%s: %s\n", m_metadata.path().c_str(),
								m_metadata.error_message().c_str());
			}
			return ret;
		}
		virtual ~InvertedIndexKVS()
		{
			close();
		}
	};
}

#endif
