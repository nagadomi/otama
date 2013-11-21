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
#include "nv_color_hist.h"

nv_matrix_t nv_color_boc_static;
#define NV_COLOR_HIST_IMG_SIZE 512.0f

void
nv_color_hist(nv_matrix_t *hist, int hist_j,
			  const nv_matrix_t *image)
{
	const float step = 2.0f;
	float cell_width = NV_MAX(((float)image->cols / NV_COLOR_HIST_IMG_SIZE * step), 1.0f);
	float cell_height = NV_MAX(((float)image->rows / NV_COLOR_HIST_IMG_SIZE * step), 1.0f);
	int sample_rows = NV_FLOOR_INT(image->rows / cell_height)-1;
	int sample_cols = NV_FLOOR_INT(image->cols / cell_width)-1;
	int i, y;
	int threads = nv_omp_procs();
	const int vq_len = sample_rows * sample_cols;
	int *vq = nv_alloc_type(int, vq_len);
	nv_matrix_t *hsv = nv_matrix_alloc(3, threads);

	NV_ASSERT(hist->n == NV_COLOR_HIST_DIM);
	
#ifdef _OPENMP
#pragma omp parallel for num_threads(threads)
#endif
	for (y = 0; y < sample_rows; ++y) {
		int thread_id = nv_omp_thread_id();
		int x;
		int yi = (int)(y * cell_height);
		for (x = 0; x < sample_cols; ++x) {
			int j = NV_MAT_M(image, yi, (int)(x * cell_width));
			int idx = y * sample_cols + x;
			nv_color_bgr2hsv_scalar(hsv, thread_id, image, j);
			vq[idx] = nv_nn(&nv_color_boc_static, hsv, thread_id);
		}
	}
	nv_vector_zero(hist, hist_j);
	for (i = 0; i < vq_len; ++i) {
		NV_MAT_V(hist, hist_j, vq[i]) += vq[i];
	}
	// power normalize
	nv_vector_sqrt(hist, hist_j, hist, hist_j);
	nv_vector_normalize(hist, hist_j);
	
	nv_matrix_free(&hsv);
	nv_free(vq);
}


void
nv_color_shist(nv_matrix_t *hist, int hist_j, const nv_matrix_t *image)
{
	static const int nv_color_sboc_norm_e[4] = {
		9 * (NV_COLOR_HIST_COLOR),
		13 * (NV_COLOR_HIST_COLOR),
		15 * (NV_COLOR_HIST_COLOR),
		16 * (NV_COLOR_HIST_COLOR)
	};
	static const float nv_color_sboc_w[4] = { 0.4f, 0.25f, 0.15f, 0.2f };
	static const int sboc_level = (int)(sizeof(nv_color_sboc_norm_e) / sizeof(int));
	const float step = 2.0f;
	float cell_width = NV_MAX(((float)image->cols / NV_COLOR_HIST_IMG_SIZE * step), 1.0f);
	float cell_height = NV_MAX(((float)image->rows / NV_COLOR_HIST_IMG_SIZE * step), 1.0f);
	int sample_rows = NV_FLOOR_INT(image->rows / cell_height)-1;
	int sample_cols = NV_FLOOR_INT(image->cols / cell_width)-1;
	const int vq_len = sample_rows * sample_cols;	
	int j, i, y;
	int hhr = sample_rows / 3 + 1;
	int hhc = sample_cols / 3 + 1;
	int hr = sample_rows / 2 + 1;
	int hhhr = sample_rows / 4 + 1;
	int threads = nv_omp_procs();
	nv_matrix_t *hsv = nv_matrix_alloc(3, threads);
	int *vq = nv_alloc_type(int, vq_len);
	int *vq_lr3 = nv_alloc_type(int, vq_len);
	int *vq_tb4 = nv_alloc_type(int, vq_len);
	int *vq_tb3 = nv_alloc_type(int, vq_len);
	int *vq_tb2 = nv_alloc_type(int, vq_len);
	nv_matrix_t *hist_tmp = nv_matrix_alloc(NV_COLOR_HIST_COLOR, 16);
	
	NV_ASSERT(hist->n == NV_COLOR_SHIST_DIM);
	
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
			int o = NV_MAT_M(image, yi, (int)(x * cell_width));
			int c3 = x / hhc;
			int idx = y * sample_cols + x;
			
			nv_color_bgr2hsv_scalar(hsv, thread_id, image, o);
			vq[idx] = nv_nn(&nv_color_boc_static, hsv, thread_id);
			vq_tb4[idx] = r4;
			vq_tb3[idx] = r3;
			vq_tb2[idx] = r2;
			vq_lr3[idx] = c3;
		}
	}
	nv_matrix_zero(hist_tmp);
	for (i = 0; i < vq_len; ++i) {
		NV_MAT_V(hist_tmp, vq_tb3[i] * 3 + vq_lr3[i], vq[i]) += 1.0f;
		NV_MAT_V(hist_tmp, 9 + vq_tb4[i], vq[i]) += 1.0f;
		NV_MAT_V(hist_tmp, 13 + vq_tb2[i], vq[i]) += 1.0f;
		NV_MAT_V(hist_tmp, 15, vq[i]) += 1.0f;
	}
	nv_vector_zero(hist, hist_j);
	for (i = 0; i < hist_tmp->m; ++i) {
		float scale = 1.0f / nv_vector_max(hist_tmp, i);
		for (j = 0; j < hist_tmp->n; ++j) {
			float v = (NV_MAT_V(hist_tmp, i, j) * scale) * 2.0f;
			if (v > 0.02f) {
				NV_MAT_V(hist, hist_j, i * hist_tmp->n + j) = sqrtf((float)(((int)v) + 1));
			}
		}
	}
	for (j = 0, i = 0; j < sboc_level; ++j) {
		int ist = i;
		int o;
		float norm = 0.0f, scale;
		for (; i < nv_color_sboc_norm_e[j]; ++i) {
			norm += NV_MAT_V(hist, hist_j, i) * NV_MAT_V(hist, hist_j, i);
		}
		scale = 1.0f / sqrtf(norm) * sqrtf(nv_color_sboc_w[j]);
		for (o = ist; o < nv_color_sboc_norm_e[j]; ++o) {
			NV_MAT_V(hist, hist_j, o) = NV_MAT_V(hist, hist_j, o) * scale;
		}
	}
	nv_matrix_free(&hsv);
	nv_matrix_free(&hist_tmp);
	nv_free(vq);
	nv_free(vq_lr3);
	nv_free(vq_tb4);
	nv_free(vq_tb3);
	nv_free(vq_tb2);
}
