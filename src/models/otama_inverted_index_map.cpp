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
#include "nv_core.h"
#include "nv_num.h"
#include "otama_log.h"
#include "otama_dbi.h"
#include "otama_omp_lock.hpp"
#include "otama_inverted_index_map.hpp"
#include <string>
#include <queue>
#include <iterator>
#include <vector>
#include <map>
#include <algorithm>
#include <inttypes.h>

using namespace otama;

InvertedIndexMap::InvertedIndexMap(otama_variant_t *options)
	: InvertedIndex(options)
{
#ifdef _OPENMP
	omp_init_nest_lock(&m_lock);
#endif
	m_last_commit_no = -1;
	m_last_no = -1;
}

otama_status_t
InvertedIndexMap::begin_writer(void)
{
#ifdef _OPENMP
	omp_set_nest_lock(&m_lock);
#endif
	
	return OTAMA_STATUS_OK;
}

otama_status_t
InvertedIndexMap::begin_reader(void)
{
#ifdef _OPENMP
	omp_set_nest_lock(&m_lock);
#endif
	return OTAMA_STATUS_OK;
}

otama_status_t InvertedIndexMap::end(void)
{
#ifdef _OPENMP
	omp_unset_nest_lock(&m_lock);
#endif
	return OTAMA_STATUS_OK;	
}

otama_status_t
InvertedIndexMap::set_flag(int64_t no, uint8_t flag)
{
	otama_status_t ret = OTAMA_STATUS_OK;
	metadata_t::iterator rec;
	
	rec = m_metadata.find(no);
	if (rec != m_metadata.end()) {
		rec->second.flag = flag;
	} else {
		OTAMA_LOG_ERROR("record not found(%"PRId64")", no);
		ret = OTAMA_STATUS_NODATA;
	}
	
	return ret;
}

otama_status_t
InvertedIndexMap::clear(void)
{
	m_metadata.clear();
	m_inverted_index.clear();
	return OTAMA_STATUS_OK;
}

otama_status_t
InvertedIndexMap::open(void)
{
	begin_writer();
	{
		m_metadata.clear();
		m_inverted_index.clear();
	}
	end();
	
	return OTAMA_STATUS_OK;
}

otama_status_t
InvertedIndexMap::close(void)
{
	begin_writer();
	{
		m_metadata.clear();
		m_inverted_index.clear();
	}
	end();

	return OTAMA_STATUS_OK;
}

InvertedIndexMap::~InvertedIndexMap()
{
}

int64_t
InvertedIndexMap::get_last_commit_no(void)
{
	return m_last_commit_no;
}

bool
InvertedIndexMap::set_last_commit_no(int64_t no)
{
	m_last_commit_no = no;
	return true;
}

int64_t
InvertedIndexMap::get_last_no(void)
{
	return m_last_no;
}

bool
InvertedIndexMap::set_last_no(int64_t no)
{
	m_last_no = no;
	return true;
}

otama_status_t
InvertedIndexMap::set(int64_t no,
					  const otama_id_t *id,
					  const InvertedIndex::sparse_vec_t &vec)
{
	metadata_record_t rec;
	InvertedIndex::sparse_vec_t::const_iterator i;
	std::pair<metadata_t::iterator, bool> ret;
	
	rec.norm = norm(vec);
	rec.flag = 0;
	memcpy(&rec.id, id, sizeof(*id));

	ret = m_metadata.insert(metadata_t::value_type(no, rec));
	if (ret.second) {
		for (i = vec.begin(); i != vec.end(); ++i) {
			inverted_index_t::iterator iv;
			iv = m_inverted_index.find(*i);
			if (iv != m_inverted_index.end()) {
				iv->second.push_back(no);
			} else {
				VariableByteCodeVector iv_rec;
				iv_rec.push_back(no);
				m_inverted_index.insert(inverted_index_t::value_type(*i, iv_rec));
			}
		}
	}
	
	return OTAMA_STATUS_OK;
}

otama_status_t
InvertedIndexMap::search_cosine(
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
	c = (size_t)count();
	hits[0].reserve(c * 3);
	for (i = 1; i < num_threads; ++i) {
		hits[i].reserve(c * 3 / num_threads);
	}

#ifdef _OPENMP
#pragma omp parallel for num_threads(num_threads)
#endif
	for (i = 0; i < (int)vec.size(); ++i) {
		int thread_id = nv_omp_thread_id();
		uint32_t hash = vec[i];
		std::vector<similarity_temp_t> &hit = hits[thread_id];
		inverted_index_t::iterator v = m_inverted_index.find(hash);
		if (v != m_inverted_index.end()) {
			std::vector<int64_t> nos;
			std::vector<int64_t>::const_iterator j;
			
			v->second.decode(nos);
			for (j = nos.begin(); j != nos.end(); ++j) {
				similarity_temp_t hi;
				hi.no = *j;
				hi.w = (*m_similarity_func)(hash);
				hi.w *= hi.w;
				hit.push_back(hi);
			}
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
		std::vector<similarity_temp_t>::iterator j;
		
		for (j = hits[0].begin(); j != hits[0].end(); ++j)
		{
			if (j->no == no) {
				w += j->w;
				++count;
			} else {
				if (count > m_hit_threshold) {
					metadata_t::const_iterator rec = m_metadata.find(no);
					if (rec != m_metadata.end()) {
						if ((rec->second.flag & FLAG_DELETE) == 0) {
							float similarity = w / (query_norm * rec->second.norm);
							if (n > (int)topn.size()) {
								similarity_result_t t;
								memcpy(&t.id, &rec->second.id, sizeof(t.id));
								t.similarity = similarity;
								topn.push(t);
							} else if (topn.top().similarity < similarity) {
								similarity_result_t t;
								memcpy(&t.id, &rec->second.id, sizeof(t.id));
								t.similarity = similarity;
								topn.push(t);
								topn.pop();
							}
						}
					}
				}
				no = j->no;
				w = j->w;
				count = 1;
			}
		}
		if (count > m_hit_threshold) {
			metadata_t::const_iterator rec = m_metadata.find(no);
			if (rec != m_metadata.end()) {
				if ((rec->second.flag & FLAG_DELETE) == 0) {
					float similarity = w / (query_norm * rec->second.norm);
					if (n > (int)topn.size()) {
						similarity_result_t t;
						memcpy(&t.id, &rec->second.id, sizeof(t.id));
						t.similarity = similarity;
						topn.push(t);
					} else if (topn.top().similarity < similarity) {
						similarity_result_t t;
						memcpy(&t.id, &rec->second.id, sizeof(t.id));
						t.similarity = similarity;
						topn.push(t);
						topn.pop();
					}
				}
			}
		}
	}
	
	*results = otama_result_alloc(n);
	result_max = NV_MIN(n, (int)topn.size());
	
	for (l = result_max - 1; l >= 0; --l) {
		const similarity_result_t &p = topn.top();
		set_result(*results, l, &p.id, p.similarity);
		topn.pop();
	}
	otama_result_set_count(*results, result_max);
	
	end();
	OTAMA_LOG_DEBUG("search: ranking: %ldms", nv_clock() - t);	
	
	delete [] hits;

	return OTAMA_STATUS_OK;
}

int64_t
InvertedIndexMap::count(void)
{
	return (int64_t)m_metadata.size();
}

bool
InvertedIndexMap::sync(void)
{
	return true;
}

int64_t
InvertedIndexMap::hash_count(uint32_t hash)
{
	inverted_index_t::iterator v = m_inverted_index.find(hash);
	if (v != m_inverted_index.end()) {
		return v->second.count();
	}
	
	return 0;
}
