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
#ifndef OTAMA_DRIVER_HPP
#define OTAMA_DRIVER_HPP

#include "nv_core.h"
#include "otama_id.h"
#include "otama_feature_raw.h"
#include "otama_log.h"
#include "otama_result.h"
#include "otama_dbi.h"
#include "otama_util.h"
#include "otama_image.h"
#include "otama_image_internal.h"
#include "otama_omp_lock.hpp"
#include <inttypes.h>
#include <vector>
#include <string>
#include <cctype>

namespace otama
{
	template<typename T>
	class Driver: public DriverInterface
	{
	protected:
		static const int FLAG_DELETE = 0x01;
		
		std::string m_prefix;
		std::string m_data_dir;
		std::string m_hash_conditions;
		
		bool m_load_fv;
#ifdef _OPENMP
		omp_nest_lock_t *m_lock;
#endif
		virtual std::string name(void) = 0;
		virtual T *feature_new(void) = 0;
		virtual void feature_free(T*fixed) = 0;
		virtual otama_feature_raw_free_t feature_free_func(void) = 0;
		
		virtual void feature_extract(T *fixed, nv_matrix_t *image) = 0;
		virtual int feature_extract_file(T *fixed, const char *file,
										 otama_variant_t *options) = 0;
		virtual int feature_extract_data(T *fixed, const void *data, size_t data_len,
										 otama_variant_t *options) = 0;
		virtual int feature_deserialize(T *fixed, const char *s) = 0;
		virtual char *feature_serialize(const T *fixed) = 0;
		virtual void feature_serialized_free(char *s)	{ nv_free(s); }
		virtual void feature_copy(T *to, const T*from) = 0;
		
		virtual otama_status_t feature_search(otama_result_t **results, int n,
											  const T *query,
											  otama_variant_t *options) = 0;
		virtual float feature_similarity(const T *fixed1, const T *fixed2,
										  otama_variant_t *options) = 0;
		
		virtual otama_status_t try_load_local(otama_id_t *id,
											  uint64_t seq,
											  T *fixed) = 0;
		virtual otama_status_t load(const otama_id_t *id,
									T *fixed) = 0;
		virtual otama_status_t insert(const otama_id_t *id,
									  const T *fixed) = 0;
		virtual otama_status_t exists_master(bool &e, uint64_t &seq,
											 const otama_id_t *id) = 0;
		virtual otama_status_t update_flag(const otama_id_t *id, uint8_t flag) = 0;

		
		const char *DEFAULT_DATA_DIR(void) { return "."; }
		virtual std::string table_name(void) { return this->name(); }
		
		std::string data_dir(void) { return m_data_dir; }
		void data_dir(const std::string &dir) { m_data_dir.assign(dir); }
		void prefix(const std::string &name) { m_prefix.assign(name); }
		std::string prefix(void) { return m_prefix; }
		
		std::string
		prefixed_name(const std::string &name)
		{
			if (this->prefix().empty()) {
				return name;
			} else {
				return this->prefix() + "_" + name;
			}
		}
		std::string
		parse_hash_conditions(otama_variant_t *hash)
		{
			std::string conditions = "(";
			bool error;
			
			if (OTAMA_VARIANT_IS_ARRAY(hash)) {
				int i;
				int count = (int)otama_variant_array_count(hash);
				for (i = 0; i < count; ++i) {
					if (i != 0) {
						conditions += " AND ";
					}
					conditions += parse_hash_token(
						otama_variant_to_string(otama_variant_array_at(hash, i)),
						error);
					if (error) {
						return std::string();
					}
				}
			} else {
				conditions += parse_hash_token(
					otama_variant_to_string(hash),
					error);
				if (error) {
					return std::string();
				}
			}
			return conditions + ")";
		}
		
		bool
		valid_hash_token(const char *token)
		{
			while (*token != '\0') {
				if (!(std::isdigit(*token) || ('a' <= *token && *token <= 'z'))) {
					return false;
				}
				++token;
			}
			return true;
		}
		
		std::string
		parse_hash_token(const char *_token, bool &error)
		{
			char *p1, *p2;
			std::string conditions;
			char token[128];
			char like[128];
			char hex[16];
			int64_t  i1, i2;

			error = false;
	
			strncpy(token, _token, sizeof(token) - 1);
			p1 = token;
			
			if ((p2 = strchr(token, '-'))) {
				bool first = true;
				
				*p2 = '\0';
				++p2;
				if (strlen(p1) > 8
					|| strlen(p1) != strlen(p2)
					|| !valid_hash_token(p1)
					|| !valid_hash_token(p2))
				{
					error = true;
					OTAMA_LOG_ERROR("invalid shard option: %s", _token);
					return std::string();
				}
				i1 = nv_strtoll(p1, NULL, 16);
				i2 = nv_strtoll(p2, NULL, 16);
				if (i1 > i2) {
					error = true;
					OTAMA_LOG_ERROR("invalid shard option: %s", _token);
					return std::string();
				}
				while (i1 <= i2) {
					if (first) {
						first = false;
					} else {
						conditions += " OR ";
					}
					nv_snprintf(hex, sizeof(hex) - 1, "%"PRIx64"%%", i1);
					nv_snprintf(like, sizeof(like) - 1,
								" otama_id like '%s' ", hex);
					conditions += like;
					
					OTAMA_LOG_DEBUG("%s", like);
					
					++i1;
				}
			} else {
				if (strlen(p1) > 8 || !valid_hash_token(p1)) {
					error = true;
					OTAMA_LOG_ERROR("invalid shard option: %s", _token);
					return std::string();
				}
				i1 = nv_strtoll(p1, NULL, 16);
				nv_snprintf(hex, sizeof(hex) - 1, "%"PRIx64"%%", i1);
				nv_snprintf(like, sizeof(like) - 1,
							" otama_id like '%s' ", hex);
				
				OTAMA_LOG_DEBUG("%s", like);
				
				conditions = like;
			}
			
			return conditions;
		}
		
		inline void
		set_result(otama_result_t *results, int i,
						const otama_id_t *id, float cosine)
		{
			otama_variant_t *hash = otama_result_value(results, i);
			otama_variant_t *c;

			otama_variant_set_hash(hash);
			c = otama_variant_hash_at(hash, "similarity");
			otama_variant_set_float(c, cosine);
			otama_result_set_id(results, i, id);
		}
		
		otama_status_t
		extract(otama_id_t *id,
				bool &has_id,
				T *fixed,
				bool &has_feature,
				bool &exist,
				otama_variant_t *data)
		{
			otama_variant_t *file, *blob, *image, *vid, *feature_string, *raw;
			otama_status_t ret;

			has_id = false;
			has_feature = false;
			exist = false;
			
			if (OTAMA_VARIANT_IS_STRING(file = otama_variant_hash_at(data, "file"))) {
				ret = otama_id_file(id, otama_variant_to_string(file));
				if (ret != OTAMA_STATUS_OK) {
					return ret;
				}
				has_id = true;
				if (try_load(id, fixed) != OTAMA_STATUS_OK) {
					if (feature_extract_file(fixed, otama_variant_to_string(file), data) != 0) {
						return OTAMA_STATUS_SYSERROR;				
					}
				} else {
					exist = true;
				}
				has_feature = true;
			} else if (OTAMA_VARIANT_IS_BINARY(blob = otama_variant_hash_at(data, "data"))) {
				otama_id_data(id, 
							  otama_variant_to_binary_ptr(blob),
							  otama_variant_to_binary_len(blob));
				has_id = true;
				if (try_load(id, fixed) != OTAMA_STATUS_OK) {
					if (feature_extract_data(fixed,
											 otama_variant_to_binary_ptr(blob),
											 otama_variant_to_binary_len(blob),
											 data) != 0)
					{
						return OTAMA_STATUS_INVALID_ARGUMENTS;
					}
				} else {
					exist = true;
				}
				has_feature = true;
			} else if (OTAMA_VARIANT_IS_POINTER(image =	otama_variant_hash_at(data, "image"))) {
				otama_image_t *p = (otama_image_t *)otama_variant_to_pointer(image);
				
				if (p) {
					if (p->has_id) {
						memcpy(id, &p->id, sizeof(*id));
						has_id = true;
					} else {
						otama_id_data(id, 
									  p->image->v,
									  (size_t)p->image->list_step *
									  p->image->list * sizeof(float));
						has_id = true;
					}
					if (try_load(id, fixed) != OTAMA_STATUS_OK) {
						feature_extract(fixed, p->image);
					} else {
						exist = true;
					}
					has_feature = true;
				} else {
					return OTAMA_STATUS_INVALID_ARGUMENTS;
				}
			} else if (OTAMA_VARIANT_IS_STRING(feature_string = otama_variant_hash_at(data, "string"))) {
				has_id = false;
				if (feature_deserialize(fixed, otama_variant_to_string(feature_string)) != 0) {
					return OTAMA_STATUS_INVALID_ARGUMENTS;
				}
				has_feature = true;
			} else if (OTAMA_VARIANT_IS_POINTER(raw =
												otama_variant_hash_at(data, "raw")))
			{
				const otama_feature_raw_t *p = (const otama_feature_raw_t*)otama_variant_to_pointer(raw);
				has_id = false;
				if (p) {
					feature_copy(fixed, (const T *)(p->raw));
					has_feature = true;
				} else {
					return OTAMA_STATUS_INVALID_ARGUMENTS;
				}
			} else if (OTAMA_VARIANT_IS_STRING(vid = otama_variant_hash_at(data, "id"))) {
				if (otama_id_hexstr2bin(id, otama_variant_to_string(vid)) != OTAMA_STATUS_OK) {
					return OTAMA_STATUS_INVALID_ARGUMENTS;
				}
				has_id = true;
				if (try_load(id, fixed) != OTAMA_STATUS_OK) {
					has_feature = false;
				} else {
					exist = true;
					has_feature = true;
				}
			} else if (OTAMA_VARIANT_IS_BINARY(vid))	{
				if (otama_variant_to_binary_len(vid) != sizeof(*id)) {
					return OTAMA_STATUS_INVALID_ARGUMENTS;
				}
				memcpy(id, otama_variant_to_binary_ptr(vid), sizeof(*id));
				has_id = true;
				if (try_load(id, fixed) != OTAMA_STATUS_OK) {
					has_feature = false;
				} else {
					exist = true;
					has_feature = true;
				}
			} else {
				return OTAMA_STATUS_INVALID_ARGUMENTS;
			}
	
			return OTAMA_STATUS_OK;
		}

		otama_status_t
		try_load(otama_id_t *id,
				 T *fixed)
		{
			uint64_t seq;
			bool e;
			otama_status_t sret;

			if (m_load_fv) {
				sret = exists_master(e, seq, id);
				
				if (sret != OTAMA_STATUS_OK) {
					return sret;
				}
	
				if (e) {
					sret = try_load_local(id, seq, fixed);
					if (sret != OTAMA_STATUS_OK) {
						sret = load(id, fixed);
						if (sret != OTAMA_STATUS_OK) {
							return sret;
						}
					}
				} else {
					return OTAMA_STATUS_NODATA;
				}
			} else {
				return OTAMA_STATUS_NODATA;
			}
			
			return OTAMA_STATUS_OK;
		}

	public:
		Driver(otama_variant_t *options)
		{
			otama_variant_t *driver, *value;
			
#ifdef _OPENMP
			m_lock = new omp_nest_lock_t;
			omp_init_nest_lock(m_lock);
#endif
			m_load_fv = true;
			this->data_dir(DEFAULT_DATA_DIR());

			if (!OTAMA_VARIANT_IS_NULL(value =
									   otama_variant_hash_at(options, "namespace")))
			{
				prefix(otama_variant_to_string(value));
			}
			
			driver = otama_variant_hash_at(options, "driver");
			if (OTAMA_VARIANT_IS_HASH(driver)) {
				if (!OTAMA_VARIANT_IS_NULL(value = otama_variant_hash_at(driver, "data_dir"))) {
					this->data_dir(otama_variant_to_string(value));
				}
				value = otama_variant_hash_at(driver, "shard");
				if (OTAMA_VARIANT_IS_ARRAY(value) || OTAMA_VARIANT_IS_STRING(value)) {
					m_hash_conditions = parse_hash_conditions(value);
				}
				if (!OTAMA_VARIANT_IS_NULL(value = otama_variant_hash_at(driver, "load_fv"))) {
					m_load_fv = otama_variant_to_bool(value) ? true : false;
				}
			}
			OTAMA_LOG_DEBUG("namespace         => %s", this->prefix().c_str());
			OTAMA_LOG_DEBUG("driver[data_dir]   => %s", this->data_dir().c_str());
			OTAMA_LOG_DEBUG("driver[hash]:sql   => %s", m_hash_conditions.c_str());
			OTAMA_LOG_DEBUG("driver[load_fv] => %d",
							m_load_fv ? 1:0);
		}
		
		virtual ~Driver()
		{
			close();
#ifdef _OPENMP
			omp_destroy_nest_lock(m_lock);
			delete m_lock;
#endif
		}
		
		virtual otama_status_t
		open(void)
		{
			return OTAMA_STATUS_OK;
		}
		
		virtual otama_status_t
		close(void)
		{
			return OTAMA_STATUS_OK;
		}
		
		virtual otama_status_t
		feature_string(std::string &features,
					   otama_variant_t *data)
		{
			otama_status_t ret;
			otama_id_t id;
			T *fixed = this->feature_new();
			bool has_id, has_feature, exist;
			char *s;
	
			ret = extract(&id, has_id, fixed, has_feature, exist, data);
			if (ret != OTAMA_STATUS_OK) {
				feature_free(fixed);
				return ret;
			}
			if (!has_feature) {
				feature_free(fixed);
				return OTAMA_STATUS_INVALID_ARGUMENTS;
			}
			s = feature_serialize(fixed);
			features = std::string(s);
			feature_serialized_free(s);
			feature_free(fixed);
			
			return ret;
		}

		virtual otama_status_t
		feature_raw(otama_feature_raw_t **raw, otama_variant_t *data)
		{
			otama_status_t ret;
			otama_id_t id;
			T *pfixed = this->feature_new();
			bool has_id, has_feature, exist;

			*raw = NULL;
			
			ret = extract(&id, has_id, pfixed, has_feature, exist, data);
			if (ret != OTAMA_STATUS_OK) {
				feature_free(pfixed);
				return ret;
			}
			if (!has_feature) {
				feature_free(pfixed);
				return OTAMA_STATUS_INVALID_ARGUMENTS;
			}
			*raw = new otama_feature_raw_t;
			(*raw)->raw = (void *)pfixed;
			(*raw)->self_free = this->feature_free_func();
			
			return ret;
		}
		
		virtual otama_status_t
		insert(otama_id_t *id,
			   otama_variant_t *data)
		{
			otama_status_t ret;
			T *fixed = this->feature_new();
			bool has_id, has_feature, exist;

			ret = extract(id, has_id, fixed, has_feature, exist, data);
			if (ret != OTAMA_STATUS_OK) {
				feature_free(fixed);
				return ret;
			}
			if (!(has_id && has_feature)) {
				feature_free(fixed);
				return OTAMA_STATUS_INVALID_ARGUMENTS;
			}
			if (!exist) {
				ret = insert(id, fixed);
				if (ret != OTAMA_STATUS_OK) {
					feature_free(fixed);
					return ret;
				}
			} else {
				ret = update_flag(id, 0);
			}
			feature_free(fixed);
	
			return ret;
		}

		virtual otama_status_t
		search(otama_result_t **results, int n,
			   otama_variant_t *query)
		{
			otama_id_t id;
			T *fixed = this->feature_new();
			bool has_id, has_feature, exist;
			otama_status_t ret;
			
			if (!OTAMA_VARIANT_IS_HASH(query)) {
				feature_free(fixed);
				return OTAMA_STATUS_INVALID_ARGUMENTS;
			}
			
			ret = extract(&id, has_id, fixed, has_feature, exist, query);
			if (ret != OTAMA_STATUS_OK) {
				feature_free(fixed);
				return ret;
			}
			if (!has_feature) {
				feature_free(fixed);
				return OTAMA_STATUS_INVALID_ARGUMENTS;
			}
			ret = feature_search(results, n, fixed, query);
			feature_free(fixed);
			
			return ret;
		}
		
		virtual otama_status_t
		exists(bool &result, const otama_id_t *id)
		{
			uint64_t dummy = 0;
			return exists_master(result, dummy, id);
		}

		virtual otama_status_t
		similarity(float *v,
			  otama_variant_t *query,
			  otama_variant_t *data)
		{
			T *fixed1, *fixed2;
			otama_id_t id1, id2;
			bool has_feature1, has_feature2, has_id1, has_id2, exist1, exist2;
			otama_status_t ret;
			
			if (!OTAMA_VARIANT_IS_HASH(query) || !OTAMA_VARIANT_IS_HASH(data)) {
				return OTAMA_STATUS_INVALID_ARGUMENTS;
			}
			fixed1 = this->feature_new();
			ret = extract(&id1, has_id1, fixed1, has_feature1, exist1, query);
			if (ret != OTAMA_STATUS_OK) {
				feature_free(fixed1);
				return ret;
			}
			if (!has_feature1) {
				feature_free(fixed1);
				return OTAMA_STATUS_INVALID_ARGUMENTS;
			}
			
			fixed2 = this->feature_new();
			ret = extract(&id2, has_id2, fixed2, has_feature2, exist2, data);
			if (ret != OTAMA_STATUS_OK) {
				feature_free(fixed1);
				feature_free(fixed2);
				return ret;
			}
			if (!has_feature2) {
				feature_free(fixed1);
				feature_free(fixed2);
				return OTAMA_STATUS_INVALID_ARGUMENTS;
			}
			*v = feature_similarity(fixed1, fixed2, query);
				
			feature_free(fixed1);
			feature_free(fixed2);
			
			return OTAMA_STATUS_OK;
		}
		
		virtual otama_status_t sync(void) { return OTAMA_STATUS_OK;	}
		virtual bool is_active(void) { return false; };
		virtual otama_status_t set(const std::string &key, otama_variant_t *value) { return OTAMA_STATUS_INVALID_ARGUMENTS;	}
		virtual otama_status_t unset(const std::string &key) { return OTAMA_STATUS_INVALID_ARGUMENTS; }
		virtual otama_status_t get(const std::string &key, otama_variant_t *value) { return OTAMA_STATUS_INVALID_ARGUMENTS;	}
		virtual otama_status_t invoke(const std::string &method, otama_variant_t *output, otama_variant_t *input) { return OTAMA_STATUS_INVALID_ARGUMENTS;	}
	};
}

#endif
