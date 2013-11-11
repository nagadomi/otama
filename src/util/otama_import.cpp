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

typedef enum {
	OTAMA_IMPORT_FORMAT_NORMAL,
	OTAMA_IMPORT_FORMAT_LABELED
} otama_import_format_e;

static otama_import_format_e
lookup_format(const char *test_line)
{
	FILE *fp;
	fp = fopen(test_line, "rb");
	if (fp == NULL) {
		const char *file = strstr(test_line, " ");
		if (file != NULL) {
			file += 1;
			fp = fopen(file, "rb");
			if (fp == NULL) {
				return OTAMA_IMPORT_FORMAT_NORMAL;
			}
			fclose(fp);
			return OTAMA_IMPORT_FORMAT_LABELED;
		}
		return OTAMA_IMPORT_FORMAT_NORMAL;
	}
	fclose(fp);
	return OTAMA_IMPORT_FORMAT_NORMAL;
}

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
	otama_import_format_e format = OTAMA_IMPORT_FORMAT_NORMAL;
	
	if (fp == NULL) {
		fprintf(stderr, "otama_import: fopen failed: %s\n",
				filename);
		return -1;
	}
	
	while (fgets(line, sizeof(line) - 1, fp)) {
		len = strlen(line);
		line[len-1] = '\0';
		list.push_back(std::string(line));
	}
	fclose(fp);
	
	end = list.size();
	if (end > 0) {
		format = lookup_format(list[0].c_str());
	}
	for (i = 0; i < threads; ++i) {
		otama_status_t ret;
		ret = otama_open_opt(&otama[i], config);
		if (ret != OTAMA_STATUS_OK) {
			fprintf(stderr, "otama_import: otama_open_opt failed\n");
			return -1;
		}
	}

#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic, 1) num_threads(threads)
#endif
	for (i = 0; i < end; ++i) {
		char hex[OTAMA_ID_HEXSTR_LEN];
		otama_id_t id;
		otama_status_t ret;
		int thread_id = nv_omp_thread_id();
		const char *import_file = list[i].c_str();

		if (format == OTAMA_IMPORT_FORMAT_LABELED) {
			const char *f = strstr(import_file, " ");
			if (f != NULL) {
				import_file = f + 1;
			}
		}
		ret = otama_insert_file(otama[thread_id], &id, import_file);
		if (ret != OTAMA_STATUS_OK) {
			fprintf(stderr, "otama_import: otama_insert_file failed: %s\n",
					import_file);
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
	otama_import_format_e format = OTAMA_IMPORT_FORMAT_NORMAL;
	
	fp = fopen(filename, "r");
	if (fp == NULL) {
		fprintf(stderr, "otama_import: fopen failed: %s\n",
				filename);
		return -1;
	}
	ret = otama_open_opt(&otama, config);
	if (ret != OTAMA_STATUS_OK) {
		fprintf(stderr, "otama_import: otama_open_opt failed\n");
		return -1;
	}
	
	while (fgets(line, sizeof(line) - 1, fp)) {
		len = strlen(line);
		line[len-1] = '\0';
		list.push_back(std::string(line));
	}
	fclose(fp);
	end = list.size();
	if (end > 0) {
		format = lookup_format(list[0].c_str());
	}
	
#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic, 1) num_threads(threads)
#endif
	for (i = 0; i < end; ++i) {
		char hex[OTAMA_ID_HEXSTR_LEN];
		otama_id_t id;
		otama_status_t ret;
		const char *import_file = list[i].c_str();
		
		if (format == OTAMA_IMPORT_FORMAT_LABELED) {
			const char *f = strstr(import_file, " ");
			if (f != NULL) {
				import_file = f + 1;
			}
		}
		ret = otama_insert_file(otama, &id, import_file);
		if (ret != OTAMA_STATUS_OK) {
			fprintf(stderr, "otama_import: otama_insert_file failed: %s\n",
					import_file);
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
				fprintf(stderr, "otama_search: otama_yaml_read_file failed: %s: parse error or empty.\n", nv_getopt_optarg);
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
	if (level == OTAMA_LOG_LEVEL_DEBUG) {
		otama_log_set_level(level);
	}

	if (multi_client) {
		iret = import_multi_client(filename, config);
	} else {
		iret = import_parallel(filename, config);
	}
	otama_variant_pool_free(&pool);
	
	return iret;
}
