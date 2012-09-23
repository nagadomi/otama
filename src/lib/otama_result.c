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

#include "otama_result.h"
#include "otama_variant.h"
#include "nv_core.h"

struct otama_result {
	int count;
	otama_id_t *ids;
	otama_variant_t *value_array;
	otama_variant_pool_t *pool;
};

otama_result_t *
otama_result_alloc(int count)
{
	otama_result_t *results = nv_alloc_type(otama_result_t, 1);
	
	results->ids = nv_alloc_type(otama_id_t, count);
	results->pool = otama_variant_pool_alloc();
	results->value_array = otama_variant_new(results->pool);
	otama_variant_set_array(results->value_array);

	results->count = count;
	memset(results->ids, 0, sizeof(otama_id_t) * count);

	return results;
}

void
otama_result_free(otama_result_t **results)
{
	if (results && *results) {
		otama_variant_pool_free(&(*results)->pool);
		nv_free((*results)->ids);
		nv_free(*results);
		*results = NULL;
	}
}

int
otama_result_set_count(otama_result_t *results, int n)
{
	NV_ASSERT(results->count >= n);
	
	results->count = n;
	
	return results->count;
}

int
otama_result_count(const otama_result_t *results)
{
	return results->count;
}

const otama_id_t *
otama_result_get_id(const otama_result_t *results, int i)
{
	return &results->ids[i];
}

otama_variant_t *
otama_result_value(const otama_result_t *results, int i)
{
	return otama_variant_array_at(results->value_array, i);
}

const otama_id_t *
otama_result_id(const otama_result_t *results, int i)
{
	return &results->ids[i];
}

void
otama_result_set_id(otama_result_t *results, int i, const otama_id_t *id)
{
	memcpy(&results->ids[i], id, sizeof(*id));
}
