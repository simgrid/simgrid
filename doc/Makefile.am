DOCSOURCES = $(shell find @top_srcdir@/src @top_srcdir@/include -name "*.[chl]") $(shell find @top_srcdir@/src @top_srcdir@/include -name "*.doc")

EXTRA_DIST = html Doxyfile.API Doxyfile.Examples  Doxyfile.main

DISTCLEANFILES = api_index.doc

html: $(DOCSOURCES) Doxyfile.API  Doxyfile.Examples  Doxyfile.main
	touch api_index.doc
	doxygen @srcdir@/Doxyfile.main
	doxygen @srcdir@/Doxyfile.API
	doxygen @srcdir@/Doxyfile.Examples
	@top_srcdir@/tools/doxygen/index_create.pl html/API/API.tag api_index.doc
	cp simgrid_logo.png simgrid_modules.jpg html/
	doxygen @srcdir@/Doxyfile.API
	doxygen @srcdir@/Doxyfile.Examples
	doxygen @srcdir@/Doxyfile.main
#	@cd html ; @top_srcdir@/tools/fix_doxygen2.pl `find . -name '*.html'`
#	@rm -rf html/API html/examples
#	for i in `find doc/html -name "*.html" -exec grep -l "group_[^_]"  {} \;` ; do tools/fix_doxygen.pl $$i; rm $$i.bak; done

clean: 
	@rm -rf html/
