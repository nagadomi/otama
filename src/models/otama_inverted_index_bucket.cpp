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
#include "otama_inverted_index_bucket.hpp"
#include <string>
#include <queue>
#include <iterator>
#include <vector>
#include <map>
#include <algorithm>
#include <inttypes.h>

using namespace otama;

InvertedIndexBucket::InvertedIndexBucket(otama_variant_t *options)
	: InvertedIndex(options)
{
#ifdef _OPENMP
	omp_init_nest_lock(&m_lock);
#endif
	m_last_commit_no = -1;
	m_last_no = -1;
}

void
InvertedIndexBucket::reserve(size_t hash_max)
{
	m_inverted_index.reserve(hash_max);
}

otama_status_t
InvertedIndexBucket::begin_writer(void)
{
#ifdef _OPENMP
	omp_set_nest_lock(&m_lock);
#endif
	
	return OTAMA_STATUS_OK;
}

otama_status_t
InvertedIndexBucket::begin_reader(void)
{
#ifdef _OPENMP
	omp_set_nest_lock(&m_lock);
#endif
	return OTAMA_STATUS_OK;
}

otama_status_t InvertedIndexBucket::end(void)
{
#ifdef _OPENMP
	omp_unset_nest_lock(&m_lock);
#endif
	return OTAMA_STATUS_OK;	
}

otama_status_t
InvertedIndexBucket::set_flag(int64_t no, uint8_t flag)
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
InvertedIndexBucket::clear(void)
{
	m_metadata.clear();
	m_inverted_index.clear();
	m_last_commit_no = -1;
	m_last_no = -1;
	
	return OTAMA_STATUS_OK;
}

otama_status_t
InvertedIndexBucket::vacuum(void)
{
	return OTAMA_STATUS_OK;
}

otama_status_t
InvertedIndexBucket::open(void)
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
InvertedIndexBucket::close(void)
{
	begin_writer();
	{
		m_metadata.clear();
		m_inverted_index.clear();
	}
	end();

	return OTAMA_STATUS_OK;
}

InvertedIndexBucket::~InvertedIndexBucket()
{
}

int64_t
InvertedIndexBucket::get_last_commit_no(void)
{
	return m_last_commit_no;
}

bool
InvertedIndexBucket::set_last_commit_no(int64_t no)
{
	m_last_commit_no = no;
	return true;
}

int64_t
InvertedIndexBucket::get_last_no(void)
{
	return m_last_no;
}

bool
InvertedIndexBucket::set_last_no(int64_t no)
{
	m_last_no = no;
	return true;
}

otama_status_t
InvertedIndexBucket::set(int64_t no,
						 const otama_id_t *id,
						 const InvertedIndex::sparse_vec_t &vec)
{
	metadata_record_t rec;
	int i;
	std::pair<metadata_t::iterator, bool> ret;
	
	rec.norm = norm(vec);
	rec.flag = 0;
	memcpy(&rec.id, id, sizeof(*id));

	ret = m_metadata.insert(metadata_t::value_type(no, rec));
	if (ret.second) {
		if (vec.size()) {
			// sorted
			if (m_inverted_index.size() <= vec.back()) {
				m_inverted_index.resize(vec.back() + 1);
			}
		}
#ifdef _OPENMP
#pragma omp parallel for
#endif
		for (i = 0; i < (int)vec.size(); ++i) {
			NV_ASSERT(m_inverted_index.size() > vec[i]);
			m_inverted_index[vec[i]].push_back(no);
		}
	}
	
	return OTAMA_STATUS_OK;
}

otama_status_t
InvertedIndexBucket::search_cosine(
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
#pragma omp parallel for num_threads(num_threads) schedule(dynamic, 32)
#endif
	for (i = 0; i < (int)vec.size(); ++i) {
		int thread_id = nv_omp_thread_id();
		uint32_t hash = vec[i];
		std::vector<similarity_temp_t> &hit = hits[thread_id];
		if (m_inverted_index.size() > hash) {
			std::vector<int64_t> nos;
			std::vector<int64_t>::const_iterator j;
			
			m_inverted_index[hash].decode(nos);
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
		std::vector<similarity_temp_t>::const_iterator j;
		
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
InvertedIndexBucket::count(void)
{
	return (int64_t)m_metadata.size();
}

bool
InvertedIndexBucket::sync(void)
{
	return true;
}

bool
InvertedIndexBucket::update_count(void)
{
	return true;
}

int64_t
InvertedIndexBucket::hash_count(uint32_t hash)
{
	if (m_inverted_index.size() > hash) {
		return m_inverted_index[hash].count();
	}
	return 0;
}
