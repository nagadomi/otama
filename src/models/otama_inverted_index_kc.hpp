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
#ifndef OTAMA_INVERTED_INDEX_KC_HPP
#define OTAMA_INVERTED_INDEX_KC_HPP

#include "otama_inverted_index.hpp"
#include "otama_kyotocabinet.hpp"
#include <kcpolydb.h>
#include <kcdbext.h>
#include <kccompress.h>
#include <string>
#include <queue>
#include <algorithm>

namespace otama
{
	class InvertedIndexKC: public InvertedIndex
	{
	protected:
		std::string m_metadata_options;
		std::string m_inverted_index_options;
		bool m_keep_alive;
		
		typedef struct {
			float norm;
			uint8_t flag;
		} metadata_record_t;
		
		KyotoCabinet<kyotocabinet::PolyDB,
					 int64_t, metadata_record_t> m_metadata;
		KyotoCabinet<kyotocabinet::PolyDB,
					 int64_t, otama_id_t> m_ids;
		KyotoCabinet<kyotocabinet::IndexDB,
					 uint32_t, uint8_t> m_inverted_index;

		std::string id_file_name(void);
		std::string metadata_file_name(void);
		std::string inverted_index_file_name(void);

		otama_status_t
		set_vbc(int64_t no, const InvertedIndex::sparse_vec_t &vec);
		void decode_vbc(uint32_t hash, std::vector<int64_t> &vec);
		
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
		
		typedef std::priority_queue<similarity_result_t, std::vector<similarity_result_t> > topn_t;

		static const int COUNT_TOPN_MIN = 128;
	public:
		InvertedIndexKC(otama_variant_t *options);
		
		virtual otama_status_t open(void);
		virtual otama_status_t close(void);
		virtual otama_status_t clear(void);

		virtual otama_status_t
		search_cosine(otama_result_t **results, int n,
					  const sparse_vec_t &hash);
		
		virtual int64_t count(void);
		virtual bool sync(void);
		virtual int64_t hash_count(uint32_t hash);
		virtual otama_status_t begin_writer(void);
		virtual otama_status_t begin_reader(void);
		virtual otama_status_t end(void);
		
		/* begin_writer required */
		virtual otama_status_t set(int64_t no, const otama_id_t *id,
								   const sparse_vec_t &hash);
		virtual otama_status_t set_flag(int64_t no, uint8_t flag);
		virtual int64_t get_last_commit_no(void);
		virtual bool set_last_commit_no(int64_t no);
		virtual int64_t get_last_no(void);
		virtual bool set_last_no(int64_t no);
		virtual ~InvertedIndexKC();
	};
}

#endif
#endif
