INCLUDES= -I$(top_srcdir)/include -I$(top_srcdir)/src/include

EXTRA_DIST=synchro.xml
include $(top_srcdir)/examples/gras/tests.mk

# AUTOMAKE variable definition
noinst_PROGRAMS=synchro_philosopher synchro_simulator

synchro_simulator_SOURCES=	_synchro_simulator.c philosopher.c
synchro_simulator_LDADD=	$(top_builddir)/src/libsimgrid.la

synchro_philosopher_SOURCES=	_synchro_philosopher.c  philosopher.c
synchro_philosopher_LDADD=	$(top_builddir)/src/libgras.la

# Take care of generatated sources
# (manually since the child is not in the deployment file, so we need an extra argument to the stub generator)

_synchro_simulator.c _synchro_philosopher.c: $(srcdir)/synchro.xml $(top_builddir)/tools/gras/gras_stub_generator@EXEEXT@
	[ x@EXEEXT@ = x ] || exenv=wine; $$exenv $(top_builddir)/tools/gras/gras_stub_generator@EXEEXT@ synchro $(srcdir)/synchro.xml

$(top_builddir)/tools/gras/gras_stub_generator@EXEEXT@:
	make -C $(top_builddir)/tools/gras/ gras_stub_generator@EXEEXT@

CLEANFILES = _synchro_simulator.c synchro.mk _synchro_philosopher.c
MOSTLYCLEANFILES = synchro.trace

# Cruft
include $(top_srcdir)/acmacro/dist-files.mk
