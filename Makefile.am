SUBDIRS= include src examples doc testsuite
DISTCLEANFILES =  *~  

ACLOCAL = aclocal -I acmacro -I /usr/share/aclocal/gnome2-macros
AUTOMAKE_OPTIONS = gnu
DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc

EXTRA_DIST = bootstrap COPYING INSTALL NEWS README README.IEEE TODO AUTHORS ChangeLog\
	acmacro/aci.m4 \
	tools/gras-check-arch	tools/compile-remote-worker.in

TAGS: 
	etags `find . -name "*.[ch]"`

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

publish: dist
	 @echo "----[ Put the documentation on the local filesystem ]----"
	 rm -rf ~/public_html/gras/doc
	 mkdir ~/public_html/gras/doc
	 cp -r doc/html ~/public_html/gras/doc
	 @echo "----[ Put the tarball @VERSION@ on the local filesystem ]----"
	 cp @PACKAGE@-@VERSION@.tar.gz ~/public_html/gras/dl
	 @echo "----[ Sync remote host ]----"
	 make -C public_html publish

mail:
	 @ver=`dpkg-parsechangelog -lChangeLog| egrep '^Version: ' | sed 's/Version: //'`;\
	  (echo "Hello," ; \
	   echo; \
           echo "A new version of GRAS is available. Here is the changelog:";\
	   echo;echo;\
           dpkg-parsechangelog -lChangeLog ;\
           echo;echo; \
	   echo "It is available from the official website:";\
	   echo "  http://grail.sdsc.edu/simgrid/";\
	   echo;echo "Cheers, Mt.") | \
          mail -e \
	       -a "From: Martin.Quinson@ens-lyon.fr" \
	       -s "New version of GRAS ($$ver)" \
	       martin.quinson@ens-lyon.fr,arnaud.legrand@ens-lyon.fr


splint:
	splint `find src -name '*.c' | grep -v RL | grep -v SG` +matchanyintegral -warnposix +boolint -Inws_portability/Include/ -Isrc/include -Isrc/base -Isrc

##
## Cruft for remote compilation
##

MACHINES ?= valnure.cs.ucsb.edu sperm.cs.ucsb.edu     basalt.cs.ucsb.edu \
	    graal.ens-lyon.fr   allo-psmn.ens-lyon.fr \
	    nala.cs.utk.edu

# ACTION: What to do there
# possible values:
#  - clean:   erase any previously existing source tree and 
#             open the new open
#  - touch:   touch every file of the source tree to deal with 
#             clock brokenness. May help, may harm.
#  - config:  launch configure
#  - compile: run 'make'
#  - install: run 'make install'
#  - check:   run 'make check'
#
# default value:
ACTION ?= clean untar config compile check

REMOTE_PREFIX ?='${HOME}/gras'

remote: @PACKAGE@-@VERSION@.tar.gz tools/compile-remote-worker
	@echo;echo "----[ Recompile the package on remote hosts ]----"
	@test -e buildlogs/@PACKAGE@-@VERSION@ || mkdir -p buildlogs/@PACKAGE@-@VERSION@
	@failed=0;\
	 for site in $(MACHINES) ; do \
	   machine=`echo $$site |sed 's/^\([^%]*\)%.*$$/\1/'`;\
	   machine2=`echo $$site |sed 's/^\([^%]*\)%\(.*\)$$/\2/'`;\
	   cmd="\"sh -c 'env REMOTE_PREFIX=$(REMOTE_PREFIX) $(REMOTE_PREFIX)/src/compile-remote-worker $(ACTION) 2>&1'\"";\
	   if echo $$site | grep  '%' >/dev/null ; then \
	     echo "----[ Compile on $$machine2 (behide $$machine) ]----";\
	   else \
	     machine=$$site;\
	     echo "----[ Compile on $$machine ]----";\
	   fi;\
	   \
	   echo "-- Copy the data over"; \
	   scp @PACKAGE@-@VERSION@.tar.gz tools/compile-remote-worker \
	       $$machine:$(REMOTE_PREFIX)/src;\
	   \
	   echo "-- Compiling... (the output gets into buildlogs/@PACKAGE@-@VERSION@/$$site.log)"; \
	   if echo $$site | grep  '%' >/dev/null ; then \
	     if ssh $$machine "env REMOTE_PREFIX=$(REMOTE_PREFIX) ssh -A $$machine2 $$cmd" 2>&1 > buildlogs/@PACKAGE@-@VERSION@/$$site.log;\
	     then echo "Sucessful"; else failed=1;echo "Failed (check buildlogs/@PACKAGE@-@VERSION@/$$site.log)"; fi;echo; \
	   else \
	     if ssh $$machine "eval $$cmd" 2>&1 > buildlogs/@PACKAGE@-@VERSION@/$$site.log ;\
	     then echo "Sucessful"; else failed=1;echo "Failed (check buildlogs/@PACKAGE@-@VERSION@/$$site.log)"; fi;echo; \
	   fi;\
	done;\
	tools/compile-stats;\
	exit $$failed

endif
