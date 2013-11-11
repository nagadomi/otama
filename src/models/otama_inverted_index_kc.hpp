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
#include "otama_inverted_index_kvs.hpp"
#include "otama_kyotocabinet.hpp"
#include <kcpolydb.h>
#include <kcdbext.h>
#include <kccompress.h>
#include <string>
#include <queue>
#include <algorithm>

namespace otama
{
	class InvertedIndexKC: public InvertedIndexKVS<
		KyotoCabinet<kyotocabinet::PolyDB,
					 int64_t,
					 InvertedIndex::metadata_record_t >,
		KyotoCabinet<kyotocabinet::PolyDB,
					 int64_t,
					 otama_id_t>,
		KyotoCabinet<kyotocabinet::IndexDB,
					 uint32_t, uint8_t>
		>
	{
	protected:
		std::string m_metadata_options;
		std::string m_inverted_index_options;

		virtual std::string id_file_name(void)
		{
			if (m_prefix.empty()) {
				return m_data_dir + '/' +
					std::string("_ids.kct") +
					m_metadata_options;
			}
			return m_data_dir + '/' + m_prefix +
				"_ids.kct" +
				m_metadata_options;
		}
		virtual std::string metadata_file_name(void)
		{
			if (m_prefix.empty()) {
				return m_data_dir + '/' +
					std::string("_metadata.kct") +
					m_metadata_options;
			}
			return m_data_dir + '/' + m_prefix +
				"_metadata.kct" +
				m_metadata_options;
		}
		virtual std::string inverted_index_file_name(void)
		{
			if (m_prefix.empty()) {
				return m_data_dir + '/' +
					std::string("_inverted_index.kct") +
					m_inverted_index_options;
			}
			return m_data_dir + '/' + m_prefix +
				"_inverted_index.kct" +
				m_inverted_index_options;
		}
		
	public:
		InvertedIndexKC(otama_variant_t *options)
		: InvertedIndexKVS(options)
		{
			otama_variant_t *driver, *value;
			
			m_inverted_index_options = "#opts=ls#bnum=512k#dfunit=2";
			m_metadata_options = "#bnum=1m#dfunit=2";
			
			driver = otama_variant_hash_at(options, "driver");
			if (OTAMA_VARIANT_IS_HASH(driver)) {
				if (!OTAMA_VARIANT_IS_NULL(value = otama_variant_hash_at(driver,
																		 "metadata_options")))
				{
					m_metadata_options = otama_variant_to_string(value);
				}
				if (!OTAMA_VARIANT_IS_NULL(value = otama_variant_hash_at(driver,
																		 "inverted_index_options")))
				{
					m_inverted_index_options = otama_variant_to_string(value);
				}
			}
		}
	};
}

#endif
#endif
