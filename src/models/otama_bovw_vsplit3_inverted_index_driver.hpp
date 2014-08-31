/*
 * This file is part of otama.
 *
 * Copyright (C) 2014 nagadomi@nurs.or.jp
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
#ifndef OTAMA_BOVW_VSPLIT3_INVERTED_INDEX_DRIVER_HPP
#define OTAMA_BOVW_VSPLIT3_INVERTED_INDEX_DRIVER_HPP

#include "otama_bovw_inverted_index_driver.hpp"

namespace otama
{
	template <nv_bovw_bit_e BIT, typename IV>
	class BOVWVSplit3InvertedIndexDriver:
		public BOVWInvertedIndexDriver<BIT, IV>
	{
	protected:
		typedef typename BOVWInvertedIndexDriver<BIT, IV>::FT FT;
		typedef typename BOVWInvertedIndexDriver<BIT, IV>::T T;
		
		class IdfWVSplit: public InvertedIndex::WeightFunction {
		public:
			T *ctx;
			nv_bovw_rerank_method_t rerank_method;
			virtual float operator()(uint32_t x)
			{
				switch (rerank_method) {
				case NV_BOVW_RERANK_IDF:
					return ctx->idf(ctx->remove_vsplit3_flag(x));
				case NV_BOVW_RERANK_NONE:
					break;
				}
				return 1.0f;
			}
		};
		IdfWVSplit m_idf_vsplit3_w;
		virtual InvertedIndex::WeightFunction *
		feature_weight_func(void)
		{
			return &m_idf_vsplit3_w;
		}
		virtual void
		feature_extract(FT *fv, nv_matrix_t *image)
		{
			// T::sparse_t == FT
			this->m_ctx->extract_vsplit3(*fv, image);
		}
		
		virtual int
		feature_extract_file(FT *fv, const char *file,
							 otama_variant_t *options)
		{
			if (this->m_ctx->extract_vsplit3(*fv, file)) {
				return 1;
			}
			
			return 0;
		}
		
		virtual int
		feature_extract_data(FT *fv,
							 const void *data, size_t data_len,
							 otama_variant_t *options)
		{
			if (this->m_ctx->extract_vsplit3(*fv, data, data_len)) {
				return 1;
			}
			
			return 0;
		}
	public:
		static inline std::string
		itos(int i)	{ char buff[128]; sprintf(buff, "%d", i); return std::string(buff);	}
		virtual std::string
		name(void)
		{
			return this->prefixed_name(std::string("otama_bovw") + itos(BIT/1024) + "k_vsplit3_iv");
		}
		BOVWVSplit3InvertedIndexDriver(otama_variant_t *options)
		: BOVWInvertedIndexDriver<BIT, IV>(options)
		{
		}		
		virtual ~BOVWVSplit3InvertedIndexDriver()
		{
		}
	};
}
#endif

