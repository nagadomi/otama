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
#include "otama_inverted_index_kvs.hpp"
#include "otama_leveldb.hpp"
#include "leveldb/db.h"
#include "leveldb/write_batch.h"

#include <string>
#include <queue>
#include <algorithm>

namespace otama
{
	class InvertedIndexLevelDB: public InvertedIndexKVS<
		LevelDB<int64_t, InvertedIndex::metadata_record_t>,
		LevelDB<int64_t, otama_id_t>,
		LevelDB<uint32_t, uint8_t>
		>
	{
	protected:
		virtual std::string id_file_name(void)
		{
			if (m_prefix.empty()) {
				return m_data_dir + '/' + std::string("_ids.ldb");
			}
			return m_data_dir + '/' + m_prefix + "_ids.ldb";
		}
		virtual std::string metadata_file_name(void)
		{
			if (m_prefix.empty()) {
				return m_data_dir + '/' + std::string("_metadata.ldb");
			}
			return m_data_dir + '/' + m_prefix + "_metadata.ldb";
		}
		virtual std::string inverted_index_file_name(void)
		{
			if (m_prefix.empty()) {
				return m_data_dir + '/' + std::string("_inverted_index.ldb");
			}
			return m_data_dir + '/' + m_prefix + "_inverted_index.ldb";
		}

		
	public:
		InvertedIndexLevelDB(otama_variant_t *options)
		: InvertedIndexKVS(options)
		{}

	};
}

#endif
#endif
