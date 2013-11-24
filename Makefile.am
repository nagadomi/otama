ACLOCAL_AMFLAGS = -I m4
EXTRA_DIST = autogen.sh COPYING README.md \
ruby/bovw2k.yaml \
ruby/bovw8k.yaml \
ruby/bovw512k.yaml \
ruby/bovw512k_iv.yaml \
ruby/vlad_nodb.yaml \
ruby/extconf.rb \
ruby/otama.c \
ruby/test_similarity.rb \
ruby/test.rb \
ruby/test.sh

SUBDIRS    = src src/tests vcproj examples image

bovw-benchmark: all
	cd src && ./nvbovw/nv_bovw_benchmark

vlad-benchmark: all
	cd src && ./nvvlad/nv_vlad_benchmark

otama_test: all
	cd src/tests && $(MAKE) $(AM_MAKEFLAGS) otama_test

gdb: otama_test
	cd src/tests && LD_LIBRARY_PATH=../.libs gdb ./.libs/otama_test

valgrind: otama_test
	cd src/tests && \
	LD_LIBRARY_PATH=../.libs valgrind -v --tool=memcheck --leak-check=full --show-reachable=yes --log-file=./valgrind.log ./.libs/otama_test; \
	cat ./valgrind.log

if BUILDING_RUBY
all-local: 
	cd ruby; \
	if test ! -e otama.$(OBJEXT); then \
		$(RUBY) extconf.rb --with-otama-include=$(abs_top_srcdir)/src/lib --with-otama-lib=$(abs_top_srcdir)/src/.libs && $(MAKE) all; \
	fi

check-local:
	cd ruby; \
	LD_LIBRARY_PATH=$(abs_top_srcdir)/src/.libs ./test.sh $(RUBY)

install-exec-hook:
	cd ruby; \
	$(MAKE) install

clean-local:
	cd ruby; \
	$(MAKE) clean
endif
