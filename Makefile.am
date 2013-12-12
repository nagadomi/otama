ACLOCAL_AMFLAGS = -I m4
EXTRA_DIST = autogen.sh COPYING README.md \
ruby/bovw2k.yaml \
ruby/bovw8k.yaml \
ruby/bovw512k.yaml \
ruby/bovw512k_iv.yaml \
ruby/bovw512k_nodb.yaml \
ruby/vlad_nodb.yaml \
ruby/extconf.rb \
ruby/otama.c \
ruby/test_similarity.rb \
ruby/test_kvs.rb \
ruby/test.rb \
ruby/test.sh

SUBDIRS    = src src/tests vcproj examples image

bovw-benchmark: all
	cd $(abs_top_builddir)/src && ./nvbovw/nv_bovw_benchmark

vlad-benchmark: all
	cd $(abs_top_builddir)/src && ./nvvlad/nv_vlad_benchmark

otama_test: all
	cd $(abs_top_srcdir)/src/tests && $(MAKE) $(AM_MAKEFLAGS) otama_test

gdb: otama_test
	cd $(abs_top_builddir)/src/tests && LD_LIBRARY_PATH=../.libs gdb ./.libs/otama_test

valgrind: otama_test
	cd $(abs_top_builddir)/src/tests && \
	LD_LIBRARY_PATH=../.libs valgrind -v --tool=memcheck --leak-check=full --show-reachable=yes --log-file=./valgrind.log ./.libs/otama_test; \
	cat ./valgrind.log

if BUILDING_RUBY
all-local:
	$(MKDIR_P) $(abs_top_builddir)/ruby; \
	cd $(abs_top_builddir)/ruby; \
	if test ! -e otama.$(OBJEXT); then \
		$(RUBY) $(abs_top_srcdir)/ruby/extconf.rb --with-otama-include=$(abs_top_srcdir)/src/lib --with-otama-lib=$(abs_top_builddir)/src/.libs --with-ldflags="-Wl,-rpath,'$(libdir)'" && $(MAKE) $(AM_MAKEFLAGS) all; \
	fi

check-local:
	cd $(abs_top_builddir)/ruby; \
	LD_LIBRARY_PATH="$(abs_top_builddir)/src/.libs" NV_BOVW_PKGDATADIR="$(abs_top_builddir)/src/nvbovw"  $(abs_top_srcdir)/ruby/test.sh $(RUBY) $(abs_top_srcdir)/ruby

install-exec-hook:
	cd $(abs_top_builddir)/ruby; \
	$(MAKE) $(AM_MAKEFLAGS) install

clean-local:
	$(MKDIR_P) $(abs_top_builddir)/ruby; \
	cd $(abs_top_builddir)/ruby; \
	if test -e Makefile; then \
		$(MAKE) $(AM_MAKEFLAGS) clean; \
	fi
endif
