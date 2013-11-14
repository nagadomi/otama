EXTRA_DIST = 

noinst_LTLIBRARIES = libnvlmcaex.la
bin_PROGRAMS = nv_lmca_train

libnvlmcaex_la_LIBADD = 
libnvlmcaex_la_CFLAGS = -I$(srcdir) -I$(srcdir)/../nvcolorex -I$(srcdir)/../nvvlad  $(CFLAGS) -DPKGDATADIR=\""$(pkgdatadir)"\"
libnvlmcaex_la_CXXFLAGS = $(libnvlmcaex_la_CFLAGS)
libnvlmcaex_la_LDFLAGS = -no-undefined 

libnvlmcaex_la_SOURCES = nv_lmca.hpp

nv_lmca_train_SOURCES = nv_lmca_train.cpp
nv_lmca_train_CFLAGS = -I$(srcdir) -I$(srcdir)/../nvcolorex $(CFLAGS) -I$(srcdir)/../nvvlad -I$(srcdir)/../nvvlad $(CFLAGS) -DPKGDATADIR=\""$(pkgdatadir)"\"
nv_lmca_train_CXXFLAGS = $(nv_lmca_train_CFLAGS)
nv_lmca_train_LDFLAGS = -no-undefined
nv_lmca_train_LDADD = $(builddir)/libnvlmcaex.la $(builddir)/../nvvlad/libnvvlad.la $(builddir)/../nvcolorex/libnvcolorex.la

pkgdata_DATA = 
CLEANFILES = 
