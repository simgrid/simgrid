#
# Makefile for Include
#
# $Id$
#

# include common variables
include ../Makedefs

# headers to be installed 
HEADERS =	diagnostic.h \
		dnsutil.h \
		formatutil.h \
		messages.h \
		osutil.h \
		protocol.h \
		config_portability.h \
		strutil.h \
		fbuff.h \
		timeouts.h \
		forecast_api.h

.PHONY: all clean dist install

all: 
	@$(CP) $(HEADERS) $(buildincdir)

clean:

dist:
	@$(INSTALL) -d $(DIST_ROOT)/Include
	@$(CP) Makefile *.h.in *.h $(DIST_ROOT)/Include

install:
	@echo "Installing header files..."
	@$(CP) $(HEADERS) $(incdir)

