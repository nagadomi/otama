EXTRA_DIST = nv_bovw_internal.h nv_bovw512k_nega.kmt.gz nv_bovw512k_posi.kmt.gz nv_bovw512k_idf.mat.gz \
nv_bovw2k_nega.kmt.gz nv_bovw2k_posi.kmt.gz nv_bovw2k_idf.mat.gz \
nv_bovw8k_nega.kmt.gz nv_bovw8k_posi.kmt.gz nv_bovw8k_idf.mat.gz

noinst_LTLIBRARIES = libnvbovw.la
noinst_PROGRAMS = nv_bovw_benchmark
bin_PROGRAMS = nv_bovw_train

libnvbovw_la_LIBADD = 
libnvbovw_la_CFLAGS = -I$(srcdir) -I$(srcdir)/../nvcolorex $(CFLAGS)  -DPKGDATADIR=\""$(pkgdatadir)"\"
libnvbovw_la_CXXFLAGS = $(libnvbovw_la_CFLAGS)
libnvbovw_la_CPPFLAGS = $(libnvbovw_la_CFLAGS)
libnvbovw_la_LDFLAGS = -no-undefined 

libnvbovw_la_SOURCES = nv_bovw.hpp

nv_bovw_benchmark_SOURCES = nv_bovw_benchmark.cpp
nv_bovw_benchmark_CFLAGS = -I$(srcdir) -I$(srcdir)/../nvcolorex  -DPKGDATADIR=\""$(pkgdatadir)"\"
nv_bovw_benchmark_CXXFLAGS = $(nv_bovw_benchmark_CFLAGS)
nv_bovw_benchmark_CPPFLAGS = $(nv_bovw_benchmark_CFLAGS)
nv_bovw_benchmark_LDFLAGS = 
nv_bovw_benchmark_LDADD = $(builddir)/libnvbovw.la $(builddir)/../nvcolorex/libnvcolorex.la

nv_bovw_train_SOURCES = nv_bovw_train.cpp
nv_bovw_train_CFLAGS = -I$(srcdir) -I$(srcdir)/../nvcolorex $(CFLAGS)  -DPKGDATADIR=\""$(pkgdatadir)"\"
nv_bovw_train_CXXFLAGS = $(nv_bovw_train_CFLAGS)
nv_bovw_train_CPPFLAGS = $(nv_bovw_train_CFLAGS)
nv_bovw_train_LDFLAGS = 
nv_bovw_train_LDADD = $(builddir)/libnvbovw.la $(builddir)/../nvcolorex/libnvcolorex.la

pkgdata_DATA = nv_bovw512k_posi.kmtb nv_bovw512k_nega.kmtb nv_bovw512k_idf.matb \
nv_bovw2k_posi.kmtb nv_bovw2k_nega.kmtb nv_bovw2k_idf.matb \
nv_bovw8k_posi.kmtb nv_bovw8k_nega.kmtb nv_bovw8k_idf.matb

nv_bovw512k_posi.kmt:
	gzip -d -c $(srcdir)/nv_bovw512k_posi.kmt.gz > $(builddir)/nv_bovw512k_posi.kmt

nv_bovw512k_nega.kmt:
	gzip -d -c $(srcdir)/nv_bovw512k_nega.kmt.gz > $(builddir)/nv_bovw512k_nega.kmt

nv_bovw512k_idf.mat:
	gzip -d -c $(srcdir)/nv_bovw512k_idf.mat.gz > $(builddir)/nv_bovw512k_idf.mat

nv_bovw512k_posi.kmtb: nv_bovw512k_posi.kmt
	nv_kmeans_tree_text2bin $(builddir)/nv_bovw512k_posi.kmt $(builddir)/nv_bovw512k_posi.kmtb

nv_bovw512k_nega.kmtb: nv_bovw512k_nega.kmt
	nv_kmeans_tree_text2bin nv_bovw512k_nega.kmt $(builddir)/nv_bovw512k_nega.kmtb

nv_bovw512k_idf.matb: nv_bovw512k_idf.mat
	nv_matrix_text2bin $(builddir)/nv_bovw512k_idf.mat $(builddir)/nv_bovw512k_idf.matb

nv_bovw8k_posi.kmt:
	gzip -d -c $(srcdir)/nv_bovw8k_posi.kmt.gz > $(builddir)/nv_bovw8k_posi.kmt

nv_bovw8k_nega.kmt:
	gzip -d -c $(srcdir)/nv_bovw8k_nega.kmt.gz > $(builddir)/nv_bovw8k_nega.kmt

nv_bovw8k_idf.mat:
	gzip -d -c $(srcdir)/nv_bovw8k_idf.mat.gz > $(builddir)/nv_bovw8k_idf.mat

nv_bovw8k_posi.kmtb: nv_bovw8k_posi.kmt
	nv_kmeans_tree_text2bin $(builddir)/nv_bovw8k_posi.kmt $(builddir)/nv_bovw8k_posi.kmtb

nv_bovw8k_nega.kmtb: nv_bovw8k_nega.kmt
	nv_kmeans_tree_text2bin $(builddir)/nv_bovw8k_nega.kmt $(builddir)/nv_bovw8k_nega.kmtb

nv_bovw8k_idf.matb: nv_bovw8k_idf.mat
	nv_matrix_text2bin $(builddir)/nv_bovw8k_idf.mat $(builddir)/nv_bovw8k_idf.matb

nv_bovw2k_posi.kmt:
	gzip -d -c $(srcdir)/nv_bovw2k_posi.kmt.gz > $(builddir)/nv_bovw2k_posi.kmt

nv_bovw2k_nega.kmt:
	gzip -d -c $(srcdir)/nv_bovw2k_nega.kmt.gz > $(builddir)/nv_bovw2k_nega.kmt

nv_bovw2k_idf.mat:
	gzip -d -c $(srcdir)/nv_bovw2k_idf.mat.gz > $(builddir)/nv_bovw2k_idf.mat

nv_bovw2k_posi.kmtb: nv_bovw2k_posi.kmt
	nv_kmeans_tree_text2bin $(builddir)/nv_bovw2k_posi.kmt $(builddir)/nv_bovw2k_posi.kmtb

nv_bovw2k_nega.kmtb: nv_bovw2k_nega.kmt
	nv_kmeans_tree_text2bin $(builddir)/nv_bovw2k_nega.kmt $(builddir)/nv_bovw2k_nega.kmtb

nv_bovw2k_idf.matb: nv_bovw2k_idf.mat
	nv_matrix_text2bin $(builddir)/nv_bovw2k_idf.mat $(builddir)/nv_bovw2k_idf.matb

CLEANFILES = *.matb *.mat *.kmt *.kmtb
