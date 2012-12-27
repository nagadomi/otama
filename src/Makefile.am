EXTRA_DIST = lib/posix/otama_mmap.c lib/windows/otama_mmap.c lib/otama_config_msvc.h libotama-0.def

SUBDIRS    = nvcolorex nvbovw nvvlad nvlmcaex

lib_LTLIBRARIES = libotama.la
libotama_la_LIBADD = $(builddir)/nvcolorex/libnvcolorex.la $(builddir)/nvbovw/libnvbovw.la $(builddir)/nvvlad/libnvvlad.la $(builddir)/nvlmcaex/libnvlmcaex.la
libotama_la_CFLAGS = -I$(srcdir)/models -I$(srcdir)/lib -I$(srcdir)/nvcolorex -I$(srcdir)/nvbovw -I$(srcdir)/nvlmcaex -I$(srcdir)/nvvlad -DPKGDATADIR=\""$(pkgdatadir)"\"
libotama_la_CXXFLAGS = $(libotama_la_CFLAGS)
libotama_la_CPPFLAGS = $(libotama_la_CFLAGS)
libotama_la_LDFLAGS = -no-undefined -rpath "$(libdir)" -version-number @OTAMA_MAJOR@:@OTAMA_MINOR@:0

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
models/otama_inverted_index_kc.cpp \
models/otama_inverted_index_map.cpp \
models/otama_inverted_index_bucket.cpp \
models/otama_driver_factory.cpp \
models/otama_variable_byte_code_vector.hpp \
models/otama_kyotocabinet.hpp \
models/otama_fixed_strage.hpp \
models/otama_inverted_index.hpp \
models/otama_inverted_index_kc.hpp \
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

bin_PROGRAMS = otama_pull otama_import otama_create_table otama_drop_table

otama_pull_CFLAGS = $(libotama_la_CFLAGS)
otama_pull_CXXFLAGS = $(otama_pull_CFLAGS)
otama_pull_CPPFLAGS = $(otama_pull_CFLAGS)
otama_pull_LDFLAGS = 
otama_pull_SOURCES = util/otama_pull.c
otama_pull_LDADD = libotama.la

otama_import_CFLAGS = $(otama_pull_CFLAGS)
otama_import_CXXFLAGS = $(otama_pull_CFLAGS)
otama_import_CPPFLAGS = $(otama_pull_CFLAGS)
otama_import_LDFLAGS = 
otama_import_SOURCES = util/otama_import.cpp
otama_import_LDADD = libotama.la

otama_create_table_CFLAGS = $(otama_pull_CFLAGS)
otama_create_table_CXXFLAGS = $(otama_pull_CFLAGS)
otama_create_table_CPPFLAGS = $(otama_pull_CFLAGS)
otama_create_table_LDFLAGS = 
otama_create_table_SOURCES = util/otama_create_table.c
otama_create_table_LDADD = libotama.la

otama_drop_table_CFLAGS = $(otama_pull_CFLAGS)
otama_drop_table_CXXFLAGS = $(otama_pull_CFLAGS)
otama_drop_table_CPPFLAGS = $(otama_pull_CFLAGS)
otama_drop_table_LDFLAGS = 
otama_drop_table_SOURCES = util/otama_drop_table.c
otama_drop_table_LDADD = libotama.la

pkgconfigdir = @pkgconfigdir@
pkgconfig_DATA = otama.pc

CLEANFILES = otama.pc
