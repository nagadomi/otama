EXTRA_DIST = lib/posix/otama_mmap.c lib/windows/otama_mmap.c lib/otama_config_msvc.h util/otama_lmca_train  libotama-0.def

SUBDIRS    = nvcolorex nvbovw nvvlad nvlmcaex

# libotama
lib_LTLIBRARIES = libotama.la
libotama_la_LIBADD = $(builddir)/nvcolorex/libnvcolorex.la $(builddir)/nvbovw/libnvbovw.la $(builddir)/nvvlad/libnvvlad.la $(builddir)/nvlmcaex/libnvlmcaex.la
libotama_la_CFLAGS = -I$(srcdir)/models -I$(srcdir)/lib -I$(srcdir)/nvcolorex -I$(srcdir)/nvbovw -I$(srcdir)/nvlmcaex -I$(srcdir)/nvvlad -DPKGDATADIR=\""$(pkgdatadir)"\"
libotama_la_CXXFLAGS = $(libotama_la_CFLAGS)
libotama_la_LDFLAGS = -no-undefined -rpath "$(libdir)" -version-number @OTAMA_MAJOR@:@OTAMA_MINOR@:@OTAMA_REVISION@

libotama_la_SOURCES = \
lib/otama_internal.h \
lib/otama_util.c \
lib/otama_id.c \
lib/otama_log.c \
lib/otama_mmap.c \
lib/otama_status.c \
lib/otama_variant.cpp \
lib/otama_result.c \
lib/otama_yaml.c \
lib/otama_image.c \
lib/otama_dbi.cpp \
lib/otama_dbi_pgsql.c \
lib/otama_dbi_sqlite3.c \
lib/otama_dbi_mysql.c \
lib/otama.cpp \
lib/otama.h \
lib/otama_feature_raw.h \
lib/otama_util.h \
lib/otama_config.h \
lib/otama_config_auto.h \
lib/otama_id.h \
lib/otama_log.h \
lib/otama_portable.h \
lib/otama_mmap.h \
lib/otama_status.h \
lib/otama_variant.h \
lib/otama_result.h \
lib/otama_yaml.h \
lib/otama_image.h \
lib/otama_dbi.h \
lib/otama_dbi_internal.h \
lib/otama_image_internal.h \
models/otama_inverted_index_map.cpp \
models/otama_inverted_index_bucket.cpp \
models/otama_driver_factory.cpp \
models/otama_variable_byte_code_vector.hpp \
models/otama_kyotocabinet.hpp \
models/otama_leveldb.hpp \
models/otama_fixed_strage.hpp \
models/otama_inverted_index.hpp \
models/otama_inverted_index_kc.hpp \
models/otama_inverted_index_leveldb.hpp \
models/otama_inverted_index_kvs.hpp \
models/otama_inverted_index_map.hpp \
models/otama_inverted_index_bucket.hpp \
models/otama_omp_lock.hpp \
models/otama_driver.hpp \
models/otama_dbi_driver.hpp \
models/otama_nodb_driver.hpp \
models/otama_fixed_driver.hpp \
models/otama_inverted_index_driver.hpp \
models/otama_bovw_fixed_driver.hpp \
models/otama_lmca_fixed_driver.hpp \
models/otama_bovw_inverted_index_driver.hpp \
models/otama_bovw_nodb_driver.hpp \
models/otama_sboc_nodb_driver.hpp \
models/otama_sboc_fixed_driver.hpp \
models/otama_vlad_nodb_driver.hpp \
models/otama_driver_interface.hpp \
models/otama_driver_factory.hpp 

include_HEADERS = \
lib/otama_config.h \
lib/otama_config_auto.h \
lib/otama_portable.h \
lib/otama_feature_raw.h \
lib/otama.h \
lib/otama_id.h \
lib/otama_log.h \
lib/otama_status.h \
lib/otama_util.h \
lib/otama_variant.h \
lib/otama_result.h \
lib/otama_yaml.h \
lib/otama_dbi.h \
lib/otama_image.h

pkgconfigdir = @pkgconfigdir@
pkgconfig_DATA = otama.pc

CLEANFILES = otama.pc

# util
bin_PROGRAMS = otama_pull otama_import otama_create_database otama_drop_database otama_drop_index otama_search otama_vacuum_index
bin_SCRIPTS = util/otama_lmca_train

otama_pull_CFLAGS = -I$(srcdir)/models -I$(srcdir)/lib -I$(srcdir)/nvcolorex -I$(srcdir)/nvbovw -I$(srcdir)/nvlmcaex -I$(srcdir)/nvvlad -DPKGDATADIR=\""$(pkgdatadir)"\"
otama_pull_CXXFLAGS = $(otama_pull_CFLAGS)
otama_pull_LDFLAGS = 
otama_pull_SOURCES = util/otama_pull.c
otama_pull_LDADD = $(builddir)/libotama.la

otama_import_CFLAGS = $(otama_pull_CFLAGS)
otama_import_CXXFLAGS = $(otama_pull_CFLAGS)
otama_import_LDFLAGS = 
otama_import_SOURCES = util/otama_import.cpp
otama_import_LDADD = $(builddir)/libotama.la

otama_create_database_CFLAGS = $(otama_pull_CFLAGS)
otama_create_database_CXXFLAGS = $(otama_pull_CFLAGS)
otama_create_database_LDFLAGS = 
otama_create_database_SOURCES = util/otama_create_database.c
otama_create_database_LDADD = $(builddir)/libotama.la

otama_drop_database_CFLAGS = $(otama_pull_CFLAGS)
otama_drop_database_CXXFLAGS = $(otama_pull_CFLAGS)
otama_drop_database_LDFLAGS = 
otama_drop_database_SOURCES = util/otama_drop_database.c
otama_drop_database_LDADD = $(builddir)/libotama.la

otama_drop_index_CFLAGS = $(otama_pull_CFLAGS)
otama_drop_index_CXXFLAGS = $(otama_pull_CFLAGS)
otama_drop_index_LDFLAGS = 
otama_drop_index_SOURCES = util/otama_drop_index.c
otama_drop_index_LDADD = $(builddir)/libotama.la

otama_search_CFLAGS = $(otama_pull_CFLAGS)
otama_search_CXXFLAGS = $(otama_pull_CFLAGS)
otama_search_LDFLAGS = 
otama_search_SOURCES = util/otama_search.c
otama_search_LDADD = $(builddir)/libotama.la

otama_vacuum_index_CFLAGS = $(otama_pull_CFLAGS)
otama_vacuum_index_CXXFLAGS = $(otama_pull_CFLAGS)
otama_vacuum_index_LDFLAGS = 
otama_vacuum_index_SOURCES = util/otama_vacuum_index.c
otama_vacuum_index_LDADD = $(builddir)/libotama.la

