// g++ ukbench_vlad.cpp  -Wall -fopenmp -D_GLIBCXX_PARALLEL -O2 -o ukbench_vlad -lotama
#include "otama.h"
#include <string>
#include <vector>
#include <set>
#include <algorithm>

#define FILELIST "./filelist.txt"

int main(void)
{
	char line[8192];
	typedef std::pair<std::string, otama_feature_raw_t *> list_t;
	typedef std::pair<float, int> score_t;	
	std::vector<list_t> list;
	FILE *fp;
	int i;
	otama_t *otama;
	otama_variant_pool_t *pool;
	otama_variant_t *config, *driver;
	size_t len;
	int sum;
	int count;

	setvbuf(stdout, 0, _IONBF, 0);
	
	fp = fopen(FILELIST, "r");
	if (fp == NULL) {
		perror(FILELIST);
		return -1;
	}
	while (fgets(line, sizeof(line) - 1, fp)) {
		len = strlen(line);
		line[len-1] = '\0';
		list.push_back(std::make_pair(std::string(line), (otama_feature_raw_t *)NULL));
	}
	std::sort(list.begin(), list.end());
	fclose(fp);

	pool = otama_variant_pool_alloc();
	config = otama_variant_new(pool);
	otama_variant_set_hash(config);
	otama_variant_set_hash(driver = otama_variant_hash_at(config, "driver"));
	otama_variant_set_string(otama_variant_hash_at(driver, "name"), "vlad_nodb");
	
	if (otama_open_opt(&otama, config) != OTAMA_STATUS_OK) {
		return -1;
	}

#pragma omp parallel for schedule(dynamic, 1)
	for (i = 0; i < (int)list.size(); ++i) {
		otama_feature_raw_file(otama, &list[i].second, list[i].first.c_str());
		printf("loading %d..\r", i);
	}

	count = list.size();
	sum = 0;
	for (i = 0; i < (int)list.size(); ++i) {
		list_t query = list[i];
		int j;
		std::vector<score_t> scores;
		
		scores.resize(count);
#pragma omp parallel for schedule(dynamic, 1)
		for (j = 0; j < (int)list.size(); ++j) {
			float score;
			otama_similarity_raw(otama, &score, query.second, list[j].second);
			scores[j] = std::make_pair(score, j);
		}
		std::sort(scores.begin(), scores.end(), std::greater<score_t>());
		for (j = 0; j < 4; ++j) {
			if (i / 4 == scores[j].second / 4) {
				++sum;
			}
		}
		printf("%5d (%.2f)\r", i, (float)sum / i);
	}
	printf("%5d (%.2f)\n", i, (float)sum / count);

	for (i = 0; i < (int)list.size(); ++i) {
		otama_feature_raw_free(&list[i].second);
	}
	otama_close(&otama);
	
	return 0;
}
