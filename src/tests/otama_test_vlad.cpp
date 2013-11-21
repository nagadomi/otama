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
#include "nv_core.h"
#include "nv_num.h"
#include "nv_vlad.hpp"

template<nv_vlad_word_e K>
static void
otama_test_vlad_main(void)
{
	nv_vlad_ctx<K> ctx;
	nv_matrix_t *vlad = nv_matrix_alloc(nv_vlad_ctx<K>::DIM, 4);

	void *p1, *p2, *p3;
	size_t len1, len2, len3;
	char *s1, *s2;
	long t;
	float similarity1, similarity2;
	
	OTAMA_TEST_NAME;
	
	t = nv_clock();
	ctx.extract(vlad, 0, OTAMA_TEST_IMG);
	printf("%s: %ldms\n", OTAMA_TEST_IMG, nv_clock() - t);
	t = nv_clock();
	ctx.extract(vlad, 1, OTAMA_TEST_IMG_NEGA);
	printf("%s: %ldms\n", OTAMA_TEST_IMG_NEGA, nv_clock() - t);
	t = nv_clock();
	ctx.extract(vlad, 2, OTAMA_TEST_IMG_SCALE);
	printf("%s: %ldms\n", OTAMA_TEST_IMG_SCALE, nv_clock() - t);

	similarity1 = ctx.similarity(vlad, 0, vlad, 1);
	similarity2 = ctx.similarity(vlad, 0, vlad, 0);
	printf("%E < %E\n", similarity1, similarity2);
	NV_ASSERT(similarity1 < similarity2);
	
	similarity1 = ctx.similarity(vlad, 0, vlad, 2);
	similarity2 = ctx.similarity(vlad, 0, vlad, 0);
	printf("%E < %E\n", similarity1, similarity2);
	NV_ASSERT(similarity1 < similarity2);
	
	similarity1 = ctx.similarity(vlad, 0, vlad, 1);
	similarity2 = ctx.similarity(vlad, 0, vlad, 2);
	printf("%E < %E\n", similarity1, similarity2);
	NV_ASSERT(similarity1 < similarity2);
	
	otama_test_read_file(OTAMA_TEST_IMG, &p1, &len1);
	otama_test_read_file(OTAMA_TEST_IMG_NEGA, &p2, &len2);
	otama_test_read_file(OTAMA_TEST_IMG_SCALE, &p3, &len3);
	
	ctx.extract(vlad, 0, p1, len1);
	ctx.extract(vlad, 1, p2, len2);
	ctx.extract(vlad, 2, p3, len3);
	
	similarity1 = ctx.similarity(vlad, 0, vlad, 1);
	similarity2 = ctx.similarity(vlad, 0, vlad, 0);
	printf("%E < %E\n", similarity1, similarity2);
	NV_ASSERT(similarity1 < similarity2);
	
	similarity1 = ctx.similarity(vlad, 0, vlad, 2);
	similarity2 = ctx.similarity(vlad, 0, vlad, 0);
	printf("%E < %E\n", similarity1, similarity2);
	NV_ASSERT(similarity1 < similarity2);
	
	similarity1 = ctx.similarity(vlad, 0, vlad, 1);
	similarity2 = ctx.similarity(vlad, 0, vlad, 2);
	printf("%E < %E\n", similarity1, similarity2);
	NV_ASSERT(similarity1 < similarity2);
	
	s1 = ctx.serialize(vlad, 0);
	NV_ASSERT(s1 != NULL);
	ctx.deserialize(vlad, 3, s1);
	s2 = ctx.serialize(vlad, 3);
	NV_ASSERT(s2 != NULL);
	NV_ASSERT(strcmp(s1, s2) == 0);

	similarity1 = ctx.similarity(vlad, 0, vlad, 3);
	printf("%E == 1.0f\n", similarity1);
	NV_ASSERT(OTAMA_TEST_EQ1(similarity1));

	nv_free(s1);
	nv_free(s2);
	nv_free(p1);
	nv_free(p2);
	nv_free(p3);
	nv_matrix_free(&vlad);
}

void
otama_test_vlad(void)
{
	otama_test_vlad_main<NV_VLAD_512>();
	otama_test_vlad_main<NV_VLAD_128>();
}
