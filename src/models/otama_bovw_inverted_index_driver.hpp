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
#ifndef OTAMA_BOVW_INVERTED_INDEX_TABLE_HPP
#define OTAMA_BOVW_INVERTED_INDEX_TABLE_HPP

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
		const static int SEARCH_SCALE = 10;
		
		typedef nv_bovw_ctx<BIT, nv_bovw_dummy_color_t> T;
		nv_bovw_rerank_method_t m_rerank_method;
		
		class IdfW: public InvertedIndex::ScoreFunction {
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
		
		virtual InvertedIndex::sparse_vec_t *
		feature_new(void)
		{
			return new InvertedIndex::sparse_vec_t;
		}

		virtual void
		feature_free(InvertedIndex::sparse_vec_t *fixed)
		{
			delete fixed;
		}
		static void
		feature_raw_free(void *p)
		{
			InvertedIndex::sparse_vec_t *ft = (InvertedIndex::sparse_vec_t *)p;
			delete ft;
		}
		virtual otama_feature_raw_free_t
		feature_free_func(void)
		{
			return feature_raw_free;
		}
		
		virtual void
		feature_extract(InvertedIndex::sparse_vec_t *fixed, nv_matrix_t *image)
		{
			// T::sparse_t == InvertedIndex::sparse_vec_t
			m_ctx->extract(*fixed, image);
		}
		
		virtual int
		feature_extract_file(InvertedIndex::sparse_vec_t *fixed, const char *file)
		{
			if (m_ctx->extract(*fixed, file)) {
				return 1;
			}
			
			return 0;
		}
		
		virtual int
		feature_extract_data(InvertedIndex::sparse_vec_t *fixed,
							 const void *data, size_t data_len)
		{
			if (m_ctx->extract(*fixed, data, data_len)) {
				return 1;
			}
			
			return 0;
		}
		
		virtual int
		feature_deserialize(InvertedIndex::sparse_vec_t *fixed, const char *s)
		{
			return m_ctx->deserialize(fixed, s);
		}
		
		virtual char *
		feature_serialize(const InvertedIndex::sparse_vec_t *fixed)
		{
			std::string s;
			char *ret;
			
			m_ctx->serialize(s, fixed);
			ret = nv_alloc_type(char, s.size() + 1);
			strcpy(ret, s.c_str());
			
			return ret;
		}

		virtual InvertedIndex::ScoreFunction *
		feature_similarity_func(void)
		{
			return &m_idf_w;
		}
		
		virtual void
		feature_to_sparse_vec(InvertedIndex::sparse_vec_t &svec,
							  const InvertedIndex::sparse_vec_t *fixed)
		{
			svec = *fixed;
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
					   const InvertedIndex::sparse_vec_t *query,
					   otama_variant_t *options)
		{
			return this->m_inverted_index->search_cosine(results, n, *query);
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
			
			this->m_inverted_index->begin_reader();
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
			this->m_inverted_index->end();
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
			: InvertedIndexDriver<InvertedIndex::sparse_vec_t, IV>(options)
		{
			otama_variant_t *driver, *value;
			
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
			m_ctx = new T;
			m_ctx->open();
			
			m_idf_w.rerank_method = m_rerank_method;
			m_idf_w.ctx = m_ctx;
			// bucket.reserve
			this->m_inverted_index->reserve((size_t)BIT);
		}
		~BOVWInvertedIndexDriver()
		{
			delete m_ctx;
		}
		
		virtual otama_status_t
		set(const std::string &key, otama_variant_t *value)
		{
			OTAMA_LOG_DEBUG("set key: %s\n", key.c_str());
			
			if (key == "print_idf") {
				print_idf(value);
				return OTAMA_STATUS_OK;
			}
			return InvertedIndexDriver<InvertedIndex::sparse_vec_t, IV>::set(key, value);
		}
	};
}

#endif
