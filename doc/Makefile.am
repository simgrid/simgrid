DOCSOURCES = $(shell find @top_srcdir@/src @top_srcdir@/include -name "*.[chl]") $(shell find @top_srcdir@/src @top_srcdir@/include -name "*.doc")

EXTRA_DIST = html Doxyfile.API Doxyfile.Examples  Doxyfile.main

html: $(DOCSOURCES) Doxyfile.API  Doxyfile.Examples  Doxyfile.main
	@touch @top_srcdir@/doc/api_index.doc
	@doxygen Doxyfile.main
	@doxygen Doxyfile.API
	@doxygen Doxyfile.Examples
	@@top_srcdir@/tools/doxygen/index_create.pl html/API/API.tag @top_srcdir@/doc/api_index.doc
	@cp simgrid_logo.png simgrid_modules.jpg html/
	@doxygen Doxyfile.API
	@doxygen Doxyfile.Examples
	@doxygen Doxyfile.main
#	@cd html ; @top_srcdir@/tools/fix_doxygen2.pl `find . -name '*.html'`
#	@rm -rf html/API html/examples
#	for i in `find doc/html -name "*.html" -exec grep -l "group_[^_]"  {} \;` ; do tools/fix_doxygen.pl $$i; rm $$i.bak; done

clean: 
	@rm -rf html/
