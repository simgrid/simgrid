#
# Makefile for the Portability library
#
# $Id$

include Makedefs

# extra files to be put into the distribution
DIST_FILES	= 	Makedefs.in \
			Makefile \
			config.guess \
			config.sub \
			install-sh \
			configure 

# these are the targets
LIBRARY =	$(buildlibdir)/libNwsPortability.a

# what we are going to put in the library
OBJS = 		$(buildobjdir)/diagnostic.o \
		$(buildobjdir)/dnsutil.o \
		$(buildobjdir)/formatutil.o \
		$(buildobjdir)/osutil.o \
		$(buildobjdir)/messages.o \
		$(buildobjdir)/strutil.o \
		$(buildobjdir)/timeouts.o \
		$(buildobjdir)/protocol.o


.PHONY: clean confclean distclean precondition dist all common

all: precondition common $(OBJS)
	$(AR) $(ARFLAGS) $(LIBRARY) $(OBJS)
	$(RANLIB) $(LIBRARY)

common: 
	@for subdir in $(SUBDIRS); do \
		(cd $$subdir && $(MAKE) all) || exit $$? ; done

clean:
	@for subdir in $(SUBDIRS); do \
		(cd $$subdir && $(MAKE) clean) || exit $$? ; done
	@$(RM) -f $(OBJS)

confclean:
	@echo "Removing configurations files ..."
	@$(RM) -f Makedefs Include/config_portability.h config.status config.log config.cache
	
distclean: confclean
	@echo "Removing build directories ..."
	@$(RM) -rf $(builddir) $(DIST_NAME)

dist: 
	@$(RM) -rf $(DIST_ROOT) $(DIST_NAME)
	@$(INSTALL) -d $(DIST_ROOT)
	@for subdir in $(SUBDIRS); do \
		(cd $$subdir && $(MAKE) dist) || exit $$? ; done
	@$(CP)  $(DIST_FILES) $(DIST_ROOT)
	@$(CP) *.c $(DIST_ROOT)
	@$(TAR) cf $(TOP)/$(DIST_NAME) Portability
	@$(RM) -rf $(DIST_ROOT)

.PHONY: install
install: all 
	@$(INSTALL) -d $(prefix)
	@$(INSTALL) -d $(exec_prefix)
	@$(INSTALL) -d $(bindir)
	@$(INSTALL) -d $(libdir)
	@$(INSTALL) -d $(incdir)
	@$(CP) $(buildbindir)/* $(bindir)
	@$(CP) $(buildlibdir)/* $(libdir)
	@$(CP) $(buildincdir)/* $(incdir)

tags:
	$(CTAGS) -R 

precondition: 
	@if test `./config.guess` != "$(configured)"; then \
		echo "Portability was configured for $(configured): run make confclean and reconfigure for the `./config.guess`!";\
		exit 1; fi
	@$(INSTALL) -d $(buildobjdir)
	@$(INSTALL) -d $(buildlibdir)
	@$(INSTALL) -d $(buildincdir)
	@$(INSTALL) -d $(buildbindir)

Makedefs: Makedefs.in config.status
	./config.status

config.status: configure
	./configure

$(buildobjdir)/%.o : %.c Include/%.h Include/config_portability.h
	$(CC) $(INCLUDES) $(CFLAGS) -g -c $< -o $@
	

