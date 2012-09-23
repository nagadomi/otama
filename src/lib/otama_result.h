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
#ifndef OTAMA_RESULT_H
#define OTAMA_RESULT_H

#include "otama_id.h"
#include "otama_variant.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct otama_result otama_result_t;


otama_result_t *otama_result_alloc(int count);
int	otama_result_set_count(otama_result_t *results, int n);
void otama_result_set_id(otama_result_t *results, int i,
						 const otama_id_t *id);

int otama_result_count(const otama_result_t *results);
const otama_id_t *otama_result_id(const otama_result_t *results, int index);
otama_variant_t *otama_result_value(const otama_result_t *results, int index);
void otama_result_free(otama_result_t **results);

#ifdef __cplusplus
}
#endif

#endif
