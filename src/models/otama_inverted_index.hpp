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
#ifndef OTAMA_INVERTED_INDEX_HPP
#define OTAMA_INVERTED_INDEX_HPP

#include "otama_variant.h"
#include "otama_result.h"
#include <string>
#include <vector>
#include <functional>

namespace otama
{
	class InvertedIndex
	{
	public:
		class ScoreFunction {
		public:
			virtual float operator ()(uint32_t x)
			{
				return 1.0f;
			}
		};
		typedef std::vector<uint32_t> sparse_vec_t;
		typedef struct {
			float norm;
			uint8_t flag;
		} metadata_record_t;
		typedef struct {
			int64_t no;
			otama_id_t id;
			InvertedIndex::sparse_vec_t vec;
		} batch_record_t;
		typedef std::vector<batch_record_t> batch_records_t;
		
	protected:
		static const int HIT_THRESHOLD = 8;
		static const uint8_t FLAG_DELETE = 0x01;
		std::string m_data_dir;
		std::string m_prefix;
		int m_hit_threshold;
		ScoreFunction *m_similarity_func;
		
		static inline void
		set_result(otama_result_t *results, int i,
				   const otama_id_t *id, float similarity)
		{
			otama_variant_t *hash = otama_result_value(results, i);
			otama_variant_t *c;

			otama_variant_set_hash(hash);
			c = otama_variant_hash_at(hash, "similarity");
			otama_variant_set_float(c, similarity);
			otama_result_set_id(results, i, id);
		}
		
		inline float
		norm(const sparse_vec_t &vec)
		{
			float dot = 0.0f;
			sparse_vec_t::const_iterator i;
			for (i = vec.begin(); i != vec.end(); ++i) {
				float w = (*m_similarity_func)(*i);
				dot += w * w;
			}
			return sqrtf(dot);
		}
		
		virtual bool
		setup(void)
		{
			int64_t no = get_last_no();
			int64_t commit_no = get_last_commit_no();
			bool ret;
			
			ret = set_last_no(no);
			if (ret) {
				ret = set_last_commit_no(commit_no);
			}
			
			return ret;
		}
		
	public:
		InvertedIndex(otama_variant_t *options)
		{
			otama_variant_t *driver, *value;
			
			m_data_dir = ".";
			m_hit_threshold = HIT_THRESHOLD;
			
			driver = otama_variant_hash_at(options, "driver");
			if (OTAMA_VARIANT_IS_HASH(driver)) {
				if (!OTAMA_VARIANT_IS_NULL(value = otama_variant_hash_at(driver, "data_dir")))
				{
					m_data_dir = otama_variant_to_string(value);
				}
				if (!OTAMA_VARIANT_IS_NULL(value = otama_variant_hash_at(driver,
																		 "hit_threshold")))
				{
					m_hit_threshold = (int)otama_variant_to_int(value);
					if (m_hit_threshold < 1) {
						m_hit_threshold = 1;
					}
				}
			}
		}
		void similarity_func(ScoreFunction *func) { m_similarity_func = func; }
		void prefix(const std::string &prefix) { m_prefix = prefix; }
		
		virtual otama_status_t open(void) = 0;
		virtual otama_status_t close(void) = 0;
		virtual otama_status_t clear(void) = 0;
		virtual otama_status_t vacuum(void) = 0;
		
		typedef enum {
			METHOD_COSINE,
			METHOD_COUNT
		} search_method_e;
		
		virtual otama_status_t
		search_cosine(otama_result_t **results, int n,
					  const sparse_vec_t &vec) = 0;
		
		virtual int64_t hash_count(uint32_t hash) = 0;
		virtual int64_t count(void) = 0;
		virtual otama_status_t begin_writer(void) = 0;
		virtual otama_status_t begin_reader(void) = 0;
		virtual otama_status_t end(void) = 0;
		
		/* begin_writer required */
		virtual otama_status_t set(int64_t no, const otama_id_t *id,
								   const sparse_vec_t &hash) = 0;
		virtual otama_status_t
		batch_set(const batch_records_t records)
		{
			batch_records_t::const_iterator i;
			otama_status_t ret;
			for (i = records.begin(); i != records.end(); ++i) {
				ret = set(i->no, &i->id, i->vec);
				if (ret != OTAMA_STATUS_OK) {
					return ret;
				}
			}
			return OTAMA_STATUS_OK;
		}
		virtual otama_status_t set_flag(int64_t no, uint8_t flag) = 0;
		virtual int64_t get_last_commit_no(void) = 0;
		virtual bool set_last_commit_no(int64_t no) = 0;
		virtual int64_t get_last_no(void) = 0;
		virtual bool set_last_no(int64_t no) = 0;
		virtual bool sync(void) = 0;
		virtual bool update_count(void) = 0;
		virtual void reserve(size_t hash_max) {/* do nothing*/};
		virtual ~InvertedIndex() {};
		
	};
}

#endif
