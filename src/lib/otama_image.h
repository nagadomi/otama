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

#ifndef OTAMA_IMAGE_H
#define OTAMA_IMAGE_H
#include "otama_config.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	int x;
	int y;
	int width;
	int height;   
} otama_rect_t;

typedef struct otama_image otama_image_t;

otama_image_t *otama_image_load(const char *file);
otama_image_t *otama_image_load_rgb8(int width, int height, const void *data);
otama_image_t *otama_image_load_bgr8(int width, int height, const void *data); 
otama_image_t *otama_image_data(const void *data, size_t data_len);
otama_image_t *otama_image_crop(const otama_image_t *src, otama_rect_t rect);
void otama_image_free(otama_image_t **image);

#if OTAMA_WINDOWS
# include <windows.h>

otama_image_t *otama_image_load_dib(HBITMAP hDIB);

#endif

#ifdef __cplusplus
}
#endif

#endif
