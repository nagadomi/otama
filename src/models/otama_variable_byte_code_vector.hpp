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
#include "nv_core.h"
#ifndef OTAMA_VARIABLE_BYTE_CODE_VECTOR_HPP
#define OTAMA_VARIABLE_BYTE_CODE_VECTOR_HPP
#include <vector>

namespace otama
{
	class VBCVector {
	private:
		uint8_t *m_data;
		size_t m_data_size;
		size_t m_data_reserve_size;
		static const int INITIAL_SIZE = 4;
		static const int EXTEND_SIZE = 128;
		
	public:
		VBCVector():m_data(0) {
			clear();
		}
		VBCVector(const VBCVector &lhs) {
			m_data_size = lhs.m_data_size;
			m_data_reserve_size = lhs.m_data_reserve_size;
			if (m_data_reserve_size > 0) {
				m_data = nv_alloc_type(uint8_t, m_data_reserve_size);
				if (m_data_size > 0) {
					memcpy(m_data, lhs.m_data, m_data_size * sizeof(uint8_t));
				}
			} else {
				m_data = NULL;
			}
		}
		~VBCVector() {
			if (m_data) {
				nv_free(m_data);
				m_data = NULL;
			}
		}
		inline void
		push_back(uint8_t v)
		{
			if (m_data_reserve_size > m_data_size) {
				m_data[m_data_size++] = v;
			} else {
				m_data_reserve_size += EXTEND_SIZE;
				m_data = (uint8_t *)nv_realloc(m_data, m_data_reserve_size * sizeof(uint8_t));
				m_data[m_data_size++] = v;
			}
		}
		inline void
		clear(void)
		{
			if (m_data) {
				nv_free(m_data);
				m_data = NULL;
			}
			m_data_reserve_size = INITIAL_SIZE;
			m_data_size = 0;
			m_data = (uint8_t *)nv_alloc_type(uint8_t, m_data_reserve_size);
		}
		inline size_t
		size(void) const
		{
			return m_data_size;
		}
		inline size_t
		at(size_t i) const
		{
			NV_ASSERT(i < m_data_size);
			return m_data[i];
		}
	};
	//typedef std::vector<uint8_t> VBCVector;
	
	static inline void
	vbc_push_back(VBCVector &vbc, int64_t &last_no, int64_t no)
	{
		assert(last_no <= no);
		uint64_t a = no - last_no;
		if (a == 0) {
			vbc.push_back(0);
		} else {
			do {
				uint8_t v = (a & 0x7f);
				a >>= 7;
				if (a) {
					vbc.push_back(v | 0x80U);
				} else {
					vbc.push_back(v);
					break;
				}
			} while(1);
		}
		last_no = no;
	}
	
	static inline void
	vbc_decode(std::vector<int64_t> &vec, const VBCVector &vbc)
	{
		static const int s_t[8] = { 0, 7, 14, 21, 28, 35, 42, 49 };
		size_t i;
		size_t n = vbc.size();
		int64_t a = 0;
		int64_t last_no = 0;
		int j = 0;
		
		vec.clear();
		for (i = 0; i < n; ++i) {
			uint8_t v = vbc.at(i);
			if ((v & 0x80) != 0) {
				a |= ((int64_t)(v & 0x7f) << s_t[j]);
				++j;
			} else {
				int64_t no = last_no + (((int64_t)v << s_t[j]) | a);
				vec.push_back(no);
				last_no = no;
				j = 0;
				a = 0;
			}
		}
	}
	
	class VariableByteCodeVector {
	private:
		int64_t m_last_no;
		VBCVector m_vbc;
	public:
		VariableByteCodeVector() { m_last_no = 0; }
		
		inline void
		push_back(int64_t no)
		{
			vbc_push_back(m_vbc, m_last_no, no);
		}
		inline void
		decode(std::vector<int64_t> &vec)
		{
			vbc_decode(vec, m_vbc);
		}
		inline int64_t
		count(void)
		{
			std::vector<int64_t> vec;
			decode(vec);
			return vec.size();
		}
	};
}

#endif
