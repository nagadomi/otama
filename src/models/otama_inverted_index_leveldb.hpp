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
#ifndef OTAMA_INVERTED_INDEX_LEVELDB_HPP
#define OTAMA_INVERTED_INDEX_LEVELDB_HPP

#include "otama_inverted_index.hpp"
#include "otama_leveldb.hpp"
#include <string>
#include <queue>
#include <iterator>
#include <vector>
#include <map>
#include <algorithm>
#include <inttypes.h>

namespace otama
{
	class InvertedIndexLevelDB :public InvertedIndex
	{
	protected:
		static const int COUNT_TOPN_MIN = 128;
		
		LevelDB<int64_t, InvertedIndex::metadata_record_t, 16 * 1048576, 0> m_metadata;
		LevelDB<int64_t, otama_id_t, 16 * 1048576, 0> m_ids;
		LevelDB<uint32_t, uint8_t, 0, 64 * 1048576> m_inverted_index;
		
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
		
		std::string id_file_name(void);
		std::string metadata_file_name(void);
		std::string inverted_index_file_name(void);
		otama_status_t set_vbc(int64_t no, const sparse_vec_t &vec);
		void decode_vbc(uint32_t hash, std::vector<int64_t> &vec);
		void init_index_buffer(index_buffer_t &index_buffer,
							   last_no_buffer_t &last_no_buffer,
							   const batch_records_t &records);
		void set_index_buffer(index_buffer_t &index_buffer,
							  last_no_buffer_t &last_no_buffer,
							  const batch_records_t &records);
		
		otama_status_t write_index_buffer(const index_buffer_t &index_buffer,
										  const last_no_buffer_t &last_no_buffer,
										  const batch_records_t &records);

		bool verify_index(void);

	public:
		InvertedIndexLevelDB(otama_variant_t *options);
		virtual otama_status_t open(void);
		virtual otama_status_t close(void);
		virtual otama_status_t clear(void);
		virtual otama_status_t vacuum(void);

		virtual otama_status_t search_cosine(otama_result_t **results, int n,
											 const sparse_vec_t &vec);

		virtual int64_t count(void);
		virtual bool sync(void);
		virtual bool update_count(void);
		virtual int64_t hash_count(uint32_t hash);
		virtual otama_status_t set(int64_t no, const otama_id_t *id,
								   const sparse_vec_t &vec);
		virtual otama_status_t batch_set(const batch_records_t records);
		virtual otama_status_t set_flag(int64_t no, uint8_t flag);
		virtual int64_t get_last_commit_no(void);
		virtual bool set_last_commit_no(int64_t no);
		virtual int64_t get_last_no(void);
		virtual bool set_last_no(int64_t no);
		virtual ~InvertedIndexLevelDB();
	};
}

#endif
#endif

