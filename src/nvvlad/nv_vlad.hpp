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

/* VLAD Full
 *  Reference:
 *   H. Jégou, M. Douze, C. Schmid, and P. Pérez, 
 *   Aggregating local descriptors into a compact image representation, CVPR 2010.
 *   http://lear.inrialpes.fr/pubs/2010/JDSP10/
 */
#ifndef NV_VLAD_HPP
#define NV_VLAD_HPP

#include "nv_core.h"
#include "nv_num.h"
#include "nv_io.h"
#include "nv_ip.h"

typedef enum {
	NV_VLAD_128 = 128,
	NV_VLAD_160 = 160,
	NV_VLAD_256 = 256,
	NV_VLAD_512 = 512
} nv_vlad_word_e;

extern "C" nv_matrix_t nv_vlad128_posi;
extern "C" nv_matrix_t nv_vlad128_nega;	
extern "C" nv_matrix_t nv_vlad160_posi;
extern "C" nv_matrix_t nv_vlad160_nega;	
extern "C" nv_matrix_t nv_vlad256_posi;
extern "C" nv_matrix_t nv_vlad256_nega;	
extern "C" nv_matrix_t nv_vlad512_posi;
extern "C" nv_matrix_t nv_vlad512_nega;

template <nv_vlad_word_e K>
class nv_vlad_ctx {
public:
	static const int DIM = K * NV_KEYPOINT_DESC_N;
	static const float IMG_SIZE() {return  512.0f; }
	static const int KEYPOINTS = 3000;
	static const int KP = K / 2;
	
private:
	nv_keypoint_ctx_t *m_ctx;
	
	const nv_matrix_t *
	POSI(void)
	{
		switch (K) {
		case NV_VLAD_512:
			return &nv_vlad512_posi;
		case NV_VLAD_128:
			return &nv_vlad128_posi;
		case NV_VLAD_160:
			return &nv_vlad160_posi;
		case NV_VLAD_256:
			return &nv_vlad256_posi;
		default:
			NV_ASSERT(0);
			return NULL;
		}
	}
	const nv_matrix_t *
	NEGA(void)
	{
		switch (K) {
		case NV_VLAD_512:
			return &nv_vlad512_nega;
		case NV_VLAD_128:
			return &nv_vlad128_nega;
		case NV_VLAD_160:
			return &nv_vlad160_nega;
		case NV_VLAD_256:
			return &nv_vlad256_nega;
		default:
			NV_ASSERT(0);
			return NULL;
		}
	}
	
	void
	feature_vector(nv_matrix_t *vec,
				   int vec_j,
				   nv_matrix_t *key_vec,
				   nv_matrix_t *desc_vec,
				   int desc_m
		)
	{
		int i;
		int procs = nv_omp_procs();
		nv_matrix_t *vec_tmp = nv_matrix_alloc(vec->n, procs);
		const nv_matrix_t *posi = POSI();
		const nv_matrix_t *nega = NEGA();

		nv_matrix_zero(vec_tmp);

#ifdef _OPENMP
#pragma omp parallel for num_threads(procs)
#endif	
		for (i = 0; i < desc_m; ++i) {
			int j;
			int thread_id = nv_omp_thread_id();
			nv_vector_normalize(desc_vec, i);
			
			if (NV_MAT_V(key_vec, i, NV_KEYPOINT_RESPONSE_IDX) > 0.0f) {
				int label = nv_nn(posi, desc_vec, i);
				
				for (j = 0; j < posi->n; ++j) {
					NV_MAT_V(vec_tmp, thread_id, label * NV_KEYPOINT_DESC_N + j) +=
						NV_MAT_V(desc_vec, i, j) - NV_MAT_V(posi, label, j);
				}
			} else {
				int label = nv_nn(nega, desc_vec, i);
				int vl = (KP + label) * NV_KEYPOINT_DESC_N;
				for (j = 0; j < nega->n; ++j) {
					NV_MAT_V(vec_tmp, thread_id, (vl + j)) +=
						NV_MAT_V(desc_vec, i, j) - NV_MAT_V(nega, label, j);
				}
			}
		}
		nv_vector_zero(vec, vec_j);
		for (i = 0; i < procs; ++i) {
			nv_vector_add(vec, vec_j, vec, vec_j, vec_tmp, i);
		}
		nv_vector_normalize(vec, vec_j);
		
		nv_matrix_free(&vec_tmp);
	}
	
public:
	nv_vlad_ctx()
	{
		switch (K) {
		case NV_VLAD_512:
		{
			nv_keypoint_param_t param = {
				NV_KEYPOINT_THRESH,
				NV_KEYPOINT_EDGE_THRESH,
				4.0f,
				19,
				NV_KEYPOINT_NN,
				NV_KEYPOINT_DETECTOR_STAR,
				NV_KEYPOINT_DESCRIPTOR_GRADIENT_HISTOGRAM
			};
			m_ctx = nv_keypoint_ctx_alloc(&param);
		}
		break;
		case NV_VLAD_256:
		{
			nv_keypoint_param_t param = {
				16,
				NV_KEYPOINT_EDGE_THRESH,
				4.0f,
				NV_KEYPOINT_LEVEL,
				0.3f,
				NV_KEYPOINT_DETECTOR_STAR,
				NV_KEYPOINT_DESCRIPTOR_GRADIENT_HISTOGRAM
			};
			m_ctx = nv_keypoint_ctx_alloc(&param);
		}
		case NV_VLAD_160:
		{
			nv_keypoint_param_t param = {
				12,
				NV_KEYPOINT_EDGE_THRESH,
				4.0f,
				NV_KEYPOINT_LEVEL,
				0.3f,
				NV_KEYPOINT_DETECTOR_STAR,
				NV_KEYPOINT_DESCRIPTOR_GRADIENT_HISTOGRAM
			};
			m_ctx = nv_keypoint_ctx_alloc(&param);
		}
		case NV_VLAD_128:
		{
			nv_keypoint_param_t param = {
				16,
				NV_KEYPOINT_EDGE_THRESH,
				4.0f,
				NV_KEYPOINT_LEVEL,
				0.3f,
				NV_KEYPOINT_DETECTOR_STAR,
				NV_KEYPOINT_DESCRIPTOR_GRADIENT_HISTOGRAM
			};
			m_ctx = nv_keypoint_ctx_alloc(&param);
		}
		break;
		default:
			NV_ASSERT(0);
			break;
		}
	}
	~nv_vlad_ctx()
	{
		nv_keypoint_ctx_free(&m_ctx);
	}
	
	static void
	power_normalize(nv_matrix_t *vlad, int j, float a = 0.5f)
	{
		int i;
		if (a == 0.5f) {
			for (i = 0; i < vlad->n; ++i) {
				NV_MAT_V(vlad, j, i) = NV_SIGN(NV_MAT_V(vlad, j, i)) * sqrtf(fabsf(NV_MAT_V(vlad, j, i)));
			}
		} else {
			for (i = 0; i < vlad->n; ++i) {
				NV_MAT_V(vlad, j, i) = NV_SIGN(NV_MAT_V(vlad, j, i)) * powf(fabsf(NV_MAT_V(vlad, j, i)), a);
			}
		}
	}
	
	void
	extract_dense(nv_matrix_t *vlad, int j,
				  const nv_matrix_t *image,
				  nv_keypoint_dense_t *dense,
				  int ndense
		)
	{
		NV_ASSERT(vlad->n == DIM);
		int desc_m;
		nv_matrix_t *key_vec;
		nv_matrix_t *desc_vec;
		float scale = IMG_SIZE() / (float)NV_MAX(image->rows, image->cols);
		nv_matrix_t *resize = nv_matrix3d_alloc(3, (int)(image->rows * scale),
												(int)(image->cols * scale));
		nv_matrix_t *gray = nv_matrix3d_alloc(1, resize->rows, resize->cols);
		nv_matrix_t *smooth = nv_matrix3d_alloc(1, resize->rows, resize->cols);
		int i;
		int km = 0;
		for (i = 0; i < ndense; ++i) {
			km += dense[i].rows * dense[i].cols;
		}
		km *= 2;
		key_vec = nv_matrix_alloc(NV_KEYPOINT_KEYPOINT_N, km);
		desc_vec = nv_matrix_alloc(NV_KEYPOINT_DESC_N, km);
		nv_resize(resize, image);
		nv_gray(gray, resize);
		nv_gaussian5x5(smooth, 0, gray, 0);
		
		nv_matrix_zero(desc_vec);
		nv_matrix_zero(key_vec);
		
		desc_m = nv_keypoint_dense_ex(m_ctx, key_vec, desc_vec, smooth, 0,
									  dense, ndense);
		feature_vector(vlad, j, key_vec, desc_vec, desc_m);
		
		nv_matrix_free(&gray);
		nv_matrix_free(&resize);
		nv_matrix_free(&smooth);
		nv_matrix_free(&key_vec);
		nv_matrix_free(&desc_vec);
	}
	
	void
	extract(nv_matrix_t *vlad, int j,
			const nv_matrix_t *image)
	{
		NV_ASSERT(vlad->n == DIM);
		int desc_m;
		nv_matrix_t *key_vec = nv_matrix_alloc(NV_KEYPOINT_KEYPOINT_N, KEYPOINTS);
		nv_matrix_t *desc_vec = nv_matrix_alloc(NV_KEYPOINT_DESC_N, KEYPOINTS);
		float scale = IMG_SIZE() / (float)NV_MAX(image->rows, image->cols);
		nv_matrix_t *resize = nv_matrix3d_alloc(3, (int)(image->rows * scale),
												(int)(image->cols * scale));
		nv_matrix_t *gray = nv_matrix3d_alloc(1, resize->rows, resize->cols);
		nv_matrix_t *smooth = nv_matrix3d_alloc(1, resize->rows, resize->cols);
		
		nv_resize(resize, image);
		nv_gray(gray, resize);
		nv_gaussian5x5(smooth, 0, gray, 0);
		
		nv_matrix_zero(desc_vec);
		nv_matrix_zero(key_vec);
		
		desc_m = nv_keypoint_ex(m_ctx, key_vec, desc_vec, smooth, 0);
		feature_vector(vlad, j, key_vec, desc_vec, desc_m);
		
		nv_matrix_free(&gray);
		nv_matrix_free(&resize);
		nv_matrix_free(&smooth);
		nv_matrix_free(&key_vec);
		nv_matrix_free(&desc_vec);
	}
	
	int
	extract(nv_matrix_t *vlad, int j,
			const void *image_data, size_t image_data_len)
	{
		NV_ASSERT(vlad->n == DIM);
		nv_matrix_t *image = nv_decode_image(image_data, image_data_len);
	 
		if (image == NULL) {
			return -1;
		}
		
		extract(vlad, j, image);
		nv_matrix_free(&image);
		
		return 0;
	}
	
	int
	extract(nv_matrix_t *vlad, int j, const char *image_filename)
	{
		NV_ASSERT(vlad->n == DIM);
		nv_matrix_t *image = nv_load_image(image_filename);
	 
		if (image == NULL) {
			return -1;
		}
		
		extract(vlad, j, image);
		nv_matrix_free(&image);
		
		return 0;
	}

	int
	deserialize(nv_matrix_t *vlad, int j, const char *s)
	{
		nv_matrix_t *vec = nv_deserialize_matrix(s);
	
		if (vec == NULL || vec->n != vlad->n) {
			return -1;
		}
		nv_vector_copy(vlad, j, vec, 0);
		nv_matrix_free(&vec);
		
		return 0;
	}
	
	char *
	serialize(const nv_matrix_t *vlad, int j)
	{
		nv_matrix_t *mat = nv_matrix_alloc(vlad->n, 1);
		char *s;
		
		nv_vector_copy(mat, 0, vlad, j);
		s = nv_serialize_matrix(mat);
		nv_matrix_free(&mat);
		
		return s;
	}
	
	float
	similarity(const nv_matrix_t *v1, int j1,
			   const nv_matrix_t *v2, int j2)
	{
		float dot = nv_vector_dot(v1, j1, v2, j2);
		return NV_MAX(0.0f, dot);
	}
};
#endif

