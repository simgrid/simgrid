# Makefile chunk which allows to display the files which should be included
# into the distribution.

# It is intended to be included in all Makefile.am 

dist-files:
	@for n in $(DISTFILES) ; do echo $(SRCFILE)$$n; done
	@echo
	@for n in $(DIST_SUBDIRS) ; do if [ x$$n != x. ] ; then $(MAKE) -C $$n dist-files SRCFILE=$(SRCFILE)$$n/ ; fi; done
