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

#ifndef NV_COLOR_MAJOR_H
#define NV_COLOR_MAJOR_H

#include "nv_core.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	float color1[3];
	float color2[3];
} nv_color_major2_t;

float
nv_color_major2_similarity(const nv_color_major2_t *a,
					  const nv_color_major2_t *b);

void nv_color_major2(nv_color_major2_t *major, const nv_matrix_t *image);

#ifdef __cplusplus
}
#endif

#endif


