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
#ifndef OTAMA_VLAD_TABLE_HPP
#define OTAMA_VLAD_TABLE_HPP

#include "otama_nodb_driver.hpp"
#include "nv_vlad.h"
#include <typeinfo>

namespace otama
{
	class VLADNoDBDriver: public NoDBDriver<nv_vlad_t>
	{
	protected:
		virtual nv_vlad_t *
		feature_new(void)
		{
			return nv_vlad_alloc();
		}
		
		virtual void
		feature_free(nv_vlad_t *fixed)
		{
			nv_vlad_free(&fixed);
		}
		static void
		feature_raw_free(void *p)
		{
			nv_vlad_t *ft = (nv_vlad_t *)p;
			nv_vlad_free(&ft);
		}
		virtual otama_feature_raw_free_t
		feature_free_func(void)
		{
			return feature_raw_free;
		}
		
		virtual void
		feature_copy(nv_vlad_t *to, const nv_vlad_t *from)
		{
			nv_vlad_copy(to, from);
		}
		
		virtual void
		feature_extract(nv_vlad_t *fixed, nv_matrix_t *image)
		{
			nv_vlad(fixed, image);
		}
		
		virtual int
		feature_extract_file(nv_vlad_t *fixed, const char *file)
		{
			return nv_vlad_file(fixed, file);
		}
		
		virtual int
		feature_extract_data(nv_vlad_t *fixed,
								   const void *data, size_t data_len)
		{
			return nv_vlad_data(fixed, data, data_len);
		}
		
		virtual int
		feature_deserialize(nv_vlad_t *fixed, const char *s)
		{
			nv_vlad_t *vlad = nv_vlad_deserialize(s);
			if (vlad) {
				nv_vlad_copy(fixed, vlad);
				nv_vlad_free(&vlad);
				return 0;
			} else {
				return -1;
			}
		}
		
		virtual char *
		feature_serialize(const nv_vlad_t *fixed)
		{
			return nv_vlad_serialize(fixed);
		}
		
		virtual float
		feature_similarity(const nv_vlad_t *fixed1,
							const nv_vlad_t *fixed2,
							otama_variant_t *options)
		{
			return nv_vlad_similarity(fixed1, fixed2);
		}
		
	public:
		virtual std::string name(void) { return "otama_vlad_nodb"; }
		VLADNoDBDriver(otama_variant_t *options)
			: NoDBDriver<nv_vlad_t>(options)
		{}
		~VLADNoDBDriver() {}
	};
}

#endif
