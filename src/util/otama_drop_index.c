/*
 * This file is part of otama.
 *
 * Copyright (C) 2013 nagadomi@nurs.or.jp
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
#include "nv_util.h"

static void
print_usage(void)
{
	printf(
		"otama_drop_index [OPTIONS] -c file\n"
		"    -h                  display this help and exit.\n"
		"    -d                  log_level = DEBUG.\n"
		"    -c file             path to configuration file.(config.yaml)\n"
		"%s %s\n", OTAMA_PACKAGE, OTAMA_VERSION);
}

int
main(int argc, char **argv)
{
	otama_t *otama;
	otama_variant_pool_t *pool = otama_variant_pool_alloc();
	otama_variant_t *config = NULL;
	otama_status_t ret;
	otama_log_level_e level = OTAMA_LOG_LEVEL_NOTICE;
	int opt;
	
	while ((opt = nv_getopt(argc, argv, "hdc:")) != -1){
		switch (opt) {
		case 'h':
			print_usage();
			return 0;
		case 'd':
			level = OTAMA_LOG_LEVEL_DEBUG;
			break;
		case 'c':
			config = otama_yaml_read_file(nv_getopt_optarg, pool);
			if (config == NULL) {
				fprintf(stderr, "otama_drop_index: otama_yaml_read_file failed: %s: parse error or empty.\n", nv_getopt_optarg);
				otama_variant_pool_free(&pool);				
				return -1;
			}
			break;
		default:
			print_usage();
			otama_variant_pool_free(&pool);			
			return 0;
		}
	}
	if (level == OTAMA_LOG_LEVEL_DEBUG) {
		otama_log_set_level(level);
	}
	
	if (!config) {
		print_usage();
		otama_variant_pool_free(&pool);		
		return -1;
	}
	ret = otama_open_opt(&otama, config);	
	if (ret != OTAMA_STATUS_OK) {
		fprintf(stderr, "otama_drop_index: otama_open failed: %s\n", otama_status_message(ret));
		otama_variant_pool_free(&pool);		
		return -1;
	}
	
	ret = otama_drop_index(otama);
	if (ret != OTAMA_STATUS_OK) {
		fprintf(stderr, "otama_drop_index: otama_drop_index failed: %s\n", otama_status_message(ret));
		otama_close(&otama);
		otama_variant_pool_free(&pool);		
		return -1;
	}
	otama_close(&otama);
	otama_variant_pool_free(&pool);
	
	return 0;
}
