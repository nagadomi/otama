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

#ifndef OTAMA_IMAGE_INTERNAL
#define OTAMA_IMAGE_INTERNAL

#ifdef __cplusplus
extern "C" {
#endif

#include "otama_config.h"
#include "otama_id.h"
#include "nv_core.h"

struct otama_image
{
	otama_id_t id;
	nv_matrix_t *image;
	int has_id;
};

#ifdef __cplusplus
}
#endif
	
#endif
