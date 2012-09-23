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

#include "nv_color_major.h"
#include "nv_ml.h"

#define NV_COLOR_MAJOR_IMG_SIZE 512.0f
#define NV_COLOR_MAJOR_K 32

float
nv_color_major2_similarity(const nv_color_major2_t *a,
					  const nv_color_major2_t *b)
{
	float similarity = 0.0f;
	float dists[4] = {0.0f, 0.0f, 0.0f, 0.0f};
	float dist;
	int i;
	
	for (i = 0; i < 3; ++i) {
		dists[0] += (a->color1[i] - b->color1[i]) * (a->color1[i] - b->color1[i]);
	}
	for (i = 0; i < 3; ++i) {
		dists[1] += (a->color2[i] - b->color2[i]) * (a->color2[i] - b->color2[i]);
	}
	for (i = 0; i < 3; ++i) {
		dists[2] += (a->color2[i] - b->color1[i]) * (a->color2[i] - b->color1[i]);
	}
	for (i = 0; i < 3; ++i) {
		dists[3] += (a->color1[i] - b->color2[i]) *	(a->color1[i] - b->color2[i]);
	}
	dist = NV_MIN(dists[3], NV_MIN(dists[2], NV_MIN(dists[0], dists[1])));
	if (dist == 0.0) {
		similarity = 1.0f;
	} else {
		similarity = sqrtf(dist);
		similarity = expf(-(similarity * similarity) / (2 * 50 * 50));
	}
	return similarity;
}

void nv_color_major2(nv_color_major2_t *major,
					 const nv_matrix_t *image)
{
	const float step = 4.0f;
	float cell_width = NV_MAX(((float)image->cols / NV_COLOR_MAJOR_IMG_SIZE * step), 1.0f);
	float cell_height = NV_MAX(((float)image->rows / NV_COLOR_MAJOR_IMG_SIZE * step), 1.0f);
	int sample_rows = NV_FLOOR_INT(image->rows / cell_height)-1;
	int sample_cols = NV_FLOOR_INT(image->cols / cell_width)-1;
	int i, y;
	nv_matrix_t *samples = nv_matrix_alloc(3, sample_rows * sample_cols);
	nv_matrix_t *count = nv_matrix_alloc(3, NV_COLOR_MAJOR_K);
	nv_matrix_t *centroids = nv_matrix_alloc(3, NV_COLOR_MAJOR_K);
	nv_matrix_t *labels = nv_matrix_alloc(1, samples->m);
	nv_matrix_t *rank = nv_matrix_alloc(4, NV_COLOR_MAJOR_K);

#ifdef _OPENMP
#pragma omp parallel for num_threads(nv_omp_procs())
#endif
	for (y = 0; y < sample_rows; ++y) {
		int x;
		int yi = (int)(y * cell_height);
		for (x = 0; x < sample_cols; ++x) {
			int j = NV_MAT_M(image, yi, (int)(x * cell_width));
			nv_vector_copy(samples, y * sample_cols + x, image, j);
		}
	}
	
	nv_kmeans(centroids, count, labels, samples, NV_COLOR_MAJOR_K, 20);
	for (i = 0; i < count->m; ++i) {
		NV_MAT_V(rank, i, 0) = NV_MAT_V(centroids, i, 0);
		NV_MAT_V(rank, i, 1) = NV_MAT_V(centroids, i, 1);
		NV_MAT_V(rank, i, 2) = NV_MAT_V(centroids, i, 2);
		NV_MAT_V(rank, i, 3) = NV_MAT_V(count, i, 0);
	}
	nv_matrix_sort(rank, 3, NV_SORT_DIR_DESC);
	major->color1[0] = NV_FLOOR(NV_MAT_V(rank, 0, 0));
	major->color1[1] = NV_FLOOR(NV_MAT_V(rank, 0, 1));
	major->color1[2] = NV_FLOOR(NV_MAT_V(rank, 0, 2));
	major->color2[0] = NV_FLOOR(NV_MAT_V(rank, 1, 0));
	major->color2[1] = NV_FLOOR(NV_MAT_V(rank, 1, 1));
	major->color2[2] = NV_FLOOR(NV_MAT_V(rank, 1, 2));
	
	nv_matrix_free(&centroids);
	nv_matrix_free(&samples);
	nv_matrix_free(&count);
	nv_matrix_free(&rank);
	nv_matrix_free(&labels);
}
