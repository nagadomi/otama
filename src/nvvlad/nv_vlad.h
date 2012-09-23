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

#ifndef NV_VLAD_H
#define NV_VLAD_H

#include "nv_core.h"
#include "nv_ip.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	nv_matrix_t *vec;
	nv_keypoint_ctx_t *ctx;
} nv_vlad_t;

typedef struct nv_vlad_result {
	float similarity;
	uint64_t index;
} nv_vlad_result_t;

nv_vlad_t *nv_vlad_alloc(void);
void nv_vlad_free(nv_vlad_t **vlad);
void nv_vlad(nv_vlad_t *vlad, const nv_matrix_t *image);
int nv_vlad_data(nv_vlad_t *vlad, const void *image_data, size_t image_data_len);
int nv_vlad_file(nv_vlad_t *vlad, const char *image_filename);

void nv_vlad_copy(nv_vlad_t *dest, const nv_vlad_t *src);

float nv_vlad_similarity(const nv_vlad_t *vlad1, const nv_vlad_t *vlad2);

// nv_vlad_simhash
#define NV_VLAD_SIMHASH_INT_BLOCKS 128
typedef struct {
	NV_ALIGNED(uint64_t, simhash[NV_VLAD_SIMHASH_INT_BLOCKS], 16);
	float norm;
} nv_vlad_simhash_t;

void nv_vlad_simhash_init(void);
void nv_vlad_simhash(nv_vlad_simhash_t *vlad, const nv_matrix_t *image);
int nv_vlad_simhash_data(nv_vlad_simhash_t *vlad,
						 const void *image_data, size_t image_data_len);
int nv_vlad_simhash_file(nv_vlad_simhash_t *vlad, const char *image_filename);
void nv_vlad_simhash_bit(nv_vlad_simhash_t *vlad, const nv_matrix_t *vlad_vec, int j);
int nv_vlad_simhash_search(nv_vlad_result_t *results, int k,
						   const nv_vlad_simhash_t *db,
						   int64_t ndb,
						   const nv_vlad_simhash_t *query);
nv_vlad_t *nv_vlad_deserialize(const char *s);
char *nv_vlad_serialize(const nv_vlad_t *vlad);

#ifdef __cplusplus
}
#endif

#endif
