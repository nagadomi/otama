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
#ifndef OTAMA_BOVW_FIXED_TABLE_HPP
#define OTAMA_BOVW_FIXED_TABLE_HPP

#include "otama_fixed_driver.hpp"
#include "nv_bovw.hpp"
#include <typeinfo>

namespace otama
{
	template <nv_bovw_bit_e BIT, typename COLOR_CLASS>
	class BOVWFixedDriver:
		public FixedDriver<typename nv_bovw_ctx<BIT, COLOR_CLASS>::dense_t >
	{
	protected:
		typedef nv_bovw_ctx<BIT, COLOR_CLASS> T;
		typedef typename T::dense_t FT;
		
		static const int CLUSTER_K = 16;
		static const int COLOR_NN = 3;
		static const float OVERFIT_TH() { return 0.8f; }
		static const float DEFAULT_COLOR_WEIGHT() { return 0.32f; }
		
		bool m_strip;
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
		feature_extract(FT *fixed, nv_matrix_t *image)
		{
			m_ctx->extract(fixed, image);
		}
		
		virtual int
		feature_extract_file(FT *fixed, const char *file)
		{
			return m_ctx->extract(fixed, file);
		}
		
		virtual int
		feature_extract_data(FT *fixed,
								   const void *data, size_t data_len)
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
			const int first_n = m_strip ? (n * CLUSTER_K / 2) : n;
			nv_bovw_result_t *first_results = nv_alloc_type(nv_bovw_result_t, first_n);
			int nresult = 0;
			int i, j, results_size;
			FT bovw = *query;
			float color_weight = m_color_weight;
			otama_variant_t *cw;
			
			if (OTAMA_VARIANT_IS_HASH(cw = otama_variant_hash_at(options, "color_weight"))) {
				color_weight = otama_variant_to_float(cw);
			}
			
			*results = otama_result_alloc(n);
			nresult = m_ctx->search(first_results, first_n,
									FixedDriver<FT>::m_mmap->vec(),
									FixedDriver<FT>::m_mmap->count(),
									&bovw,
									m_rerank_method, color_weight);
			
			for (j = i = 0; j < nresult; ++j) {
				if ((FixedDriver<FT>::m_mmap->flag_at(first_results[j].index) &
					 FixedDriver<FT>::FLAG_DELETE) == 0)
				{
					first_results[i] = first_results[j];
					++i;
				}
			}
			nresult = i;
			if (nresult > CLUSTER_K && m_strip) {
				int  max_k, step;
				nv_matrix_t *similarity, *labels, *centroid, *count;
				
				similarity = nv_matrix_alloc(1, nresult);
				labels = nv_matrix_alloc(1, nresult);
				centroid = nv_matrix_alloc(1, CLUSTER_K);
				count = nv_matrix_alloc(1, CLUSTER_K);
				step = nresult / CLUSTER_K;
		
				nv_matrix_zero(labels);
				nv_matrix_zero(count);
				
				for (i = 0; i < nresult; ++i) {
					NV_MAT_V(similarity, i, 0) = first_results[i].similarity;
				}
				for (i = 0; i < CLUSTER_K; ++i) {
					NV_MAT_V(centroid, i, 0) = NV_MAT_V(similarity, i * step, 0);
				}
				nv_kmeans_em(centroid, count, labels, similarity, CLUSTER_K, 50);
		
				max_k = (int)NV_MAT_V(labels, 0, 0);
				
				results_size = 0;
				for (i = 0; i < nresult && i < n; ++i) {
					if ((int)NV_MAT_V(labels, i, 0) == max_k) {
						set_result(*results, results_size++,
								   FixedDriver<FT>::m_mmap->id_at(first_results[i].index),
								   first_results[i].similarity);
					} else {
						break;
					}
				}
				
				if ((int)results_size < nresult && (int)results_size < n) {
					int over_fit = 1;
					for (i = 0; i < results_size; ++i) {
						if (first_results[i].similarity < OVERFIT_TH()) {
							over_fit = 0;
							break;
						}
					}
					if (over_fit) {
						int next_label = results_size;
						max_k = (int)NV_MAT_V(labels, next_label, 0);
						
						for (i = results_size; i < nresult && i < n; ++i) {
							if ((int)NV_MAT_V(labels, i, 0) == max_k) {
								set_result(*results, results_size++,
										   FixedDriver<FT>::m_mmap->id_at(first_results[i].index),
										   first_results[i].similarity);
							} else {
								break;
							}
						}
					}
					otama_result_set_count(*results, results_size);
				}
				nv_matrix_free(&similarity);
				nv_matrix_free(&labels);
				nv_matrix_free(&centroid);
				nv_matrix_free(&count);
			} else {
				results_size = 0;
				for (i = 0; i < nresult && i < n; ++i) {
					set_result(*results, results_size++,
							   FixedDriver<FT>::m_mmap->id_at(first_results[i].index),
							   first_results[i].similarity);
				}
				otama_result_set_count(*results, results_size);		
			}

			nv_free(first_results);
	
			return OTAMA_STATUS_OK;
		}

		void update_idf(otama_variant_t *value)
		{
			int stopword_th = 0;
			int64_t i, count = this->m_mmap->count();
			nv_matrix_t *freq = nv_matrix_alloc(T::BIT, 1);
			nv_matrix_t *vec = nv_matrix_alloc(T::BIT, 1);
			const FT *db = this->m_mmap->vec();
			
			if (value) {
				stopword_th = (int)otama_variant_to_int(value);
			}
			nv_matrix_zero(freq);
			
			for (i = 0; i < count; ++i) {
				m_ctx->decode(vec, 0, &db[i]);
				nv_vector_add(freq, 0, freq, 0, vec, 0);
			}
			m_ctx->update_idf(freq, 0, count, stopword_th);
			
			nv_matrix_free(&freq);
			nv_matrix_free(&vec);
		}
		
	public:
		static inline std::string
		itos(int i)	{ char buff[128]; sprintf(buff, "%d", i); return std::string(buff);	}
		virtual std::string
		name(void)
		{
			if (typeid(COLOR_CLASS) == typeid(nv_color_sboc_t)) {
				return prefixed_name(std::string("otama_bovw") + itos(BIT/1024) + "k_sboc");
			} else {
				return prefixed_name(std::string("otama_bovw") + itos(BIT/1024) + "k");
			}
		}
		
		BOVWFixedDriver(otama_variant_t *options)
			: FixedDriver<FT>(options)
		{
			otama_variant_t *driver, *value;
			
			m_color = nv_matrix_alloc(3, 1);
			m_color_weight = DEFAULT_COLOR_WEIGHT();
			m_strip = false;
			m_rerank_method = NV_BOVW_RERANK_IDF;
			
			driver = otama_variant_hash_at(options, "driver");
			if (OTAMA_VARIANT_IS_HASH(driver)) {
				if (!OTAMA_VARIANT_IS_NULL(value = otama_variant_hash_at(driver, "color_weight"))) {
					m_color_weight = otama_variant_to_float(value);
				}
				if (!OTAMA_VARIANT_IS_NULL(value = otama_variant_hash_at(driver, "strip"))) {
					m_strip = otama_variant_to_bool(value) ? true : false;
				}
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
			
			OTAMA_LOG_DEBUG("driver[color_weight] => %f", m_color_weight);
			OTAMA_LOG_DEBUG("driver[strip] => %d", m_strip ? 1 : 0);
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
		~BOVWFixedDriver() {
			nv_matrix_free(&m_color);
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
			} else if (key == "strip") {
				m_strip = otama_variant_to_bool(value) ? true : false;
				return OTAMA_STATUS_OK;
			} else if (key == "update_idf") {
				update_idf(value);
				return OTAMA_STATUS_OK;
			}
			return FixedDriver<FT>::set(key, value);
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
			} else if (key == "strip") {
				m_strip = false;
				return OTAMA_STATUS_OK;
			}
			
			return FixedDriver<FT>::unset(key);
		}
	};
}

#endif

