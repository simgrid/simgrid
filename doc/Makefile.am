# Copyright (c) 2004-2007. The SimGrid team. All right reserved.

# This file is part of the SimGrid project. This is free software:
# You can redistribute and/or modify it under the terms of the
# GNU LGPL (v2.1) licence.


DOCSOURCES = $(shell find @top_srcdir@/tools/gras  @top_srcdir@/src @top_srcdir@/include -name "*.[chl]") \
 $(shell find @srcdir@ -name "*.doc" |grep -v logcategories.doc)
FIGS=simgrid_modules.fig simgrid_modules2.fig amok_bw_test.fig amok_bw_sat.fig gras_comm.fig
PNGS=$(patsubst %.fig,%.png,$(FIGS)) webcruft/simgrid_logo.png webcruft/simgrid_logo_small.png webcruft/poster_thumbnail.png

EXTRA_DIST = html Doxyfile \
 modules.doc FAQ.doc contrib.doc index.doc publis.doc footer.html history.doc  \
 $(foreach file,$(FIGS),fig/$(file)) $(PNGS)


CLEANFILES = .*.toc
DISTCLEANFILES = index-API.doc simgrid.tag
MAINTAINERCLEANFILES = html

if MAINTAINER_MODE
simgrid_modules.map: fig/simgrid_modules.fig
	fig2dev -Lmap fig/simgrid_modules.fig | grep -e IMG -e MAP -e AREA | \
	sed 's/imagemap/simgrid_modules/g' | sed 's/\.gif/.png/' | \
	sed 's/<IMG /<IMG style="border:0px "/g' | sed 's/\.gif/.png/' \
	> simgrid_modules.map

%.png: fig/%.fig
	fig2dev -Lpng $^ > $@

html: Doxyfile ./logcategories.doc simgrid_modules.map $(PNGS) $(DOCSOURCES) \
      html/gras.html html/amok.html html/msg.html html/simdag.html
	: # Setting up
	touch index-API.doc .FAQ.doc.toc .index.doc.toc .contrib.doc.toc .history.doc.toc
	test -e html || mkdir html
	cp $(foreach file,$(PNGS),@srcdir@/$(file)) html/
	cp @srcdir@/webcruft/Paje_MSG_screenshot_thn.jpg @srcdir@/webcruft/Paje_MSG_screenshot.jpg html/
	cp @srcdir@/simgrid.css html/
	: # First pass
	doxygen Doxyfile >/dev/null
	@top_srcdir@/tools/doxygen/index_create.pl simgrid.tag index-API.doc
	@top_srcdir@/tools/doxygen/toc_create.pl \
	   @srcdir@/FAQ.doc @srcdir@/index.doc @srcdir@/contrib.doc \
	   @srcdir@/gtut-introduction.doc @srcdir@/history.doc
	: # Second pass
	doxygen Doxyfile >/dev/null
	: # Post-processing
	rm -f html/dir* 
	@top_srcdir@/tools/doxygen/index_php.pl @srcdir@/index.php.in html/index.html index.php
	@top_srcdir@/tools/doxygen/doxygen_postprocesser.pl

html/gras.html:
	mkdir -p html
	echo "<html><META HTTP-EQUIV='Refresh' content='0;URL=http://simgrid.gforge.inria.fr/doc/group__GRAS__API.html'>" > $@
	echo "<center><h2><br><a href='http://simgrid.gforge.inria.fr/doc/group__GRAS__API.html'>Grid Reality And Simulation.</a></h2></center></html>" >> $@
html/amok.html:
	echo "<html><META HTTP-EQUIV='Refresh' content='0;URL=http://simgrid.gforge.inria.fr/doc/group__AMOK__API.html'>" > $@
	echo "<center><h2><br><a href='http://simgrid.gforge.inria.fr/doc/group__AMOK__API.html'>Advanced Metacomputing Overlay Kit.</a></h2></center></html>" >> $@
html/msg.html:
	echo "<html><META HTTP-EQUIV='Refresh' content='0;URL=http://simgrid.gforge.inria.fr/doc/group__MSG__API.html'>" > $@
	echo "<center><h2><br><a href='http://simgrid.gforge.inria.fr/doc/group__MSG__API.html'>Meta SimGrid.</a></h2></center></html>" >> $@
html/simdag.html:
	echo "<html><META HTTP-EQUIV='Refresh' content='0;URL=http://simgrid.gforge.inria.fr/doc/group__SD__API.html'>" > $@
	echo "<center><h2><br><a href='http://simgrid.gforge.inria.fr/doc/group__SD__API.html'>DAG Simulator.</a></h2></center></html>" >> $@

publish:
	$(MAKE) -C .. sync-gforge
endif

./logcategories.doc: $(DOCSOURCES)
	pwd=`pwd`; rm -f $$pwd/logcategories.doc; set -e; \
	echo "/* Generated file, do not edit */"          > $$pwd/logcategories.doc; \
	echo "/** \addtogroup XBT_log_cats"              >> $$pwd/logcategories.doc; \
	echo "    @{"                                    >> $$pwd/logcategories.doc; \
	cd @top_srcdir@/src; ../tools/doxygen/xbt_log_extract_hierarchy.pl >> $$pwd/logcategories.doc; \
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


check-gtut-tocs:
	@echo Check individual TOCs
	@for n in gtut-tour-*.doc ; do \
	  sed -n '/Table of Contents/,/hr/p' $$n|grep ref > tmp.curtoc; \
	  \
	  grep -E '\\s?u?b?section' $$n | grep -v _toc| \
	     sed -e 's/\\section //' -e 's/\\subsection /subsection/' -e 's/ .*//' |\
	     sed -e 's/^/ - \\ref /' -e 's/- \\ref subsection/   - \\ref /' > tmp.realtoc; \
	  \
	  if ! diff -q tmp.curtoc tmp.realtoc >/dev/null; then \
	    echo Wrong toc for $$n. Should be ; \
	    cat tmp.realtoc; \
	  fi ; \
	done
	@rm tmp.realtoc tmp.curtoc
	@echo "Check main TOC"
	@for n in gtut-tour-*doc; do \
	  grep @page $$n |sed -e 's/@page //' -e 's/ .*//' -e 's/^/ - \\ref /' >>tmp.realtoc;\
	  sed -n -e '/Table of Contents/,/hr/p' $$n|sed -e 's/^/   /'|\
	     grep -v toc|grep -v '<hr>'|grep [^[:blank:]] >>tmp.realtoc;\
	done
	@grep ' *- \\ref' gtut-tour.doc > tmp.curtoc
	@if ! diff -b -u tmp.curtoc tmp.realtoc ; then \
	  echo "Wrong toc for gtut-tour.doc Right one is in tmp.realtoc"; \
	else \
	  rm tmp.realtoc; \
	fi ;
	@rm tmp.curtoc

.PHONY: html html/

include $(top_srcdir)/acmacro/dist-files.mk
