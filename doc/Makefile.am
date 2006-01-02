DOCSOURCES = $(shell find @top_srcdir@/tools/gras  @top_srcdir@/src @top_srcdir@/include -name "*.[chl]") \
 $(shell find @srcdir@ -name "*.doc")

EXTRA_DIST = html Doxyfile \
 modules.doc FAQ.doc index.doc publis.doc footer.html simgrid_logo.png simgrid_modules.png

CLEANFILES = .*.toc
DISTCLEANFILES = index-API.doc simgrid.tag
MAINTAINERCLEANFILES = html

if MAINTAINER_MODE
simgrid_modules.map: simgrid_modules.fig
	fig2dev -Lmap simgrid_modules.fig | grep -e IMG -e MAP -e AREA | \
	sed 's/imagemap/simgrid_modules/g' | sed 's/\.gif/.png/' | \
	sed 's/<IMG /<IMG style="border:0px "/g' | sed 's/\.gif/.png/' \
	> simgrid_modules.map

simgrid_modules.png: simgrid_modules.fig
	fig2dev -Lpng simgrid_modules.fig > simgrid_modules.png

html: $(DOCSOURCES) Doxyfile logcategories.doc simgrid_modules.map simgrid_modules.png
	: # Setting up
	touch index-API.doc .FAQ.doc.toc
	cp @srcdir@/simgrid_logo.png @srcdir@/simgrid_modules.png html/
	cp @srcdir@/Paje_MSG_screenshot_thn.jpg @srcdir@/Paje_MSG_screenshot.jpg html/
	cp @srcdir@/simgrid.css html/
	: # First pass
	doxygen Doxyfile >/dev/null
	../tools/doxygen/index_create.pl simgrid.tag index-API.doc
	../tools/doxygen/toc_create.pl FAQ.doc index.doc
	: # Second pass
	doxygen Doxyfile >/dev/null
	: # Post-processing
	rm html/dir*
	../tools/doxygen/index_php.pl index.php.in html/index.html index.php
	../tools/doxygen/doxygen_postprocesser.pl
endif

logcategories.doc: $(DOCSOURCES)
	pwd=`pwd`; rm $$pwd/logcategories.doc; set -e; \
	echo "/* Generated file, do not edit */"          > $$pwd/logcategories.doc; \
	echo "/** \addtogroup XBT_log_cats"              >> $$pwd/logcategories.doc; \
	echo "    @{"                                    >> $$pwd/logcategories.doc; \
	cd @top_srcdir@/src; ./xbt_log_extract_hierarchy >> $$pwd/logcategories.doc; \
	echo "@}*/"                                      >> $$pwd/logcategories.doc;

install-data-local: 
	@for file in `find html/ -type d` ; do \
	   echo "test -z \"$(prefix)/doc/simgrid/$$file\" || $(mkdir_p) \"$(prefix)/doc/simgrid/$$file\"";\
	   test -z "$(prefix)/doc/simgrid/$$file" || $(mkdir_p) "$(prefix)/doc/simgrid/$$file";\
	done
	@for file in `find html/ -type f` ; do \
	   echo " $(INSTALL_DATA) '$$file' '$(prefix)/doc/simgrid/$$file'";\
	   $(INSTALL_DATA) $$file $(prefix)/doc/simgrid/$$file ;\
	done

uninstall-local:
	for file in `find html/ -type f` ; do \
	   echo " rm -f '$(prefix)/doc/simgrid/$$file'";\
	   rm -f $(prefix)/doc/simgrid/$$file ;\
	done

.PHONY: html

include $(top_srcdir)/acmacro/dist-files.mk
