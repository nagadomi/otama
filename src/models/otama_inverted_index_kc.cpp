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
#include "nv_core.h"
#include "nv_num.h"
#include "otama_log.h"
#include "otama_dbi.h"
#include "otama_omp_lock.hpp"
#include "otama_inverted_index_kc.hpp"
#include <string>
#include <queue>
#include <iterator>
#include <vector>
#include <map>
#include <algorithm>
#include <inttypes.h>

using namespace otama;

#define OTAMA_INVERTED_INDEX_COMMIT_NO_KEY "_LAST_COMMIT_NO"
#define OTAMA_INVERTED_INDEX_COMMIT_NO_KEY_LEN 15
#define OTAMA_INVERTED_INDEX_NO_KEY "_LAST_NO"
#define OTAMA_INVERTED_INDEX_NO_KEY_LEN 8

InvertedIndexKC::InvertedIndexKC(otama_variant_t *options)
	: InvertedIndex(options)
{
	otama_variant_t *driver, *value;
	
	m_keep_alive = true;
	m_inverted_index_options = "#opts=ls#bnum=512k#dfunit=32";
	m_metadata_options = "#bnum=1m#dfunit=32";
	
	driver = otama_variant_hash_at(options, "driver");
	if (OTAMA_VARIANT_IS_HASH(driver)) {
		if (!OTAMA_VARIANT_IS_NULL(value = otama_variant_hash_at(driver,
																 "keep_alive")))
		{
			m_keep_alive = otama_variant_to_bool(value);
		}
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

otama_status_t
InvertedIndexKC::begin_writer(void)
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

otama_status_t
InvertedIndexKC::begin_reader(void)
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

otama_status_t
InvertedIndexKC::end(void)
{
	m_ids.end();
	m_metadata.end();
	m_inverted_index.end();
	
	return OTAMA_STATUS_OK;
}

std::string
InvertedIndexKC::id_file_name(void)
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

std::string
InvertedIndexKC::metadata_file_name(void)
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

std::string
InvertedIndexKC::inverted_index_file_name(void)
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

otama_status_t
InvertedIndexKC::set_flag(int64_t no, uint8_t flag)
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

otama_status_t
InvertedIndexKC::clear(void)
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

otama_status_t
InvertedIndexKC::open(void)
{
	otama_status_t ret = OTAMA_STATUS_OK;

	m_metadata.path(metadata_file_name());
	m_inverted_index.path(inverted_index_file_name());
	m_ids.path(id_file_name());

	if (m_keep_alive) {
		if (!m_inverted_index.keep_alive_open(
				kyotocabinet::BasicDB::OWRITER | kyotocabinet::BasicDB::OCREATE))
		{
			OTAMA_LOG_ERROR("%s: %s\n", m_inverted_index.path().c_str(),
							m_inverted_index.error_message().c_str());
			return OTAMA_STATUS_SYSERROR;
		}
		if (!m_metadata.keep_alive_open(
				kyotocabinet::BasicDB::OWRITER | kyotocabinet::BasicDB::OCREATE))
		{
			OTAMA_LOG_ERROR("%s: %s\n", m_metadata.path().c_str(),
							m_metadata.error_message().c_str());
			return OTAMA_STATUS_SYSERROR;
		}
		if (!m_ids.keep_alive_open(
				kyotocabinet::BasicDB::OWRITER | kyotocabinet::BasicDB::OCREATE))
		{
			OTAMA_LOG_ERROR("%s: %s\n", m_ids.path().c_str(),
							m_ids.error_message().c_str());
			return OTAMA_STATUS_SYSERROR;
		}
		if (!setup()) {
			ret = OTAMA_STATUS_SYSERROR;
		}
	} else {
		ret = begin_writer();
		if (ret != OTAMA_STATUS_OK) {
			return ret;
		}
		if (!setup()) {
			ret = OTAMA_STATUS_SYSERROR;
		}
		end();
	}
	
	return ret;
}

otama_status_t
InvertedIndexKC::close(void)
{
	m_inverted_index.close();
	m_metadata.close();
	m_ids.close();
	
	return OTAMA_STATUS_OK;
}

InvertedIndexKC::~InvertedIndexKC()
{
	close();
}

int64_t
InvertedIndexKC::get_last_commit_no(void)
{
	size_t sp;
	int64_t last_modified = 0;
	int64_t *p = (int64_t *)m_metadata.get(OTAMA_INVERTED_INDEX_COMMIT_NO_KEY,
										   OTAMA_INVERTED_INDEX_COMMIT_NO_KEY_LEN, &sp);
	if (p) {
		if (sp == sizeof(last_modified)) {
			last_modified = *p;
		}
		m_metadata.free_value(p);
	} else {
		last_modified = -1;
	}
	
	return last_modified;
}

bool
InvertedIndexKC::set_last_commit_no(int64_t no)
{
	bool ret = m_metadata.set(OTAMA_INVERTED_INDEX_COMMIT_NO_KEY,
							  OTAMA_INVERTED_INDEX_COMMIT_NO_KEY_LEN, &no, sizeof(no));
	if (!ret) {
		OTAMA_LOG_ERROR("%s: %s\n", m_metadata.path().c_str(),
						m_metadata.error_message().c_str());
	}
	return ret;
}

int64_t
InvertedIndexKC::get_last_no(void)
{
	size_t sp;
	int64_t no;
	int64_t *p = (int64_t *)m_metadata.get(OTAMA_INVERTED_INDEX_NO_KEY,
										   OTAMA_INVERTED_INDEX_NO_KEY_LEN, &sp);
	if (p) {
		no = *p;
		m_metadata.free_value<int64_t>(p);
	} else {
		no = -1;
	}
	
	return no;
}

bool
InvertedIndexKC::set_last_no(int64_t no)
{
	bool ret;

	ret = m_metadata.set(OTAMA_INVERTED_INDEX_NO_KEY, OTAMA_INVERTED_INDEX_NO_KEY_LEN, &no, sizeof(no));
	if (!ret) {
		OTAMA_LOG_ERROR("%s: %s\n", m_metadata.path().c_str(),
						m_metadata.error_message().c_str());
	}
	return ret;

}

otama_status_t
InvertedIndexKC::set(int64_t no,
					 const otama_id_t *id,
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


otama_status_t
InvertedIndexKC::search_cosine(
	otama_result_t **results, int n,
	const sparse_vec_t &vec
	)
{
	int l, result_max, i;
	long t;
	int num_threads  = nv_omp_procs();
	std::vector<similarity_temp_t> *hits;
	size_t c;
	otama_status_t ret;
	topn_t topn;

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
	hits[0].reserve(c * 3);
	for (i = 1; i < num_threads; ++i) {
		hits[i].reserve(c * 3 / num_threads);
	}
	
#ifdef _OPENMP
#pragma omp parallel for num_threads(num_threads) schedule(dynamic, 32)
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
		
		for (std::vector<similarity_temp_t>::iterator j = hits[0].begin();
			 j != hits[0].end(); ++j)
		{
			if (j->no == no) {
				w += j->w;
				++count;
			} else {
				if (count > m_hit_threshold) {
					metadata_record_t *rec = m_metadata.get(&no);
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

int64_t
InvertedIndexKC::count(void)
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

int64_t
InvertedIndexKC::hash_count(uint32_t hash)
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

bool
InvertedIndexKC::sync(void)
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

void
InvertedIndexKC::decode_vbc(uint32_t hash, std::vector<int64_t> &vec)
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

// variable byte code vector
otama_status_t
InvertedIndexKC::set_vbc(int64_t no, const InvertedIndex::sparse_vec_t &vec)
{
	InvertedIndex::sparse_vec_t::const_iterator i;
	otama_status_t ret = OTAMA_STATUS_OK;

	for (i = vec.begin(); i != vec.end(); ++i) {
		uint32_t hash = *i;
		const uint64_t last_no_key = (uint64_t)hash << 32;
		size_t sp = 0;
		const int64_t *last_no_v = (const int64_t *)m_inverted_index.get(&last_no_key,
																		 sizeof(last_no_key), &sp);
		bool bret;
		int64_t last_no = 0;
		uint64_t a;

		if (last_no_v) {
			last_no = *last_no_v;
			m_inverted_index.free_value(last_no_v);
		}
		
		NV_ASSERT(last_no < no);
		
		a = no - last_no;
		while (a) {
			uint8_t v = (a & 0x7f);
			a >>= 7;
			if (a) {
				v |= 0x80U;
			}
			bret = m_inverted_index.append(&hash, &v);
			if (!bret) {
				OTAMA_LOG_ERROR("%s", m_inverted_index.error_message().c_str());
				ret = OTAMA_STATUS_SYSERROR;
				break;
			}
		}
		if (ret != OTAMA_STATUS_OK) {
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

#endif

