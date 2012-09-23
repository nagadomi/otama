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

#ifndef NV_BOVW_INTERNAL_H
#define NV_BOVW_INTERNAL_H

#include "nv_core.h"
#include "nv_ip.h"
#include "nv_ml.h"

#ifdef __cplusplus
extern "C" {
#endif

/* bovw */
#define NV_BOVW_BOVW_POSI_N   1024

#define NV_BOVW_BIT_INDEX(n) ((n) / 64)
#define NV_BOVW_BIT_BIT(n)   ((n) % 64)

int nv_bovw_keypoints(nv_matrix_t *desc_vec,	nv_matrix_t *key_vec, const nv_matrix_t *image);

#ifdef __cplusplus
}
#endif

#endif
