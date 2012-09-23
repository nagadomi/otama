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

#ifndef OTAMA_DRIVER_INTERFACE_HPP
#define OTAMA_DRIVER_INTERFACE_HPP

#include "otama_id.h"
#include "otama_feature_raw.h"
#include "otama_status.h"
#include "otama_result.h"
#include <string>

namespace otama
{
	class DriverInterface
	{
	protected:
		DriverInterface(void) {};
		
	public:
		virtual otama_status_t open(void) = 0;
		virtual otama_status_t close(void) = 0;
		virtual bool is_active(void) = 0;
		
		virtual otama_status_t create_table(void) = 0;
		virtual otama_status_t drop_table(void) = 0;
		
		virtual otama_status_t feature_string(std::string &features,
											  otama_variant_t *data) = 0;
		virtual otama_status_t feature_raw(otama_feature_raw_t **raw, otama_variant_t *data) = 0;
		
		static otama_status_t
		feature_raw_free(otama_feature_raw_t **raw)
		{
			if (raw && *raw) {
				(*raw)->self_free((*raw)->raw);
				delete *raw;
				*raw = NULL;
			}
			return OTAMA_STATUS_OK;
		}
		
		virtual otama_status_t insert(otama_id_t *id, otama_variant_t *data) = 0;
		virtual otama_status_t exists(bool &result, const otama_id_t *id) = 0;
		virtual otama_status_t remove(const otama_id_t *id) = 0;
		virtual otama_status_t search(otama_result_t **results, int n,
									  otama_variant_t *query) = 0;
		virtual otama_status_t similarity(float *v,
										  otama_variant_t *data1,
										  otama_variant_t *data2) = 0;
		
		virtual otama_status_t pull(void) = 0;
		virtual otama_status_t count(int64_t *count) = 0;
		
		virtual otama_status_t set(const std::string &key, otama_variant_t *value) = 0;
		virtual otama_status_t unset(const std::string &key) = 0;
		virtual otama_status_t get(const std::string &key, otama_variant_t *value) = 0;
		
		virtual ~DriverInterface() {};
	};
}

#endif
