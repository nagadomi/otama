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

#ifdef _OPENMP
#ifndef OTAMA_OMP_HPP
#define OTAMA_OMP_HPP
#include <omp.h>

namespace otama
{
	class OMPLock
	{
	protected:
		omp_nest_lock_t *m_lock;
		
	public:
		OMPLock(omp_nest_lock_t *lock)
		{
			m_lock = lock;
			omp_set_nest_lock(m_lock);
		}
		
		~OMPLock()
		{
			omp_unset_nest_lock(m_lock);
		}
	};
}

#endif
#endif

