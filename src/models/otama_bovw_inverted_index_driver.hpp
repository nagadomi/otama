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
#ifndef OTAMA_BOVW_INVERTED_INDEX_DRIVER_HPP
#define OTAMA_BOVW_INVERTED_INDEX_DRIVER_HPP

#include "nv_bovw.hpp"
#include "otama_inverted_index_driver.hpp"
#include <functional>

namespace otama
{
	template <nv_bovw_bit_e BIT, typename IV>
	class BOVWInvertedIndexDriver:
		public InvertedIndexDriver<InvertedIndex::sparse_vec_t, IV>
	{
	protected:
		typedef InvertedIndex::sparse_vec_t FT;
		typedef nv_bovw_ctx<BIT, nv_bovw_dummy_color_t> T;
		nv_bovw_rerank_method_t m_rerank_method;
		size_t m_fit_area;
		std::string m_idf_file;
		
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
		T *m_ctx;
		
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
		feature_extract(FT *fv, nv_matrix_t *image)
		{
			// T::sparse_t == FT
			m_ctx->extract(*fv, image);
		}
		
		virtual int
		feature_extract_file(FT *fv, const char *file,
							 otama_variant_t *options)
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
			InvertedIndex::WeightFunction *score_func = this->feature_weight_func();
			InvertedIndex::sparse_vec_t intersection;
			InvertedIndex::sparse_vec_t::const_iterator it;
			float norm1 = 0.0f, norm2 = 0.0f, dot = 0.0f;
			
			std::set_intersection(fv1->begin(), fv1->end(),
								  fv2->begin(), fv2->end(),
								  std::back_inserter(intersection));
			
			for (it = fv1->begin(); it != fv1->end(); ++it) {
				float w = (*score_func)(*it);
				norm1 += w * w;
			}
			for (it = fv2->begin(); it != fv2->end(); ++it) {
				float w = (*score_func)(*it);				
				norm2 += w * w;
			}
			for (it = intersection.begin(); it != intersection.end(); ++it) {
				float w = (*score_func)(*it);
				dot += w * w;
			}
			
			return dot / (sqrtf(norm1) * sqrtf(norm2));
		}
		
		virtual void
		feature_to_sparse_vec(FT &svec,
							  const FT *fv)
		{
			svec = *fv;
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
		
		static float
		knn_func(const nv_matrix_t *vec1, int j1,
				 const nv_matrix_t *vec2, int j2)
		{
			return -nv_vector_dot(vec1, j1, vec2, j2);
		}
		
		virtual otama_status_t
		feature_search(otama_result_t **results, int n,
					   const FT *query,
					   otama_variant_t *options)
		{
			return this->m_inverted_index->search(results, n, *query);
		}

		void
		print_idf(otama_variant_t *argv)
		{
			int stopword_th = 0;
			int64_t count;
			nv_matrix_t *freq = nv_matrix_alloc(T::BIT, 1);
			uint32_t hash;
			int stopword_count = 0;
			
			if (argv) {
				stopword_th = (int)otama_variant_to_int(argv);
			}
			OTAMA_LOG_DEBUG("begin idf_print, stopword_th: %d", stopword_th);
			
			count = this->m_inverted_index->count();
			
			nv_matrix_zero(freq);
			for (hash = 0; hash < (uint32_t)T::BIT; ++hash) {
				NV_MAT_V(freq, 0, hash) = (float)this->m_inverted_index->hash_count(hash);
			}
			m_ctx->update_idf(freq, 0, count, stopword_th);
			
			for (hash = 0; hash < (uint32_t)T::BIT; ++hash) {
				if (m_ctx->idf(hash) == 0.0f) {
					++stopword_count;
				}
			}
			nv_save_matrix_fp(stdout, m_ctx->idf_vector());
			
			OTAMA_LOG_DEBUG("stop word count %d", stopword_count);
			
			nv_matrix_free(&freq);
		}
		
		otama_status_t
		save_idf(otama_variant_t *argv)
		{
			int64_t stopword_th = 1;
			int64_t count;
			uint32_t hash;
			char filename[8192] = "./idf.matb";
			nv_matrix_t *freq = nv_matrix_alloc(T::BIT, 1);
			nv_matrix_t *idf = nv_matrix_alloc(T::BIT, 1);
			otama_status_t ret = OTAMA_STATUS_OK;
			int feature_count = 0;
			int i;
			
			if (OTAMA_VARIANT_IS_HASH(argv)) {
				otama_variant_t *file = otama_variant_hash_at(argv, "filename");
				otama_variant_t *stopword = otama_variant_hash_at(argv, "stopword");
				if (!OTAMA_VARIANT_IS_NULL(file)) {
					strncpy(filename, otama_variant_to_string(file), sizeof(filename)-1);
				}
				if (!OTAMA_VARIANT_IS_NULL(stopword)) {
					stopword_th = otama_variant_to_int(stopword);
				}
			} else {
				strncpy(filename, otama_variant_to_string(argv), sizeof(filename)-1);
			}
			count = this->m_inverted_index->count();
			nv_matrix_zero(freq);
			for (hash = 0; hash < (uint32_t)T::BIT; ++hash) {
				NV_MAT_V(freq, 0, hash) = (float)this->m_inverted_index->hash_count(hash);
			}
			m_ctx->calc_idf(idf, 0, freq, 0, count, stopword_th);
			for (i = 0; i < idf->n; ++i) {
				if (NV_MAT_V(idf, 0, i) > 0.0f) {
					feature_count += 1;
				}
			}
			OTAMA_LOG_DEBUG("idf_save: filename: %s, stopword_th: %d, features: %d/%d",
							filename, stopword_th, feature_count, (int)T::BIT);
			
			if (nv_save_matrix_bin(filename, idf) != 0) {
				OTAMA_LOG_ERROR("%s: failed to save idf", filename);
				ret = OTAMA_STATUS_SYSERROR;
			}
			nv_matrix_free(&freq);
			nv_matrix_free(&idf);
			
			return ret;
		}
		
	public:
		static inline std::string
		itos(int i)	{ char buff[128]; sprintf(buff, "%d", i); return std::string(buff);	}
		virtual std::string
		name(void)
		{
			return this->prefixed_name(std::string("otama_bovw") + itos(BIT/1024) + "k_iv");
		}
		
		BOVWInvertedIndexDriver(otama_variant_t *options)
		: InvertedIndexDriver<FT, IV>(options)
		{
			otama_variant_t *driver, *value;
			
			m_ctx = NULL;
			m_rerank_method = NV_BOVW_RERANK_IDF;
			m_fit_area = 0;
			m_idf_file.clear();
			
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
				if (!OTAMA_VARIANT_IS_NULL(value = otama_variant_hash_at(driver, "fit_area"))) {
					m_fit_area = otama_variant_to_int(value);
				}
				if (!OTAMA_VARIANT_IS_NULL(value = otama_variant_hash_at(driver, "idf_file"))) {
					m_idf_file.assign(otama_variant_to_string(value));
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
			
			// bucket.reserve
			this->m_inverted_index->reserve((size_t)BIT);
		}
		~BOVWInvertedIndexDriver()
		{
			delete m_ctx;
		}
		
		virtual otama_status_t
		open(void)
		{
			otama_status_t ret;
			
			ret = InvertedIndexDriver<FT, IV>::open();
			if (ret != OTAMA_STATUS_OK) {
				return ret;
			}
			m_ctx = new T;
			if (m_idf_file.size() == 0) {
				if (m_ctx->open() != 0) {
					return OTAMA_STATUS_SYSERROR;
				}
			} else {
				if (m_ctx->open_with_idf(m_idf_file.c_str()) != 0) {
					return OTAMA_STATUS_SYSERROR;
				}
			}
			m_ctx->set_fit_area(m_fit_area);
			m_idf_w.ctx = m_ctx;
			
			return OTAMA_STATUS_OK;
		}

		virtual otama_status_t
		close(void)
		{
			delete m_ctx;
			m_ctx = NULL;
			m_idf_w.ctx = NULL;
			return InvertedIndexDriver<FT, IV>::close();
		}
		
		virtual otama_status_t
		invoke(const std::string &method, otama_variant_t *output, otama_variant_t *input)
		{
			OTAMA_LOG_DEBUG("invoke: %s\n", method.c_str());
			
			if (method == "print_idf") {
				print_idf(input);
				otama_variant_set_null(output);
				return OTAMA_STATUS_OK;
			} else if (method == "save_idf") {
				otama_status_t ret = save_idf(input);
				otama_variant_set_null(output);
				return ret;
			}
			return InvertedIndexDriver<FT, IV>::invoke(method, output, input);
		}
	};
}

#endif
