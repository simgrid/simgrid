SUBDIRS= include src tools examples testsuite doc
DISTCLEANFILES =  *~  

ACLOCAL = aclocal-1.9 -I acmacro
AUTOMAKE_OPTIONS = gnu
DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc

EXTRA_DIST = bootstrap COPYING INSTALL NEWS README README.IEEE TODO AUTHORS ChangeLog \
	acmacro/aci.m4 \
	tools/gras-check-arch	tools/compile-remote-worker.in \
	tools/MSG_visualization tools/platform_generation \
        tools/doxygen/index_create.pl

SOURCE_FILES = $(shell find . -name "*.[ch]")
TAGS: $(SOURCE_FILES)
	@etags $(SOURCE_FILES)

tags: TAGS



if MAINTAINER_MODE
##
## The following is only interesting for me, I guess. 
## Some of the targets will only work on my machine ;)
##

release: distcheck remote debian publish

debian: dist
	make -C ~/CVSIMPORT/pkg-grid/gras maintainerclean
	cp @PACKAGE@-@VERSION@.tar.gz ~/CVSIMPORT/pkg-grid/gras/
	cp @PACKAGE@-@VERSION@.tar.gz ~/CVSIMPORT/pkg-grid/gras/@PACKAGE@_@VERSION@.orig.tar.gz
	make -C ~/CVSIMPORT/pkg-grid/gras deb check
	dput local *.changes

publish-old: dist
	 @echo "----[ Put the documentation on the local filesystem ]----"
	 rm -rf ~/public_html/gras/doc
	 mkdir ~/public_html/gras/doc
	 cp -r doc/html ~/public_html/gras/doc
	 @echo "----[ Put the tarball @VERSION@ on the local filesystem ]----"
	 cp @PACKAGE@-@VERSION@.tar.gz ~/public_html/gras/dl
	 @echo "----[ Sync remote host ]----"
	 make -C public_html publish

publish: dist
	 @echo "----[ Put the tarball @VERSION@ on gcl ]----"
	 scp @PACKAGE@-@VERSION@.tar.gz gcl.ucsd.edu:/home/www/simgrid/dl/
	 scp -r doc/html/* gcl.ucsd.edu:/home/www/simgrid/
	 scp ChangeLog gcl.ucsd.edu:/home/www/simgrid/dl/ChangeLog
	 @echo "----[ Put the tarball @VERSION@ on the local filesystem ]----"
	 cp @PACKAGE@-@VERSION@.tar.gz ~/public_html/simgrid2
	 cp -r doc/html ~/public_html/simgrid2/doc/
	 cp ChangeLog ~/public_html/simgrid2/
	 make -C ~/public_html

mail:
	 @ver=`dpkg-parsechangelog -lChangeLog | egrep '^Version: ' | sed 's/Version: //'`;\
	  (echo "Hello," ; \
	   echo; \
           echo "A new version of SimGrid is available. Here is the changelog:";\
	   echo;echo;\
           dpkg-parsechangelog -lChangeLog ;\
           echo;echo; \
	   echo "It is available from the official website:";\
	   echo "  http://grail.sdsc.edu/simgrid/";\
	   echo;echo "Cheers, the SimGrid team.") | \
          mail -e \
	       -a "From: Arnaud.Legrand@imag.fr" \
	       -s "New version of GRAS ($$ver)" \
	       simgrid2-users@listes.ens-lyon.fr

splint:
	splint `make dist-files | grep -v $(PWD) | egrep '\.[ch]$$' |sort -u` \
	       +matchanyintegral -warnposix +boolint \
	       +show-summary +stats\
	       -Iinclude -Isrc/include -Isrc/base -Isrc
.PHONY: splint
##
## Cruft for remote compilation
##

MACHINES ?= graal.ens-lyon.fr   allo-psmn.ens-lyon.fr \
            valnure.cs.ucsb.edu sperm.cs.ucsb.edu     basalt.cs.ucsb.edu \
	    nala.cs.utk.edu     AIX

# ACTIONS: What to do there
# possible values:
#  - clean:   erase any previously existing source tree and 
#             open the new open
#  - untar:   open the tarball which was just copied
#  - touch:   touch every file of the source tree to deal with 
#             clock brokenness. May help, may harm.
#  - config:  launch configure
#  - compile: run 'make'
#  - install: run 'make install'
#  - check:   run 'make check'
#
# default value:
ACTIONS ?= clean untar config compile check

REMOTE_PREFIX ?='simgrid'

remote: @PACKAGE@-@VERSION@.tar.gz tools/compile-remote-worker
	@echo;echo "----[ Recompile the package on remote hosts ]----"
	@test -e @srcdir@/buildlogs/@PACKAGE@-@VERSION@ || mkdir -p @srcdir@/buildlogs/@PACKAGE@-@VERSION@
	@failed=0;\
	 for site in $(MACHINES) ; do \
	   machine=`echo $$site |sed 's/^\([^%]*\)%.*$$/\1/'`;\
	   machine2=`echo $$site |sed 's/^\([^%]*\)%\(.*\)$$/\2/'`;\
	   cmd="\"sh -c 'env REMOTE_PREFIX=$(REMOTE_PREFIX) $(REMOTE_PREFIX)/src/compile-remote-worker $(ACTIONS) 2>&1'\"";\
	   if echo $$site | grep  '%' >/dev/null ; then \
	     echo "----[ Compile on $$machine2 (behind $$machine) ]----";\
	   else \
	     machine=$$site;\
	     echo "----[ Compile on $$machine ]----";\
	   fi;\
	   \
	   echo "-- Copy the data over"; \
	   scp @srcdir@/@PACKAGE@-@VERSION@.tar.gz tools/compile-remote-worker \
	       $$machine:$(REMOTE_PREFIX)/src;\
	   \
	   echo "-- Compiling... (the output gets into @srcdir@/buildlogs/@PACKAGE@-@VERSION@/$$site.log)"; \
	   if echo $$site | grep  '%' >/dev/null ; then \
	     if ssh $$machine "env REMOTE_PREFIX=$(REMOTE_PREFIX) ssh -A $$machine2 $$cmd" 2>&1 > @srcdir@/buildlogs/@PACKAGE@-@VERSION@/$$site.log;\
	     then echo "Sucessful"; else failed=1;echo "Failed (check @srcdir@/buildlogs/@PACKAGE@-@VERSION@/$$site.log)"; fi;echo; \
	   else \
	     if ssh $$machine "eval $$cmd" 2>&1 > @srcdir@/buildlogs/@PACKAGE@-@VERSION@/$$site.log ;\
	     then echo "Sucessful"; else failed=1;echo "Failed (check @srcdir@/buildlogs/@PACKAGE@-@VERSION@/$$site.log)"; fi;echo; \
	   fi;\
	done;\
	@srcdir@/tools/compile-stats;\
	exit $$failed

endif

include $(top_srcdir)/acmacro/dist-files.mk
