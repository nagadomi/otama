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

#ifndef NV_COLOR_BOC_H
#define NV_COLOR_BOC_H

#include "nv_core.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NV_COLOR_BOC_INT_BLOCKS 4
typedef struct {
	uint64_t color[NV_COLOR_BOC_INT_BLOCKS];
	float norm;
} nv_color_boc_t;
float nv_color_boc_similarity(const nv_color_boc_t *a,
						 const nv_color_boc_t *b);
void nv_color_boc(nv_color_boc_t *boc, const nv_matrix_t *image);
char *nv_color_boc_serialize(const nv_color_boc_t *boc);
int nv_color_boc_deserialize(nv_color_boc_t *boc, const char *s);

#define NV_COLOR_SBOC_INT_BLOCKS (4 * (9 + 4 + 2 + 1))
typedef struct {
	uint64_t color[NV_COLOR_SBOC_INT_BLOCKS];
	float norm[4];
} nv_color_sboc_t;
float nv_color_sboc_similarity(const nv_color_sboc_t *a,
						  const nv_color_sboc_t *b);
void nv_color_sboc(nv_color_sboc_t *boc, const nv_matrix_t *image);
int nv_color_sboc_data(nv_color_sboc_t *boc, const void *data, size_t data_len);
int nv_color_sboc_file(nv_color_sboc_t *boc, const char *file);
char *nv_color_sboc_serialize(const nv_color_sboc_t *boc);
int nv_color_sboc_deserialize(nv_color_sboc_t *boc, const char *s);

typedef struct nv_color_boc_result {
	float cosine;
	uint64_t index;
} nv_color_boc_result_t;

int nv_color_sboc_search(nv_color_boc_result_t *results, int k,
						 const nv_color_sboc_t *db, int64_t ndb,
						 const nv_color_sboc_t *query);
	
int nv_color_boc_search(nv_color_boc_result_t *results, int k,
						const nv_color_boc_t *db, int64_t ndb,
						const nv_color_boc_t *query);

	
#ifdef __cplusplus
}
#endif

#endif
