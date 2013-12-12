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
#ifndef OTAMA_SBOC_NODB_DRIVER_HPP
#define OTAMA_SBOC_NODB_DRIVER_HPP

#include "nv_color_boc.h"
#include "otama_nodb_driver.hpp"

namespace otama
{
	class SBOCNoDBDriver: public NoDBDriver<nv_color_sboc_t>
	{
	protected:
		virtual nv_color_sboc_t *
		feature_new(void)
		{
			return new nv_color_sboc_t;
		}
		
		virtual void
		feature_copy(nv_color_sboc_t *to, const nv_color_sboc_t *from)
		{
			*to = *from;
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
		
		virtual float
		feature_similarity(const nv_color_sboc_t *fixed1,
							const nv_color_sboc_t *fixed2,
							otama_variant_t *options)
		{
			return nv_color_sboc_similarity(fixed1, fixed2);
		}
		
	public:
		virtual std::string name(void) { return prefixed_name("otama_sboc"); }
		
		SBOCNoDBDriver(otama_variant_t *options)
			: NoDBDriver<nv_color_sboc_t>(options)
		{
		}
	};
};

#endif

