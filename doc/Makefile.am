DOCSOURCES = $(shell find @top_srcdir@/tools/gras  @top_srcdir@/src @top_srcdir@/include -name "*.[chl]") \
 $(shell find @srcdir@ -name "*.doc")

EXTRA_DIST = html Doxyfile \
 modules.doc FAQ.doc index.doc publis.doc footer.html simgrid_logo.png simgrid_modules.png

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
	touch index-API.doc
	doxygen Doxyfile >/dev/null
	../tools/doxygen/index_create.pl simgrid.tag index-API.doc
	../tools/doxygen/toc_create.pl FAQ.doc FAQ.toc
	../tools/doxygen/toc_create.pl index.doc index.toc
	doxygen Doxyfile >/dev/null
	cp @srcdir@/simgrid_logo.png @srcdir@/simgrid_modules.png html/
	cp @srcdir@/Paje_MSG_screenshot_thn.jpg @srcdir@/Paje_MSG_screenshot.jpg html/
	for i in `find html -name '*.html'` ; do \
            cp $$i $$i.bak ; \
            cat $$i.bak |  \
               sed -e 's|<li><a href="index.html"><span>Main\&nbsp;Page</span></a></li>|<li><a href="index.html"><span>Overview</span></a></li>       <li><a href="faq.html"><span>FAQ</span></a></li>|' \
                   -e 's|<li><a href="annotated.html"><span>Data\&nbsp;Structures</span></a></li>|<li><a href="publis.html"><span>Publications</span></a></li>|g' \
                   -e 's|<span>Modules</span>|<span>Modules API</span>|g' \
                   -e 's|<li><a href="dirs.html"><span>Directories</span></a></li>||g' \
            > $$i ; \
            rm $$i.bak ; \
        done
	../tools/doxygen/index_php.pl index.php.in html/index.html index.php
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
