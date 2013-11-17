EXTRA_DIST = config

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
otama_test_variant.c

noinst_DATA = config/lmca_vlad.mat config/lmca_hsv.mat config/lmca_vladhsv.mat

config/lmca_vlad.mat:
	gzip -d -c $(srcdir)/config/lmca_vlad.mat.gz > $(builddir)/config/lmca_vlad.mat

config/lmca_hsv.mat:
	gzip -d -c $(srcdir)/config/lmca_hsv.mat.gz > $(builddir)/config/lmca_hsv.mat

config/lmca_vladhsv.mat:
	gzip -d -c $(srcdir)/config/lmca_vladhsv.mat.gz > $(builddir)/config/lmca_vladhsv.mat

check-local:
	$(MKDIR_P) ./data
	$(MKDIR_P) ./data/node1
	$(MKDIR_P) ./data/node2
	$(builddir)/otama_test$(EXEEXT)

all-local:
	$(MKDIR_P) ./data
	$(MKDIR_P) ./data/node1
	$(MKDIR_P) ./data/node2

clean-local:
	rm -fr ./data/*

