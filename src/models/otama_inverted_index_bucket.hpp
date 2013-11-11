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
#ifndef OTAMA_INVERTED_INDEX_BUCKET_HPP
#define OTAMA_INVERTED_INDEX_BUCKET_HPP

#if OTAMA_WITH_UNORDERED_MAP
#  include <unordered_map>
#elsif OTAMA_WITH_UNORDERED_MAP_TR1
#  include <tr1/unordered_map>
#else
#  include <map>
#endif

#include "nv_core.h"
#include "otama_variable_byte_code_vector.hpp"
#include "otama_inverted_index.hpp"
#include <string>
#include <queue>
#include <algorithm>

namespace otama
{
	class InvertedIndexBucket: public InvertedIndex
	{
	protected:
#ifdef _OPENMP
		omp_nest_lock_t m_lock;
#endif
		typedef struct {
			otama_id_t id;
			float norm;
			uint8_t flag;
		} metadata_record_t;
		
#if OTAMA_WITH_UNORDERED_MAP
		typedef std::unordered_map<int64_t, metadata_record_t> metadata_t;
#elsif OTAMA_WITH_UNORDERED_MAP_TR1
		typedef std::tr1::unordered_map<int64_t, metadata_record_t> metadata_t;
#else
		typedef std::map<int64_t, metadata_record_t> metadata_t;
#endif
		typedef std::vector<VariableByteCodeVector> inverted_index_t;
		
		metadata_t m_metadata;
		inverted_index_t m_inverted_index;
		int64_t m_last_commit_no;
		int64_t m_last_no;

		typedef struct similarity_result {
			otama_id_t id;
			float similarity;
			
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
		
		typedef std::priority_queue<similarity_result_t, std::vector<similarity_result_t> > topn_t;
		
	public:
		InvertedIndexBucket(otama_variant_t *options);
		
		virtual otama_status_t open(void);
		virtual otama_status_t close(void);
		virtual otama_status_t clear(void);
		virtual otama_status_t vacuum(void);

		virtual otama_status_t
		search_cosine(otama_result_t **results, int n,
					  const sparse_vec_t &hash);

		virtual int64_t hash_count(uint32_t hash);
		virtual int64_t count(void);
		virtual otama_status_t begin_writer(void);
		virtual otama_status_t begin_reader(void);
		virtual otama_status_t end(void);
		
		/* begin_writer required */
		virtual otama_status_t set(int64_t no, const otama_id_t *id,
								   const InvertedIndex::sparse_vec_t &hash);
		virtual otama_status_t set_flag(int64_t no, uint8_t flag);
		virtual int64_t get_last_commit_no(void);
		virtual bool set_last_commit_no(int64_t no);
		virtual int64_t get_last_no(void);
		virtual bool set_last_no(int64_t no);
		virtual bool update_count(void);
		virtual bool sync(void);
		virtual ~InvertedIndexBucket();

		virtual void reserve(size_t hash_max);
	};
}

#endif
