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
#include "otama_log.h"
#include "otama_status.h"
#include "nv_core.h"
#include "nv_util.h"
#include <vector>
#include <string>
#include <signal.h>

using namespace std;

static int
import_multi_client(const char *filename, otama_variant_t *config)
{
	FILE *fp = fopen(filename, "r");
	char line[8192];
	size_t len;
	std::vector<std::string> list;
	int i;
	int end;
	int threads = nv_omp_procs();
	otama_t **otama = nv_alloc_type(otama_t *, 	threads);
	
	if (fp == NULL) {
		OTAMA_LOG_ERROR("open failed: %s", filename);
		return -1;
	}
	
	while (fgets(line, sizeof(line) - 1, fp)) {
		len = strlen(line);
		line[len-1] = '\0';
		list.push_back(std::string(line));
	}
	end = list.size();
	fclose(fp);

	for (i = 0; i < threads; ++i) {
		otama_status_t ret;
		ret = otama_open_opt(&otama[i], config);
		if (ret != OTAMA_STATUS_OK) {
			OTAMA_LOG_ERROR("otama_open failed: %s", otama_status_message(ret));
			return -1;
		}
	}

#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic, 1) num_threads(threads)
#endif
	for (i = 0; i < end; ++i) {
		const char *import_file = list[i].c_str();
		char hex[OTAMA_ID_HEXSTR_LEN];
		otama_id_t id;
		otama_status_t ret;
		int thread_id = nv_omp_thread_id();
		
		ret = otama_insert_file(otama[thread_id], &id, import_file);
		if (ret != OTAMA_STATUS_OK) {
			OTAMA_LOG_ERROR("%s: %s", import_file, otama_status_message(ret));
		} else {
			otama_id_bin2hexstr(hex, &id);
#ifdef _OPENMP
#pragma omp critical (otama_printf)
#endif
			{
				printf("%s %s\n", hex, import_file);
			}
		}
	}

	for (i = 0; i < threads; ++i) {
		otama_close(&otama[i]);
	}
	
	return 0;
}

static int
import_parallel(const char *filename, otama_variant_t *config)
{
	FILE *fp;
	char line[8192];
	size_t len;
	std::vector<std::string> list;
	int i;
	int end;
	int threads = nv_omp_procs();
	otama_t *otama= NULL;
	otama_status_t ret;
	
	fp = fopen(filename, "r");
	if (fp == NULL) {
		OTAMA_LOG_ERROR("%s: %s\n", filename, strerror(errno));
		return -1;
	}
	ret = otama_open_opt(&otama, config);
	if (ret != OTAMA_STATUS_OK) {
		OTAMA_LOG_ERROR("otama_open: %s\n", otama_status_message(ret));
		return -1;
	}
	
	while (fgets(line, sizeof(line) - 1, fp)) {
		len = strlen(line);
		line[len-1] = '\0';
		list.push_back(std::string(line));
	}
	end = list.size();
	fclose(fp);
	
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic, 1) num_threads(threads)
#endif
	for (i = 0; i < end; ++i) {
		const char *import_file = list[i].c_str();
		char hex[OTAMA_ID_HEXSTR_LEN];
		otama_id_t id;
		otama_status_t ret;
		
		ret = otama_insert_file(otama, &id, import_file);
		if (ret != OTAMA_STATUS_OK) {
			OTAMA_LOG_ERROR("%s: %s", import_file, otama_status_message(ret));
		} else {
			otama_id_bin2hexstr(hex, &id);
#ifdef _OPENMP
#pragma omp critical (otama_printf)
#endif
			{
				printf("%s %s\n", hex, import_file);
			}
		}
	}
	otama_close(&otama);
	
	return 0;
}

static void
print_usage(void)
{
	printf(
		"otama_import [OPTIONS] -c config_file image_list_file\n"
		"    -h                  display this help and exit.\n"
		"    -d                  log_level = DEBUG.\n"
		"    -m                  multi client mode.\n"
		"    -c file             path to configuration file.(config.yaml)\n"
		"%s %s\n", OTAMA_PACKAGE, OTAMA_VERSION);
}

static void
handle_segv(int i)
{
	fprintf(stderr, "---- SEGV\n");
	NV_BACKTRACE;
	abort();
}

int
main(int argc, char **argv)
{
	int iret;
	int opt;
	otama_log_level_e level = OTAMA_LOG_LEVEL_NOTICE;
	int multi_client = 0;
	const char *filename = NULL;
	otama_variant_pool_t *pool = otama_variant_pool_alloc();
	otama_variant_t *config = NULL;

	signal(SIGSEGV, handle_segv);
	
	while ((opt = nv_getopt(argc, argv, "hdmc:")) != -1){
		switch (opt) {
		case 'h':
			print_usage();
			return 0;
		case 'm':
			multi_client = 1;
			break;
		case 'd':
			level = OTAMA_LOG_LEVEL_DEBUG;
			break;
		case 'c':
			config = otama_yaml_read_file(nv_getopt_optarg, pool);
			if (config == NULL) {
				OTAMA_LOG_ERROR("%s: parse error or empty.", nv_getopt_optarg);
				return -1;
			}
			break;
		default:
			print_usage();
			return 0;
		}
	}
	if (!config) {
		print_usage();
		otama_variant_pool_free(&pool);		
		return -1;
	}
	argc -= nv_getopt_optind;
	argv += nv_getopt_optind;
	if (argc != 1) {
		print_usage();
		return -1;
	}
	filename = argv[0];

	otama_log_set_level(level);

	if (multi_client) {
		iret = import_multi_client(filename, config);
	} else {
		iret = import_parallel(filename, config);
	}
	otama_variant_pool_free(&pool);
	
	return iret;
}
