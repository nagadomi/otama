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

#include "nv_bovw.hpp"
#include "nv_color_boc.h"

#define DATA_M     1000000
#define TRIES      4
#define RESULT_M   4
#define KEYPOINT_M 1024

#define FILE1 "../image/lena.jpg"
#define FILE2 "../image/baboon.png"

#define DUMMY_COLOR 1
//typedef nv_bovw_ctx<NV_BOVW_BIT2K, nv_color_sboc_t> bovw;
//typedef nv_bovw_ctx<NV_BOVW_BIT8K, nv_bovw_dummy_color_t> bovw;
//typedef nv_bovw_ctx<NV_BOVW_BIT8K, nv_color_sboc_t> bovw;
///typedef nv_bovw_ctx<NV_BOVW_BIT512K, nv_color_sboc_t> bovw;
typedef nv_bovw_ctx<NV_BOVW_BIT512K, nv_bovw_dummy_color_t> bovw;

// from nv_color_sboc.c
static int nv_color_sboc_norm_e[4] = { 9 * 4, 13 * 4, 15 * 4, 16 * 4};
static float nv_color_sboc_w[4] = { 0.4f, 0.25f, 0.15f, 0.2f };
#define NV_COLOR_SBOC_LEVEL (int)(sizeof(nv_color_sboc_norm_e) / sizeof(int))


void
random_bovw(bovw::dense_t *bovw)
{
	int j;
	int cand = KEYPOINT_M / 2;
	int rand_m = cand + (int)(nv_rand() * (float)cand);
	uint64_t popcnt;

	for (j = 0; j < rand_m; ++j) {
		uint64_t rand_i = NV_ROUND_INT(nv_rand() * (sizeof(uint64_t) * 8 * bovw::INT_BLOCKS));
		uint64_t r_i = rand_i / 64ULL;
		uint64_t r_d = rand_i % 64ULL;
		
		bovw->bovw[r_i] |= (1ULL << r_d);
	}
	
	popcnt = 0;
	for (j = 0; j < bovw::INT_BLOCKS; ++j) {
		popcnt += NV_POPCNT_U64(bovw->bovw[j]);
	}
	if (popcnt == 0) {
		bovw->norm = FLT_MAX;
	} else {
		bovw->norm = sqrtf((float)popcnt);
	}
#if !DUMMY_COLOR
	int i;
	for (j = 0; j < NV_COLOR_SBOC_INT_BLOCKS * 2; ++j) {
		uint64_t rand_i = NV_ROUND_INT(nv_rand() * (sizeof(uint64_t) * 8 * NV_COLOR_SBOC_INT_BLOCKS));
		uint64_t r_i = rand_i / 64ULL;
		uint64_t r_d = rand_i % 64ULL;
		
		bovw->boc.color[r_i] |= (1ULL << r_d);
	}
	for (j = 0, i = 0; j < NV_COLOR_SBOC_LEVEL; ++j) {
		popcnt = 0;
		for (; i < nv_color_sboc_norm_e[j]; ++i) {
			popcnt += NV_POPCNT_U64(bovw->boc.color[i]);
		}
		if (popcnt == 0) {
			bovw->boc.norm[j] = FLT_MAX; // a / (norm) => 0
		} else {
			bovw->boc.norm[j] = sqrtf((float)popcnt);
		}
	}
#endif	
}

void
search_benchmark(const bovw::dense_t *db)
{
	int j, i;
	nv_bovw_result_t results[RESULT_M];
	bovw *ctx = new bovw();

	ctx->open();
	
	printf("\n---- %s\n", __FUNCTION__);
#if _OPENMP
	printf("- OpenMP %d threads \n", nv_omp_procs());
#endif
#if (defined(__SSE4_2__) || defined(__POPCNT__))
	printf("- use POPCNT \n");
#endif

	for (j = 0; j < TRIES; ++j) {
		long t = nv_clock();
		int query_j = NV_ROUND_INT(nv_rand() * DATA_M);
		int n = ctx->search(results, RESULT_M, db, DATA_M, &db[query_j],
							NV_BOVW_RERANK_IDF,
							0.0f);

		printf("----- \nquery: %d (BOVW)\n", query_j);
		for (i = 0; i < n; ++i) {
			printf("\t%"PRId64"\t%f\n", results[i].index, results[i].similarity);
		}
		printf("%ldms\n", nv_clock() - t);

		assert((uint64_t)query_j == results[0].index);
	}

	delete ctx;
}

void 
bovw_benchmark(const char *file)
{
	int j;
	bovw *ctx = new bovw();

	ctx->open();
	
	printf("\n---- %s\n", __FUNCTION__);
	
	for (j = 0; j < TRIES; ++j) {
		bovw::dense_t bovw;
		long t = nv_clock();
		
		ctx->extract(&bovw, file);
		printf("%ldms\n\n", nv_clock() -t);
	}

	delete ctx;
}

void 
sbovw_benchmark(const char *file)
{
	bovw *ctx = new bovw();
	int j;
	bovw::sparse_t svec;

	ctx->open();
	
	printf("\n---- %s\n", __FUNCTION__);
	
	for (j = 0; j < TRIES; ++j) {
		long t = nv_clock();
		
		ctx->extract(svec, file);
		printf("%ldms\n\n", nv_clock() -t);
	}

	delete ctx;
}

void 
serialize_benchmark(bovw::dense_t *db)
{
	int j;
	long t;
	char str_vec[bovw::SERIALIZE_LEN][100];
	bovw *ctx = new bovw;
	bovw::dense_t bovw[100];

	ctx->open();
	
	printf("\n---- %s\n", __FUNCTION__);

	printf("serialize: ");
	t = nv_clock();
	for (j = 0; j < DATA_M; ++j) {
		ctx->serialize(str_vec[j % 10], &db[j]);
	}
	printf("%ldms\n", nv_clock() - t);

	printf("deserialize: ");
	t = nv_clock();
	for (j = 0; j < DATA_M; ++j) {
		ctx->deserialize(&bovw[j % 10], str_vec[j % 10]);
	}
	printf("%ldms\n", nv_clock() - t);

	delete ctx;
}

void 
idf_benchmark(bovw::dense_t *db)
{
	int j;
	long t;
	nv_matrix_t *idf = nv_matrix_alloc(bovw::BIT, 1);
	bovw *ctx = new bovw();

	ctx->open();

	printf("\n---- %s\n", __FUNCTION__);

	printf("idf: ");
	t = nv_clock();

#ifdef _OPENMP
#pragma omp parallel for
#endif
	for (j = 0; j < DATA_M; ++j) {
		ctx->tfidf(idf, 0, &db[j]);
	}
	printf("%ldms\n", nv_clock() - t);
	
	nv_matrix_free(&idf);
	delete ctx;
}

void
bovw_benchmark(void)
{
	bovw::dense_t *db;
	int j;

	bovw_benchmark(FILE1);
	bovw_benchmark(FILE2);
	sbovw_benchmark(FILE1);
	sbovw_benchmark(FILE2);

	db = nv_alloc_type(bovw::dense_t, DATA_M);
	memset(db, 0, sizeof(bovw::dense_t) * DATA_M);
	nv_srand_time();
	
	printf("make %d data..\n", DATA_M);
	for (j = 0; j < DATA_M; ++j) {
		random_bovw(&db[j]);
	}
	
	search_benchmark(db);
	serialize_benchmark(db);
	idf_benchmark(db);
	
	nv_free(db);
}

int main(void)
{
	nv_setenv("NV_BOVW_PKGDATADIR", "./nvbovw");
	bovw_benchmark();
	return 0;
}
