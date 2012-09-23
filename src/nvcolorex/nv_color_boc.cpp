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

#include "nv_core.h"
#include "nv_ip.h"
#include "nv_io.h"
#include "nv_ml.h"
#include "nv_color_boc.h"
#include <vector>
#include <queue>

extern "C" nv_matrix_t nv_color_boc_static;

#define NV_COLOR_BOC_COLOR  64
#define NV_COLOR_BOC_IMG_SIZE 512.0f

static inline float
similarity_ex(const nv_color_boc_t *a,
		 const nv_color_boc_t *b)
{
	uint64_t popcnt =
		NV_POPCNT_U64(a->color[0] & b->color[0]) +
		NV_POPCNT_U64(a->color[1] & b->color[1]) +
		NV_POPCNT_U64(a->color[2] & b->color[2]) +
		NV_POPCNT_U64(a->color[3] & b->color[3]);
		
	return (float)popcnt / (a->norm * b->norm);
}

float
nv_color_boc_similarity(const nv_color_boc_t *a,
				   const nv_color_boc_t *b)
{
	return similarity_ex(a, b);
}

char *
nv_color_boc_serialize(const nv_color_boc_t *boc)
{
	static const char *digits = "0123456789abcdef";
	char *s = nv_alloc_type(char, (NV_COLOR_BOC_INT_BLOCKS * 16) + 1);
	int si = 0;
	int i;
	
	for (i = 0; i < NV_COLOR_BOC_INT_BLOCKS; ++i) {
		int j;
		for (j = 0; j < 16; ++j) {
			s[si++] = digits[((boc->color[i] >> ((15 - j) * 4)) & 0xf)];
		}
	}
	s[si] = '\0';
	
	return s;
}

int
nv_color_boc_deserialize(nv_color_boc_t *boc, const char *s)
{
	int i, j;
	uint64_t popcnt = 0;
	
	for (i = 0; i < NV_COLOR_BOC_INT_BLOCKS; ++i) {
		boc->color[i] = 0;
		for (j = 0; j < 16; ++j) {
			char c = s[i * 16 + j];
			if ('0' <= c && c <= '9') {
				boc->color[i] |= ((uint64_t)(c - '0') << ((15 - j) * 4));
			} else if ('a' <= c && c <= 'f') {
				boc->color[i] |= ((uint64_t)(10 + c - 'a') << ((15 - j) * 4));
			} else if ('A' <= c && c <= 'F') {
				boc->color[i] |= ((uint64_t)(10 + c - 'A') << ((15 - j) * 4));
			} else {
				/* feature string malformed */
				return -1;
			}
		}
	}
	for (i = 0; i < NV_COLOR_BOC_INT_BLOCKS; ++i) {
		popcnt += NV_POPCNT_U64(boc->color[i]);
	}
	if (popcnt == 0) {
		boc->norm = FLT_MAX;
	} else {
		boc->norm = sqrtf((float)popcnt);
	}
	
	return 0;
}

void
nv_color_boc(nv_color_boc_t *boc,
			 const nv_matrix_t *image)
{
	static const uint64_t index_table[4] = {
		1ULL, 3ULL, 7ULL, 15ULL
	};
	const float step = 2.0f;
	float cell_width = NV_MAX(((float)image->cols / NV_COLOR_BOC_IMG_SIZE * step), 1.0f);
	float cell_height = NV_MAX(((float)image->rows / NV_COLOR_BOC_IMG_SIZE * step), 1.0f);
	
	nv_matrix_t *hist = nv_matrix_alloc(NV_COLOR_BOC_COLOR, 1);
	int sample_rows = NV_FLOOR_INT(image->rows / cell_height)-1;
	int sample_cols = NV_FLOOR_INT(image->cols / cell_width)-1;
	int i, y;
	float hist_scale;
	uint64_t popcnt;
	int threads = nv_omp_procs();
	const int vq_len = sample_rows * sample_cols;
	int *vq = nv_alloc_type(int, vq_len);
	float max_v;
	nv_matrix_t *hsv = nv_matrix_alloc(3, threads);
	
	nv_matrix_zero(hist);
#ifdef _OPENMP
#pragma omp parallel for num_threads(threads)
#endif
	for (y = 0; y < sample_rows; ++y) {
		int thread_id = nv_omp_thread_id();
		int x;
		int yi = (int)(y * cell_height);
		for (x = 0; x < sample_cols; ++x) {
			int j = NV_MAT_M(image, yi, (int)(x * cell_width));
			nv_color_bgr2hsv_scalar(hsv, thread_id, image, j);
			vq[y * sample_cols + x] = nv_nn(&nv_color_boc_static, hsv, thread_id);
		}
	}
	for (i = 0; i < vq_len; ++i) {
		NV_MAT_V(hist, 0, vq[i]) += 1.0f;
	}
	max_v = nv_vector_max(hist, 0);
	hist_scale = 1.0f / max_v;
	
	memset(boc->color, 0, sizeof(boc->color));
	for (i = 0; i < hist->n; ++i) {
		float v = (NV_MAT_V(hist, 0, i) * hist_scale) * 3.9f;
		if (v > 0.01f) {
			int bit_index = i * 4;
			int int_index = bit_index / 64;
			uint64_t int_bit_index = bit_index % 64;
			boc->color[int_index] |= (index_table[(int)v] << int_bit_index);
		}
	}
	
	popcnt = 0;
	for (i = 0; i < NV_COLOR_BOC_INT_BLOCKS; ++i) {
		popcnt += NV_POPCNT_U64(boc->color[i]);
	}
	if (popcnt == 0) {
		boc->norm = FLT_MAX; // a / (norm) => 0
	} else {
		boc->norm = sqrtf((float)popcnt);
	}
	
	nv_matrix_free(&hist);
	nv_matrix_free(&hsv);
	nv_free(vq);
}

static int nv_color_sboc_norm_e[4] = { 9 * 4, 13 * 4, 15 * 4, 16 * 4};
static float nv_color_sboc_w[4] = { 0.4f, 0.25f, 0.15f, 0.2f };
#define NV_COLOR_SBOC_LEVEL (int)(sizeof(nv_color_sboc_norm_e) / sizeof(int))

static inline float
similarity_ex(const nv_color_sboc_t *a,
		 const nv_color_sboc_t *b)
{
	int i, j;
	float similarity = 0.0f;
	
	for (j = 0, i = 0; j < NV_COLOR_SBOC_LEVEL; ++j) {
		uint64_t popcnt = 0;
		for (; i < nv_color_sboc_norm_e[j]; ++i) {
			popcnt += NV_POPCNT_U64(a->color[i] & b->color[i]);
		}
		if (popcnt > 0) {
			similarity += ((float)popcnt / (a->norm[j] * b->norm[j])) * nv_color_sboc_w[j];
		}
	}
	
	return similarity;
}

float
nv_color_sboc_similarity(const nv_color_sboc_t *a,
					const nv_color_sboc_t *b)
{
	return similarity_ex(a, b);
}

void
nv_color_sboc(nv_color_sboc_t *boc, const nv_matrix_t *image)
{
	static const uint64_t index_table[4] = { 1ULL, 3ULL, 7ULL, 15ULL };
	const float step = 2.0f;
	float cell_width = NV_MAX(((float)image->cols / NV_COLOR_BOC_IMG_SIZE * step), 1.0f);
	float cell_height = NV_MAX(((float)image->rows / NV_COLOR_BOC_IMG_SIZE * step), 1.0f);
	nv_matrix_t *hist = nv_matrix_alloc(NV_COLOR_BOC_COLOR, NV_COLOR_SBOC_INT_BLOCKS / 4);
	int sample_rows = NV_FLOOR_INT(image->rows / cell_height)-1;
	int sample_cols = NV_FLOOR_INT(image->cols / cell_width)-1;
	int j, i, y;
	const int vq_len = sample_rows * sample_cols;
	int *vq = nv_alloc_type(int, vq_len);
	int *vq_lr3 = nv_alloc_type(int, vq_len);
	int *vq_tb4 = nv_alloc_type(int, vq_len);
	int *vq_tb3 = nv_alloc_type(int, vq_len);
	int *vq_tb2 = nv_alloc_type(int, vq_len);
	int hhr = sample_rows / 3 + 1;
	int hhc = sample_cols / 3 + 1;
	int hr = sample_rows / 2 + 1;
	int hhhr = sample_rows / 4 + 1;
	int threads = nv_omp_procs();
	nv_matrix_t *hsv = nv_matrix_alloc(3, threads);
	
#ifdef _OPENMP
#pragma omp parallel for num_threads(threads)
#endif
	for (y = 0; y < sample_rows; ++y) {
		int thread_id = nv_omp_thread_id();
		int x;
		int yi = (int)(y * cell_height);
		int r4 = y / hhhr;
		int r3 = y / hhr;
		int r2 = y / hr;
		
		for (x = 0; x < sample_cols; ++x) {
			int j = NV_MAT_M(image, yi, (int)(x * cell_width));
			int c3 = x / hhc;
			int idx = y * sample_cols + x;
			
			nv_color_bgr2hsv_scalar(hsv, thread_id, image, j);
			vq[idx] = nv_nn(&nv_color_boc_static, hsv, thread_id);
			vq_tb4[idx] = r4;
			vq_tb3[idx] = r3;
			vq_tb2[idx] = r2;
			vq_lr3[idx] = c3;
		}
	}
	
	nv_matrix_zero(hist);
	for (i = 0; i < vq_len; ++i) {
		NV_MAT_V(hist, vq_tb3[i] * 3 + vq_lr3[i], vq[i]) += 1.0f;
		NV_MAT_V(hist, 9 + vq_tb4[i], vq[i]) += 1.0f;
		NV_MAT_V(hist, 13 + vq_tb2[i], vq[i]) += 1.0f;
		NV_MAT_V(hist, 15, vq[i]) += 1.0f;
	}
	
	memset(boc->color, 0, sizeof(boc->color));
	for (j = 0; j < hist->m; ++j) {
		float max_v = nv_vector_max(hist, j);
		float hist_scale = 1.0f / max_v;
		int base = j * 4;
		
		for (i = 0; i < hist->n; ++i) {
			float v = (NV_MAT_V(hist, j, i) * hist_scale) * 3.99f;
			if (v > 0.01f) {
				int bit_index = i * 4;
				int int_index = bit_index / 64;
				uint64_t int_bit_index = bit_index % 64;
				boc->color[base + int_index] |= (index_table[(int)v] << int_bit_index);
			}
		}
	}
	for (j = 0, i = 0; j < NV_COLOR_SBOC_LEVEL; ++j) {
		uint64_t popcnt = 0;
		for (; i < nv_color_sboc_norm_e[j]; ++i) {
			popcnt += NV_POPCNT_U64(boc->color[i]);
		}
		if (popcnt == 0) {
			boc->norm[j] = FLT_MAX; // a / (norm) => 0
		} else {
			boc->norm[j] = sqrtf((float)popcnt);
		}
	}
	
	nv_matrix_free(&hist);
	nv_matrix_free(&hsv);
	nv_free(vq);
	nv_free(vq_lr3);
	nv_free(vq_tb4);
	nv_free(vq_tb3);
	nv_free(vq_tb2);
}

int
nv_color_sboc_file(nv_color_sboc_t *boc, const char *file)
{
	nv_matrix_t *image = nv_load_image(file);
	
	if (image == NULL) {
		return -1;
	}
	
	nv_color_sboc(boc, image);
	nv_matrix_free(&image);
	
	return 0;
}

int
nv_color_sboc_data(nv_color_sboc_t *boc, const void *data, size_t data_len)
{
	nv_matrix_t *image = nv_decode_image(data, data_len);
	
	if (image == NULL) {
		return -1;
	}
	
	nv_color_sboc(boc, image);
	nv_matrix_free(&image);
	
	return 0;
}

char *
nv_color_sboc_serialize(const nv_color_sboc_t *boc)
{
	static const char *digits = "0123456789abcdef";
	char *s = nv_alloc_type(char, (NV_COLOR_SBOC_INT_BLOCKS * 16) + 1);
	int si = 0;
	int i;
	
	for (i = 0; i < NV_COLOR_SBOC_INT_BLOCKS; ++i) {
		int j;
		for (j = 0; j < 16; ++j) {			
			s[si++] = digits[((boc->color[i] >> ((15 - j) * 4)) & 0xf)];
		}
	}
	s[si] = '\0';
	
	return s;
}

int
nv_color_sboc_deserialize(nv_color_sboc_t *boc, const char *s)
{
	int i, j;
	
	if (strlen(s) != (NV_COLOR_SBOC_INT_BLOCKS * 16)) {
		return -1;
	}
	for (i = 0; i < NV_COLOR_SBOC_INT_BLOCKS; ++i) {
		boc->color[i] = 0;
		for (j = 0; j < 16; ++j) {
			char c = s[i * 16 + j];
			if ('0' <= c && c <= '9') {
				boc->color[i] |= ((uint64_t)(c - '0') << ((15 - j) * 4));
			} else if ('a' <= c && c <= 'f') {
				boc->color[i] |= ((uint64_t)(10 + c - 'a') << ((15 - j) * 4));
			} else if ('A' <= c && c <= 'F') {
				boc->color[i] |= ((uint64_t)(10 + c - 'A') << ((15 - j) * 4));
			} else {
				/* feature string malformed */
				return -1;
			}
		}
	}
	for (j = 0, i = 0; j < NV_COLOR_SBOC_LEVEL; ++j) {
		uint64_t popcnt = 0;
		for (; i < nv_color_sboc_norm_e[j]; ++i) {
			popcnt += NV_POPCNT_U64(boc->color[i]);
		}
		if (popcnt == 0) {
			boc->norm[j] = FLT_MAX; // a / (norm) => 0
		} else {
			boc->norm[j] = sqrtf((float)popcnt);
		}
	}

	return 0;
}

static inline bool
operator>(const nv_color_boc_result_t &d1,
		  const nv_color_boc_result_t &d2)
{
	if (d1.cosine > d2.cosine) {
		return true;
	}
	return false;
}

typedef std::priority_queue<nv_color_boc_result_t, std::vector<nv_color_boc_result_t>,
							std::greater<std::vector<nv_color_boc_result_t>::value_type> > nv_color_boc_topn_t;

template<typename T> static int
search_ex(nv_color_boc_result_t *results, int k,
		  const T *db, int64_t ndb,
		  const T *query)
{
	int64_t j;
	int64_t jmax;
	int_fast8_t threads = nv_omp_procs();
	nv_color_boc_topn_t topn;
	float *cosine_min = nv_alloc_type(float, threads);
	nv_color_boc_topn_t *topn_temp = new nv_color_boc_topn_t[threads];
	
	for (j = 0; j < threads; ++j) {
		cosine_min[j] = -FLT_MAX;
	}
	
#ifdef _OPENMP
#pragma omp parallel for num_threads(threads)
#endif
	for (j = 0; j < ndb; ++j) {
		int_fast8_t thread_idx = nv_omp_thread_id();
		nv_color_boc_result_t new_node;
		
		/* bit cosine */
		new_node.cosine = similarity_ex(query, &db[j]);
		new_node.index = j;
		
		if (topn_temp[thread_idx].size() < (unsigned int)k) {
			topn_temp[thread_idx].push(new_node);
			cosine_min[thread_idx] = topn_temp[thread_idx].top().cosine;
		} else if (new_node.cosine > cosine_min[thread_idx]) {
			topn_temp[thread_idx].pop();
			topn_temp[thread_idx].push(new_node);
			cosine_min[thread_idx] = topn_temp[thread_idx].top().cosine;
		}
	}
	
	for (j = 0; j < threads; ++j) {
		while (!topn_temp[j].empty()) {
			topn.push(topn_temp[j].top());
			topn_temp[j].pop();
		}
	}
	while (topn.size() > (unsigned int)k) {
		topn.pop();
	}
	jmax = NV_MIN(k, ndb);
	for (j = jmax - 1; j >= 0; --j) {
		results[j] = topn.top();
		topn.pop();
	}
	
	delete [] topn_temp;
	nv_free(cosine_min);
	
	return (int)jmax;
}


int
nv_color_sboc_search(nv_color_boc_result_t *results, int k,
					 const nv_color_sboc_t *db, int64_t ndb,
					 const nv_color_sboc_t *query)
{
	return search_ex<nv_color_sboc_t>(results, k, db, ndb, query);
}

int
nv_color_boc_search(nv_color_boc_result_t *results, int k,
					const nv_color_boc_t *db, int64_t ndb,
					const nv_color_boc_t *query)
{
	return search_ex<nv_color_boc_t>(results, k, db, ndb, query);
}
