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
#include "otama.h"
#include "otama_util.h"
#include "otama_test.h"
#include "nv_core.h"

void
otama_test_similarity_api(const char *config)
{
	otama_t *otama;
	float similarity1, similarity2;
	void *data1, *data2;
	size_t data1_len, data2_len;
	char *s1, *s2;
	otama_feature_raw_t *p1, *p2;

	OTAMA_TEST_NAME;
	printf("config: %s\n", config);
	
	NV_ASSERT(otama_open(&otama, config) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_similarity_file(otama, &similarity1, OTAMA_TEST_IMG, OTAMA_TEST_IMG_NEGA)
			  == OTAMA_STATUS_OK);
	printf("%s x %s = %f\n", OTAMA_TEST_IMG, OTAMA_TEST_IMG_NEGA, similarity1);
	NV_ASSERT(otama_similarity_file(otama, &similarity2, OTAMA_TEST_IMG, OTAMA_TEST_IMG_SCALE)
			  == OTAMA_STATUS_OK);
	printf("%f < %f\n", similarity1, similarity2);
	NV_ASSERT(similarity1 < similarity2);

	// print only..
	printf("%s x %s = %f\n", OTAMA_TEST_IMG, OTAMA_TEST_IMG_SCALE, similarity2);
	NV_ASSERT(otama_similarity_file(otama, &similarity2, OTAMA_TEST_IMG, OTAMA_TEST_IMG_ROTATE)
			  == OTAMA_STATUS_OK);
	printf("%s x %s = %f\n", OTAMA_TEST_IMG, OTAMA_TEST_IMG_ROTATE, similarity2);
	
	NV_ASSERT(otama_similarity_file(otama, &similarity2, OTAMA_TEST_IMG, OTAMA_TEST_IMG_AFFINE)
			  == OTAMA_STATUS_OK);
	printf("%s x %s = %f\n", OTAMA_TEST_IMG, OTAMA_TEST_IMG_AFFINE, similarity2);
	
	otama_test_read_file(OTAMA_TEST_IMG, &data1, &data1_len);
	otama_test_read_file(OTAMA_TEST_IMG_NEGA, &data2, &data2_len);
	NV_ASSERT(otama_similarity_data(otama, &similarity2, data1, data1_len, data2, data2_len)
			  == OTAMA_STATUS_OK);
	printf("%f == %f\n", similarity1, similarity2);
	NV_ASSERT(!(fabsf(similarity2 - similarity1) > FLT_EPSILON));
	
	NV_ASSERT(otama_feature_string_file(otama, &s1, OTAMA_TEST_IMG) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_feature_string_data(otama, &s2, data2, data2_len) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_similarity_string(otama, &similarity2, s1, s2)
			  == OTAMA_STATUS_OK);
	printf("%f == %f\n", similarity1, similarity2);
	NV_ASSERT(OTAMA_TEST_EQ0(similarity2 - similarity1));

	NV_ASSERT(otama_feature_raw_file(otama, &p1, OTAMA_TEST_IMG) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_feature_raw_data(otama, &p2, data2, data2_len) == OTAMA_STATUS_OK);
	NV_ASSERT(otama_similarity_raw(otama, &similarity2, p1, p2)
			  == OTAMA_STATUS_OK);
	printf("%f == %f\n", similarity1, similarity2);
	NV_ASSERT(OTAMA_TEST_EQ0(similarity2 - similarity1));
	
	otama_close(&otama);
	otama_feature_string_free(&s1);
	otama_feature_string_free(&s2);
	otama_feature_raw_free(&p1);
	otama_feature_raw_free(&p2);
	nv_free(data1);
	nv_free(data2);
}
