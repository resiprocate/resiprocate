VERSION := pre-0.9.6
BUILD_DIR := build
include   $(BUILD_DIR)/Makefile.common

all:
	$(MAKE) -C tools
	$(MAKE) -C src

clean:
	$(MAKE) -C src clean
	$(MAKE) -C tools clean
	$(RM) doc/doxygen/html/*
	$(RM) -r doc/doxygen/rtf
	$(RM) doc/doxygen-warnlog.txt
	$(RM) doc/doxygen/osc-dox-html*.tar.gz

distclean: clean
	$(RM) $(BUILD_DIR)/Makefile.conf

install:
	$(MAKE) -C src install

.PHONY: docs tarball release

docs:
	-@doxygen
	-@cp doc/doxygen/doxygen.css doc/doxygen/html
	-@(cd doc/doxygen; ln -s html doxygen; tar hzcf osc-dox-html-$(VERSION).tar.gz doxygen; rm doxygen)


###########################################################################
# Tarfile related stuff follows

ifneq ($(filter %tarball, $(MAKECMDGOALS)),)
VERSION := $(VERSION)-svn$(shell svnversion .)-$(shell date +%Y%m%d)
endif

VERSION := $(subst :,_,$(VERSION))

DISTFILES := CHANGES COPYING README Doxyfile LICENSE Makefile \
             src tools doc/images doc/doxygen visual-studio configure build
DISTFILES += $(wildcard doc/standards/*.txt)
DISTFILES := $(foreach x, $(DISTFILES), opensigcomp-$(VERSION)/$x)

EXCLUDE := $(shell find * -name .svn)
EXCLUDE := $(foreach x, $(EXCLUDE), --exclude opensigcomp-$(VERSION)/$x)

tarball: distclean
	@echo Making tarball for version $(VERSION)
	@ln -s . opensigcomp-$(VERSION)
	-@tar zcvf osc-$(VERSION).tar.gz $(EXCLUDE) $(DISTFILES)
	@$(RM) opensigcomp-$(VERSION)

release: tarball

show.%:
	@echo $*=$($*)
