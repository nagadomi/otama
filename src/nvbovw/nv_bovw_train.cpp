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
#include "nv_bovw.hpp"
#include "nv_color_boc.h"
#include <vector>
#include <string>
#include <map>
#include <algorithm>

#define KEYPOINT_D          72
#define KCLASS              1024
#define IMAGE_SIZE          512.0f
#define KMEANS_STEP         60
static int s_keypoint_max = 1800;

typedef nv_bovw_ctx<NV_BOVW_BIT512K, nv_bovw_dummy_color_t> bovw;
//typedef nv_bovw_ctx<NV_BOVW_BIT8K, nv_bovw_dummy_color_t> bovw;
static int s_keypoint_maxs[] = {
	1800,
	1024,
	3000
};
static nv_keypoint_param_t s_param[] = {
	{
		NV_KEYPOINT_THRESH,
		NV_KEYPOINT_EDGE_THRESH,
		NV_KEYPOINT_MIN_R,
		NV_KEYPOINT_LEVEL,
		NV_KEYPOINT_NN,
		NV_KEYPOINT_DETECTOR_STAR,
		NV_KEYPOINT_DESCRIPTOR_GRADIENT_HISTOGRAM
	},
	{
		16.0f,
		NV_KEYPOINT_EDGE_THRESH,
		4.0,
		15,
		0.8f,
		NV_KEYPOINT_DETECTOR_STAR,
		NV_KEYPOINT_DESCRIPTOR_GRADIENT_HISTOGRAM
	},
	{
		16.0f,
		NV_KEYPOINT_EDGE_THRESH,
		4.0f,
		NV_KEYPOINT_LEVEL,
		0.3f,
		NV_KEYPOINT_DETECTOR_STAR,
		NV_KEYPOINT_DESCRIPTOR_GRADIENT_HISTOGRAM
	}
};

static nv_keypoint_ctx_t *s_ctx = NULL;

static
void extract_keypoints(std::vector<std::string> &files,
					   nv_matrix_t *data, nv_matrix_t *labels,
					   int sign, int dm)
{
	int m;
	int data_m = 0;
	
	nv_matrix_zero(data);

#ifdef _OPENMP
#pragma omp parallel for schedule(dynamic, 1)
#endif
	for (m = 0; m < (int)files.size(); ++m) {
		int i;
		int desc_m;
		int rand_m = nv_rand_index(files.size());
		
		if (data_m < dm) {
			nv_matrix_t *image = nv_load(files[rand_m].c_str());
			if (image == NULL) {
				printf("cant load %s\n", files[rand_m].c_str());
				continue;
			}
			
			nv_matrix_t *desc_vec = nv_matrix_alloc(NV_KEYPOINT_DESC_N, s_keypoint_max);
			nv_matrix_t *key_vec = nv_matrix_alloc(NV_KEYPOINT_KEYPOINT_N, desc_vec->m);
			
			float scale = IMAGE_SIZE / (float)NV_MAX(image->rows, image->cols);
			nv_matrix_t *gray = nv_matrix3d_alloc(1, image->rows, image->cols);
			nv_matrix_t *resize = nv_matrix3d_alloc(1, (int)(image->rows * scale),
											(int)(image->cols * scale));
			nv_matrix_t *smooth = nv_matrix3d_alloc(1, resize->rows, resize->cols);
	
			nv_gray(gray, image);
			nv_resize(resize, gray);
			nv_gaussian5x5(smooth, 0, resize, 0);

			desc_m = 0;
			nv_matrix_zero(desc_vec);

			desc_m = nv_keypoint_ex(s_ctx, key_vec, desc_vec, smooth, 0);

#ifdef _OPENMP
#pragma omp critical (nv_bovw_extract_keypoints)
#endif
			{
				int l;
				int samples = NV_MIN(desc_m, (int)((dm / (int)files.size()) * 1.5));
				int *r_idx = (int *)nv_malloc(sizeof(int) * desc_m);
				
				nv_shuffle_index(r_idx, 0, desc_m);
				
				for (l = 0; l < samples; ++l) {
					i = r_idx[l];
					if (data_m < dm) {
						if (NV_SIGN(sign) > 0) {
							if (NV_MAT_V(key_vec, i, NV_KEYPOINT_RESPONSE_IDX) > 0.0f) {
								nv_vector_copy(data, data_m, desc_vec, i);
								//nv_vector_normalize(data, data_m);
								NV_MAT_V(labels, data_m, 0) = (float)rand_m;
								++data_m;
							}
						} else {
							if (NV_MAT_V(key_vec, i, NV_KEYPOINT_RESPONSE_IDX) < 0.0f) {
								nv_vector_copy(data, data_m, desc_vec, i);
								//nv_vector_normalize(data, data_m);								
								NV_MAT_V(labels, data_m, 0) = (float)rand_m;
								++data_m;
							}
						}
					}
				}
				nv_free(r_idx);
			}
			nv_matrix_free(&desc_vec);
			nv_matrix_free(&key_vec);
			nv_matrix_free(&resize);
			nv_matrix_free(&gray);
			nv_matrix_free(&smooth);
			nv_matrix_free(&image);
		}
	}
	nv_matrix_m(data, data_m);
	nv_matrix_m(labels, data_m);	
}

static int
read_filelist(const char *file, std::vector<std::string> &list)
{
	FILE *fp = fopen(file, "r");
	char line[8192];
	size_t len;
	
	if (fp == NULL) {
		printf("%s: %s\n",file, strerror(errno));
		return -1;
	}
	while (fgets(line, sizeof(line) - 1, fp)) {
		len = strlen(line);
		line[len-1] = '\0';
		list.push_back(std::string(line));
	}
	list.size();
	fclose(fp);

	return 0;
}

static
int extract(int sign, int dm)
{
	nv_matrix_t *data = nv_matrix_alloc(KEYPOINT_D,  dm);
	nv_matrix_t *labels = nv_matrix_alloc(1,  dm);
	std::vector<std::string> list;
	
	printf("load file\n");
	if (data != NULL) {
		printf("data alloc ok\n");
	} else {
		fprintf(stderr, "data alloc ng\n");
		return -1;
	}
	if (labels != NULL) {
		printf("labels alloc ok\n");
	}else {
		fprintf(stderr, "labels alloc ng\n");		
		return -1;
	}

	if (read_filelist("filelist.txt", list) != 0) {
		return -1;
	}
	nv_matrix_zero(data);
	nv_matrix_zero(labels);	
	
	extract_keypoints(list, data, labels, sign, dm);

	if (NV_SIGN(sign) > 0) {
		nv_save_matrix_bin("keypoints_posi.vec", data);
		nv_save_matrix_bin("keypoints_posi_labels.vec", labels);
	} else {
		nv_save_matrix_bin("keypoints_nega.vec", data);
		nv_save_matrix_bin("keypoints_nega_labels.vec", labels);
	}
	
	nv_matrix_free(&labels);
	nv_matrix_free(&data);

	return 0;
}

// 8192/65536用
static int
kmeans_tree_clustering(const char *keypoint_file,
					   const char *tree_file,
					   const char *base_file,
					   const int *width,
					   int width_len
	)
{
	char fname[256];
	nv_matrix_t *data = nv_load_matrix_bin(keypoint_file);
	nv_kmeans_tree_t *tree;
	int i;
	nv_matrix_t *count;

	printf("width =");
	for (i = 0; i < width_len; ++i) {
		printf("%d,", width[i]);
	}
	printf("\n");
	fflush(stdout);
	
	if (data == NULL) {
		fprintf(stderr, "%s: %s\n", strerror(errno), keypoint_file);
		return -1;
	}
	nv_vector_normalize_all(data);
	printf("data: %d, %d\n", data->n, data->m);

	nv_kmeans_progress(1);
	nv_kmeans_tree_progress(1);
	tree = nv_kmeans_tree_alloc(data->n, width, width_len);
	count = nv_matrix_alloc(1, tree->m);

	if (base_file) {
		nv_kmeans_tree_t *base_tree = nv_load_kmeans_tree_text(base_file);
		nv_kmeans_tree_inherit_train(tree, base_tree, data, KMEANS_STEP);
		nv_kmeans_tree_free(&base_tree);
	} else {
		nv_kmeans_tree_train(tree, data, KMEANS_STEP);
	}
	nv_save_kmeans_tree_text(tree_file, tree);
	
	printf("count: data: %d\n", data->m);
	nv_matrix_zero(count);
	for (i = 0; i < data->m; ++i) {
		int label = nv_kmeans_tree_predict_label(tree, data, i);
		if (label < 0 || label >= count->m) {
			fprintf(stderr, "assert!!!: %d\n", label);
		}
		NV_MAT_V(count, label, 0) += 1.0f;
	}
	sprintf(fname, "count_%s.vec", tree_file);
	nv_save_matrix(fname, count);

	nv_matrix_free(&data);
	nv_matrix_free(&count);	
	
	return 0;
}

static int
idf(const char *posi_count_file,
		 const char *nega_count_file,
		 const char *idf_file
)
{
	nv_matrix_t *posi_count = nv_load_matrix(posi_count_file);
	nv_matrix_t *nega_count = nv_load_matrix(nega_count_file);	
	nv_matrix_t *idf = NULL;
	float count;
	int i;
	int ret = 0;
	
	if (posi_count == NULL) {
		fprintf(stderr, "cannot read %s\n", posi_count_file);
		ret = - 1;
		goto exit_;
	}
	if (posi_count == NULL) {
		fprintf(stderr, "cannot read %s\n", nega_count_file);
		ret = -1;
		goto exit_;		
	}
	
	idf = nv_matrix_alloc(nega_count->m + posi_count->m, 1);
	count = 0.0f;
	for (i = 0; i < posi_count->m; ++i) {
		count += NV_MAT_V(posi_count, i, 0);
	}
	for (i = 0; i < nega_count->m; ++i) {
		count += NV_MAT_V(nega_count, i, 0);
	}
	printf("%d(posi:%d, nega:%d) class, data: %f\n",
		   (int)idf->m, (int)posi_count->m, (int)nega_count->m, count);
	
	for (i = 0; i < posi_count->m; ++i) {
		NV_MAT_V(idf, 0, i) = logf((count + 0.5f) / (NV_MAT_V(posi_count, i, 0) + 0.5f)) / logf(2.0) + 1.0f;
	}
	for (i = 0; i < nega_count->m; ++i) {
		NV_MAT_V(idf, 0, posi_count->m + i) = logf((count + 0.5f) / (NV_MAT_V(nega_count, i, 0) + 0.5f)) / logf(2.0f) + 1.0f;
	}
	nv_save_matrix(idf_file, idf);

exit_:
	nv_matrix_free(&posi_count);
	nv_matrix_free(&nega_count);
	nv_matrix_free(&idf);
	
	return ret;
}

static
int dist(const char *kmt_posi_file,
		 const char *kmt_nega_file,
		 const char *kp_posi_file,
		 const char *kp_nega_file,
		 const char *dist_file)
{
	nv_kmeans_tree_t *kmt_posi = nv_load_kmeans_tree_text(kmt_posi_file);
	nv_kmeans_tree_t *kmt_nega = nv_load_kmeans_tree_text(kmt_nega_file);
	nv_matrix_t *dist = nv_matrix_alloc(2, kmt_posi->m + kmt_nega->m);
	nv_matrix_t *kp;
	int i;
	
	for (i = 0; i < dist->m; ++i) {
		NV_MAT_V(dist, i, 0) = FLT_MAX;
		NV_MAT_V(dist, i, 1) = -FLT_MAX;
	}
	
	kp = nv_load_matrix_bin(kp_posi_file);
	if (kp == NULL) {
		return -1;
	}
	nv_vector_normalize_all(kp);	
	for (i = 0; i < kp->m; ++i) {
		nv_int_float_t l = nv_kmeans_tree_predict_label_and_dist(kmt_posi, kp, i);
		if (NV_MAT_V(dist, l.i, 0) > l.f) {
			NV_MAT_V(dist, l.i, 0) = l.f;
		}
		if (NV_MAT_V(dist, l.i, 1) < l.f) {
			NV_MAT_V(dist, l.i, 1) = l.f;
		}
	}
	nv_matrix_free(&kp);
	
	kp = nv_load_matrix_bin(kp_nega_file);
	if (kp == NULL) {
		return -1;
	}
	nv_vector_normalize_all(kp);
	for (i = 0; i < kp->m; ++i) {
		nv_int_float_t l = nv_kmeans_tree_predict_label_and_dist(kmt_nega, kp, i);
		if (NV_MAT_V(dist, kmt_posi->m + l.i, 0) > l.f) {
			NV_MAT_V(dist, kmt_posi->m + l.i, 0) = l.f;
		}
		if (NV_MAT_V(dist, kmt_posi->m + l.i, 1) < l.f) {
			NV_MAT_V(dist, kmt_posi->m + l.i, 1) = l.f;
		}
	}
	nv_matrix_free(&kp);

	nv_kmeans_tree_free(&kmt_posi);
	nv_kmeans_tree_free(&kmt_nega);

	nv_save_matrix(dist_file, dist);

	return 0;
}

#if 0 // ランキング学習。精度はよくなるけど学習が必要なのでやめた。
      // FIXME: 使ってない間にnv_bovwのインターフェースが変わったのでコンパイルできない
#define NN_INDEX 20
#define MAX_DATA 40000
static int
make_pairwise_data(const char *base, int n)
{
	int i, j;
	char fname[2048];
	nv_matrix_t *labels = nv_matrix_alloc(1, MAX_DATA);
	nv_matrix_t *features = nv_matrix_alloc(bovw::NV_BOVW_RANK_DIM, MAX_DATA * NN_INDEX * NN_INDEX/2);
	nv_matrix_t *feature_labels = nv_matrix_alloc(1, features->m);
	
	int data_j = 0;
	bovw::nv_bovw_t *bovws = nv_alloc_type(bovw::nv_bovw_t, MAX_DATA);
	
	nv_matrix_zero(features);
	nv_matrix_zero(feature_labels);	
	nv_matrix_zero(labels);
	
	for (i = 0; i < n; ++i) {
		std::vector<std::string> list;
		bovw::nv_bovw_t *bovws_local;
		
		sprintf(fname, "%s/filelist_%d.txt", base, i);
		if (read_filelist(fname, list) != 0) {
			return -1;
		}

		bovws_local = nv_alloc_type(bovw::nv_bovw_t, list.size());
		
		printf("%s..\n", fname);

#ifdef _OPENMP
#pragma omp parallel for
#endif
		for (j = 0; j < (int)list.size(); ++j) {
			if (bovw::nv_bovw_file(&bovws_local[j], list[j].c_str()) != 0) {
				fprintf(stderr, "%s:%s\n", list[j].c_str(), strerror(errno));
			}
		}
		for (j = 0; j < (int)list.size(); ++j) {
			memcpy(&bovws[data_j], &bovws_local[j], sizeof(bovw::nv_bovw_t));

			NV_MAT_V(labels, data_j, 0) = (float)i;
			++data_j;
			if (data_j >= MAX_DATA) {
				fprintf(stderr, "data too big");
				return -1;
			}
		}
		nv_free(bovws_local);
	}
	nv_matrix_m(labels, data_j);

	// pairwise
	data_j = 0;
#ifdef _OPENMP
#pragma omp parallel for
#endif
	for (j = 0; j < labels->m; ++j) {
		int k;
		int j_label = (int)NV_MAT_V(labels, j, 0);
		nv_bovw_result_t index[NN_INDEX];
		nv_matrix_t *k_j_vec = nv_matrix_alloc(bovw::NV_BOVW_RANK_DIM, 1);
		nv_matrix_t *l_j_vec = nv_matrix_alloc(bovw::NV_BOVW_RANK_DIM, 1);
		nv_matrix_t *idf = nv_matrix_alloc(bovw::NV_BOVW_BIT, 1);
		int ok_c = 0;
		
		bovw::nv_bovw_idf(idf, 0, &bovws[j]);

		// 特徴ベクトルでのKNN
		bovw::nv_bovw_search(index, NN_INDEX, bovws, labels->m, &bovws[j],
							 NV_BOVW_RERANK_IDF, 0.2f);

		for (k = 0; k < NN_INDEX; ++k) {
			if ((int)NV_MAT_V(labels, (int)index[k].index, 0) == j_label) {
				++ok_c;
			}
		}
		
		// pairwise作成
		for (k = 0; k < NN_INDEX; ++k) {
			int l;
			int k_j = (int)index[k].index;
			int k_label = (int)NV_MAT_V(labels, k_j, 0);
				
			for (l = k + 1; l < NN_INDEX; ++l) {
				int l_j = (int)index[l].index;
				int l_label = (int)NV_MAT_V(labels, l_j, 0);

				if ((j_label == k_label  && j_label == l_label)) {
					continue;
				} else if (j_label != k_label && j_label != l_label) {
					continue;
				} else if ((j_label == k_label && j_label != l_label)) {
#ifdef _OPENMP
#pragma omp critical (nv_bovw_make_pairwise_data)
#endif
					{
						bovw::nv_bovw_rank_feature(k_j_vec, 0, &bovws[j], idf, 0, &bovws[k_j]);
						bovw::nv_bovw_rank_feature(l_j_vec, 0, &bovws[j], idf, 0, &bovws[l_j]);
						
						nv_vector_copy(features, data_j, k_j_vec, 0);
						nv_vector_sub(features, data_j, features, data_j, l_j_vec, 0);
						NV_MAT_V(feature_labels, data_j, 0) = 0.0f;
						++data_j;
						if (data_j >= features->m) {
							fprintf(stderr, "overflow! %d %d\n", data_j, features->m);
							exit(-1);
						}
						
#if 0
						if (p == 0) {
							printf("label: %d, %f(ok:%d,ng:%d)\n",
								   j_label, (float)ok_c/NN_INDEX, ok_c, NN_INDEX-ok_c);
							p = 1;
						}
						nv_vector_copy(features, data_j, l_j_vec, 0);
						nv_vector_sub(features, data_j, features, data_j, k_j_vec, 0);
						NV_MAT_V(feature_labels, data_j, 0) = 1.0f;
						++data_j;
						if (data_j >= features->m) {
							fprintf(stderr, "overflow! %d %d\n", data_j, features->m);
							exit(-1);
						}
#endif						
					}
				} else  {
#ifdef _OPENMP
#pragma omp critical (nv_bovw_make_pairwise_data)
#endif
					{
						//(j_label != k_label
						//	   && j_label == l_label)
						bovw::nv_bovw_rank_feature(k_j_vec, 0, &bovws[j], idf, 0, &bovws[k_j]);
						bovw::nv_bovw_rank_feature(l_j_vec, 0, &bovws[j], idf, 0, &bovws[l_j]);
						
						nv_vector_copy(features, data_j, k_j_vec, 0);
						nv_vector_sub(features, data_j, features, data_j, l_j_vec, 0);
						NV_MAT_V(feature_labels, data_j, 0) = 1.0f;
						++data_j;
						if (data_j >= features->m) {
							fprintf(stderr, "overflow! %d %d\n", data_j, features->m);
							exit(-1);
						}
#if 0
						nv_vector_copy(features, data_j, l_j_vec, 0);
						nv_vector_sub(features, data_j, features, data_j, k_j_vec, 0);
						NV_MAT_V(feature_labels, data_j, 0) = 0.0f;
						++data_j;
						if (data_j >= features->m) {
							fprintf(stderr, "overflow! %d %d\n", data_j, features->m);
							exit(-1);
						}
#endif						
					}
				}
			}
		}
		nv_matrix_free(&k_j_vec);
		nv_matrix_free(&l_j_vec);
		nv_matrix_free(&idf);
	}
	nv_matrix_m(features, data_j);
	nv_matrix_m(feature_labels, data_j);
	nv_save_matrix("pairwise_data.vec", features);
	nv_save_matrix("pairwise_labels.vec", feature_labels);

	nv_free(bovws);
	nv_matrix_free(&labels);
	nv_matrix_free(&features);
	nv_matrix_free(&feature_labels);
	
	return 0;
}

static int
train_pairwise(void)
{
	nv_matrix_t *data = nv_load_matrix("pairwise_data.vec");
	nv_matrix_t *labels = nv_load_matrix("pairwise_labels.vec");
	nv_lr_t *lr;
	FILE *fp;
	
	if (data == NULL || labels == NULL) {
		fprintf(stderr, "cand read data or labels\n");
		return -1;
	}
	
	nv_lr_progress(2);
	lr = nv_lr_alloc(data->n, 2);
	
	nv_lr_init(lr, data);
	nv_lr_train(lr, data, labels, NV_LR_PARAM(400, 0.001f,
											  NV_LR_REG_L1, 0.00001f));
	
	fp = fopen("nv_bovw_pairwise_static.c", "w");
	fputs("#include \"nv_core.h\"\n", fp);
	fputs("#include \"nv_ml.h\"\n", fp);
	nv_lr_dump_c(fp, lr, "nv_bovw_lr_pairwise", 0);
	fclose(fp);
	
	nv_lr_free(&lr);
	nv_matrix_free(&data);
	nv_matrix_free(&labels);
	
	return 0;
}

int train_pairwise_mlp(void)
{
	nv_matrix_t *data = nv_load_matrix("pairwise_data.vec");
	nv_matrix_t *labels = nv_load_matrix("pairwise_labels.vec");
	nv_mlp_t *mlp;
	FILE *fp;
	nv_matrix_t *ir = nv_matrix_alloc(1, 2);
	nv_matrix_t *hr = nv_matrix_alloc(1, 2);
	int i;
	
	if (data == NULL || labels == NULL) {
		fprintf(stderr, "cand read data or labels\n");
		return -1;
	}
	for (i = 0; i < labels->m; ++i) {
		if (NV_MAT_V(labels, i, 0) == 0.0f) {
			NV_MAT_V(labels, i, 0) = 1.0f;
		} else {
			NV_MAT_V(labels, i, 0) = -1.0f;
		}
	}
	
	NV_MAT_V(ir, 0, 0) = 0.1f;
	NV_MAT_V(ir, 1, 0) = 0.1f * 5.0f;
	NV_MAT_V(hr, 0, 0) = 0.1f;
	NV_MAT_V(hr, 1, 0) = 0.1f * 5.0f;
	
	nv_mlp_progress(1);
	mlp = nv_mlp_alloc(data->n, 1, 1);
	
	nv_mlp_init(mlp);
	for (i = 0; i < 2000; i += 5) {
		nv_mlp_train_ex(mlp, data, labels, ir, hr, 0, i, i + 5, 2000);
		
		fp = fopen("nv_bovw_pairwise_mlp_static.c", "w");
		fputs("#include \"nv_core.h\"\n", fp);
		fputs("#include \"nv_ml.h\"\n", fp);	
		nv_mlp_dump_c(fp, mlp, "nv_bovw_mlp_pairwise", 0);
		fclose(fp);
	}

	nv_mlp_free(&mlp);
	nv_matrix_free(&data);
	nv_matrix_free(&labels);
	
	return 0;
}
#endif

static int
vq(nv_matrix_t *data, const char *mat_name, int k)
{
	nv_matrix_t *centroid;
	nv_matrix_t *labels;
	nv_matrix_t *count;
	FILE *fp;
	char fname[256];
	
	centroid = nv_matrix_alloc(data->n, k);
	labels = nv_matrix_alloc(1, data->m);
	count = nv_matrix_alloc(1, k);

	nv_matrix_zero(centroid);
	nv_matrix_zero(count);
	nv_matrix_zero(labels);

	nv_kmeans_progress(1);
	nv_kmeans(centroid, count, labels, data, k, KMEANS_STEP);

	sprintf(fname, "%s.c", mat_name);
	fp = fopen(fname, "w");
	fputs("#include \"nv_core.h\"\n", fp);
	fputs("#include \"nv_ml.h\"\n", fp);
	nv_matrix_dump_c(fp, centroid, mat_name, 0);
	fclose(fp);

	nv_matrix_free(&centroid);
	nv_matrix_free(&labels);
	nv_matrix_free(&count);

	return 0;
}

static int
kmeans_clustering(const char *keypoint_file, const char *mat_file, int k)
{
	char fname[256];
	nv_matrix_t *data = nv_load_matrix_bin(keypoint_file);
	int i;
	nv_matrix_t *centroid;
	nv_matrix_t *labels;
	nv_matrix_t *count;

	printf("K = %d\n", k);
	fflush(stdout);
	
	if (data == NULL) {
		fprintf(stderr, "%s: %s\n", strerror(errno), keypoint_file);
		return -1;
	}
	nv_vector_normalize_all(data);
	printf("data: %d, %d\n", data->n, data->m);

	nv_kmeans_progress(1);
	
	centroid = nv_matrix_alloc(data->n, k);
	labels = nv_matrix_alloc(1, data->m);
	count = nv_matrix_alloc(1, k);

	nv_matrix_zero(centroid);
	nv_matrix_zero(labels);
	nv_matrix_zero(count);
	
	nv_kmeans(centroid, count, labels, data, k, KMEANS_STEP);
	nv_save_matrix_text(mat_file, centroid);
	
	nv_matrix_zero(count);
	for (i = 0; i < data->m; ++i) {
		int label = nv_nn(centroid, data, i);
		NV_MAT_V(count, label, 0) += 1.0f;
	}
	sprintf(fname, "count_%s.vec", mat_file);
	nv_save_matrix(fname, count);
	printf("count: data: %d\n", data->m);

	nv_matrix_free(&data);
	nv_matrix_free(&count);
	nv_matrix_free(&centroid);
	nv_matrix_free(&labels);

	return 0;
}

#define COLOR_VQ_CLASS 64

int color_vq(void)
{
	nv_matrix_t *data = nv_matrix_alloc(3, 5160 * 128 * 128);
	nv_matrix_t *data_hsv = nv_matrix_alloc(3, 5160 * 128 * 128);
	std::vector<std::string> list;
	unsigned int i;
	int data_j = 0;
	
	if (read_filelist("filelist.txt", list) != 0) {
		return -1;
	}
	
	for (i = 0; i < list.size(); ++i) {
		nv_matrix_t *image = nv_load_image(list[i].c_str());
		if (image) {
			const float step = 4.0f;
			float cell_width = NV_MAX(((float)image->cols
									   / IMAGE_SIZE * step), 1.0);
			float cell_height = NV_MAX(((float)image->rows
										/ IMAGE_SIZE * step), 1.0);
			int sample_rows = NV_FLOOR_INT(image->rows / cell_height);
			int sample_cols = NV_FLOOR_INT(image->cols / cell_width);
			int y, x;
			
			for (y = 0; y < sample_rows - 1; ++y) {
				int yi = (int)(y * cell_height);
				for (x = 0; x < sample_cols - 1; ++x) {
					int j = NV_MAT_M(image, yi, (int)(x * cell_width));
					{
						nv_vector_copy(data, data_j, image, j);
						++data_j;
						if (data_j >= data->m) {
							goto end_;
						}
					}
				}
			}
			nv_matrix_free(&image);
			printf("%d\r", i);
		}
	}
end_:
	printf("\n");
	nv_matrix_m(data, data_j);
	nv_matrix_m(data_hsv, data_j);

	//vq(data, "nv_bovw_color_static", COLOR_VQ_CLASS);
	
	nv_color_bgr2hsv(data_hsv, data);
	vq(data_hsv, "nv_bovw_color_hsv_static", COLOR_VQ_CLASS);

	nv_matrix_free(&data);
	nv_matrix_free(&data_hsv);

	return 0;
}

static void
help(void)
{
	puts("nv_bovw_train [OPTIONS]\n"
		 "    -n           data size.\n"
		 "    -t 0|1|2     keypoint type.(0=512k,1=2k,8k,2=VLAD)"
		 "    -w 1024,256  kmeans(tree) nodes"
		 "    -e [1|-1]    1.1. extract keypoint from filelist.txt.\n"
		 "    -k [1|-1]    1.2. VQ(kmeans-tree).\n"
		 "    -i           1.3. IDF.\n"
		 "    -e [1|-1]    2.1. extract keypoint from filelist.txt.\n"
		 "    -v [1|-1]    2.2. VQ(kmeans).\n"
		 "    -r           2.3. IDF.\n"
		);
}

int
main(int argc, char** argv)
{
	typedef enum {
		UNKNOWN,
		EXTRACT,
		KMEANS_VQ,
		KMEANS_IDF,
		TREE_VQ,
		TREE_IDF,
		TREE_DIST,
		MAKE_NN_DATA,
		TRAIN_PAIRWISE,
		COLOR_VQ
	} mode_t;
	
	int opt;
	int sign = 1;
	mode_t mode = UNKNOWN;
	int ret = 0;
	char base[1024] = {0};
	int *width = nv_alloc_type(int, 10);
	int width_len = 1;
	int dm = 2000 * 10000;
	int param_i = 0;

	width[0] = 1024;
	
	while ((opt = nv_getopt(argc, argv, "e:k:im:n:pcw:b:hdv:rt:")) != -1) {	
		switch (opt) {
		case 't':
			param_i = atoi(nv_getopt_optarg);
			break;
		case 'd':
			mode = TREE_DIST;
			break;
		case 'b':
			strncpy(base, nv_getopt_optarg, sizeof(base)-1);
			break;
		case 'e':
			mode = EXTRACT;
			sign = atoi(nv_getopt_optarg);
			break;
		case 'k':
			mode = TREE_VQ;
			sign = atoi(nv_getopt_optarg);
			break;
		case 'v':
			mode = KMEANS_VQ;
			sign = atoi(nv_getopt_optarg);
			break;
		case 'i':
			mode = TREE_IDF;
			break;
		case 'r':
			mode = KMEANS_IDF;
			break;
		case 'm':
			mode = MAKE_NN_DATA;
			strncpy(base, nv_getopt_optarg, sizeof(base)-1);
			break;
		case 'n':
			dm = atoi(nv_getopt_optarg);
			break;
		case 'p':
			mode =TRAIN_PAIRWISE;
			break;
		case 'c':
			mode = COLOR_VQ;
			break;
		case 'w': {
			int i = 0;
			const char *p = nv_getopt_optarg;
			char *errp;

			while (*p) {
				if (*p ==',') {
					++p;
					continue;
				}
				errp = NULL;
				width[i] = (int)strtol(p, &errp, 10);
				if (errp != p) {
					++i;
				}
				p = errp;
			}
			width_len = i;
		}
			break;
		case 'h':
		default:
			help();
			return 0;
		}
	}
	s_keypoint_max = s_keypoint_maxs[param_i];
	s_ctx = nv_keypoint_ctx_alloc(&s_param[param_i]);
	
	switch (mode) {
	case EXTRACT:
		ret = extract(sign, dm);
		break;
#if 0		
	case MAKE_NN_DATA:
		if (n == 0) {
			fprintf(stderr, "-n is zero\n");
		} else {
			ret = make_pairwise_data(base, n);
		}
		break;
	case TRAIN_PAIRWISE:
		ret = train_pairwise();
		break;
#endif		
	case COLOR_VQ:
		ret = color_vq();
		break;
	case TREE_VQ:
		if (NV_SIGN(sign) > 0) {
			ret = kmeans_tree_clustering("keypoints_posi.vec",
										 "nv_bovw_posi.kmt",
										 *base == '\0' ? NULL : base,
										 width,
										 width_len
				);
		} else {
			ret = kmeans_tree_clustering("keypoints_nega.vec",
										 "nv_bovw_nega.kmt",
										 *base == '\0' ? NULL : base,
										 width,
										 width_len
				);
		}
		break;
	case TREE_IDF:
		ret = idf("count_nv_bovw_posi.kmt.vec",
				  "count_nv_bovw_nega.kmt.vec",
				  "nv_bovw_idf.mat");
		break;
	case TREE_DIST:
		ret = dist("nv_bovw_posi.kmt",
				   "nv_bovw_nega.kmt",
				   "keypoints_posi.vec",
				   "keypoints_nega.vec",
				   "nv_bovw_dist.mat");
		break;
	case KMEANS_VQ:
		if (NV_SIGN(sign) > 0) {
			ret = kmeans_clustering("keypoints_posi.vec",
									"nv_vlad_posi.mat",
									width[0]);
		} else {
			ret = kmeans_clustering("keypoints_nega.vec",
									"nv_vlad_nega.mat",
									width[0]);
		}
		break;
	case KMEANS_IDF:
		ret = idf("count_nv_vlad_posi.mat.vec",
				  "count_nv_vlad_nega.mat.vec",
				  "nv_vlad_idf.mat");
		break;
	default:
		help();
		return 0;
	}
	
	
	return ret;
}

#if __GNUC__
__attribute__((destructor))
static void destroy(void)
{
	nv_keypoint_ctx_free(&s_ctx);
}
#endif
