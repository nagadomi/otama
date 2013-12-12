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
#ifndef OTAMA_NODB_DRIVER_HPP
#define OTAMA_NODB_DRIVER_HPP

#include "nv_ml.h"
#include "otama_id.h"
#include "otama_log.h"
#include "otama_driver.hpp"

#include <inttypes.h>
#include <vector>
#include <string>

namespace otama
{
	template<typename T>
	class NoDBDriver: public Driver<T>
	{
	protected:
		virtual otama_status_t
		exists_master(bool &exists, uint64_t &seq,
					  const otama_id_t *id)
		{
			return OTAMA_STATUS_NOT_IMPLEMENTED;			
		}
		
		virtual otama_status_t
		update_flag(const otama_id_t *id, uint8_t flag)
		{
			return OTAMA_STATUS_NOT_IMPLEMENTED;
		}
		
		virtual otama_status_t
		insert(const otama_id_t *id,
			   const T *fv)
		{
			return OTAMA_STATUS_NOT_IMPLEMENTED;
		}
		
		virtual otama_status_t
		load(const otama_id_t *id,
			 T *fv)
		{
			return OTAMA_STATUS_NOT_IMPLEMENTED;
		}

		virtual otama_status_t
		feature_search(otama_result_t **results, int n,
							 const T *query,
							 otama_variant_t *options)
		{
			return OTAMA_STATUS_NOT_IMPLEMENTED;
		}
		
	public:
		NoDBDriver(otama_variant_t *options)
			: Driver<T>(options)
		{
			Driver<T>::m_load_fv = false;
		}
		
		virtual otama_status_t
		open(void)
		{
			return Driver<T>::open();
		}
		
		virtual otama_status_t
		create_database(void)
		{
			return OTAMA_STATUS_NOT_IMPLEMENTED;
		}
		
		virtual otama_status_t
		drop_database(void)
		{
			return OTAMA_STATUS_NOT_IMPLEMENTED;
		}
		
		virtual otama_status_t
		drop_index(void)
		{
			return OTAMA_STATUS_NOT_IMPLEMENTED;
		}
		
		virtual otama_status_t
		vacuum_index(void)
		{
			return OTAMA_STATUS_NOT_IMPLEMENTED;
		}
		
		virtual bool
		is_active(void)
		{
			return true;
		}

		virtual otama_status_t
		pull(void)
		{
			return OTAMA_STATUS_NOT_IMPLEMENTED;
		}
		
		virtual otama_status_t
		remove(const otama_id_t *id)
		{
			return OTAMA_STATUS_NOT_IMPLEMENTED;
		}

		virtual otama_status_t
		count(int64_t *count)
		{
			return OTAMA_STATUS_NOT_IMPLEMENTED;
		}

		virtual otama_status_t
		load_local(otama_id_t *id,
				   uint64_t seq,
				   T *bovw)
		{
			return OTAMA_STATUS_NOT_IMPLEMENTED;
		}
		
		virtual ~NoDBDriver()
		{
		}
		
		using Driver<T>::insert;
		using Driver<T>::search;
	};
}

#endif
