EXTRA_DIST = \
lmca_vlad.mat.gz lmca_hsv.mat.gz lmca_vladhsv.mat.gz \
config/bovw2k.yaml \
config/bovw2k_boc.yaml \
config/bovw2k_nodb.yaml \
config/bovw2k_sboc.yaml \
config/bovw2k_sboc_nodb.yaml \
config/bovw512k_iv.yaml \
config/bovw512k_iv_ldb.yaml \
config/bovw512k_iv_ldb_node1.yaml \
config/bovw512k_iv_ldb_node2.yaml \
config/bovw512k_nodb.yaml \
config/bovw512k_sboc.yaml \
config/bovw8k.yaml \
config/bovw8k_nodb.yaml \
config/bovw8k_node1.yaml \
config/bovw8k_node2.yaml \
config/color.yaml \
config/color_nodb.yaml \
config/id.yaml \
config/id_nodb.yaml \
config/lmca_hsv.yaml \
config/lmca_hsv_nodb.yaml \
config/lmca_vlad.yaml \
config/lmca_vlad_colorcode.yaml \
config/lmca_vlad_colorcode_nodb.yaml \
config/lmca_vlad_hsv.yaml \
config/lmca_vlad_hsv_nodb.yaml \
config/lmca_vlad_nodb.yaml \
config/lmca_vladhsv.yaml \
config/lmca_vladhsv_nodb.yaml \
config/sboc.yaml \
config/sboc_nodb.yaml \
config/sim.yaml \
config/sim_nodb.yaml \
config/vlad128_nodb.yaml \
config/vlad_nodb.yaml

check_PROGRAMS = otama_test
otama_test_CFLAGS = -I$(srcdir)/../models -I$(srcdir)/../lib -I$(srcdir)/../nvcolorex -I$(srcdir)/../nvbovw -I$(srcdir)/../nvvlad  \
-DPKGDATADIR=\""$(pkgdatadir)"\" \
-DOTAMA_TEST_IMG=\"$(top_srcdir)/image/lena.jpg\" \
-DOTAMA_TEST_IMG_SCALE=\"$(top_srcdir)/image/lena-768x768.jpg\" \
-DOTAMA_TEST_IMG_ROTATE=\"$(top_srcdir)/image/lena-rotate.jpg\" \
-DOTAMA_TEST_IMG_AFFINE=\"$(top_srcdir)/image/lena-affine.jpg\" \
-DOTAMA_TEST_IMG_NEGA=\"$(top_srcdir)/image/baboon.png\" \
-DOTAMA_TEST_IMG_WHITE=\"$(top_srcdir)/image/white.png\" \
-DOTAMA_TEST_CONFIG_DIR=\"$(srcdir)/config\" \
-DNV_BOVW_PKGDATADIR=\"$(builddir)/../nvbovw\"

otama_test_CXXFLAGS = $(otama_test_CFLAGS)
otama_test_LDADD = $(builddir)/../libotama.la


otama_test_SOURCES = otama_test.cpp otama_test.h \
otama_test_vlad.cpp \
otama_test_bovw.cpp \
otama_test_api.c \
otama_test_similarity_api.c \
otama_test_cluster.c \
otama_test_dbi.c \
otama_test_variant.c \
otama_test_kvs.c

lmca_vlad.mat:
	gzip -d -c $(srcdir)/lmca_vlad.mat.gz > $(builddir)/lmca_vlad.mat

lmca_hsv.mat:
	gzip -d -c $(srcdir)/lmca_hsv.mat.gz > $(builddir)/lmca_hsv.mat

lmca_vladhsv.mat:
	gzip -d -c $(srcdir)/lmca_vladhsv.mat.gz > $(builddir)/lmca_vladhsv.mat

check-local: lmca_vlad.mat lmca_hsv.mat lmca_vladhsv.mat
	$(builddir)/otama_test$(EXEEXT)

all-local:
	$(MKDIR_P) $(builddir)/data
	$(MKDIR_P) $(builddir)/data/node1
	$(MKDIR_P) $(builddir)/data/node2

clean-local:
	rm -fr $(builddir)/data/*
	rm -f $(builddir)/lmca_vladhsv.mat
	rm -f $(builddir)/lmca_hsv.mat
	rm -f $(builddir)/lmca_vlad.mat
