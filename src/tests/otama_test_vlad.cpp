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
#include "nv_vlad.h"

void
otama_test_vlad(void)
{
	nv_vlad_t *vlad1 = nv_vlad_alloc();
	nv_vlad_t *vlad2 = nv_vlad_alloc();
	nv_vlad_t *vlad3 = nv_vlad_alloc();
	nv_vlad_t *vlad4;
	void *p1, *p2, *p3;
	size_t len1, len2, len3;
	char *s1, *s2;
	long t;
	
	float similarity1, similarity2;
	
	OTAMA_TEST_NAME;
	
	NV_ASSERT(vlad1 != NULL && vlad2 != NULL && vlad3 != NULL);
	
	t = nv_clock();
	nv_vlad_file(vlad1, OTAMA_TEST_IMG);
	printf("%s: %ldms\n", OTAMA_TEST_IMG, nv_clock() - t);
	t = nv_clock();
	nv_vlad_file(vlad2, OTAMA_TEST_IMG_NEGA);
	printf("%s: %ldms\n", OTAMA_TEST_IMG_NEGA, nv_clock() - t);
	t = nv_clock();
	nv_vlad_file(vlad3, OTAMA_TEST_IMG_SCALE);
	printf("%s: %ldms\n", OTAMA_TEST_IMG_SCALE, nv_clock() - t);

	similarity1 = nv_vlad_similarity(vlad1, vlad2);
	similarity2 = nv_vlad_similarity(vlad1, vlad1);
	printf("%E < %E\n", similarity1, similarity2);
	NV_ASSERT(similarity1 < similarity2);
	
	similarity1 = nv_vlad_similarity(vlad1, vlad3);
	similarity2 = nv_vlad_similarity(vlad1, vlad1);
	printf("%E < %E\n", similarity1, similarity2);
	NV_ASSERT(similarity1 < similarity2);
	
	similarity1 = nv_vlad_similarity(vlad1, vlad2);
	similarity2 = nv_vlad_similarity(vlad1, vlad3);
	printf("%E < %E\n", similarity1, similarity2);
	NV_ASSERT(similarity1 < similarity2);
	
	otama_test_read_file(OTAMA_TEST_IMG, &p1, &len1);
	otama_test_read_file(OTAMA_TEST_IMG_NEGA, &p2, &len2);
	otama_test_read_file(OTAMA_TEST_IMG_SCALE, &p3, &len3);
	
	nv_vlad_data(vlad1, p1, len1);
	nv_vlad_data(vlad2, p2, len2);
	nv_vlad_data(vlad3, p3, len3);

	similarity1 = nv_vlad_similarity(vlad1, vlad2);
	similarity2 = nv_vlad_similarity(vlad1, vlad1);
	printf("%E < %E\n", similarity1, similarity2);
	NV_ASSERT(similarity1 < similarity2);
	
	similarity1 = nv_vlad_similarity(vlad1, vlad3);
	similarity2 = nv_vlad_similarity(vlad1, vlad1);
	printf("%E < %E\n", similarity1, similarity2);
	NV_ASSERT(similarity1 < similarity2);
	
	similarity1 = nv_vlad_similarity(vlad1, vlad2);
	similarity2 = nv_vlad_similarity(vlad1, vlad3);
	printf("%E < %E\n", similarity1, similarity2);
	NV_ASSERT(similarity1 < similarity2);

	s1 = nv_vlad_serialize(vlad1);
	NV_ASSERT(s1 != NULL);
	vlad4 = nv_vlad_deserialize(s1);
	NV_ASSERT(vlad4 != NULL);
	s2 = nv_vlad_serialize(vlad4);
	NV_ASSERT(s2 != NULL);
	NV_ASSERT(strcmp(s1, s2) == 0);

	similarity1 = nv_vlad_similarity(vlad1, vlad4);
	printf("%E == 1.0f\n", similarity1);
	NV_ASSERT(OTAMA_TEST_EQ1(similarity1));
	
	nv_vlad_free(&vlad1);
	nv_vlad_free(&vlad2);
	nv_vlad_free(&vlad3);
	nv_vlad_free(&vlad4);
	nv_free(s1);
	nv_free(s2);
	nv_free(p1);
	nv_free(p2);
	nv_free(p3);
}
