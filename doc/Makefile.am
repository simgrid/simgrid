DOCSOURCES = $(shell find @top_srcdir@/src @top_srcdir@/include -name "*.[chl]") $(shell find @top_srcdir@/src @top_srcdir@/include -name "*.doc")

EXTRA_DIST = html Doxyfile.API Doxyfile.Examples Doxyfile.main \
 FAQ.doc index.doc footer.html simgrid_logo.png simgrid_modules.jpg

DISTCLEANFILES = api_index.doc
MAINTAINERCLEANFILES = html

if MAINTAINER_MODE
html: $(DOCSOURCES) Doxyfile.API Doxyfile.Examples Doxyfile.main logcategories.doc
	touch api_index.doc
	doxygen Doxyfile.main >/dev/null
	doxygen Doxyfile.API >/dev/null
	doxygen Doxyfile.Examples >/dev/null
	@top_srcdir@/tools/doxygen/index_create.pl html/API/API.tag api_index.doc
	cp @srcdir@/simgrid_logo.png @srcdir@/simgrid_modules.jpg html/
	doxygen Doxyfile.API >/dev/null
	doxygen Doxyfile.Examples >/dev/null
	doxygen Doxyfile.main >/dev/null
#	@cd html ; @top_srcdir@/tools/fix_doxygen2.pl `find . -name '*.html'`
#	@rm -rf html/API html/examples
#	for i in `find doc/html -name "*.html" -exec grep -l "group_[^_]"  {} \;` ; do tools/fix_doxygen.pl $$i; rm $$i.bak; done
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
