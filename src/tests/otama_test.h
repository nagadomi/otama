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

#ifndef OTAMA_TEST_HPP
#define OTAMA_TEST_HPP
#include <nv_core.h>

#ifdef __cplusplus
extern "C" {
#endif	

#define OTAMA_TEST_NAME (printf("---- %s\n", __FUNCTION__))
#define OTAMA_TEST_EPSILON (FLT_EPSILON * 4.0f)
#define OTAMA_TEST_EQ1(v) (!(fabsf((v) - 1.0f) > 0.00001f))
#define OTAMA_TEST_EQ0(v) (!(fabsf(v) > 0.00001f))
	
size_t otama_test_read_file(const char *file, void **p, size_t *len);
void otama_test_vlad(void);
void otama_test_bovw(void);
void otama_test_pqh(void);
void otama_test_api(const char *config);
void otama_test_similarity_api(const char *config);
void otama_test_cluster(const char *config1, const char *config2);
void otama_test_dbi(void);
	
#ifdef __cplusplus
}
#endif	

#endif

