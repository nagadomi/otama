ACLOCAL_AMFLAGS = -I m4
EXTRA_DIST = autogen.sh COPYING docs .gitignore README.md \
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

bovw-benchmark:
	cd src; ./nvbovw/nv_bovw_benchmark; cd ..;

pqh-benchmark:
	cd src; ./nvpqh/nv_pqh_benchmark; cd ..;

vlad-benchmark:
	cd src; ./nvvlad/nv_vlad_benchmark; cd ..;

ruby-ext:
	cd ruby; ruby exconf.rb; make; cd ..;

test-ruby:
	cd ruby; ./test.sh ; cd ..

valgrind:
	cd src/tests; LD_LIBRARY_PATH=../.libs valgrind -v --tool=memcheck --leak-check=full --show-reachable=yes --log-file=./valgrind.log ./.libs/otama_test; cat ./valgrind.log; cd -

if BUILDING_DEB
DEBSOURCEPKG=$(PACKAGE)_$(VERSION).orig.tar.gz
DEBSOURCEDIR=$(PACKAGE)-$(VERSION)
DEBIANDIR=packages/debian
deb: dist
	@$(ECHO) "Building Debian package..."; \
	if $(TEST) ! -d $(DEBIANDIR); then \
		$(ECHO) "The files needed for building a Debian package are not" \
		"included by default in the distribution. To build a package, check" \
		"out the project from source control."; \
		exit 1; \
	fi; \
	$(MV) $(PACKAGE)-$(VERSION).tar.gz $(DEBSOURCEPKG) && \
	$(TAR) --extract --gunzip --file $(DEBSOURCEPKG) && \
	$(MKDIR_P) $(DEBSOURCEDIR)/debian && \
	$(CP) -r $(DEBIANDIR)/ $(DEBSOURCEDIR) && \
	cd $(DEBSOURCEDIR) && \
	$(DEBUILD) $(DPKG_FLAGS); \
	cd ..;
else
deb:
	@$(ECHO) \
	"To build a Debian package, you must run configure with --enable-deb."; \
	exit 1
endif
clean-local:
	-rm -rf $(DEBSOURCEDIR)

CLEANFILES = *.deb *.debian.tar.gz *.dsc *.changes *.build *.upload $(DEBSOURCEPKG)
