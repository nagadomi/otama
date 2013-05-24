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
#ifndef OTAMA_LMCA_FIXED_TABLE_HPP
#define OTAMA_LMCA_FIXED_TABLE_HPP

#include "otama_fixed_driver.hpp"
#include "nv_lmca.hpp"
#include <string>

namespace otama
{
	template<nv_lmca_feature_e F, typename C>
	class LMCAFixedDriver:
		public FixedDriver<typename nv_lmca_ctx<F, C>::vector_t>
	{
	protected:
		typedef nv_lmca_ctx<F, C> T;
		typedef typename T::vector_t FT;
		
		std::string m_lmca_file;
		std::string m_lmca2_file;
		std::string m_vq_file;
		float m_color_threshold;
		float m_color_weight;
		
		static const float DEFAULT_COLOR_THRESHOLD() { return 0.7f; }
		static const float DEFAULT_COLOR_WEIGHT() { return 0.5f; }
		static const typename T::color_method_e DEFAULT_COLOR_METHOD = T::COLOR_METHOD_LINEAR;
		
		typename T::color_method_e m_color_method;
		T *m_ctx;
		
		virtual FT *
		feature_new(void)
		{
			FT *p;
			int ret = nv_aligned_malloc((void **)&p, 32, sizeof(FT));
			if (ret) {
				return NULL;
			}
			
			return p;
		}
		
		virtual void
		feature_free(FT *fixed)
		{
			nv_aligned_free(fixed);
		}
		
		static void
		feature_raw_free(void *p)
		{
			nv_aligned_free(p);
		}
		
		virtual otama_feature_raw_free_t
		feature_free_func(void)
		{
			return feature_raw_free;
		}
		
		virtual void
		feature_extract(FT *fixed, nv_matrix_t *image)
		{
			m_ctx->extract(fixed, image);
		}

		float *
		colorcode(const nv_lmca_empty_color_t *, float *v,
				  otama_variant_t *options) { return NULL; }
		float *
		colorcode(const nv_lmca_hsv_t *, float *v,
				  otama_variant_t *options) { return NULL; }
		float *
		colorcode(const nv_lmca_colorcode_t *, float *v,
				  otama_variant_t *options)
		{
			otama_variant_t *colorcode = otama_variant_hash_at(options, "colorcode");
			if (OTAMA_VARIANT_IS_ARRAY(colorcode)) {
				int i;
				v[0] = v[1] = v[2] = v[3] = -255.0f;
				for (i = 0; i < 4 && i < otama_variant_array_count(colorcode); ++i) {
					v[i] = otama_variant_to_float(otama_variant_array_at(colorcode, i));
				}
				return v;
			}
			return NULL;
		}
		
		virtual int
		feature_extract_file(FT *fixed, const char *file,
							 otama_variant_t *options)
		{
			float v[4];
			return m_ctx->extract(fixed, file, colorcode(&fixed->color, v, options));
		}
		
		virtual int
		feature_extract_data(FT *fixed,
							 const void *data, size_t data_len,
							 otama_variant_t *options)
		{
			float v[4];			
			return m_ctx->extract(fixed, data, data_len,
								  colorcode(&fixed->color, v, options));
		}
		
		virtual int
		feature_deserialize(FT *fixed, const char *s)
		{
			return m_ctx->deserialize(fixed, s);
		}
		
		virtual char *
		feature_serialize(const FT *fixed)
		{
			std::string str = m_ctx->serialize(fixed);
			char *s = nv_alloc_type(char, str.size() + 1);
			strcpy(s, str.c_str());
			return s;
		}

		virtual float
		feature_similarity(const FT *fixed1,
						   const FT *fixed2,
						   otama_variant_t *options)
		{
			otama_variant_t *value;
			float color_weight = m_color_weight;
			float color_threshold = m_color_threshold;
			typename T::color_method_e color_method = m_color_method;
			
			if (OTAMA_VARIANT_IS_HASH(options)) {
				value = otama_variant_hash_at(options, "color_method");
				if (!OTAMA_VARIANT_IS_NULL(value)) {
					const char *method = otama_variant_to_string(value);
					if (nv_strcasecmp(method, "linear") == 0) {
						color_method = T::COLOR_METHOD_LINEAR;
					} else if (nv_strcasecmp(method, "step") == 0) {
						color_method = T::COLOR_METHOD_STEP;
					}
				}
				value = otama_variant_hash_at(options, "color_threshold");
				if (!OTAMA_VARIANT_IS_NULL(value)) {
					color_threshold = otama_variant_to_float(value);
				}
				value = otama_variant_hash_at(options, "color_weight");
				if (!OTAMA_VARIANT_IS_NULL(value)) {
					color_weight = otama_variant_to_float(value);
				}
			}
			
			return T::similarity(fixed1, fixed2, color_method, color_weight, color_threshold);
		}
		
		static inline void
		set_result(otama_result_t *results, int i,
				   const otama_id_t *id, float similarity)
		{
			otama_variant_t *hash = otama_result_value(results, i);
			otama_variant_t *c;
			
			otama_variant_set_hash(hash);
			c = otama_variant_hash_at(hash, "similarity");
			otama_variant_set_float(c, similarity);
			otama_result_set_id(results, i, id);
		}

		virtual otama_status_t
		feature_search(otama_result_t **results, int n,
					   const FT *query,
					   otama_variant_t *options)
		{
			const int first_n = n * 2;
			nv_lmca_result_t *first_results = nv_alloc_type(nv_lmca_result_t, first_n);
			int nresult = 0;
			int i, j, results_size;
			otama_variant_t *value;
			float color_weight = m_color_weight;
			float color_threshold = m_color_threshold;
			typename T::color_method_e color_method = m_color_method;

			if (OTAMA_VARIANT_IS_HASH(options)) {
				value = otama_variant_hash_at(options, "color_method");
				if (!OTAMA_VARIANT_IS_NULL(value)) {
					const char *method = otama_variant_to_string(value);
					if (nv_strcasecmp(method, "linear") == 0) {
						color_method = T::COLOR_METHOD_LINEAR;
					} else if (nv_strcasecmp(method, "step") == 0) {
						color_method = T::COLOR_METHOD_STEP;
					}
				}
				value = otama_variant_hash_at(options, "color_threshold");
				if (!OTAMA_VARIANT_IS_NULL(value)) {
					color_threshold = otama_variant_to_float(value);
				}
				value = otama_variant_hash_at(options, "color_weight");
				if (!OTAMA_VARIANT_IS_NULL(value)) {
					color_weight = otama_variant_to_float(value);
				}
			}
			this->sync();
			*results = otama_result_alloc(n);
			nresult = m_ctx->search(first_results, first_n,
									FixedDriver<FT>::m_mmap->vec(),
									FixedDriver<FT>::m_mmap->count(),
									query,
									color_method,
									color_weight,
									color_threshold);
			for (j = i = 0; j < nresult; ++j) {
				if ((FixedDriver<FT>::m_mmap->flag_at(first_results[j].index) &
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
						   FixedDriver<FT>::m_mmap->id_at(first_results[i].index),
						   first_results[i].similarity);
			}
			otama_result_set_count(*results, results_size);
			
			nv_free(first_results);
			
			return OTAMA_STATUS_OK;
		}
		
	public:
		virtual std::string
		name(void)
		{
			switch (F) {
			case NV_LMCA_FEATURE_VLAD:
				return this->prefixed_name("otama_lmca_vlad");
			case NV_LMCA_FEATURE_HSV:
				return this->prefixed_name("otama_lmca_hsv");
			case NV_LMCA_FEATURE_VLADHSV:
				return this->prefixed_name("otama_lmca_vladhsv");
			case NV_LMCA_FEATURE_VLAD_COLORCODE:
				return this->prefixed_name("otama_lmca_vlad_colorcode");
			case NV_LMCA_FEATURE_VLAD_HSV:
				return this->prefixed_name("otama_lmca_vlad_hsv");
			default:
				NV_ASSERT("unknown driver" == NULL);
				return "unknown";
			}
		}
		
		LMCAFixedDriver(otama_variant_t *options)
		: FixedDriver<FT>(options),
		  m_color_threshold(0.0f), m_color_method(T::COLOR_METHOD_LINEAR)
		{
			otama_variant_t *driver, *value;
			
			m_color_method = DEFAULT_COLOR_METHOD;
			m_color_threshold = DEFAULT_COLOR_THRESHOLD();
			switch (F) {
			case NV_LMCA_FEATURE_VLAD_HSV:
			case NV_LMCA_FEATURE_VLAD_COLORCODE:
				m_color_weight = DEFAULT_COLOR_WEIGHT();
				break;
			case NV_LMCA_FEATURE_VLADHSV:
			case NV_LMCA_FEATURE_VLAD:
			case NV_LMCA_FEATURE_HSV:
			default:
				m_color_weight = 0.0f;
				break;
			}
			
			driver = otama_variant_hash_at(options, "driver");
			if (OTAMA_VARIANT_IS_HASH(driver)) {
				value = otama_variant_hash_at(driver, "metric");
				if (!OTAMA_VARIANT_IS_NULL(value)) {
					if (OTAMA_VARIANT_IS_ARRAY(value)) {
						int64_t len = otama_variant_array_count(value);
						if (len == 1) {
							m_lmca_file = otama_variant_to_string(value);
						} else if (len > 1) {
							m_lmca_file = otama_variant_to_string(otama_variant_array_at(value, 0));
							m_lmca2_file = otama_variant_to_string(otama_variant_array_at(value, 1));
						}
					} else {
						m_lmca_file = otama_variant_to_string(value);
					}
				}
				value = otama_variant_hash_at(driver, "vq");
				if (!OTAMA_VARIANT_IS_NULL(value)) {
					m_vq_file = otama_variant_to_string(value);
				}
				value = otama_variant_hash_at(driver, "color_weight");
				if (!OTAMA_VARIANT_IS_NULL(value)) {
					m_color_weight = otama_variant_to_float(value);
				}
				value = otama_variant_hash_at(driver, "color_method");
				if (!OTAMA_VARIANT_IS_NULL(value)) {
					const char *method = otama_variant_to_string(value);
					if (nv_strcasecmp(method, "linear") == 0) {
						m_color_method = T::COLOR_METHOD_LINEAR;
					} else if (nv_strcasecmp(method, "step") == 0) {
						m_color_method = T::COLOR_METHOD_STEP;
					}
				}
				value = otama_variant_hash_at(driver, "color_threshold");
				if (!OTAMA_VARIANT_IS_NULL(value)) {
					m_color_threshold = otama_variant_to_float(value);
				}
			}
			m_ctx = new T;
		}

		otama_status_t
		open(void)
		{
			otama_status_t ret;
			ret = FixedDriver<FT>::open();
			if (ret != OTAMA_STATUS_OK) {
				return ret;
			}
			if (m_lmca_file.size() == 0) {
				OTAMA_LOG_ERROR("%s", "metric[0] file is NULL");
				return OTAMA_STATUS_INVALID_ARGUMENTS;
			}
			if (F == NV_LMCA_FEATURE_VLAD_HSV) {
				if (m_lmca2_file.size() == 0) {
					OTAMA_LOG_ERROR("%s", "metric[1] file is NULL");
					return OTAMA_STATUS_INVALID_ARGUMENTS;
				}
				if (m_ctx->open(m_lmca_file.c_str(), m_lmca2_file.c_str()) != 0) {
					OTAMA_LOG_ERROR("invalid metric file: %s, %s",
									m_lmca_file.c_str(),
									m_lmca2_file.c_str());
					return OTAMA_STATUS_SYSERROR;
				}
			} else {
				if (m_ctx->open(m_lmca_file.c_str()) != 0) {
					OTAMA_LOG_ERROR("invalid metric file: %s", m_lmca_file.c_str());
					return OTAMA_STATUS_SYSERROR;
				}
			}
			if (m_vq_file.size() != 0) {
				if (m_ctx->set_vq_table(m_vq_file.c_str()) != 0) {
					OTAMA_LOG_ERROR("invalid vq file: %s", m_vq_file.c_str());
					return OTAMA_STATUS_SYSERROR;
				}
			}
			return OTAMA_STATUS_OK;
		}
		~LMCAFixedDriver() {
			delete m_ctx;
		}
		
		virtual otama_status_t
		set(const std::string &key, otama_variant_t *value)
		{
#ifdef _OPENMP
			OMPLock lock(FixedDriver<FT>::m_lock);
#endif
			OTAMA_LOG_DEBUG("set key: %s\n", key.c_str());
			
			if (key == "color_weight") {
				m_color_weight = otama_variant_to_float(value);
				return OTAMA_STATUS_OK;
			}
			return FixedDriver<FT>::set(key, value);
		}
		
		virtual otama_status_t
		get(const std::string &key,
			otama_variant_t *value)
		{
#ifdef _OPENMP
			OMPLock lock(FixedDriver<FT>::m_lock);
#endif
			if (key == "color_weight") {
				otama_variant_set_float(value, m_color_weight);
				return OTAMA_STATUS_OK;
			}
			return FixedDriver<FT>::get(key, value);
		}
		
		virtual otama_status_t
		unset(const std::string &key)
		{
#ifdef _OPENMP
			OMPLock lock(FixedDriver<FT>::m_lock);
#endif
			OTAMA_LOG_DEBUG("unset key: %s\n", key.c_str());
			if (key == "color_weight") {
				m_color_weight = DEFAULT_COLOR_WEIGHT();
				return OTAMA_STATUS_OK;
			}
			return FixedDriver<FT>::unset(key);
		}
	};
}

#endif

