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

#include "otama.h"
#include "otama_util.h"
#include "nv_core.h"
#include <vector>
#include <new>
#include <map>
#include <string>
#include "otama_driver_factory.hpp"
#if OTAMA_WINDOWS
#  include <windows.h>
#endif

#if OTAMA_WITH_LZO
#include <lzo/lzo1x.h>
#endif

struct otama_instance {
	otama::DriverInterface *driver;
	otama_variant_t *config;
	otama_variant_pool_t *pool;
};

static void
set_log_file(otama_variant_t *config)
{
	if (OTAMA_VARIANT_IS_HASH(config)) {
		otama_variant_t *log = otama_variant_hash_at(config, "log");
		if (!OTAMA_VARIANT_IS_NULL(log)) {
			otama_log_set_file(otama_variant_to_string(log));
		}
	}
}

otama_status_t
otama_open(otama_t **otama,
		   const char *config_path)
{
	otama_t *o = nv_alloc_type(otama_t, 1);
	otama_status_t ret;

	*otama = NULL;
	o->pool = otama_variant_pool_alloc();
	o->config = otama_yaml_read_file(config_path, o->pool);
	if (!o->config) {
		OTAMA_LOG_ERROR("%s: cannot read file", config_path);
		otama_variant_pool_free(&(o->pool));
		nv_free(o);
		return OTAMA_STATUS_INVALID_ARGUMENTS;
	}
	set_log_file(o->config);
	
	o->driver = otama::createDriver(o->config);
	if (o->driver == NULL) {
		otama_variant_pool_free(&(o->pool));		
		nv_free(o);
		return OTAMA_STATUS_INVALID_ARGUMENTS;
	}
	
	ret = o->driver->open();
	if (ret != OTAMA_STATUS_OK) {
		o->driver->close();
		delete o->driver;
		otama_variant_pool_free(&(o->pool));
		nv_free(o);
		return ret;
	}
	*otama = o;
	
	return OTAMA_STATUS_OK;
}

otama_status_t
otama_open_opt(otama_t **otama,
			   otama_variant_t *options)
{
	otama_t *o;
	otama_status_t ret;

	*otama = NULL;
	o = nv_alloc_type(otama_t, 1);
	o->pool = otama_variant_pool_alloc();
	o->config = otama_variant_dup(options, o->pool);
	
	set_log_file(o->config);
	
	o->driver = otama::createDriver(o->config);
	if (o->driver == NULL) {
		nv_free(o);
		return OTAMA_STATUS_INVALID_ARGUMENTS;
	}
	
	ret = o->driver->open();
	if (ret != OTAMA_STATUS_OK) {
		o->driver->close();
		delete o->driver;
		nv_free(o);
		return ret;
	}
	*otama = o;
	
	return OTAMA_STATUS_OK;
}

int
otama_active(otama_t *otama)
{
	NV_ASSERT(otama != NULL);
	
	return otama->driver->is_active() ? 1:0;
}

void
otama_close(otama_t **otama)
{
	if (otama && *otama) {
		(*otama)->driver->close();
		delete (*otama)->driver;
		if ((*otama)->pool) {
			otama_variant_pool_free(&(*otama)->pool);
			(*otama)->pool = NULL;
		}
		nv_free(*otama);
		*otama = NULL;
	}
}

otama_status_t
otama_create_database(otama_t *otama)
{
	NV_ASSERT(otama != NULL);
	return otama->driver->create_database();
}

otama_status_t
otama_drop_database(otama_t *otama)
{
	NV_ASSERT(otama != NULL);
	return otama->driver->drop_database();
}

otama_status_t
otama_pull(otama_t *otama)
{
	NV_ASSERT(otama != NULL);
	return otama->driver->pull();
}

otama_status_t
otama_count(otama_t *otama, int64_t *count)
{
	NV_ASSERT(otama != NULL);
	return otama->driver->count(count);
}

otama_status_t
otama_insert(otama_t *otama, otama_id_t *id,
			 otama_variant_t *data)
{
	NV_ASSERT(otama != NULL);	
	NV_ASSERT(OTAMA_VARIANT_IS_HASH(data));
	
	return otama->driver->insert(id, data);
}

otama_status_t
otama_insert_file(otama_t *otama, otama_id_t *id,
				  const char *image_filename)
{
	otama_variant_pool_t *pool = otama_variant_pool_alloc();
	otama_variant_t *query = otama_variant_new(pool);
	otama_status_t ret;

	NV_ASSERT(otama != NULL);

	otama_variant_set_hash(query);
	otama_variant_set_string(otama_variant_hash_at(query, "file"), image_filename);
	ret = otama_insert(otama, id, query);
	
	otama_variant_pool_free(&pool);

	return ret;
}

otama_status_t
otama_insert_data(otama_t *otama, otama_id_t *id,
				  const  void *image_data, size_t image_data_len)
{
	otama_variant_pool_t *pool = otama_variant_pool_alloc();
	otama_variant_t *query = otama_variant_new(pool);
	otama_status_t ret;

	NV_ASSERT(otama != NULL);
	
	otama_variant_set_hash(query);
	otama_variant_set_binary(otama_variant_hash_at(query, "data"),
							 image_data, image_data_len);
	ret = otama_insert(otama, id, query);
	
	otama_variant_pool_free(&pool);

	return ret;
}

otama_status_t
otama_exists(otama_t *otama, int *result, const otama_id_t *id)
{
	bool bresult = false;
	otama_status_t ret;

	NV_ASSERT(otama != NULL);
	
	ret = otama->driver->exists(bresult, id);
	*result = bresult ? 1:0;
	
	return ret;
}

otama_status_t
otama_remove(otama_t *otama, const otama_id_t *id)
{
	NV_ASSERT(otama != NULL);
	return otama->driver->remove(id);
}

otama_status_t
otama_search(otama_t *otama,
			 otama_result_t **results, int n,
			 otama_variant_t *query)
{
	otama_status_t ret;
	long t = nv_clock();

	NV_ASSERT(otama != NULL);
	NV_ASSERT(OTAMA_VARIANT_IS_HASH(query));

	*results = NULL;
	ret = otama->driver->search(results, n, query);
	if (ret != OTAMA_STATUS_OK) {
		return ret;
	}

	OTAMA_LOG_DEBUG("otama_search: %dms\n", nv_clock() - t);

	return OTAMA_STATUS_OK;
}

otama_status_t
otama_search_file(otama_t *otama,
				  otama_result_t **results, int n,
				  const char *image_filename)
{
	otama_variant_pool_t *pool = otama_variant_pool_alloc();
	otama_variant_t *query = otama_variant_new(pool);
	otama_status_t ret;

	NV_ASSERT(otama != NULL);	

	otama_variant_set_hash(query);
	otama_variant_set_string(otama_variant_hash_at(query, "file"), image_filename);
	ret = otama_search(otama, results, n, query);

	otama_variant_pool_free(&pool);

	return ret;
}

otama_status_t
otama_search_data(otama_t *otama,
				  otama_result_t **results, int n,
				  const void *image_data, size_t image_data_len)
{
	otama_variant_pool_t *pool = otama_variant_pool_alloc();
	otama_variant_t *query = otama_variant_new(pool);
	otama_status_t ret;

	NV_ASSERT(otama != NULL);

	otama_variant_set_hash(query);
	otama_variant_set_binary(otama_variant_hash_at(query, "data"), image_data, image_data_len);
	ret = otama_search(otama, results, n, query);
	
	otama_variant_pool_free(&pool);

	return ret;
}

otama_status_t
otama_search_string(otama_t *otama,
					otama_result_t **results, int n,
					const char *image_feature_string)
{
	otama_variant_pool_t *pool = otama_variant_pool_alloc();
	otama_variant_t *query = otama_variant_new(pool);
	otama_status_t ret;

	NV_ASSERT(otama != NULL);

	otama_variant_set_hash(query);
	otama_variant_set_string(otama_variant_hash_at(query, "string"), image_feature_string);
	ret = otama_search(otama, results, n, query);
	
	otama_variant_pool_free(&pool);
	
	return ret;
}

otama_status_t otama_search_id(otama_t *otama,
							   otama_result_t **results, int n,
							   const otama_id_t *id)
{
	otama_variant_pool_t *pool = otama_variant_pool_alloc();
	otama_variant_t *query = otama_variant_new(pool);
	otama_status_t ret;
	
	NV_ASSERT(otama != NULL);

	otama_variant_set_hash(query);
	otama_variant_set_binary(otama_variant_hash_at(query, "id"), id, sizeof(*id));
	ret = otama_search(otama, results, n, query);
	
	otama_variant_pool_free(&pool);
	
	return ret;
}

otama_status_t
otama_search_raw(otama_t *otama,
				 otama_result_t **results, int n,
				 const otama_feature_raw_t *raw)
{
	otama_variant_pool_t *pool = otama_variant_pool_alloc();
	otama_variant_t *query = otama_variant_new(pool);
	otama_status_t ret;

	NV_ASSERT(otama != NULL);

	otama_variant_set_hash(query);
	otama_variant_set_pointer(otama_variant_hash_at(query, "raw"), raw);
	ret = otama_search(otama, results, n, query);
	
	otama_variant_pool_free(&pool);
	
	return ret;
}

otama_status_t
otama_feature_raw(otama_t *otama, otama_feature_raw_t **raw,
				  otama_variant_t *data)
{
	otama_status_t ret;
	
	NV_ASSERT(otama != NULL);
	NV_ASSERT(OTAMA_VARIANT_IS_HASH(data));
	*raw = NULL;
	
	ret = otama->driver->feature_raw(raw, data);

	return ret;
}

otama_status_t
otama_feature_raw_data(otama_t *otama, otama_feature_raw_t **raw,
					   const  void *image_data, size_t image_data_len)
{
	otama_variant_pool_t *pool = otama_variant_pool_alloc();
	otama_variant_t *data = otama_variant_new(pool);
	otama_status_t ret;

	NV_ASSERT(otama != NULL);
	*raw = NULL;
	
	otama_variant_set_hash(data);
	otama_variant_set_binary(otama_variant_hash_at(data, "data"), image_data, image_data_len);
	ret = otama_feature_raw(otama, raw, data);
	otama_variant_pool_free(&pool);
	
	return ret;
}

otama_status_t
otama_feature_raw_file(otama_t *otama, otama_feature_raw_t **raw,
					   const  char *image_filename)
{
	otama_variant_pool_t *pool = otama_variant_pool_alloc();
	otama_variant_t *data = otama_variant_new(pool);
	otama_status_t ret;

	NV_ASSERT(otama != NULL);
	*raw = NULL;	
	
	otama_variant_set_hash(data);
	otama_variant_set_string(otama_variant_hash_at(data, "file"), image_filename);
	ret = otama_feature_raw(otama, raw, data);
	otama_variant_pool_free(&pool);
	
	return ret;
}

otama_status_t
otama_feature_raw_free(otama_feature_raw_t **raw)
{
	otama_status_t ret;
	
	ret = otama::DriverInterface::feature_raw_free(raw);

	return ret;
}


otama_status_t
otama_feature_string(otama_t *otama,
					 char **feature_string,
					 otama_variant_t *data)
{
	std::string cpp_feature_string;
	otama_status_t ret;
	
	NV_ASSERT(otama != NULL);
	NV_ASSERT(OTAMA_VARIANT_IS_HASH(data));

	*feature_string = NULL;
	
	ret = otama->driver->feature_string(cpp_feature_string, data);
	if (ret == OTAMA_STATUS_OK) {
		*feature_string = nv_alloc_type(char, cpp_feature_string.size() + 1);
		strcpy(*feature_string, cpp_feature_string.c_str());
	}

	return ret;
}

otama_status_t
otama_feature_string_data(otama_t *otama, char **feature_string,
						  const  void *image_data, size_t image_data_len)
{
	otama_variant_pool_t *pool = otama_variant_pool_alloc();
	otama_variant_t *data = otama_variant_new(pool);
	otama_status_t ret;

	NV_ASSERT(otama != NULL);
	*feature_string = NULL;
	
	otama_variant_set_hash(data);
	otama_variant_set_binary(otama_variant_hash_at(data, "data"), image_data, image_data_len);
	ret = otama_feature_string(otama, feature_string, data);
	otama_variant_pool_free(&pool);
	
	return ret;
}

otama_status_t
otama_feature_string_file(otama_t *otama, char **feature_string,
						  const  char *image_filename)
{
	otama_variant_pool_t *pool = otama_variant_pool_alloc();
	otama_variant_t *data = otama_variant_new(pool);
	otama_status_t ret;

	NV_ASSERT(otama != NULL);
	*feature_string = NULL;	
	
	otama_variant_set_hash(data);
	otama_variant_set_string(otama_variant_hash_at(data, "file"), image_filename);
	ret = otama_feature_string(otama, feature_string, data);
	otama_variant_pool_free(&pool);
	
	return ret;
}

void
otama_feature_string_free(char **feature_string)
{
	if (feature_string && *feature_string) {
		nv_free(*feature_string);
		*feature_string = NULL;
	}
}

const char *
otama_version_string(void)
{
	return OTAMA_VERSION;
}

otama_status_t
otama_similarity(otama_t *otama,
			float *similarity,
			otama_variant_t *data1,
			otama_variant_t *data2)
{
	return otama->driver->similarity(similarity, data1, data2);
}

otama_status_t
otama_similarity_file(otama_t *otama,
				 float *similarity,
				 const char *file1,
				 const char *file2)
{
	otama_variant_pool_t *pool = otama_variant_pool_alloc();
	otama_variant_t *data1 = otama_variant_new(pool);
	otama_variant_t *data2 = otama_variant_new(pool);
	otama_status_t ret;
	
	NV_ASSERT(otama != NULL);
	
	otama_variant_set_hash(data1);
	otama_variant_set_string(otama_variant_hash_at(data1, "file"), file1);
	otama_variant_set_hash(data2);
	otama_variant_set_string(otama_variant_hash_at(data2, "file"), file2);
	
	ret = otama_similarity(otama, similarity, data1, data2);
	otama_variant_pool_free(&pool);
	
	return ret;
}

otama_status_t
otama_similarity_data(otama_t *otama,
				 float *similarity,
				 const void *image_data1,
				 size_t image_data1_len,
				 const void *image_data2,
				 size_t image_data2_len)
{
	otama_variant_pool_t *pool = otama_variant_pool_alloc();
	otama_variant_t *data1 = otama_variant_new(pool);
	otama_variant_t *data2 = otama_variant_new(pool);
	otama_status_t ret;
	
	NV_ASSERT(otama != NULL);

	otama_variant_set_hash(data1);
	otama_variant_set_binary(otama_variant_hash_at(data1, "data"),
							 image_data1, image_data1_len);
	otama_variant_set_hash(data2);
	otama_variant_set_binary(otama_variant_hash_at(data2, "data"),
							 image_data2, image_data2_len);

	ret = otama_similarity(otama, similarity, data1, data2);
	otama_variant_pool_free(&pool);
	
	return ret;
}

otama_status_t
otama_similarity_string(otama_t *otama,
						float *similarity,
						const char *feature_string1,
						const char *feature_string2)
{
	otama_variant_pool_t *pool = otama_variant_pool_alloc();
	otama_variant_t *data1 = otama_variant_new(pool);
	otama_variant_t *data2 = otama_variant_new(pool);
	otama_status_t ret;
	
	NV_ASSERT(otama != NULL);

	otama_variant_set_hash(data1);
	otama_variant_set_string(otama_variant_hash_at(data1, "string"),
							 feature_string1);
	otama_variant_set_hash(data2);
	otama_variant_set_string(otama_variant_hash_at(data2, "string"),
							 feature_string2);
	
	ret = otama_similarity(otama, similarity, data1, data2);
	otama_variant_pool_free(&pool);
	
	return ret;
}

otama_status_t
otama_similarity_raw(otama_t *otama,
					 float *similarity,
					 const otama_feature_raw_t *raw1,
					 const otama_feature_raw_t *raw2)
{
	otama_variant_pool_t *pool = otama_variant_pool_alloc();
	otama_variant_t *data1 = otama_variant_new(pool);
	otama_variant_t *data2 = otama_variant_new(pool);
	otama_status_t ret;
	
	NV_ASSERT(otama != NULL);
	
	otama_variant_set_hash(data1);
	otama_variant_set_pointer(otama_variant_hash_at(data1, "raw"),
							  raw1);
	otama_variant_set_hash(data2);
	otama_variant_set_pointer(otama_variant_hash_at(data2, "raw"),
							  raw2);
	
	ret = otama_similarity(otama, similarity, data1, data2);
	otama_variant_pool_free(&pool);
	
	return ret;
}

otama_status_t
otama_set(otama_t *otama,
		  const char *key,
		  otama_variant_t *value)
{
	NV_ASSERT(otama != NULL);	
	return otama->driver->set(std::string(key), value);
}

otama_status_t
otama_get(otama_t *otama,
		  const char *key,
		  otama_variant_t *value)
{
	NV_ASSERT(otama != NULL);	
	return otama->driver->get(std::string(key), value);
}

otama_status_t
otama_unset(otama_t *otama,
			const char *key)
{
	NV_ASSERT(otama != NULL);	
	return otama->driver->unset(std::string(key));
}

otama_status_t
otama_invoke(otama_t *otama,
			 const char *method,
			 otama_variant_t *output,
			 otama_variant_t *input
	)
{
	NV_ASSERT(otama != NULL);	
	return otama->driver->invoke(std::string(method), output, input);
}

void
otama_cuda_set(int onoff)
{
	nv_cuda_set(onoff);
}

int
otama_cuda_enabled(void)
{
	return nv_cuda_enabled();
}

void
otama_omp_set_procs(int procs)
{
	nv_omp_set_procs(procs);
}


// initializer
static int otama_init_flag = 0;
static void
otama_initialize(void)
{
	if (otama_init_flag == 0) {
		otama_init_flag = 1;
		
		// nv
		nv_initialize();
		
		const char *log_level = nv_getenv("OTAMA_LOG_LEVEL");
		if (log_level) {
			if (nv_strcasecmp(log_level, "debug") == 0) {
				otama_log_set_level(OTAMA_LOG_LEVEL_DEBUG);
			} else if (nv_strcasecmp(log_level, "notice") == 0) {
				otama_log_set_level(OTAMA_LOG_LEVEL_NOTICE);
			} else if (nv_strcasecmp(log_level, "quiet") == 0) {
				otama_log_set_level(OTAMA_LOG_LEVEL_QUIET);
			}
		}
#if OTAMA_WITH_LZO
		if (lzo_init() != LZO_E_OK) {
			fprintf(stderr, "otama fatal error: lzo_init");
			abort();
		}
#endif
	}
}

#if OTAMA_WINDOWS
#  include <windows.h>
#  include <tchar.h>
#  include <stdlib.h>
#  if OTAMA_MSVC
#    include <new.h>
#  endif
static void 
otama_outofmemory(void)
{
	::MessageBox(::GetDesktopWindow(), "out of memory!", "otama", MB_ICONERROR|MB_OK);
	abort();
}

static int
otama_outofmemory_new(size_t l)
{
	::MessageBox(::GetDesktopWindow(), "out of memory!", "otama", MB_ICONERROR|MB_OK);
	//throw bad_alloc();
	abort();
	return 0;
}
#endif

#if OTAMA_GCC
static void
__attribute__ ((constructor)) otama_dll_entry_point(void)
{
	otama_initialize();
#if OTAMA_WINDOWS
	nv_set_outofmemory_func(otama_outofmemory);
#endif
}

#elif OTAMA_MSVC
static void
otama_initialize_msvc(HINSTANCE hModule)
{
	if (otama_init_flag == 0) {
		otama_initialize();
		// set pkgdatadir
		const char *cb_dir = getenv("NV_BOVW_PKGDATADIR");
		if (cb_dir == NULL || strlen(cb_dir) == 0) {
			// default NV_BOVW_PKGDATADIR for Windows
			char szDirName[_MAX_PATH];
			char szBaseName[_MAX_PATH];
			char szModuleFileName[_MAX_PATH];
				
			GetModuleFileNameA((HMODULE) hModule, szModuleFileName, _MAX_PATH-1);
			otama_split_path(szDirName, sizeof(szDirName),
				szBaseName, sizeof(szBaseName),
				szModuleFileName);
			strncat(szDirName, "/share", sizeof(szDirName)-1);
			nv_setenv("NV_BOVW_PKGDATADIR", szDirName);
			OTAMA_LOG_DEBUG("set NV_BOVW_PKGDATADIR = %s", szDirName);
		}
		_set_new_handler(otama_outofmemory_new);
		nv_set_outofmemory_func(otama_outofmemory);
	}
}

extern "C" BOOL WINAPI  
DllMain(HINSTANCE hModule, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH) {
		otama_initialize_msvc(hModule);
	}
	return TRUE;
}
#endif
