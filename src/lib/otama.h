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
#ifndef OTAMA_H
#define OTAMA_H

#include "otama_status.h"
#include "otama_feature_raw.h"
#include "otama_result.h"
#include "otama_id.h"
#include "otama_log.h"
#include "otama_variant.h"
#include "otama_yaml.h"
#include "otama_image.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct otama_instance otama_t;

void otama_cuda_set(int onoff);
int otama_cuda_enabled(void);

otama_status_t otama_open_opt(otama_t **otama,
							  otama_variant_t *options);
otama_status_t otama_open(otama_t **otama,
						  const char *config_path);

int otama_active(otama_t *otama);
otama_status_t otama_count(otama_t *otama, int64_t *count);
void otama_close(otama_t **otama);

otama_status_t otama_create_database(otama_t *otama);
otama_status_t otama_drop_database(otama_t *otama);
otama_status_t otama_drop_index(otama_t *otama);
otama_status_t otama_vacuum_index(otama_t *otama);
	
otama_status_t otama_pull(otama_t *otama);

otama_status_t otama_insert(otama_t *otama, otama_id_t *id,
							otama_variant_t *data);
otama_status_t otama_insert_file(otama_t *otama, otama_id_t *id,
								 const char *file);
otama_status_t otama_insert_data(otama_t *otama, otama_id_t *id,
								 const  void *data, size_t data_len);

otama_status_t otama_remove(otama_t *otama, const otama_id_t *id);

otama_status_t
otama_search(otama_t *otama,
			 otama_result_t **results,
			 int n,
			 otama_variant_t *query);

otama_status_t otama_search_file(otama_t *otama,
								 otama_result_t **results, int n,
								 const char *file);
otama_status_t otama_search_data(otama_t *otama,
								 otama_result_t **results, int n,
								 const void *data, size_t data_len);
otama_status_t otama_search_string(otama_t *otama,
								   otama_result_t **results, int n,
								   const char *image_feature_string);
otama_status_t otama_search_id(otama_t *otama,
							   otama_result_t **results, int n,
							   const otama_id_t *id);
otama_status_t otama_search_raw(otama_t *otama,
								otama_result_t **results, int n,
								const otama_feature_raw_t *raw);

otama_status_t otama_exists(otama_t *otama, int *result, const otama_id_t *id);


otama_status_t otama_feature_raw(otama_t *otama, otama_feature_raw_t **raw,
								 otama_variant_t *data);
otama_status_t otama_feature_raw_file(otama_t *otama, otama_feature_raw_t **raw,
									  const  char *image_filename);
otama_status_t
otama_feature_raw_data(otama_t *otama, otama_feature_raw_t **raw,
					   const  void *image_data, size_t image_data_len);
otama_status_t otama_feature_raw_free(otama_feature_raw_t **raw);
	
otama_status_t otama_feature_string(otama_t *otama, char **feature_string,
									otama_variant_t *data);
void otama_feature_string_free(char **feature_string);
otama_status_t otama_feature_string_file(otama_t *otama, char **feature_string,
										 const  char *file);
otama_status_t otama_feature_string_data(otama_t *otama, char **feature_string,
										 const  void *data, size_t data_len);

const char *otama_version_string(void);

otama_status_t otama_similarity(otama_t *otama,
								float *similarity,
								otama_variant_t *data1,
								otama_variant_t *data2);

otama_status_t otama_similarity_file(otama_t *otama,
									 float *similarity,
									 const char *file1,
									 const char *file2);
	
otama_status_t otama_similarity_data(otama_t *otama,
									 float *similarity,
									 const void *data1,
									 size_t data1_len,
									 const void *data2,
									 size_t data2_len);
otama_status_t otama_similarity_string(otama_t *otama,
									   float *similarity,
									   const char *feature_string1,
									   const char *feature_string2);
otama_status_t otama_similarity_raw(otama_t *otama,
									float *similarity,
									const otama_feature_raw_t *raw1,
									const otama_feature_raw_t *raw2);

otama_status_t otama_set(otama_t *otama,
						 const char *key,
						 otama_variant_t *value);
otama_status_t otama_unset(otama_t *otama,
						   const char *key);
	
otama_status_t otama_get(otama_t *otama,
						 const char *key,
						 otama_variant_t *value);
otama_status_t otama_invoke(otama_t *otama,
							const char *method,
							otama_variant_t *output,
							otama_variant_t *input);

void otama_omp_set_procs(int procs);

#ifdef __cplusplus
}
#endif

#endif
