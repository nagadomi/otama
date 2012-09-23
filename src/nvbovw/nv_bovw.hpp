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

#ifndef NV_BOVW_HPP
#define NV_BOVW_HPP

#include <vector>
#include <set>
#include <string>
#include <queue>
#include <algorithm>
#ifdef HAVE_CONFIG
#  include "config.h"
#endif
#include "nv_core.h"
#include "nv_ml.h"
#include "nv_io.h"
#include "nv_ip.h"
#include "nv_num.h"
#include "nv_color_boc.h"

typedef struct nv_bovw_result {
	float similarity;
	uint64_t index;
	
	inline bool
	operator>(const struct nv_bovw_result &lhs) const
	{
		return similarity > lhs.similarity;
	}
} nv_bovw_result_t;

typedef enum {
	NV_BOVW_BIT2K = 2048,
	NV_BOVW_BIT8K = 8192,
	NV_BOVW_BIT512K = 524288
} nv_bovw_bit_e;

typedef enum {
	NV_BOVW_RERANK_NONE
	,NV_BOVW_RERANK_IDF
	// NV_BOVW_RERANK_AVLAD // removed
	// NV_BOVW_RERANK_RANKING_LR // removed
} nv_bovw_rerank_method_t;

typedef struct {
	int64_t color[1];
} nv_bovw_dummy_color_t;

template <nv_bovw_bit_e N, typename C>
class nv_bovw_ctx {
public:
	static const int BIT = N;
	static const int TOP_VQ = 2048; /* NV_BOVW_KMT_POSI()->dim[0] * 2 */
	static const int INT_BLOCKS = N / 64;
	static inline const float IMG_SIZE() { return 512.0f; };
	static const int POSI_N = N / 2;
	static const int COLOR_SIZE = sizeof(((C *)NULL)->color);
	static const int COLOR_INT_BLOCKS = COLOR_SIZE == sizeof(nv_bovw_dummy_color_t) ?
		0: (COLOR_SIZE / sizeof(uint64_t));
	static const int SERIALIZE_LEN = (((COLOR_INT_BLOCKS + INT_BLOCKS) * 16 + 1) + 1);
	
	typedef struct {
		NV_ALIGNED(uint64_t, bovw[INT_BLOCKS], 16);
		float norm;
		NV_ALIGNED(C, boc, 16);
	} dense_t;
	typedef std::vector<uint32_t> sparse_t;
	
private:
	typedef std::priority_queue<nv_bovw_result_t, std::vector<nv_bovw_result_t>,
								std::greater<std::vector<nv_bovw_result_t>::value_type> > topn_t;
	static inline uint32_t BIT_INDEX(int n) { return n / 64; }
	static inline uint32_t BIT_BIT(int n) { return n % 64; }
	static inline float VQ_THRESH() { return 0.5f; }
	static const int SEARCH_FIRST_SCALE = 10;
	static const int KEYPOINT_M = N == NV_BOVW_BIT2K ? 640 : (N == NV_BOVW_BIT8K ? 768 : 1800);
	static const int HKM_NN = 4;
	
	void
	init_ctx(void)
	{
		switch (N) {
		case NV_BOVW_BIT2K: {
			nv_keypoint_param_t param = {
				16.0f,
				NV_KEYPOINT_EDGE_THRESH,
				4.0f,
				15,
				0.8f,
				NV_KEYPOINT_DETECTOR_STAR,
				NV_KEYPOINT_DESCRIPTOR_GRADIENT_HISTOGRAM
			};
			m_ctx = nv_keypoint_ctx_alloc(&param);
		} break;
		case NV_BOVW_BIT8K: {
			nv_keypoint_param_t param = {
				16.0f,
				NV_KEYPOINT_EDGE_THRESH,
				4.0,
				15,
				0.8f,
				NV_KEYPOINT_DETECTOR_STAR,
				NV_KEYPOINT_DESCRIPTOR_GRADIENT_HISTOGRAM
			};
			m_ctx = nv_keypoint_ctx_alloc(&param);
		} break;
		default: {
			nv_keypoint_param_t param = {
				NV_KEYPOINT_THRESH,
				NV_KEYPOINT_EDGE_THRESH,
				NV_KEYPOINT_MIN_R,
				NV_KEYPOINT_LEVEL,
				NV_KEYPOINT_NN,
				NV_KEYPOINT_DETECTOR_STAR,
				NV_KEYPOINT_DESCRIPTOR_GRADIENT_HISTOGRAM
			};
			m_ctx = nv_keypoint_ctx_alloc(&param);
		} break;
		}
	}
	
	static inline float
	bit_cosine(const dense_t *a, const dense_t *b)
	{
		int i;
		uint64_t count = 0;
		
		for (i = 0; i < INT_BLOCKS; ++i) {
			count += NV_POPCNT_U64(a->bovw[i] & b->bovw[i]);
		}
		
		return 	(float)count / (a->norm * b->norm);
	}
	static inline void
	color(nv_color_boc_t *boc, const nv_matrix_t *image)
	{
		nv_color_boc(boc, image);
	}
	static inline void
	color(nv_color_sboc_t *boc, const nv_matrix_t *image)
	{
		nv_color_sboc(boc, image);
	}
	static inline void
	color(nv_bovw_dummy_color_t *boc, const nv_matrix_t *image)
	{
	}
	static inline float
	color_similarity(const nv_color_boc_t *a, const nv_color_boc_t *b)
	{
		return nv_color_boc_similarity(a, b);
	}
	static inline float
	color_similarity(const nv_color_sboc_t *a, const nv_color_sboc_t *b)
	{
		return nv_color_sboc_similarity(a, b);
	}
	static inline float
	color_similarity(const nv_bovw_dummy_color_t *a, const nv_bovw_dummy_color_t *b)
	{
		return 0.0f;
	}
	static inline void
	color_serialize(char *s, const nv_color_boc_t *boc)
	{
		char *p = nv_color_boc_serialize(boc);
		NV_ASSERT(strlen(p) == COLOR_INT_BLOCKS * 16);
		strcpy(s, p);
		nv_free(p);
	}
	static inline void
	color_serialize(char *s, const nv_color_sboc_t *boc)
	{
		char *p = nv_color_sboc_serialize(boc);
		NV_ASSERT(strlen(p) == COLOR_INT_BLOCKS * 16);
		strcpy(s, p);
		nv_free(p);
	}
	static inline void
	color_serialize(char *s, const nv_bovw_dummy_color_t *boc)
	{
	}
	static inline int
	color_deserialize(nv_color_boc_t *boc, const char *s)
	{
		return nv_color_boc_deserialize(boc, s);
	}
	static inline int
	color_deserialize(nv_color_sboc_t *boc, const char *s)
	{
		return nv_color_sboc_deserialize(boc, s);
	}
	static inline int
	color_deserialize(nv_bovw_dummy_color_t *boc, const char *s)
	{
		return 0;
	}
	
	nv_kmeans_tree_t *m_posi;
	nv_kmeans_tree_t *m_nega;
	nv_matrix_t *m_idf;
	nv_keypoint_ctx_t *m_ctx;
	
	static char *
	default_data_path(char *path, size_t len, const char *filename)
	{
		const char *pkgdatadir = nv_getenv("NV_BOVW_PKGDATADIR");
		if (pkgdatadir == NULL || strlen(pkgdatadir) == 0) {
			pkgdatadir = PKGDATADIR;
		}
		nv_snprintf(path, len, "%s/%s", pkgdatadir, filename);
		
		return path;
	}
	
	void
	extract_sparse_feature(sparse_t &vec, const nv_matrix_t *smooth)
	{
		nv_matrix_t *key_vec;
		nv_matrix_t *desc_vec;
		int desc_m;
		int i;
		int procs = nv_omp_procs();
		std::set<uint32_t> *tmp = new std::set<uint32_t>[procs];
		std::set<uint32_t> tmp2;
		std::set<uint32_t>::const_iterator it;
		
		key_vec = nv_matrix_alloc(NV_KEYPOINT_KEYPOINT_N, KEYPOINT_M);
		desc_vec = nv_matrix_alloc(NV_KEYPOINT_DESC_N, KEYPOINT_M);
		
		desc_m = nv_keypoint_ex(m_ctx, key_vec, desc_vec, smooth, 0);
		
#ifdef _OPENMP
#pragma omp parallel for num_threads(procs) schedule(dynamic, 1)
#endif
		for (i = 0; i < desc_m; ++i) {
			int thread_id = nv_omp_thread_id();
			int label;
			
			nv_vector_normalize(desc_vec, i);
			
			if (NV_MAT_V(key_vec, i, NV_KEYPOINT_RESPONSE_IDX) > 0.0f) {
				label = nv_kmeans_tree_predict_label_ex(m_posi, m_posi->height, desc_vec, i, HKM_NN);
			} else {
				label = nv_kmeans_tree_predict_label_ex(m_nega, m_nega->height, desc_vec, i, HKM_NN) + POSI_N;
			}
			if (NV_MAT_V(m_idf, 0, label) > 0.0f) {
				tmp[thread_id].insert(label);
			}
		}
		for (i = 0; i < procs; ++i) {
			tmp2.insert(tmp[i].begin(), tmp[i].end());
		}
		std::copy(tmp2.begin(), tmp2.end(), std::back_inserter(vec));
		
		delete [] tmp;
		nv_matrix_free(&desc_vec);
		nv_matrix_free(&key_vec);
	}
public:
	nv_bovw_ctx(): m_posi(0), m_nega(0), m_idf(0), m_ctx(0) {}
	~nv_bovw_ctx() { close(); }
	
	int
	open(void)
	{
		char name[8192];
		char path[8192];
		
		close();
		
		nv_snprintf(name, sizeof(name) - 1, "nv_bovw%dk_posi.kmtb", BIT / 1024);
		default_data_path(path, sizeof(path) - 1, name);
		m_posi = nv_load_kmeans_tree_bin(path);
		
		nv_snprintf(name, sizeof(name) - 1, "nv_bovw%dk_nega.kmtb", BIT / 1024);
		default_data_path(path, sizeof(path) - 1, name);
		m_nega = nv_load_kmeans_tree_bin(path);
		
		nv_snprintf(name, sizeof(name) - 1, "nv_bovw%dk_idf.matb", BIT / 1024);
		default_data_path(path, sizeof(path) - 1, name);
		m_idf = nv_load_matrix_bin(path);
		
		init_ctx();
		
		if (m_posi == NULL || m_nega == NULL || m_idf == NULL || m_ctx == NULL) {
			close();
			return -1;
		}
		return 0;
	}
	
	int 	
	open(const char *posi_file,
		 const char *nega_file,
		 const char *idf_file)
	{
		close();
		
		m_posi = nv_load_kmeans_tree_bin(posi_file);
		m_nega = nv_load_kmeans_tree_bin(nega_file);
		m_idf = nv_load_matrix_bin(idf_file);

		init_ctx();
		
		if (m_posi == NULL || m_nega == NULL || m_idf == NULL || m_ctx == NULL) {
			close();
			return -1;
		}
		return 0;
	}
	
	void
	close(void)
	{
		nv_kmeans_tree_free(&m_posi);
		nv_kmeans_tree_free(&m_nega);
		nv_matrix_free(&m_idf);
		nv_keypoint_ctx_free(&m_ctx);
	}

	int
	extract(sparse_t &vec,
			const nv_matrix_t *image)
	{
		float scale = IMG_SIZE() / (float)NV_MAX(image->rows, image->cols);
		nv_matrix_t *resize = nv_matrix3d_alloc(3, (int)(image->rows * scale),
												(int)(image->cols * scale));
		nv_matrix_t *gray = nv_matrix3d_alloc(1, resize->rows, resize->cols);
		nv_matrix_t *smooth = nv_matrix3d_alloc(1, resize->rows, resize->cols);
		
		vec.clear();
		
		nv_resize(resize, image);
		nv_gray(gray, resize);
		nv_gaussian5x5(smooth, 0, gray, 0);
		extract_sparse_feature(vec, smooth);
		
		nv_matrix_free(&gray);
		nv_matrix_free(&resize);
		nv_matrix_free(&smooth);
		
		return 0;
	}
	
	int
	extract(sparse_t &vec,
			const void *data, size_t len)
	{
		nv_matrix_t *image = nv_decode_image(data, len);
	
		if (image == NULL) {
			return -1;
		}
		
		extract(vec, image);
		nv_matrix_free(&image);
		
		return 0;
	}
	
	int
	extract(sparse_t &vec,
			const char *file)
	{
		nv_matrix_t *image = nv_load_image(file);
	
		if (image == NULL) {
			return -1;
		}
		
		extract(vec, image);
		nv_matrix_free(&image);
		
		return 0;
	}
	
	int
	extract(dense_t *bovw,
			const nv_matrix_t *image)
	{
		uint64_t popcnt;
		sparse_t vec;
		sparse_t::const_iterator i;
		
		memset(bovw, 0, sizeof(*bovw));
		
		extract(vec, image);
		for (i = vec.begin(); i != vec.end(); ++i) {
			uint32_t a = BIT_INDEX(*i);
			uint32_t b = BIT_BIT(*i);
			bovw->bovw[a] |= (1ULL << b);
		}
		popcnt = vec.size();
		if (popcnt == 0) {
			bovw->norm = FLT_MAX; // a / (norm) => 0
		} else {
			bovw->norm = sqrtf((float)popcnt);
		}
		color(&bovw->boc, image);

		return 0;
	}

	int
	extract(dense_t *bovw,
			const void *data, size_t len
		)
	{
		nv_matrix_t *image = nv_decode_image(data, len);
		
		if (image == NULL) {
			return -1;
		}
		
		extract(bovw, image);
		nv_matrix_free(&image);
		
		return 0;
	}
	
	int
	extract(dense_t *bovw,
			const char *file
		)
	{
		nv_matrix_t *image = nv_load_image(file);
	
		if (image == NULL) {
			return -1;
		}
		
		extract(bovw, image);
		nv_matrix_free(&image);
		
		return 0;
	}

	int
	search(nv_bovw_result_t *results, int k,
		   const dense_t *db, int64_t ndb,
		   const dense_t *query,
		   nv_bovw_rerank_method_t rerank_method,
		   float color_weight)
	{
		int64_t j;
		int64_t jmax;
		int_fast8_t threads = nv_omp_procs();
		topn_t *topn_first = new topn_t[threads];
		topn_t topn;
		float *cosine_min = nv_alloc_type(float, threads);
		const int k_first = NV_MAX(k * SEARCH_FIRST_SCALE, 100);
		const float bovw_weight = 1.0f - color_weight;
		std::vector<nv_bovw_result_t> topn_second;

		for (j = 0; j < threads; ++j) {
			cosine_min[j] = -FLT_MAX;
		}
		if (color_weight > 0.0f) {
#ifdef _OPENMP
#pragma omp parallel for num_threads(threads)
#endif
			for (j = 0; j < ndb; ++j) {
				int_fast8_t thread_idx = nv_omp_thread_id();
				nv_bovw_result_t new_node;
				
				new_node.similarity =
					bovw_weight * bit_cosine(query, &db[j])
					+ color_weight * color_similarity(&query->boc, &db[j].boc);
				new_node.index = j;
				
				if (topn_first[thread_idx].size() < (unsigned int)k_first) {
					topn_first[thread_idx].push(new_node);
					cosine_min[thread_idx] = topn_first[thread_idx].top().similarity;
				} else if (new_node.similarity > cosine_min[thread_idx]) {
					topn_first[thread_idx].pop();
					topn_first[thread_idx].push(new_node);
					cosine_min[thread_idx] = topn_first[thread_idx].top().similarity;
				}
			}
		} else {
#ifdef _OPENMP
#pragma omp parallel for num_threads(threads)
#endif
			for (j = 0; j < ndb; ++j) {
				int_fast8_t thread_idx = nv_omp_thread_id();
				nv_bovw_result_t new_node;
				
				new_node.similarity = bit_cosine(query, &db[j]);
				new_node.index = j;
		
				if (topn_first[thread_idx].size() < (unsigned int)k_first) {
					topn_first[thread_idx].push(new_node);
					cosine_min[thread_idx] = topn_first[thread_idx].top().similarity;
				} else if (new_node.similarity > cosine_min[thread_idx]) {
					topn_first[thread_idx].pop();
					topn_first[thread_idx].push(new_node);
					cosine_min[thread_idx] = topn_first[thread_idx].top().similarity;
				}
			}
		}
		
		for (j = 1; j < threads; ++j) {
			while (!topn_first[j].empty()) {
				topn_first[0].push(topn_first[j].top());
				topn_first[j].pop();
			}
		}
		while (topn_first[0].size() > (unsigned int)k_first) {
			topn_first[0].pop();
		}
		while (topn_first[0].size()) {
			topn_second.push_back(topn_first[0].top());
			topn_first[0].pop();
		}

		switch (rerank_method) {
		case NV_BOVW_RERANK_IDF:
		{
			nv_matrix_t *query_vec = nv_matrix_alloc(BIT, 1);

			tfidf(query_vec, 0, query);
#ifdef _OPENMP
#pragma omp parallel for
#endif
			for (j = 0; j < (int64_t)topn_second.size(); ++j) {
				nv_matrix_t *data_vec = nv_matrix_alloc(BIT, 1);
				nv_bovw_result_t &ret = topn_second[(size_t)j];

				tfidf(data_vec, 0, &db[ret.index]);
				ret.similarity = (1.0f - color_weight) * nv_vector_dot(query_vec, 0, data_vec, 0)
					+ color_weight * color_similarity(&query->boc, &db[ret.index].boc);
				nv_matrix_free(&data_vec);
			}
			
			cosine_min[0] = -FLT_MAX;
			for (j = 0; j < (int64_t)topn_second.size(); ++j) {
				nv_bovw_result_t ret = topn_second[(size_t)j];
				if (topn.size() < (unsigned int)k) {
					topn.push(ret);
					cosine_min[0] = topn.top().similarity;
				} else if (ret.similarity > cosine_min[0]) {
					topn.pop();
					topn.push(ret);
					cosine_min[0] = topn.top().similarity;
				}
			}
			nv_matrix_free(&query_vec);
		} break;
		default:
			cosine_min[0] = -FLT_MAX;
			for (j = 0; j < (int)topn_second.size(); ++j) {
				nv_bovw_result_t &ret = topn_second[(size_t)j];
				if (topn.size() < (unsigned int)k) {
					topn.push(ret);
					cosine_min[0] = topn.top().similarity;
				} else if (ret.similarity > cosine_min[0]) {
					topn.pop();
					topn.push(ret);
					cosine_min[0] = topn.top().similarity;
				}
			}
			break;
		}
		
		j = 0; jmax = NV_MIN(k, ndb);
		for (j = jmax - 1; j >= 0; --j) {
			results[j] = topn.top();
			results[j].similarity = results[j].similarity;
			topn.pop();
		}
		
		delete [] topn_first;
		nv_free(cosine_min);
		
		return (int)jmax;
	}

	static void
	decode(nv_matrix_t *vec, int vec_j,
		   const dense_t *bovw)
	{
		int i;

		NV_ASSERT(vec->n == N);
		nv_vector_zero(vec, vec_j);
		
		for (i = 0; i < INT_BLOCKS; ++i) {
			int i64 = i * 64;
			int j;
			uint64_t mask = 1;
			for (j = 0; j < 64; ++j) {
				NV_MAT_V(vec, vec_j, i64 + j) = (bovw->bovw[i] & mask) ? 1.0f : 0.0f;
				mask <<= 1;
			}
		}
	}
	
	void
	convert(sparse_t &vec, const dense_t *bovw)
	{
		int i;
		
		vec.clear();
		vec.reserve(KEYPOINT_M);
		
		for (i = 0; i < INT_BLOCKS; ++i) {
			int i64 = i * 64;
			int j;
			
			if (bovw->bovw[i] != 0) {
				uint64_t mask = 1;
				uint64_t v = bovw->bovw[i];
				for (j = 0; j < 64; ++j) {
					if ((v & mask) != 0) {
						vec.push_back((uint32_t)(i64 + j));
					}
					mask <<= 1;
				}
			}
		}
	}
	
	void
	convert(dense_t *bovw, const sparse_t &vec)
	{
		sparse_t::const_iterator i;
		
		memset(bovw->bovw, 0, sizeof(bovw->bovw));
		for (i = vec.begin(); i != vec.end(); ++i) {
			uint32_t a = BIT_INDEX(*i);
			uint32_t b = BIT_BIT(*i);
			bovw->bovw[a] |= (1ULL << b);
		}
		return 0;
	}

	void
	serialize(char *s, const dense_t *bovw)
	{
		int i;
		static const char *digits = "0123456789abcdef";
		
		for (i = 0; i < INT_BLOCKS; ++i) {
			int j;
			for (j = 0; j < 16; ++j) {
				s[i * 16 + j] = digits[((bovw->bovw[i] >> ((15 - j) * 4)) & 0xf)];
			}
		}
		s[INT_BLOCKS * 16] = '_';
		color_serialize(&s[1 + INT_BLOCKS * 16], &bovw->boc);
		s[SERIALIZE_LEN-1] = '\0';
	}

	void
	serialize(std::string &hash_string, const sparse_t *hash)
	{
		sparse_t::const_iterator it;
		
		hash_string.clear();
		
		for (it = hash->begin(); it != hash->end(); ++it) {
			char str[64];
			
			nv_snprintf(str, sizeof(str)-1, "%x", *it);
			if (it != hash->begin()) {
				hash_string += " ";
			}
			hash_string += str;
		}
	}
	
	int
	deserialize(dense_t *bovw, const char *s)
	{
		int i, j;
		uint64_t popcnt;

		if (strlen(s) != SERIALIZE_LEN - 1) {
			return -1;
		}
		for (i = 0; i < INT_BLOCKS; ++i) {
			bovw->bovw[i] = 0;
			for (j = 0; j < 16; ++j) {
				char c = s[i * 16 + j];
				if ('0' <= c && c <= '9') {
					bovw->bovw[i] |= ((uint64_t)(c - '0') << ((15 - j) * 4));
				} else if ('a' <= c && c <= 'f') {
					bovw->bovw[i] |= ((uint64_t)(10 + c - 'a') << ((15 - j) * 4));
				} else if ('A' <= c && c <= 'F') {
					bovw->bovw[i] |= ((uint64_t)(10 + c - 'A') << ((15 - j) * 4));
				} else {
					/* feature string malformed */
					return -1;
				}
			}
		}
		popcnt = 0;
		for (j = 0; j < INT_BLOCKS; ++j) {
			popcnt += NV_POPCNT_U64(bovw->bovw[j]);
		}
		if (popcnt == 0) {
			bovw->norm = FLT_MAX; // a / (norm) => 0
		} else {
			bovw->norm = sqrtf((float)popcnt);
		}
		if (s[INT_BLOCKS * 16] != '_') {
			return -1;
		}
		if (color_deserialize(&bovw->boc,
							  &s[1 + INT_BLOCKS * 16]) != 0)
		{
			return -1;
		}
		
		return 0;
	}

	int
	deserialize(sparse_t *hash,
				const char *s)
	{
		char *errp = NULL;
		
		hash->clear();
		hash->reserve(KEYPOINT_M);
		
		while (!(s == NULL || *s == '\0')) {
			uint32_t h;
			
			if (isspace(*s)) {
				++s;
				continue;
			}
			
			errp = NULL;
			h = (uint32_t)strtoul(s, &errp, 16);
			if (errp != s) {
				if (NV_MAT_V(m_idf, 0, h) > 0.0f) {
					hash->push_back(h);
				}
			}
			s = errp;
		}
		
		return 0;
	}

	void
	tfidf(nv_matrix_t *idf, int idf_j,
		  const dense_t *bovw)
	{
		int i;

		for (i = 0; i < INT_BLOCKS; ++i) {
			int j;
			int i64 = i * 64;
			uint64_t mask = 1;
			
			for (j = 0; j < 64; ++j) {
				if (bovw->bovw[i] & mask) {
					NV_MAT_V(idf, idf_j, i64 + j) = NV_MAT_V(m_idf, 0, i64 + j);
				} else {
					NV_MAT_V(idf, idf_j, i64 + j) = 0.0f;
				}
				mask <<= 1;
			}
		}
		nv_vector_normalize(idf, idf_j);
	}
	
	float similarity(
		const dense_t *bovw1, const dense_t *bovw2,
		nv_bovw_rerank_method_t rerank_method,
		float color_weight)
	{
		float similarity;
	
		switch (rerank_method) {
		case NV_BOVW_RERANK_IDF:
			similarity = similarity_idf(bovw1, bovw2, color_weight);
			break;
		default:
			similarity = (1.0f - color_weight) * bit_cosine(bovw1, bovw2)
				+ color_weight * color_similarity(&bovw1->boc, &bovw2->boc);
			break;
		}
		
		return similarity;
	}
	
	float
	similarity_idf(
		const dense_t *bovw1,
		const dense_t *bovw2,
		float color_weight)
	{
		float similarity;
		nv_matrix_t *idf1 = nv_matrix_alloc(N, 1);
		nv_matrix_t *idf2 = nv_matrix_alloc(N, 1);

		tfidf(idf1, 0, bovw1);
		tfidf(idf2, 0, bovw2);
		
		similarity = (1.0f - color_weight) * nv_vector_dot(idf1, 0, idf2, 0)
			+ color_weight * color_similarity(&bovw1->boc, &bovw2->boc);
		
		nv_matrix_free(&idf1);
		nv_matrix_free(&idf2);
		
		return similarity;
	}

	void
	update_idf(nv_matrix_t *freq, int freq_j,
			   uint64_t count,
			   uint64_t stopword_th = 0)
	{
		int i;
		
		for (i = 0; i < m_idf->n; ++i) {
			if (stopword_th != 0 && stopword_th < NV_MAT_V(freq, freq_j, i)) {
				NV_MAT_V(m_idf, 0, i) = 0.0f;
			} else {
				NV_MAT_V(m_idf, 0, i) = logf((count + 0.5f) / (NV_MAT_V(freq, freq_j, i) + 0.5f)) / logf(2.0f) + 1.0f;
			}
		}
	}

	float idf(int i)
	{
		NV_ASSERT(i < BIT);
		return NV_MAT_V(m_idf, 0, i);
	}
	
	nv_matrix_t *idf_vector(void)
	{
		return m_idf;
	}
};

#endif
