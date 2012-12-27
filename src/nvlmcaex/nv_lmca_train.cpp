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

#include "nv_core.h"
#include "nv_core.h"
#include "nv_ml.h"
#include "nv_io.h"
#include "nv_ip.h"
#include "nv_num.h"
#include "nv_lmca.hpp"
#include "nv_color_boc.h"

static void
help(void)
{
	const char *name = "nv_lmca_train";	
	printf("%s [OPTIONS]\n"
		   "    -e (vlad|hsv|vladhsv) extract mode\n"
		   "    -t (vlad|hsv|vladhsv) train mode\n"
		   " train mode options\n"
		   "    -i n   iteration\n"
		   "    -k n   pull kNN k (default: 12)\n"
		   "    -n n   push kNN k (default: 16)\n"
		   "    -r f   push weight (default: 0.2, pull weight:1.0-f)\n"
		   "    -m f   margin (default: 0.5)\n"
		   "    -d f   learning rate (default: 0.1)\n"
		   "    -l s   lmca_file (resume)\n",
		   name
		);
}

typedef enum {
	UNKNOWN = 0,
	TRAIN = 1,
	EXTRACT = 2
} mode_e;

template<nv_lmca_feature_e T, typename C>
int _main(mode_e mode,
		  int nk, int mk, float margin, float push_ratio, float delta, int  max_epoch,
		  const char *base_file)
{
	nv_matrix_t *data = NULL;
	nv_matrix_t *labels = NULL;
	nv_matrix_t *l = NULL;
	bool initialize = true;
	nv_lmca_ctx<T, C> ctx;
	const char *data_file;
	const char *label_file;
	const char *metric_file;
	
	if (T == NV_LMCA_FEATURE_VLAD) {
		data_file = "vlad_data.vecb";
		label_file = "vlad_labels.vecb";
		metric_file = "lmca_vlad.mat";
	} else if (T == NV_LMCA_FEATURE_HSV) {
		data_file = "hsv_data.vecb";
		label_file = "hsv_labels.vecb";
		metric_file = "lmca_hsv.mat";
	} else {
		data_file = "vladhsv_data.vecb";
		label_file = "vladhsv_labels.vecb";
		metric_file = "lmca_vladhsv.mat";
	}
	if (base_file[0] != '\0') {
		l = nv_load_matrix(base_file);
		if (l == NULL) {
			perror(base_file);
			return -1;
		}
		if (!(l->n == nv_lmca_ctx<T, C>::RAW_DIM
			  && l->m == nv_lmca_ctx<T, C>::LMCA_DIM))
		{
			fprintf(stderr, "%s: invalid matrix\n", base_file);
			return -1;
		}
	}
	switch (mode) {
	case TRAIN:
		if (l == NULL) {
			l = nv_matrix_alloc(nv_lmca_ctx<T, C>::RAW_DIM, nv_lmca_ctx<T, C>::LMCA_DIM);
		} else {
			initialize = false;
		}
		data = nv_load_matrix_bin(data_file);
		labels = nv_load_matrix_bin(label_file);
		if (data == NULL) {
			fprintf(stderr, "%s: error\n", data_file);
			return -1;
		}
		if (labels == NULL) {
			fprintf(stderr, "%s: error\n", label_file);
			return -1;
		}
		nk = NV_MIN(data->m, nk);
		mk = NV_MIN(data->m, mk);
		
		ctx.train(l, data, labels, 1, nk, mk, margin, push_ratio, delta, max_epoch, initialize);
		nv_save_matrix_text(metric_file, l);
		nv_matrix_free(&data);
		nv_matrix_free(&labels);
		nv_matrix_free(&l);
		break;
	case EXTRACT:
		if (ctx.make_train_data(&data, &labels, "filelist.txt") != 0) {
			return -1;
		}
		nv_save_matrix_bin(data_file, data);
		nv_save_matrix_bin(label_file, labels);
		nv_matrix_free(&data);
		nv_matrix_free(&labels);
		break;
	default:
		help();
		return -1;
	}
	
	return 0;
}

int
main(int argc, char** argv)
{
	mode_e mode = UNKNOWN;
	int opt;
	int nk = 12;
	int mk = 16;
	float margin = 0.5f;
	float delta = 0.1f;
	float push_ratio = 0.2f;
	int max_epoch = 20;
	char base_file[1024] = {0};
	char feature[256] = {0};
	
	while ((opt = nv_getopt(argc, argv, "e:t:hk:m:d:n:i:r:l:")) != -1) {
		switch (opt) {
		case 'l':
			strncpy(base_file, nv_getopt_optarg, sizeof(base_file) - 1);
			break;
		case 'k':
			nk = atoi(nv_getopt_optarg);
			break;
		case 'n':
			mk = atoi(nv_getopt_optarg);
			break;
		case 'm':
			margin = atof(nv_getopt_optarg);
			break;
		case 'r':
			push_ratio = atof(nv_getopt_optarg);
			break;
		case 'd':
			delta = atof(nv_getopt_optarg);
			break;
		case 'i':
			max_epoch = atof(nv_getopt_optarg);
			break;
		case 'e':
			mode = EXTRACT;
			strncpy(feature, nv_getopt_optarg, sizeof(feature) -1);
			break;
		case 't':
			mode = TRAIN;
			strncpy(feature, nv_getopt_optarg, sizeof(feature) -1);
			break;
		case 'h':
		default:
			help();
			return -1;
		}
	}
	if (mode == UNKNOWN) {
		help();
		return -1;
	}
	if (nv_strcasecmp(feature, "vlad") == 0) {
		_main<NV_LMCA_FEATURE_VLAD, nv_lmca_empty_color_t>(
			mode, nk, mk, margin, push_ratio, delta, max_epoch, base_file);
	} else if (nv_strcasecmp(feature, "hsv") == 0) {
		_main<NV_LMCA_FEATURE_HSV, nv_lmca_empty_color_t>(
			mode, nk, mk, margin, push_ratio, delta, max_epoch, base_file);
	} else if (nv_strcasecmp(feature, "vladhsv") == 0) {
		_main<NV_LMCA_FEATURE_VLADHSV, nv_lmca_empty_color_t>(
			mode, nk, mk, margin, push_ratio, delta, max_epoch, base_file);
	} else {
		return -1;
	}
	
	return 0;
}
