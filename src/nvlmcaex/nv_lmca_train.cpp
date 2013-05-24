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

#define DEFAULT_K 20
#define DEFAULT_N(k) (k) + (k) / 2 + 2
#define DEFAULT_MARGIN 0.05f
#define DEFAULT_VQ_SAMPLES 10000000
#define DEFAULT_PUSH_HSV 0.45f
#define DEFAULT_PUSH_VLAD 0.3f
#define DEFAULT_PUSH_VLADHSV 0.3f

static void
print_usage(void)
{
	const char *name = "nv_lmca_train";	
	printf("%s [OPTIONS] [filelist.txt]\n"
		   "    -e (vlad|hsv|vladhsv) extract mode\n"
		   "    -t (vlad|hsv|vladhsv|vq) training mode\n"
		   "    -q s   path to the vlad vector quantization file(i/o)\n"
		   "    -f s   path to the data file(i/o)\n"
		   " extract mode options\n"
		   "    -x     use flip image\n"
		   " vlad/hsv/vladhsv training mode options\n"
		   "    -i n   iteration (default: 50)\n"
		   "    -k n   pull kNN k (default: 20)\n"
		   "    -n n   push kNN k (default: 32)\n"
		   "    -r f   push weight (default: vlad: 0.3, vladhsv: 0.3, hsv: 0.45, pull weight: 1.0-f)\n"
		   "    -m f   margin (default: 0.05)\n"
		   "    -d f   learning rate (default: 0.1)\n"
		   "    -o s   path to the lmca_file (output)\n"
		   "    -l s   path to the lmca_file (resume input)\n"
		   " vq training mode options\n"
		   "    -n n   max keypoints (default: 10000000)\n",
		   name
		);
}

typedef enum {
	UNKNOWN = 0,
	TRAIN = 1,
	EXTRACT = 2
} mode_e;

template<nv_lmca_feature_e T, typename C>
int lmca_main(mode_e mode, const char *vq_file,
			  int k, int k_n,
			  float margin, float push_weight, float learning_rate, int  max_epoch,
			  const char *filelist, bool flip,
			  const char *base_file,
			  const char *_data_file,
			  const char *_metric_file
	)
{
	nv_matrix_t *data = NULL;
	nv_matrix_t *labels = NULL;
	nv_matrix_t *l = NULL;
	bool initialize = true;
	nv_lmca_ctx<T, C> ctx;
	const char *metric_file;
	const char *data_file;

	if (T == NV_LMCA_FEATURE_VLAD) {
		if (_metric_file[0] != '\0') {
			metric_file = _metric_file;
		} else {
			metric_file = "lmca_vlad.mat";
		}
		if (_data_file[0] != '\0') {
			data_file = _data_file;
		} else {
			data_file = "vlad_data.matb";
		}
	} else if (T == NV_LMCA_FEATURE_HSV) {
		if (_metric_file[0] != '\0') {
			metric_file = _metric_file;
		} else {
			metric_file = "lmca_hsv.mat";
		}
		if (_data_file[0] != '\0') {
			data_file = _data_file;
		} else {
			data_file = "hsv_data.matb";
		}
	} else {
		if (_metric_file[0] != '\0') {
			metric_file = _metric_file;
		} else {
			metric_file = "lmca_vladhsv.mat";
		}
		if (_data_file[0] != '\0') {
			data_file = _data_file;
		} else {
			data_file = "vladhsv_data.matb";
		}
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
	if (vq_file[0] != '\0') {
		if (ctx.set_vq_table(vq_file) != 0) {
			fprintf(stderr, "%s: invalid VQ table\n", vq_file);
			return -1;
		}
	}
	switch (mode) {
	case TRAIN:
	{
		nv_matrix_t *mats[2];
		int len = 2;
		
		if (l == NULL) {
			l = nv_matrix_alloc(nv_lmca_ctx<T, C>::RAW_DIM, nv_lmca_ctx<T, C>::LMCA_DIM);
		} else {
			initialize = false;
		}
		
		if (nv_load_matrix_array_bin(data_file, mats, &len) != 0 || len != 2) {
			fprintf(stderr, "%s: invalid data file\n", data_file);
			return -1;
		}
		data = mats[0];
		labels = mats[1];
		if (data->m != labels->m || data->n != nv_lmca_ctx<T, C>::RAW_DIM || labels->n != 1)
		{
			nv_matrix_free(&data);
			nv_matrix_free(&labels);
			fprintf(stderr, "%s: invalid data file\n", data_file);
			return -1;
		}
		
		k = NV_MIN(data->m, k);
		k_n = NV_MIN(data->m, k_n);
		
		ctx.train(l, data, labels, 1, k, k_n, margin, push_weight, learning_rate, max_epoch, initialize);
		nv_save_matrix_text(metric_file, l);
		nv_matrix_free(&data);
		nv_matrix_free(&labels);
		nv_matrix_free(&l);
		break;
	}
	case EXTRACT:
	{
		nv_matrix_t *mats[2];
		if (ctx.make_train_data(&data, &labels, filelist, flip) != 0) {
			return -1;
		}
		mats[0] = data;
		mats[1] = labels;
		if (nv_save_matrix_array_bin(data_file, mats, 2) != 0) {
			fprintf(stderr, "%s: error\n", data_file);
			nv_matrix_free(&data);
			nv_matrix_free(&labels);
		}
		break;
	}
	default:
		print_usage();
		return -1;
	}
	
	return 0;
}

static int
vq(int n, const char *file, bool flip, const char *vq_file)
{
	nv_matrix_t *vq_posi, *vq_nega;
	int ret;
	
	vq_posi = nv_matrix_alloc(NV_KEYPOINT_DESC_N, nv_lmca_vq::VQ_DIM);
	vq_nega = nv_matrix_alloc(NV_KEYPOINT_DESC_N, nv_lmca_vq::VQ_DIM);
	
	nv_matrix_zero(vq_posi);
	nv_matrix_zero(vq_nega);
	
	if (nv_lmca_vq::train(n, vq_posi, vq_nega, file, flip) == 0) {
		nv_matrix_t *vqs[2];
		vqs[0] = vq_posi;
		vqs[1] = vq_nega;
		if (nv_save_matrix_array_text(vq_file, vqs, 2) != 0) {
			fprintf(stderr, "%s: %s\n", vq_file, strerror(errno));
			ret = -1;
		}
	} else {
		ret = -1;
	}
	nv_matrix_free(&vq_posi);
	nv_matrix_free(&vq_nega);
	
	return ret;
}


int
main(int argc, char** argv)
{
	mode_e mode = UNKNOWN;
	int opt;
	int k = DEFAULT_K;
	int n = -1;
	float margin = DEFAULT_MARGIN;
	float learning_rate = 0.1f;
	int max_epoch = 50;
	char base_file[8192] = {0};
	char feature[256] = {0};
	float push_weight = -1.0f;
	const char *filelist = NULL;
	char vq_file[8192] = {0};
	char data_file[8192] = {0};
	char metric_file[8192] = {0};
	bool flip = false;
	
	while ((opt = nv_getopt(argc, argv, "e:t:hk:m:d:n:i:r:l:q:xf:o:")) != -1) {
		switch (opt) {
		case 'f':
			strncpy(data_file, nv_getopt_optarg, sizeof(data_file) - 1);
			break;
		case 'o':
			strncpy(metric_file, nv_getopt_optarg, sizeof(metric_file) - 1);
			break;
		case 'x':
			flip = true;
			break;
		case 'l':
			strncpy(base_file, nv_getopt_optarg, sizeof(base_file) - 1);
			break;
		case 'k':
			k = atoi(nv_getopt_optarg);
			break;
		case 'n':
			n = atoi(nv_getopt_optarg);
			break;
		case 'm':
			margin = atof(nv_getopt_optarg);
			break;
		case 'r':
			push_weight = atof(nv_getopt_optarg);
			break;
		case 'd':
			learning_rate = atof(nv_getopt_optarg);
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
		case 'q':
			strncpy(vq_file, nv_getopt_optarg, sizeof(vq_file) -1);
			break;
		case 'h':
		default:
			print_usage();
			return -1;
		}
	}
	if (mode == UNKNOWN) {
		print_usage();
		return -1;
	}
	if (mode == EXTRACT) {
		argc -= nv_getopt_optind;
		argv += nv_getopt_optind;
		if (argc != 1) {
			print_usage();
			return -1;
		}
		filelist = argv[0];
	}
	if (nv_strcasecmp(feature, "vlad") == 0) {
		if (push_weight < 0.0) {
			push_weight = DEFAULT_PUSH_VLAD;
		}
		if (n < 0) {
			n = DEFAULT_N(k);
		}
		lmca_main<NV_LMCA_FEATURE_VLAD, nv_lmca_empty_color_t>(
			mode, vq_file, k, n, margin, push_weight, learning_rate, max_epoch, filelist, flip, base_file, data_file, metric_file);
	} else if (nv_strcasecmp(feature, "vladhsv") == 0) {
		if (push_weight < 0.0) {
			push_weight = DEFAULT_PUSH_VLADHSV;
		}
		if (n < 0) {
			n = DEFAULT_N(k);
		}
		lmca_main<NV_LMCA_FEATURE_VLADHSV, nv_lmca_empty_color_t>(
			mode, vq_file, k, n, margin, push_weight, learning_rate, max_epoch, filelist, flip, base_file, data_file, metric_file);
	} else if (nv_strcasecmp(feature, "hsv") == 0) {
		if (push_weight < 0.0) {
			push_weight = DEFAULT_PUSH_HSV;
		}
		if (n < 0) {
			n = DEFAULT_N(k);
		}
		lmca_main<NV_LMCA_FEATURE_HSV, nv_lmca_empty_color_t>(
			mode, vq_file, k, n, margin, push_weight, learning_rate, max_epoch, filelist, flip, base_file, data_file, metric_file);
	} else if (nv_strcasecmp(feature, "vq") == 0 && mode == TRAIN) {
		argc -= nv_getopt_optind;
		argv += nv_getopt_optind;
		if (argc != 1) {
			print_usage();
			return -1;
		}
		if (vq_file[0] == '\0') {
			print_usage();
			return -1;
		}
		filelist = argv[0];
		if (n < 0) {
			n = DEFAULT_VQ_SAMPLES;
		}
		vq(n, filelist, flip, vq_file);
	} else {
		print_usage();
		return -1;
	}
	
	return 0;
}
