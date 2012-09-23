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

#undef NDEBUG

#include "otama_test.h"
#include "nv_vlad.h"
#include "nv_core.h"
#include "nv_bovw.hpp"
#include <vector>
#include <algorithm>

template<typename T>
static float
svec_similarity(std::vector<uint32_t> &svec1,
		   std::vector<uint32_t> &svec2)
{
	std::vector<uint32_t> intersection;
	std::vector<uint32_t>::const_iterator it;
	float norm1 = 0.0f, norm2 = 0.0f, dot = 0.0f;
	T *ctx = new T;

	ctx->open();
	
	std::set_intersection(svec1.begin(), svec1.end(),
						  svec2.begin(), svec2.end(),
						  std::back_inserter(intersection));
	
	for (it = svec1.begin(); it != svec1.end(); ++it) {
		float w = ctx->idf(*it);
		norm1 += w * w;
	}
	for (it = svec2.begin(); it != svec2.end(); ++it) {
		float w = ctx->idf(*it);
		norm2 += w * w;
	}
	for (it = intersection.begin(); it != intersection.end(); ++it) {
		float w = ctx->idf(*it);
		dot += w * w;
	}

	delete ctx;
	
	return dot / (sqrtf(norm1) * sqrtf(norm2));
}

template<typename T>
static void
otama_test_bovw_tpl(void)
{
	typename T::dense_t bovw1, bovw2, bovw3;
	std::vector<uint32_t> svec1, svec2;
	T *ctx = new T;
	
	char str1[T::SERIALIZE_LEN];
	char str2[T::SERIALIZE_LEN];

	ctx->open();
	
	memset(&bovw1, 0, sizeof(bovw1));
	memset(&bovw2, 0, sizeof(bovw2));
	
	NV_ASSERT(ctx->extract(&bovw1, OTAMA_TEST_IMG) == 0);
	ctx->serialize(str1, &bovw1);
	NV_ASSERT(ctx->deserialize(&bovw2, str1) == 0);
	ctx->serialize(str2, &bovw2);
	
	NV_ASSERT(strcmp(str1, str2) == 0);
	NV_ASSERT(memcmp(&bovw1.bovw, &bovw2.bovw, sizeof(bovw1.bovw)) == 0);
	NV_ASSERT(memcmp(&bovw1.boc, &bovw2.boc, sizeof(bovw1.boc)) == 0);
	NV_ASSERT(bovw1.norm == bovw2.norm);
	NV_ASSERT(memcmp(&bovw1, &bovw2, sizeof(bovw1)) == 0);

	NV_ASSERT(OTAMA_TEST_EQ1(ctx->similarity(&bovw1, &bovw2,
											  NV_BOVW_RERANK_NONE,
											  0.2f)));
	
	NV_ASSERT(OTAMA_TEST_EQ1(ctx->similarity(&bovw1, &bovw2,
											  NV_BOVW_RERANK_IDF,
											  0.2f)));
	NV_ASSERT(ctx->extract(&bovw2, OTAMA_TEST_IMG_NEGA) == 0);
	NV_ASSERT(ctx->extract(&bovw3, OTAMA_TEST_IMG_ROTATE) == 0);

	printf("%s - %dpts\n", OTAMA_TEST_IMG, (int)(bovw1.norm * bovw1.norm));
	printf("%s - %dpts\n", OTAMA_TEST_IMG_NEGA, (int)(bovw2.norm * bovw2.norm));
	printf("%s - %dpts\n", OTAMA_TEST_IMG_ROTATE, (int)(bovw3.norm * bovw3.norm));
	
	NV_ASSERT(
		ctx->similarity(&bovw1, &bovw2,
						 NV_BOVW_RERANK_NONE,
						 0.2f) <
		ctx->similarity(&bovw1, &bovw3,
						 NV_BOVW_RERANK_NONE,
						 0.2f)
		);
	NV_ASSERT(
		ctx->similarity(&bovw1, &bovw2,
						 NV_BOVW_RERANK_IDF,
						 0.2f) <
		ctx->similarity(&bovw1, &bovw3,
						 NV_BOVW_RERANK_IDF,
						 0.2f)
		);
	NV_ASSERT(
		ctx->similarity(&bovw1, &bovw2,
						 NV_BOVW_RERANK_NONE,
						 1.0f) <
		ctx->similarity(&bovw1, &bovw3,
						 NV_BOVW_RERANK_NONE,
						 1.0f)
		);
	NV_ASSERT(
		ctx->similarity(&bovw1, &bovw2,
						 NV_BOVW_RERANK_NONE,
						 1.0f) <
		ctx->similarity(&bovw1, &bovw3,
						 NV_BOVW_RERANK_NONE,
						 1.0f)
		);

	NV_ASSERT(ctx->extract(&bovw1, OTAMA_TEST_IMG) == 0);
	ctx->convert(svec1, &bovw1);
	NV_ASSERT(ctx->extract(&bovw2, OTAMA_TEST_IMG_ROTATE) == 0);
	ctx->convert(svec2, &bovw2);
	
	printf("%f == %f\n", sqrtf(svec1.size()), (bovw1.norm));
	NV_ASSERT(OTAMA_TEST_EQ0(sqrtf(svec1.size()) - (bovw1.norm)));
	printf("%f == %f\n", sqrtf(svec2.size()), (bovw2.norm));
	NV_ASSERT(OTAMA_TEST_EQ0(sqrtf(svec2.size()) - (bovw2.norm)));

	printf("%f == %f\n",
		   ctx->similarity(&bovw1, &bovw2, NV_BOVW_RERANK_IDF, 0.0f),
		   svec_similarity<T>(svec1, svec2));
	NV_ASSERT(OTAMA_TEST_EQ0(
				  ctx->similarity(&bovw1, &bovw2, NV_BOVW_RERANK_IDF, 0.0f) -
				  svec_similarity<T>(svec1, svec2)));

	delete ctx;
}

template<typename T>
static void
otama_test_bovw_svec_tpl(void)
{
	std::vector<uint32_t> hash1, hash2, hash3, hash4, hash5, result;
	std::vector<uint32_t>::const_iterator it;
	std::string s1, s2;
	float similarity1, similarity2, similarity3, similarity4;
	T *ctx = new T;
	
	OTAMA_TEST_NAME;

	ctx->open();
	
	NV_ASSERT(ctx->extract(hash1, OTAMA_TEST_IMG) == 0);
	NV_ASSERT(ctx->extract(hash2, OTAMA_TEST_IMG_SCALE) == 0);
	NV_ASSERT(ctx->extract(hash3, OTAMA_TEST_IMG_ROTATE) == 0);
	NV_ASSERT(ctx->extract(hash4, OTAMA_TEST_IMG_AFFINE) == 0);	
	NV_ASSERT(ctx->extract(hash5, OTAMA_TEST_IMG_NEGA) == 0);	

	printf("%s -> %d\n", OTAMA_TEST_IMG, (int)hash1.size());
	printf("%s -> %d\n", OTAMA_TEST_IMG_SCALE, (int)hash2.size());
	printf("%s -> %d\n", OTAMA_TEST_IMG_ROTATE, (int)hash3.size());
	printf("%s -> %d\n", OTAMA_TEST_IMG_AFFINE, (int)hash4.size());
	printf("%s -> %d\n", OTAMA_TEST_IMG_NEGA, (int)hash5.size());

	result.clear();
	std::set_intersection(hash1.begin(), hash1.end(),
						  hash2.begin(), hash2.end(),
						  std::back_inserter(result));
	//put(result);
	similarity1 = (float)result.size()/NV_MIN(hash1.size(), hash2.size());
	printf("%s * %s: %d(%f)\n",
		   OTAMA_TEST_IMG, OTAMA_TEST_IMG_SCALE,
		   (int)result.size(),
		   similarity1);
	
	result.clear();
	std::set_intersection(hash1.begin(), hash1.end(),
						  hash3.begin(), hash3.end(),
						  std::back_inserter(result));
	//put(result);
	similarity2 = (float)result.size()/NV_MIN(hash1.size(), hash3.size());
	printf("%s * %s: %d(%f)\n",
		   OTAMA_TEST_IMG,
		   OTAMA_TEST_IMG_ROTATE,
		   (int)result.size(),
		   similarity2);	

	result.clear();
	std::set_intersection(hash1.begin(), hash1.end(),
						  hash4.begin(), hash4.end(),
						  std::back_inserter(result));
	//put(result);
	similarity3 = (float)result.size()/NV_MIN(hash1.size(), hash4.size());
	printf("%s * %s: %d(%f)\n",
		   OTAMA_TEST_IMG,
		   OTAMA_TEST_IMG_AFFINE,
		   (int)result.size(),
		   similarity3);	

	result.clear();
	std::set_intersection(hash1.begin(), hash1.end(),
						  hash5.begin(), hash5.end(),
						  std::back_inserter(result));
	//put(result);
	similarity4 = (float)result.size()/NV_MIN(hash1.size(), hash5.size());
	printf("%s * %s: %d(%f)\n",
		   OTAMA_TEST_IMG,
		   OTAMA_TEST_IMG_NEGA,
		   (int)result.size(),
		   similarity4);

	result.clear();
	std::set_intersection(hash4.begin(), hash4.end(),
						  hash5.begin(), hash5.end(),
						  std::back_inserter(result));
	//put(result);
	similarity4 = (float)result.size()/NV_MIN(hash4.size(), hash5.size());
	printf("%s * %s: %d(%f)\n",
		   OTAMA_TEST_IMG_AFFINE,
		   OTAMA_TEST_IMG_NEGA,
		   (int)result.size(),
		   similarity4);

	NV_ASSERT(similarity1 > similarity4);
	NV_ASSERT(similarity2 > similarity4);
	NV_ASSERT(similarity3 > similarity4);

	NV_ASSERT(ctx->extract(hash1, OTAMA_TEST_IMG) == 0);
	ctx->serialize(s1, &hash1);
	NV_ASSERT(ctx->deserialize(&hash2, s1.c_str()) == 0);
	ctx->serialize(s2, &hash2);
	NV_ASSERT(s1 == s2);

	delete ctx;
}

void
otama_test_bovw(void)
{
	OTAMA_TEST_NAME;
	
	printf("-- hist\n");
	otama_test_bovw_tpl<nv_bovw_ctx<NV_BOVW_BIT2K, nv_color_boc_t> >();
	otama_test_bovw_tpl<nv_bovw_ctx<NV_BOVW_BIT8K, nv_color_boc_t> >();
	otama_test_bovw_tpl<nv_bovw_ctx<NV_BOVW_BIT512K, nv_color_boc_t> >();
	otama_test_bovw_tpl<nv_bovw_ctx<NV_BOVW_BIT2K, nv_color_sboc_t> >();
	otama_test_bovw_tpl<nv_bovw_ctx<NV_BOVW_BIT8K, nv_color_sboc_t> >();
	otama_test_bovw_tpl<nv_bovw_ctx<NV_BOVW_BIT512K, nv_color_sboc_t> >();

	otama_test_bovw_svec_tpl<nv_bovw_ctx<NV_BOVW_BIT512K, nv_color_sboc_t> >();	
	printf("-- rect\n");
}
