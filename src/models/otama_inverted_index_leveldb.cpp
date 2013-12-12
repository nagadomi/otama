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
#include "nv_core.h"
#include "nv_num.h"
#include "otama_log.h"
#include "otama_util.h"
#include "otama_dbi.h"
#include "otama_omp_lock.hpp"
#include "otama_inverted_index_leveldb.hpp"
#include "otama_leveldb.hpp"
#include <string>
#include <queue>
#include <iterator>
#include <vector>
#include <map>
#include <algorithm>
#include <inttypes.h>

using namespace otama;
std::string
InvertedIndexLevelDB::id_file_name(void)
{
	if (m_prefix.empty()) {
		return m_data_dir + '/' + std::string("_ids.ldb");
	}
	return m_data_dir + '/' + m_prefix + "_ids.ldb";
}

std::string
InvertedIndexLevelDB::metadata_file_name(void)
{
	if (m_prefix.empty()) {
		return m_data_dir + '/' + std::string("_metadata.ldb");
	}
	return m_data_dir + '/' + m_prefix + "_metadata.ldb";
}

std::string
InvertedIndexLevelDB::inverted_index_file_name(void)
{
	if (m_prefix.empty()) {
		return m_data_dir + '/' + std::string("_inverted_index.ldb");
	}
	return m_data_dir + '/' + m_prefix + "_inverted_index.ldb";
}
		
otama_status_t
InvertedIndexLevelDB::set_vbc(int64_t no, const sparse_vec_t &vec)
{
	sparse_vec_t::const_iterator i;
	otama_status_t ret = OTAMA_STATUS_OK;
	std::string db_value;

	for (i = vec.begin(); i != vec.end(); ++i) {
		uint32_t hash = *i;
		const uint64_t last_no_key = (uint64_t)hash << 32;
		bool bret;
		int64_t last_no = 0;
		uint64_t a;
		std::vector<uint8_t> append_value;
		if (m_inverted_index.get(&last_no_key, sizeof(last_no_key), db_value)) {
			last_no = *(const int64_t *)db_value.data();
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

void
InvertedIndexLevelDB::decode_vbc(uint32_t hash, std::vector<int64_t> &vec)
{
	static const int s_t[8] = { 0, 7, 14, 21, 28, 35, 42, 49 };
	std::string buff;
	vec.clear();
	if (m_inverted_index.get(&hash, sizeof(hash), buff)) {
		const uint8_t *vs = (const uint8_t *)buff.data();
		size_t sp = buff.size();
		const size_t n = sp / sizeof(uint8_t);
		int64_t a = 0;
		int64_t last_no = 0;
		int j = 0;
		size_t i;
		
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
}
		
void
InvertedIndexLevelDB::init_index_buffer(index_buffer_t &index_buffer,
										last_no_buffer_t &last_no_buffer,
										const batch_records_t &records)
{
	batch_records_t::const_iterator i;
	std::string db_value;
	index_buffer.clear();
	last_no_buffer.clear();
	
	for (i = records.begin(); i != records.end(); ++i) {
		sparse_vec_t::const_iterator j;
		for (j = i->vec.begin(); j != i->vec.end(); ++j) {
			uint32_t hash = *j;
			last_no_buffer_t::const_iterator no = last_no_buffer.find(hash);
			if (no == last_no_buffer.end()) {
				const uint64_t last_no_key = (uint64_t)hash << 32;
				int64_t last_no;
						
				if (m_inverted_index.get(&last_no_key, sizeof(last_no_key), db_value)) {
					last_no = *(const int64_t *)db_value.data();
				} else {
					last_no = 0;
				}
				
				std::vector<uint8_t> empty_rec;
				last_no_buffer.insert(std::make_pair(hash, last_no));
				index_buffer.insert(std::make_pair(hash, empty_rec));
			}
		}
	}
}
		
void
InvertedIndexLevelDB::set_index_buffer(index_buffer_t &index_buffer,
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
InvertedIndexLevelDB::write_index_buffer(const index_buffer_t &index_buffer,
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
	OTAMA_LOG_DEBUG("begin index writer", 0);
			
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
			
	OTAMA_LOG_DEBUG("end index writer", 0);
	
	return ret;
}

bool
InvertedIndexLevelDB::verify_index(void)
{
	std::string db_value;
	if (m_metadata.get("_VERIFY_INDEX", 13, db_value)) {
		bool ret = false;
		if (db_value.size() == sizeof(int8_t) && *(const int8_t *)db_value.data() == 1) {
			ret = true;
			OTAMA_LOG_DEBUG("verify_index 1", 0);
		} else {
			OTAMA_LOG_DEBUG("verify_index 0", 0);
		}
		return ret;
	} else {
		OTAMA_LOG_DEBUG("verify_index null", 0);
		return true;
	}
}

InvertedIndexLevelDB::InvertedIndexLevelDB(otama_variant_t *options)
	: InvertedIndex(options)
{
	otama_variant_t *driver, *value;
	
	m_preheat_cache = true;
	
	driver = otama_variant_hash_at(options, "driver");
	if (OTAMA_VARIANT_IS_HASH(driver)) {
		if (!OTAMA_VARIANT_IS_NULL(value = otama_variant_hash_at(driver, "preheat_cache"))) {
			m_preheat_cache = otama_variant_to_bool(value);
		}
	}
	OTAMA_LOG_DEBUG("driver[preheat_cache] => %s",
					m_preheat_cache ? "true" : "false");
}

static int
heat_file(void *user_data, const char *path)
{
	FILE *fp;
	long len;
	char *buff;

	fp = fopen(path, "rb");
	if (fp == NULL) {
		return 0;
	}
	fseek(fp, 0, SEEK_END);
	len = ftell(fp);
	buff = nv_alloc_type(char, len);
	rewind(fp);
	while (fread(buff, 1, len, fp) > 0) {}
	fclose(fp);
	nv_free(buff);
	
	return 0;
}

void
InvertedIndexLevelDB::preheat_cache_dir(const std::string &dir)
{
	otama_file_each(dir.c_str(), heat_file, NULL);
}

void
InvertedIndexLevelDB::preheat_cache(void)
{
	long t = nv_clock();
	
	preheat_cache_dir(metadata_file_name());
	preheat_cache_dir(inverted_index_file_name());
	preheat_cache_dir(id_file_name());
	
	OTAMA_LOG_DEBUG("preheat_cache %ldms", nv_clock() - t);
}

otama_status_t
InvertedIndexLevelDB::open(void)
{
	otama_status_t ret = OTAMA_STATUS_OK;

	if (m_preheat_cache) {
		preheat_cache();
	}
	
	m_metadata.path(metadata_file_name());
	m_inverted_index.path(inverted_index_file_name());
	m_ids.path(id_file_name());

	if (!m_inverted_index.open()) {
		OTAMA_LOG_ERROR("%s: %s\n", m_inverted_index.path().c_str(),
						m_inverted_index.error_message().c_str());
		return OTAMA_STATUS_SYSERROR;
	}
	if (!m_metadata.open()) {
		OTAMA_LOG_ERROR("%s: %s\n", m_metadata.path().c_str(),
						m_metadata.error_message().c_str());
		return OTAMA_STATUS_SYSERROR;
	}
	if (!m_ids.open()) {
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
	
	return ret;
}
		
otama_status_t
InvertedIndexLevelDB::close(void)
{
	m_inverted_index.close();
	m_metadata.close();
	m_ids.close();
			
	return OTAMA_STATUS_OK;
}
		
otama_status_t
InvertedIndexLevelDB::clear(void)
{
	m_inverted_index.clear();
	m_metadata.clear();
	m_ids.clear();
			
	return OTAMA_STATUS_OK;
}

otama_status_t
InvertedIndexLevelDB::vacuum(void)
{
	if (!m_ids.vacuum()) {
		return OTAMA_STATUS_SYSERROR;
	}
	if (!m_metadata.vacuum()) {
		return OTAMA_STATUS_SYSERROR;
	}
	if (!m_inverted_index.vacuum()) {
		return OTAMA_STATUS_SYSERROR;
	}
			
	return OTAMA_STATUS_OK;
}

typedef struct {
	int64_t no;
	float w;
} hit_tmp_t;

otama_status_t
InvertedIndexLevelDB::search(otama_result_t **results, int n,
							 const sparse_vec_t &vec)
{
	int l, result_max, i;
	long t;
	int num_threads  = nv_omp_procs();
	std::vector<std::vector<similarity_temp_t> >hits;
	size_t c;
	std::vector<topn_t> topn;

	if (n < 1) {
		return OTAMA_STATUS_INVALID_ARGUMENTS;
	}
	topn.resize(num_threads);
	hits.resize(num_threads);
			
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
			hi.w = (*m_weight_func)(h);
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
		std::vector<hit_tmp_t> hit_tmp;
		bool has_error = false;
				
		for (std::vector<similarity_temp_t>::const_iterator j = hits[0].begin();
			 j != hits[0].end(); ++j)
		{
			if (j->no == no) {
				w += j->w;
				++count;
			} else {
				if (count > m_hit_threshold) {
					hit_tmp_t tmp;
					tmp.no = no;
					tmp.w = w;
					hit_tmp.push_back(tmp);
				}
				no = j->no;
				w = j->w;
				count = 1;
			}
		}
		if (count > m_hit_threshold) {
			hit_tmp_t tmp;
			tmp.no = no;
			tmp.w = w;
			hit_tmp.push_back(tmp);
		}
#ifdef _OPENMP
#pragma omp parallel for num_threads(num_threads) shared(has_error) schedule(dynamic, 16)
#endif
		for (i = 0; i < (int)hit_tmp.size(); ++i) {
			int thread_id = nv_omp_thread_id();
			std::string db_value;
			if (!m_metadata.get(&hit_tmp[i].no, sizeof(hit_tmp[i].no), db_value)) {
#ifdef _OPENMP
#pragma omp critical (otama_inverted_index_leveldb_search_error)
#endif
				{
					if (!has_error) {
						OTAMA_LOG_ERROR("indexes are corrupted. try to clear index.. please rebuild index using otama_pull.", 0);
						clear();
						has_error = true;
					}
				}
			} else {
				const metadata_record_t *rec = (const metadata_record_t *)db_value.data();
				if ((rec->flag & FLAG_DELETE) == 0) {
					float similarity = hit_tmp[i].w / (query_norm * rec->norm);
					if (n > (int)topn[thread_id].size()) {
						similarity_result_t t;
						t.no = hit_tmp[i].no;
						t.similarity = similarity;
						topn[thread_id].push(t);
					} else if (topn[thread_id].top().similarity < similarity) {
						similarity_result_t t;
						t.no = hit_tmp[i].no;
						t.similarity = similarity;
						topn[thread_id].push(t);
						topn[thread_id].pop();
					}
				}
			}
		}
		if (has_error) {
			return OTAMA_STATUS_SYSERROR;
		}
		for (i = 1; i < num_threads; ++i) {
			while (!topn[i].empty()) {
				topn[0].push(topn[i].top());
				topn[i].pop();
			}
		}
		while (topn[0].size() > (size_t)n) {
			topn[0].pop();
		}
	}
	
	*results = otama_result_alloc(n);
	result_max = NV_MIN(n, (int)topn[0].size());
	
	for (l = result_max - 1; l >= 0; --l) {
		const similarity_result_t &p = topn[0].top();
		std::string db_value;
		if (m_ids.get(&p.no, sizeof(p.no), db_value)) {
			const otama_id_t *id = (const otama_id_t *)db_value.data();
			set_result(*results, l, id, p.similarity);
		} else {
			OTAMA_LOG_ERROR("indexes are corrupted. try to clear index.. please rebuild index using otama_pull.", 0);
			clear();
			otama_result_free(results);
			return OTAMA_STATUS_SYSERROR;
		}
		topn[0].pop();
	}
	otama_result_set_count(*results, result_max);
	
	OTAMA_LOG_DEBUG("search: ranking: %ldms", nv_clock() - t);
			
	return OTAMA_STATUS_OK;
}
		
int64_t
InvertedIndexLevelDB::count(void)
{
	int64_t count;
	
	count = m_ids.count();
	if (count < 0) {
		count = 0;
	}
	
	return count;
}
		
bool
InvertedIndexLevelDB::sync(void)
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

bool
InvertedIndexLevelDB::update_count(void)
{
	bool ret;
			
	ret = m_ids.update_count();
	if (!ret) {
		OTAMA_LOG_ERROR("%s: %s\n", m_ids.path().c_str(),
						m_ids.error_message().c_str());
		return 0;
	}
	return true;
}

int64_t
InvertedIndexLevelDB::hash_count(uint32_t hash)
{
	std::vector<int64_t> nos;
	decode_vbc(hash, nos);
	return (int64_t)nos.size();
}
		
otama_status_t
InvertedIndexLevelDB::set(int64_t no, const otama_id_t *id,
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
InvertedIndexLevelDB::batch_set(const batch_records_t records)
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
		
otama_status_t
InvertedIndexLevelDB::set_flag(int64_t no, uint8_t flag)
{
	otama_status_t ret = OTAMA_STATUS_OK;
	bool bret;
	std::string db_value;
	if (m_metadata.get(&no, sizeof(no), db_value) && db_value.size() == sizeof(metadata_record_t)) {
		metadata_record_t rec = *(const metadata_record_t *)db_value.data();
		rec.flag = flag;
		bret = m_metadata.set(&no, &rec);
		if (!bret) {
			OTAMA_LOG_ERROR("%s", m_metadata.error_message().c_str());
			ret = OTAMA_STATUS_SYSERROR;
		}
	} else {
		OTAMA_LOG_ERROR("record not found(%"PRId64")", no);
		ret = OTAMA_STATUS_NODATA;
	}
	
	return ret;

}

int64_t
InvertedIndexLevelDB::get_last_commit_no(void)
{
	int64_t last_modified;
	std::string db_value;
	
	if (m_metadata.get("_LAST_COMMIT_NO", 15, db_value) && db_value.size() == sizeof(last_modified)) {
		last_modified = *(const int64_t *)db_value.data();
	} else {
		last_modified = -1;
	}
	
	return last_modified;
}
bool
InvertedIndexLevelDB::set_last_commit_no(int64_t no)
{
	bool ret = m_metadata.set("_LAST_COMMIT_NO", 15, &no, sizeof(no));
	if (!ret) {
		OTAMA_LOG_ERROR("%s: %s\n", m_metadata.path().c_str(),
						m_metadata.error_message().c_str());
	}
	return ret;
}

int64_t
InvertedIndexLevelDB::get_last_no(void)
{
	int64_t no;
	std::string db_value;
	
	if (m_metadata.get("_LAST_NO", 8, db_value) && db_value.size() == sizeof(no)) {
		no = *(const int64_t *)db_value.data();
	} else {
		no = -1;
	}
	
	return no;
}

bool
InvertedIndexLevelDB::set_last_no(int64_t no)
{
	bool ret;

	ret = m_metadata.set("_LAST_NO", 8, &no, sizeof(no));
	if (!ret) {
		OTAMA_LOG_ERROR("%s: %s\n", m_metadata.path().c_str(),
						m_metadata.error_message().c_str());
	}
	return ret;
}

InvertedIndexLevelDB::~InvertedIndexLevelDB()
{
	close();
}

#endif

