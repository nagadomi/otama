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
#ifndef OTAMA_BOVW_NODB_TABLE_HPP
#define OTAMA_BOVW_NODB_TABLE_HPP

#include "otama_nodb_driver.hpp"
#include "nv_bovw.hpp"
#include <typeinfo>

namespace otama
{
	template <nv_bovw_bit_e BIT, typename COLOR_CLASS>	
	class BOVWNoDBDriver: public NoDBDriver<typename nv_bovw_ctx<BIT, COLOR_CLASS>::dense_t >
	{
	protected:
		typedef nv_bovw_ctx<BIT, COLOR_CLASS> T;
		typedef typename T::dense_t FT;
		
		static const int COLOR_NN = 3;
		static const float DEFAULT_COLOR_WEIGHT() { return 0.2f; }
		
		float m_color_weight;
		nv_bovw_rerank_method_t m_rerank_method;
		nv_matrix_t *m_color;
		T *m_ctx;
		
		virtual FT *
		feature_new(void)
		{
			return new FT;
		}
		
		virtual void
		feature_free(FT *fixed)
		{
			delete fixed;
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
		feature_extract(FT *fixed, nv_matrix_t *image)
		{
			m_ctx->extract(fixed, image);
		}
		
		virtual int
		feature_extract_file(FT *fixed, const char *file, otama_variant_t *options)
		{
			return m_ctx->extract(fixed, file);
		}
		
		virtual int
		feature_extract_data(FT *fixed,
							 const void *data, size_t data_len,
							 otama_variant_t *options)
		{
			return m_ctx->extract(fixed, data, data_len);
		}
		
		virtual int
		feature_deserialize(FT *fixed, const char *s)
		{
			return m_ctx->deserialize(fixed, s);
		}
		
		virtual char *
		feature_serialize(const FT *fixed)
		{
			char *s = nv_alloc_type(char, T::SERIALIZE_LEN + 1);
			m_ctx->serialize(s, fixed);
			return s;
		}
		
		virtual float
		feature_similarity(const FT *fixed1,
							const FT *fixed2,
							otama_variant_t *options)
		{
			return m_ctx->similarity(fixed1, fixed2, m_rerank_method, m_color_weight);
		}
		
	public:
		static inline std::string
		itos(int i)	{ char buff[128]; sprintf(buff, "%d", i); return std::string(buff);	}
		
		virtual std::string
		name(void)
		{
			if (typeid(COLOR_CLASS) == typeid(nv_color_sboc_t)) {
				return this->prefixed_name(std::string("otama_bovw") + itos(BIT/1024) + "k_sboc");
			} else {
				return this->prefixed_name(std::string("otama_bovw") + itos(BIT/1024) + "k");
			}
		}
		
		BOVWNoDBDriver(otama_variant_t *options)
			: NoDBDriver<FT>(options)
		{
			otama_variant_t *driver, *value;

			m_color = nv_matrix_alloc(3, 1);
			m_color_weight = DEFAULT_COLOR_WEIGHT();
			m_rerank_method = NV_BOVW_RERANK_IDF;
			
			driver = otama_variant_hash_at(options, "driver");
			if (OTAMA_VARIANT_IS_HASH(driver)) {
				if (!OTAMA_VARIANT_IS_NULL(value = otama_variant_hash_at(driver, "color_weight"))) {
					m_color_weight = otama_variant_to_float(value);
				}
				if (!OTAMA_VARIANT_IS_NULL(value = otama_variant_hash_at(driver, "rerank_method"))) {
					const char *s = otama_variant_to_string(value);
					if (nv_strcasecmp(s, "none") == 0) {
						m_rerank_method = NV_BOVW_RERANK_NONE;
					} else if (nv_strcasecmp(s, "idf") == 0) {
						m_rerank_method = NV_BOVW_RERANK_IDF;
					} else {
						OTAMA_LOG_ERROR("invalid rerank_method `%s'", s);
					}
				}
			}
			
			OTAMA_LOG_DEBUG("driver[color_weight] => %f", m_color_weight);
			switch (m_rerank_method) {
			case NV_BOVW_RERANK_IDF:
				OTAMA_LOG_DEBUG("driver[rerank_method] => %s", "idf");
				break;
			case NV_BOVW_RERANK_NONE:
				OTAMA_LOG_DEBUG("driver[rerank_method] => %s", "none");
				break;
			}
			m_ctx = new T;
			m_ctx->open();
		}
		~BOVWNoDBDriver() {
			nv_matrix_free(&m_color);
			delete m_ctx;
		}

		virtual otama_status_t
		set(const std::string &key, otama_variant_t *value)
		{
#ifdef _OPENMP
			OMPLock lock(NoDBDriver<FT>::m_lock);
#endif
			OTAMA_LOG_DEBUG("set key: %s\n", key.c_str());
			
			if (key == "color_weight") {
				m_color_weight = otama_variant_to_float(value);
				return OTAMA_STATUS_OK;
			}
			return NoDBDriver<FT>::set(key, value);
		}
		
		virtual otama_status_t
		unset(const std::string &key)
		{
#ifdef _OPENMP
			OMPLock lock(NoDBDriver<FT>::m_lock);
#endif
			OTAMA_LOG_DEBUG("unset key: %s\n", key.c_str());
			if (key == "color_weight") {
				m_color_weight = DEFAULT_COLOR_WEIGHT();
				return OTAMA_STATUS_OK;
			}
			
			return NoDBDriver<FT>::unset(key);
		}
	};
}

#endif
