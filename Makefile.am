SUBDIRS= include src tools testsuite examples doc
DISTCLEANFILES =  *~  

ACLOCAL = aclocal-1.9 -I acmacro
AUTOMAKE_OPTIONS = gnu
DISTCHECK_CONFIGURE_FLAGS = --enable-gtk-doc

EXTRA_DIST = bootstrap COPYING INSTALL NEWS README README.IEEE TODO AUTHORS ChangeLog \
	acmacro/aci.m4 \
	tools/graspe-slave.in \
	tools/MSG_visualization tools/platform_generation

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
	 @echo "----[ Put the tarball @VERSION@ on gforge ]----"
#	 scp @PACKAGE@-@VERSION@.tar.gz gcl.ucsd.edu:/home/www/simgrid/dl/
#	 scp -r doc/html/* gcl.ucsd.edu:/home/www/simgrid/
#	 scp -r doc/html/* scm.gforge.inria.fr:/var/lib/gforge/chroot/home/groups/simgrid/htdocs/
	 scp ChangeLog gcl.ucsd.edu:/home/www/simgrid/dl/ChangeLog
	 @echo "----[ Put the tarball @VERSION@ on the local filesystem ]----"
	 cp @PACKAGE@-@VERSION@.tar.gz ~/public_html/simgrid2
	 cp -r doc/html ~/public_html/simgrid2/doc/
	 cp ChangeLog ~/public_html/simgrid2/
	 make -C ~/public_html

sync-gforge: 
	rsync --copy-links --verbose --archive --cvs-exclude --compress --delete \
	--delete-excluded --rsh=ssh -I \
	doc/html/ scm.gforge.inria.fr:/var/lib/gforge/chroot/home/groups/simgrid/htdocs/
	ssh scm.gforge.inria.fr "chgrp -R simgrid /var/lib/gforge/chroot/home/groups/simgrid/htdocs/"
	ssh scm.gforge.inria.fr "chmod g+rw -R /var/lib/gforge/chroot/home/groups/simgrid/htdocs/"

mail:
	 @ver=`dpkg-parsechangelog -lChangeLog | egrep '^Version: ' | sed 's/Version: //'`;\
	  (echo "Hello," ; \
	   echo; \
           echo "A new version of SimGrid is available. Here is the changelog:";\
	   echo;echo;\
           dpkg-parsechangelog -lChangeLog ;\
           echo;echo; \
	   echo "It is available from the official website:";\
	   echo "  http://simgrid.gforge.inria.fr";\
	   echo;echo "Cheers, the SimGrid team.") | \
          mail -e \
	       -a "From: Arnaud.Legrand@imag.fr" \
	       -s "New version of SimGrid ($$ver)" \
	       simgrid-user@lists.gforge.inria.fr

splint:
	splint `make dist-files | grep -v $(PWD) | egrep '\.[ch]$$' |sort -u` \
	       +matchanyintegral -warnposix +boolint \
	       +show-summary +stats\
	       -Iinclude -Isrc/include -Isrc/base -Isrc
.PHONY: splint sync-gforge
##
## Cruft for remote compilation
##

remote: 
	@$(top_srcdir)/tools/graspe-master

endif

include $(top_srcdir)/acmacro/dist-files.mk
