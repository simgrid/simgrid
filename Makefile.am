SUBDIRS= doc src 
DISTCLEANFILES =  *~  

ACLOCAL = aclocal -I acmacro -I /usr/share/aclocal/gnome2-macros
AUTOMAKE_OPTIONS = gnu

EXTRA_DIST = bootstrap COPYING INSTALL NEWS README README.IEEE TODO AUTHORS ChangeLog\
	acmacro/aci.m4 \
	tools/gras-check-arch	tools/compile-remote-worker.in

TAGS: 
	etags `find . -name "*.[ch]"`

tags: TAGS



if MAINTAINER_MODE
##
## The following is only interesting for me, I guess
##

publish: dist
	 @echo "----[ Put the tarball @VERSION@ on gcl ]----"
	 scp @PACKAGE@-@VERSION@.tar.gz gcl.ucsd.edu:/home/www/simgrid/dl/
	 scp -r doc/html/* gcl.ucsd.edu:/home/www/simgrid/
	 scp ChangeLog gcl.ucsd.edu:/home/www/simgrid/dl/
	 @echo "----[ Put the tarball @VERSION@ on the local filesystem ]----"
	 cp @PACKAGE@-@VERSION@.tar.gz ~/public_html/simgrid2
	 cp -r doc/html ~/public_html/simgrid2/doc/
	 cp ChangeLog ~/public_html/simgrid2/

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
	splint `find src examples -name '*.c'` +matchanyintegral -warnposix +boolint -Inws_portability/Include/ -Isrc/

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
	@for site in $(MACHINES) ; do \
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
	     if ssh -CA $$machine "env REMOTE_PREFIX=$(REMOTE_PREFIX) ssh -A $$machine2 $$cmd" 2>&1 > buildlogs/@PACKAGE@-@VERSION@/$$site.log;\
	     then echo "Sucessful"; else echo "Failed (check buildlogs/@PACKAGE@-@VERSION@/$$site.log)"; fi;echo; \
	   else \
	     if ssh -CA $$machine "eval $$cmd" 2>&1 > buildlogs/@PACKAGE@-@VERSION@/$$site.log ;\
	     then echo "Sucessful"; else echo "Failed (check buildlogs/@PACKAGE@-@VERSION@/$$site.log)"; fi;echo; \
	   fi;\
	done

endif
