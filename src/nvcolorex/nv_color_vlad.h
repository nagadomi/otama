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
#ifndef NV_COLOR_VLAD_H
#define NV_COLOR_VLAD_H

#ifdef __cplusplus
extern "C" {
#endif

#define NV_COLOR_VLAD_COLOR  64
#define NV_COLOR_SVLAD_DIM (3 * NV_COLOR_VLAD_COLOR * (9 + 4 + 2 + 1)) // 3072
#define NV_COLOR_VLAD_DIM (3 * NV_COLOR_VLAD_COLOR) // 192

void nv_color_vlad(nv_matrix_t *vlad, int vlad_j, const nv_matrix_t *image);
void nv_color_svlad(nv_matrix_t *vlad, int vlad_j, const nv_matrix_t *image);

#ifdef __cplusplus
}
#endif
	
#endif

