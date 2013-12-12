/*
 * This file is part of otama.
 *
 * Copyright (C) 2013 nagadomi@nurs.or.jp
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
#ifndef OTAMA_BOVW_SPARSE_NODB_DRIVER_HPP
#define OTAMA_BOVW_SPARSE_NODB_DRIVER_HPP

#include "otama_nodb_driver.hpp"
#include "nv_bovw.hpp"
#include <vector>

namespace otama
{
	template <nv_bovw_bit_e BIT>
	class BOVWSparseNoDBDriver: public NoDBDriver<std::vector<uint32_t> >
	{
	protected:
		typedef std::vector<uint32_t> FT;
		typedef nv_bovw_ctx<BIT, nv_bovw_dummy_color_t> T;
		nv_bovw_rerank_method_t m_rerank_method;
		T *m_ctx;
		class IdfW: public InvertedIndex::WeightFunction {
		public:
			T *ctx;
			nv_bovw_rerank_method_t rerank_method;
			virtual float operator()(uint32_t x)
			{
				switch (rerank_method) {
				case NV_BOVW_RERANK_IDF:
					return ctx->idf(x);
				case NV_BOVW_RERANK_NONE:
					break;
				}
				return 1.0f;
			}
		};
		IdfW m_idf_w;
		
		virtual FT *
		feature_new(void)
		{
			return new FT;
		}
		
		virtual void
		feature_free(FT *fv)
		{
			delete fv;
		}
		static void
		feature_raw_free(void *p)
		{
			FT *ft = (FT *)p;
			delete ft;
		}
		virtual otama_feature_raw_free_t
		feature_free_func(void)
		{
			return feature_raw_free;
		}
		
		virtual void
		feature_copy(FT *to, const FT *from)
		{
			*to = *from;
		}
		
		virtual void
		feature_extract(FT *fv, nv_matrix_t *image)
		{
			m_ctx->extract(*fv, image);
		}
		
		virtual int
		feature_extract_file(FT *fv, const char *file, otama_variant_t *options)
		{
			if (m_ctx->extract(*fv, file)) {
				return 1;
			}
			
			return 0;
		}
		
		virtual int
		feature_extract_data(FT *fv,
							 const void *data, size_t data_len,
							 otama_variant_t *options)
		{
			if (m_ctx->extract(*fv, data, data_len)) {
				return 1;
			}
			
			return 0;
		}
		
		virtual int
		feature_deserialize(FT *fv, const char *s)
		{
			return m_ctx->deserialize(fv, s);
		}
		
		virtual char *
		feature_serialize(const FT *fv)
		{
			std::string s;
			char *ret;
			
			m_ctx->serialize(s, fv);
			ret = nv_alloc_type(char, s.size() + 1);
			strcpy(ret, s.c_str());
			
			return ret;
		}

		virtual InvertedIndex::WeightFunction *
		feature_weight_func(void)
		{
			return &m_idf_w;
		}

		virtual float
		feature_similarity(const FT *fv1,
						   const FT *fv2,
						   otama_variant_t *options)
		{
			InvertedIndex::WeightFunction *weight_func = this->feature_weight_func();
			InvertedIndex::sparse_vec_t intersection;
			InvertedIndex::sparse_vec_t::const_iterator it;
			float norm1 = 0.0f, norm2 = 0.0f, dot = 0.0f;
			
			std::set_intersection(fv1->begin(), fv1->end(),
								  fv2->begin(), fv2->end(),
								  std::back_inserter(intersection));
			
			for (it = fv1->begin(); it != fv1->end(); ++it) {
				float w = (*weight_func)(*it);
				norm1 += w * w;
			}
			for (it = fv2->begin(); it != fv2->end(); ++it) {
				float w = (*weight_func)(*it);				
				norm2 += w * w;
			}
			for (it = intersection.begin(); it != intersection.end(); ++it) {
				float w = (*weight_func)(*it);
				dot += w * w;
			}
			
			return dot / (sqrtf(norm1) * sqrtf(norm2));
		}
		
	public:
		static inline std::string
		itos(int i)	{ char buff[128]; sprintf(buff, "%d", i); return std::string(buff);	}
		
		virtual std::string
		name(void)
		{
			return this->prefixed_name(std::string("otama_bovw") + itos(BIT/1024) + "k_iv");
		}
		
		BOVWSparseNoDBDriver(otama_variant_t *options)
			: NoDBDriver<FT>(options)
		{
			otama_variant_t *driver, *value;
			
			m_ctx = NULL;
			m_rerank_method = NV_BOVW_RERANK_IDF;
			
			driver = otama_variant_hash_at(options, "driver");
			if (OTAMA_VARIANT_IS_HASH(driver)) {
				if (!OTAMA_VARIANT_IS_NULL(value = otama_variant_hash_at(driver, "rerank_method"))) {
					const char *s = otama_variant_to_string(value);
					if (nv_strcasecmp(s, "none") == 0) {
						m_rerank_method = NV_BOVW_RERANK_NONE;
					} else if (nv_strcasecmp(s, "idf") == 0) {
						m_rerank_method = NV_BOVW_RERANK_IDF;
					} else {
						OTAMA_LOG_NOTICE("invalid rerank_method `%s'", s);
					}
				}
			}
			switch (m_rerank_method) {
			case NV_BOVW_RERANK_IDF:
				OTAMA_LOG_DEBUG("driver[rerank_method] => %s", "idf");
				break;
			case NV_BOVW_RERANK_NONE:
				OTAMA_LOG_DEBUG("driver[rerank_method] => %s", "none");
				break;
			}
			m_idf_w.rerank_method = m_rerank_method;
			m_idf_w.ctx = NULL;
		}
		
		virtual otama_status_t
		open(void)
		{
			otama_status_t ret;
			
			ret = NoDBDriver<FT>::open();
			if (ret != OTAMA_STATUS_OK) {
				return ret;
			}
			m_ctx = new T;
			if (m_ctx->open() != 0) {
				return OTAMA_STATUS_SYSERROR;
			}
			m_idf_w.ctx = m_ctx;
			
			return OTAMA_STATUS_OK;
		}

		virtual otama_status_t
		close(void)
		{
			delete m_ctx;
			m_ctx = NULL;
			m_idf_w.ctx = NULL;
			return NoDBDriver<FT>::close();
		}
		
		~BOVWSparseNoDBDriver() {
			delete m_ctx;
		}
	};
}

#endif
