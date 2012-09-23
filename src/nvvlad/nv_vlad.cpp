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

/* VLAD Full nv_keypoint version, K=512, D=36864
 *
 *   H. Jégou, M. Douze, C. Schmid, and P. Pérez, 
 *   Aggregating local descriptors into a compact image representation, CVPR 2010.
 *   http://lear.inrialpes.fr/pubs/2010/JDSP10/
 */
#include "nv_core.h"
#include "nv_vlad.h"
#include "nv_ml.h"
#include "nv_ip.h"
#include "nv_io.h"
#include "nv_num.h"
#include <vector>
#include <queue>
#include <sstream>

extern "C" nv_matrix_t nv_vlad_posi_static;
extern "C" nv_matrix_t nv_vlad_nega_static;

static nv_matrix_t *nv_vlad_simhash_seed = NULL;

#define NV_VLAD_IMG_SIZE 512.0f
#define NV_VLAD_KEYPOINTS 2000
#define NV_VLAD_KP 256
#define NV_VLAD_K  512

#define  NV_VLAD_BIT_INDEX(n) (n / 64)
#define  NV_VLAD_BIT_BIT(n) (n % 64)

static inline bool
operator>(const nv_vlad_result_t &d1,
		  const nv_vlad_result_t &d2)
{
	if (d1.similarity > d2.similarity) {
		return true;
	}
	return false;
}

void
nv_vlad_simhash_init(void)
{
	if (nv_vlad_simhash_seed == NULL) {
		int j;
		nv_vlad_simhash_seed = nv_matrix_alloc(NV_VLAD_K * NV_KEYPOINT_DESC_N,
											   NV_VLAD_SIMHASH_INT_BLOCKS * 64);
		for (j = 0; j < nv_vlad_simhash_seed->m; ++j) {
			nv_vector_nrand_ex(nv_vlad_simhash_seed, j,
							   0.0f, 1.0f, NV_PRIME_TABLE(j + 128));
		}
	}
}

nv_vlad_t *
nv_vlad_alloc(void)
{
	nv_vlad_t *vlad = nv_alloc_type(nv_vlad_t, 1);
	static nv_keypoint_param_t param = {
		NV_KEYPOINT_THRESH,
		NV_KEYPOINT_EDGE_THRESH,
		4.0,
		19,
		NV_KEYPOINT_NN,
		NV_KEYPOINT_DETECTOR_STAR,
		NV_KEYPOINT_DESCRIPTOR_GRADIENT_HISTOGRAM
	};
	memset(vlad, 0, sizeof(*vlad));
	vlad->vec = nv_matrix_alloc(NV_KEYPOINT_DESC_N * NV_VLAD_K, 1);
	vlad->ctx = nv_keypoint_ctx_alloc(&param);
	
	return vlad;
}

static nv_vlad_t *
nv_vlad_alloc_from(nv_matrix_t *vec)
{
	nv_vlad_t *vlad = nv_alloc_type(nv_vlad_t, 1);
	NV_ASSERT(NV_KEYPOINT_DESC_N * NV_VLAD_K == vec->n);
	
	memset(vlad, 0, sizeof(*vlad));
	vlad->vec = vec;
	
	return vlad;
}

void
nv_vlad_free(nv_vlad_t **vlad)
{
	if (vlad && *vlad) {
		nv_matrix_free(&(*vlad)->vec);
		nv_keypoint_ctx_free(&(*vlad)->ctx);
		nv_free(*vlad);
		*vlad = NULL;
	}
}

void
nv_vlad_feature_vector(nv_matrix_t *vec,
					   nv_matrix_t *key_vec,
					   nv_matrix_t *desc_vec,
					   int desc_m
	)
{
	int i;
	int procs = nv_omp_procs();
	nv_matrix_t *vec_tmp = nv_matrix_alloc(vec->n, procs);
	
	nv_matrix_zero(vec_tmp);
	nv_matrix_zero(vec);

#ifdef _OPENMP
#pragma omp parallel for num_threads(procs)
#endif	
	for (i = 0; i < desc_m; ++i) {
		int j;
		int thread_id = nv_omp_thread_id();
		nv_vector_normalize(desc_vec, i);
		
		if (NV_MAT_V(key_vec, i, NV_KEYPOINT_RESPONSE_IDX) > 0.0f) {
			int label = nv_nn(&nv_vlad_posi_static, desc_vec, i);
			
			for (j = 0; j < nv_vlad_posi_static.n; ++j) {
				NV_MAT_V(vec_tmp, thread_id, label * NV_KEYPOINT_DESC_N + j) +=
					NV_MAT_V(desc_vec, i, j) - NV_MAT_V(&nv_vlad_posi_static, label, j);
			}
		} else {
			int label = nv_nn(&nv_vlad_nega_static, desc_vec, i);
			int vl = (NV_VLAD_KP + label) * NV_KEYPOINT_DESC_N;
			for (j = 0; j < nv_vlad_nega_static.n; ++j) {
				NV_MAT_V(vec_tmp, thread_id, (vl + j)) +=
					NV_MAT_V(desc_vec, i, j) - NV_MAT_V(&nv_vlad_nega_static, label, j);
			}
		}
	}
	for (i = 0; i < procs; ++i) {
		nv_vector_add(vec, 0, vec, 0, vec_tmp, i);
	}
	nv_vector_normalize(vec, 0);

	nv_matrix_free(&vec_tmp);
}

void
nv_vlad(nv_vlad_t *vlad,
		const nv_matrix_t *image)
{
	int desc_m;
	nv_matrix_t *key_vec = nv_matrix_alloc(NV_KEYPOINT_KEYPOINT_N, NV_VLAD_KEYPOINTS);
	nv_matrix_t *desc_vec = nv_matrix_alloc(NV_KEYPOINT_DESC_N, NV_VLAD_KEYPOINTS);
	float scale = NV_VLAD_IMG_SIZE / (float)NV_MAX(image->rows, image->cols);
	nv_matrix_t *resize = nv_matrix3d_alloc(3, (int)(image->rows * scale),
											(int)(image->cols * scale));
	nv_matrix_t *gray = nv_matrix3d_alloc(1, resize->rows, resize->cols);
	nv_matrix_t *smooth = nv_matrix3d_alloc(1, resize->rows, resize->cols);
	
	nv_resize(resize, image);
	nv_gray(gray, resize);
	nv_gaussian5x5(smooth, 0, gray, 0);
	
	nv_matrix_zero(desc_vec);
	nv_matrix_zero(key_vec);
	
	desc_m = nv_keypoint_ex(vlad->ctx, key_vec, desc_vec, smooth, 0);
	nv_vlad_feature_vector(vlad->vec, key_vec, desc_vec, desc_m);
	
	nv_matrix_free(&gray);
	nv_matrix_free(&resize);
	nv_matrix_free(&smooth);
	nv_matrix_free(&key_vec);
	nv_matrix_free(&desc_vec);
}

int
nv_vlad_file(nv_vlad_t *vlad,
			 const char *image_filename
	)
{
	nv_matrix_t *image = nv_load_image(image_filename);
	 
	if (image == NULL) {
		return -1;
	}
	
	nv_vlad(vlad, image);
	nv_matrix_free(&image);
	
	return 0;
}
int
nv_vlad_data(nv_vlad_t *vlad,
			 const void *image_data, size_t image_data_len
	)
{
	nv_matrix_t *image = nv_decode_image(image_data, image_data_len);
	
	if (image == NULL) {
		return -1;
	}
	
	nv_vlad(vlad, image);
	
	nv_matrix_free(&image);
	
	return 0;
}

float nv_vlad_similarity(const nv_vlad_t *vlad1,
						 const nv_vlad_t *vlad2)
{
	float vlad_similarity = 0.0f;
	float similarity;

	vlad_similarity = nv_vector_dot(vlad1->vec, 0, vlad2->vec, 0);
	if (vlad_similarity < 0.0f) {
		vlad_similarity = 0.0f;
	}
	similarity = vlad_similarity;
	
	return similarity;
}

float nv_vlad_idf_similarity(
	nv_vlad_t *vlad1, nv_vlad_t *vlad2,
	nv_matrix_t *idf)
{
	nv_matrix_t *w = nv_matrix_alloc(vlad1->vec->n, 1);
	nv_matrix_t *v1 = nv_matrix_alloc(vlad1->vec->n, 1);
	nv_matrix_t *v2 = nv_matrix_alloc(vlad1->vec->n, 1);
	float vlad_similarity;
	float similarity;
	int i, j;

	for (i = 0; i < NV_VLAD_K; ++i) {
		for (j = 0; j < NV_KEYPOINT_DESC_N; ++j) {
			NV_MAT_V(w, 0, i * NV_KEYPOINT_DESC_N + j) = NV_MAT_V(idf, 0, i);
		}
	}
	nv_vector_mul(v1, 0, w, 0, vlad1->vec, 0);
	nv_vector_mul(v2, 0, w, 0, vlad2->vec, 0);
	
	vlad_similarity = nv_vector_dot(v1, 0, v2, 0) /
		(nv_vector_norm(v1, 0) * nv_vector_norm(v2, 0));
	if (vlad_similarity < 0.0f) {
		vlad_similarity = 0.0f;
	}
	similarity = vlad_similarity;
	
	nv_matrix_free(&w);
	nv_matrix_free(&v1);
	nv_matrix_free(&v2);
	
	return similarity;
}

void nv_vlad_copy(nv_vlad_t *dest, const nv_vlad_t *src)
{
	nv_matrix_copy_all(dest->vec, src->vec);
}

static void
nv_vlad_idf_calc(nv_matrix_t *idf, int idf_j,
				 nv_matrix_t *freq, int freq_j,
				 uint64_t count)
{
	int i;
	
	for (i = 0; i < idf->n; ++i) {
		NV_MAT_V(idf, idf_j, i) = logf((count - NV_MAT_V(freq, freq_j, i) + 0.5f) / (NV_MAT_V(freq, freq_j, i) + 0.5f));
	}
}

void nv_vlad_simhash(nv_vlad_simhash_t *vlad, const nv_matrix_t *image)
{
	nv_vlad_t *vr = nv_vlad_alloc();
	
	memset(vlad, 0, sizeof(*vlad));
	
	nv_vlad(vr, image);
	nv_vlad_simhash_bit(vlad, vr->vec, 0);
	
	nv_vlad_free(&vr);
}

void nv_vlad_simhash_bit(nv_vlad_simhash_t *vlad, const nv_matrix_t *vlad_vec, int j)
{
	int i;
	for (i = 0; i < NV_VLAD_SIMHASH_INT_BLOCKS * 64; ++i) {
		if (nv_vector_dot(nv_vlad_simhash_seed, i, vlad_vec, j) > 0.0f) {
			int a = NV_VLAD_BIT_INDEX(i);
			int b = NV_VLAD_BIT_BIT(i);
			vlad->simhash[a] |= (1ULL << b);
		}
	}
}

int nv_vlad_simhash_data(nv_vlad_simhash_t *vlad,
						 const void *image_data, size_t image_data_len)
{
	nv_matrix_t *image = nv_decode_image(image_data, image_data_len);
	if (image == NULL) {
		return -1;
	}
	nv_vlad_simhash(vlad, image);
	nv_matrix_free(&image);
	
	return 0;
}

int nv_vlad_simhash_file(nv_vlad_simhash_t *vlad,
						 const char *image_filename)
{
	nv_matrix_t *image = nv_load_image(image_filename);
	
	if (image == NULL) {
		return -1;
	}
	nv_vlad_simhash(vlad, image);
	nv_matrix_free(&image);

	return 0;	
}

static inline float
NV_VLAD_BIT_HAMMING(const nv_vlad_simhash_t *a, const nv_vlad_simhash_t *b)
{
	uint64_t xor_count = 0;
	int i;
	for (i = 0; i < NV_VLAD_SIMHASH_INT_BLOCKS; ++i) {
		xor_count += NV_POPCNT_U64(a->simhash[i] ^ b->simhash[i]);
	}	
	return (float)((NV_VLAD_SIMHASH_INT_BLOCKS * 64) - xor_count) /
		(float)(NV_VLAD_SIMHASH_INT_BLOCKS * 64);
}

typedef std::priority_queue<nv_vlad_result_t, std::vector<nv_vlad_result_t>,
								std::greater<std::vector<nv_vlad_result_t>::value_type> > nv_vlad_topn_t;

int
nv_vlad_simhash_search(nv_vlad_result_t *results, int k,
					   const nv_vlad_simhash_t *db, int64_t ndb,
					   const nv_vlad_simhash_t *query)
{
	int64_t j;
	int64_t jmax;
	int_fast8_t threads = nv_omp_procs();
	nv_vlad_topn_t *topn_first = new nv_vlad_topn_t[threads];	
	float *similarity_min = nv_alloc_type(float, threads);
	const int k_first = k;
	
	for (j = 0; j < threads; ++j) {
		similarity_min[j] = -FLT_MAX;
	}
	
#ifdef _OPENMP
#pragma omp parallel for num_threads(threads)
#endif
	for (j = 0; j < ndb; ++j) {
		int_fast8_t thread_idx = nv_omp_thread_id();
		nv_vlad_result_t new_node;
		
		new_node.similarity = NV_VLAD_BIT_HAMMING(query, &db[j]);
		new_node.index = j;
		
		if (topn_first[thread_idx].size() < (unsigned int)k_first) {
			topn_first[thread_idx].push(new_node);
			similarity_min[thread_idx] = topn_first[thread_idx].top().similarity;
		} else if (new_node.similarity > similarity_min[thread_idx]) {
			topn_first[thread_idx].pop();
			topn_first[thread_idx].push(new_node);
			similarity_min[thread_idx] = topn_first[thread_idx].top().similarity;
		}
	}
	for (j = 1; j < threads; ++j) {
		while (!topn_first[j].empty()) {
			topn_first[0].push(topn_first[j].top());
			topn_first[j].pop();
		}
	}
	while (topn_first[0].size() > (unsigned int)k) {
		topn_first[0].pop();
	}
	
	j = 0; jmax = NV_MIN(k, ndb);
	for (j = jmax - 1; j >= 0; --j) {
		results[j] = topn_first[0].top();
		topn_first[0].pop();
	}
	
	delete [] topn_first;
	nv_free(similarity_min);

	return (int)jmax;
}

nv_vlad_t *
nv_vlad_deserialize(const char *s)
{
	nv_matrix_t *vec = nv_deserialize_matrix(s);
	
	if (vec == NULL) {
		return NULL;
	}
	
	return nv_vlad_alloc_from(vec);
}

char *
nv_vlad_serialize(const nv_vlad_t *vlad)
{
	return nv_serialize_matrix(vlad->vec);
}
