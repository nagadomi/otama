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
#ifndef OTAMA_SBOC_FIXED_DRIVER_HPP
#define OTAMA_SBOC_FIXED_DRIVER_HPP

#include "nv_color_boc.h"
#include "otama_fixed_driver.hpp"

namespace otama
{
	class SBOCFixedDriver: public FixedDriver<nv_color_sboc_t>
	{
	protected:
		virtual nv_color_sboc_t *
		feature_new(void)
		{
			return new nv_color_sboc_t;
		}
		
		virtual void
		feature_free(nv_color_sboc_t *fixed)
		{
			delete fixed;
		}
		
		static void
		feature_raw_free(void *p)
		{
			nv_color_sboc_t *ft = (nv_color_sboc_t *)p;
			delete ft;
		}
		virtual otama_feature_raw_free_t
		feature_free_func(void)
		{
			return feature_raw_free;
		}
		
		virtual void
		feature_extract(nv_color_sboc_t *fixed, nv_matrix_t *image)
		{
			nv_color_sboc(fixed, image);
		}
		
		virtual int
		feature_extract_file(nv_color_sboc_t *fixed, const char *file,
							 otama_variant_t *options)
		{
			return nv_color_sboc_file(fixed, file);
		}
		
		virtual int
		feature_extract_data(nv_color_sboc_t *fixed,
							 const void *data, size_t data_len,
							 otama_variant_t *options)
		{
			return nv_color_sboc_data(fixed, data, data_len);
		}
		
		virtual int
		feature_deserialize(nv_color_sboc_t *fixed, const char *s)
		{
			return nv_color_sboc_deserialize(fixed, s);
		}
		
		virtual char *
		feature_serialize(const nv_color_sboc_t *fixed)
		{
			return nv_color_sboc_serialize(fixed);
		}

		static inline void
		set_result(otama_result_t *results, int i,
				   const otama_id_t *id, float cosine)
		{
			otama_variant_t *hash = otama_result_value(results, i);
			otama_variant_t *c;

			otama_variant_set_hash(hash);
			c = otama_variant_hash_at(hash, "similarity");
			otama_variant_set_float(c, cosine);
			otama_result_set_id(results, i, id);
		}

		virtual otama_status_t
		feature_search(otama_result_t **results, int n,
							 const nv_color_sboc_t *query,
							 otama_variant_t *options)
		{
			const int first_n = n * 16;
			nv_color_boc_result_t *first_results = nv_alloc_type(nv_color_boc_result_t,
																 first_n);
			int nresult = 0;
			int i, j, results_size;

			*results = otama_result_alloc(n);
			
			sync();
			
			nresult = nv_color_sboc_search(first_results, first_n,
										   m_mmap->vec(), m_mmap->count(),
										   query);
			for (j = i = 0; j < nresult; ++j) {
				if ((m_mmap->flag_at(first_results[j].index) &
					 this->FLAG_DELETE) == 0)
				{
					first_results[i] = first_results[j];
					++i;
				}
			}
			nresult = i;
			results_size = 0;
			for (i = 0; i < nresult && i < n; ++i) {
				set_result(*results, results_size++,
						   m_mmap->id_at(first_results[i].index),
						   first_results[i].cosine);
			}
			otama_result_set_count(*results, results_size);
			
			nv_free(first_results);			
			
			return OTAMA_STATUS_OK;
		}
		
		virtual float
		feature_similarity(const nv_color_sboc_t *fixed1,
							const nv_color_sboc_t *fixed2,
							otama_variant_t *options)
		{
			return nv_color_sboc_similarity(fixed1, fixed2);
		}
		
	public:
		virtual std::string name(void) { return prefixed_name("otama_sboc"); } 
		SBOCFixedDriver(otama_variant_t *options)
			: FixedDriver<nv_color_sboc_t>(options)
		{
		}
	};
}

#endif

