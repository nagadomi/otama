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
#include "nv_vlad.hpp"
#include <typeinfo>

namespace otama
{
	template <nv_vlad_word_e K>
	class VLADNoDBDriver: public NoDBDriver<nv_matrix_t>
	{
	protected:
		nv_vlad_ctx<K> m_ctx;
		
		virtual nv_matrix_t *
		feature_new(void)
		{
			return nv_matrix_alloc(nv_vlad_ctx<K>::DIM, 1);
		}
		
		virtual void
		feature_free(nv_matrix_t *fv)
		{
			nv_matrix_free(&fv);
		}
		static void
		feature_raw_free(void *p)
		{
			nv_matrix_t *ft = (nv_matrix_t *)p;
			nv_matrix_free(&ft);
		}
		virtual otama_feature_raw_free_t
		feature_free_func(void)
		{
			return feature_raw_free;
		}
		
		virtual void
		feature_copy(nv_matrix_t *to, const nv_matrix_t *from)
		{
			nv_vector_copy(to, 0, from, 0);
		}
		
		virtual void
		feature_extract(nv_matrix_t *fv, nv_matrix_t *image)
		{
			m_ctx.extract(fv, 0, image);
		}
		
		virtual int
		feature_extract_file(nv_matrix_t *fv, const char *file,
							 otama_variant_t *options)
		{
			return m_ctx.extract(fv, 0, file);
		}
		
		virtual int
		feature_extract_data(nv_matrix_t *fv,
							 const void *data, size_t data_len,
							 otama_variant_t *options)
		{
			return m_ctx.extract(fv, 0, data, data_len);
		}
		
		virtual int
		feature_deserialize(nv_matrix_t *fv, const char *s)
		{
			return m_ctx.deserialize(fv, 0, s);
		}
		
		virtual char *
		feature_serialize(const nv_matrix_t *fv)
		{
			return m_ctx.serialize(fv, 0);
		}
		
		virtual float
		feature_similarity(const nv_matrix_t *fv1,
							const nv_matrix_t *fv2,
							otama_variant_t *options)
		{
			return m_ctx.similarity(fv1, 0, fv2, 0);
		}
		
	public:
		virtual std::string name(void) { return "otama_vlad_nodb"; }
		VLADNoDBDriver(otama_variant_t *options)
			: NoDBDriver<nv_matrix_t>(options)
		{}
		~VLADNoDBDriver() {}
	};
}

#endif
