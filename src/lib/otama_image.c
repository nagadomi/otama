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

#include "otama_config.h"
#include "otama_image.h"
#include "otama_image_internal.h"
#include "nv_io.h"
#include "nv_ip.h"

otama_image_t *
otama_image_load(const char *file)
{
	nv_matrix_t *image = nv_load_image(file);
	if (image) {
		otama_image_t *p = nv_alloc_type(otama_image_t, 1);

		p->has_id = 1;
		p->image = image;
		otama_id_file(&p->id, file);
		return p;
	} else {
		return NULL;
	}
}

otama_image_t *
otama_image_data(const void *data, size_t data_len)
{
	nv_matrix_t *image = nv_decode_image(data, data_len);
	if (image) {
		otama_image_t *p = nv_alloc_type(otama_image_t, 1);
		
		p->has_id = 1;
		p->image = image;
		otama_id_data(&p->id, data, data_len);
		return p;
	} else {
		return NULL;
	}
}

otama_image_t *
otama_image_crop(const otama_image_t *src, otama_rect_t roi)
{
	otama_image_t *p = nv_alloc_type(otama_image_t, 1);
	nv_rect_t rect;

	rect.x = roi.x;
	rect.y = roi.y;
	rect.width = roi.width;
	rect.height = roi.height;

	p->has_id = 0;
	p->image = nv_crop(src->image, rect);
	memset(&p->id, 0, sizeof(otama_id_t)); // TODO: insert

	return p;
}

void
otama_image_free(otama_image_t **image)
{
	if (image && *image) {
		nv_matrix_free(&(*image)->image);
		nv_free(*image);
		*image = NULL;
	}
}

otama_image_t *
otama_image_load_rgb8(int width, int height, const void *data)
{
	otama_image_t *p = nv_alloc_type(otama_image_t, 1);
	int x, y, c;
	const uint8_t *d = (const uint8_t *)data;

	p->has_id = 0;
	p->image = nv_matrix3d_alloc(3, height, width);
	memset(&p->id, 0, sizeof(otama_id_t)); // TODO: insert
	
	for (y = 0; y < height; ++y) {
		for (x = 0; x < height; ++x) {
			for (c = 0; c < 3; ++c) {
				NV_MAT3D_V(p->image, y, x, 2 - c) = (float)*d++;
			}
		}
	}

	return p;
}

otama_image_t *
otama_image_load_bgr8(int width, int height, const void *data)
{
	otama_image_t *p = nv_alloc_type(otama_image_t, 1);
	int x, y, c;
	const uint8_t *d = (const uint8_t *)data;
	
	p->has_id = 0;
	p->image = nv_matrix3d_alloc(3, height, width);
	memset(&p->id, 0, sizeof(otama_id_t)); // TODO: insert
	
	for (y = 0; y < height; ++y) {
		for (x = 0; x < height; ++x) {
			for (c = 0; c < 3; ++c) {
				NV_MAT3D_V(p->image, y, x, c) = (float)*d++;
			}
		}
	}

	return p;
}

#if OTAMA_WINDOWS

otama_image_t *
otama_image_load_dib(HBITMAP hDIB)
{
	nv_matrix_t *image = nv_decode_dib(hDIB);
	if (image) {
		otama_image_t *p = nv_alloc_type(otama_image_t, 1);
		p->has_id = 0;
		p->image = image;
		memset(&p->id, 0, sizeof(otama_id_t)); // TODO: insert
		return p;
	} else {
		return NULL;
	}
}

#endif
