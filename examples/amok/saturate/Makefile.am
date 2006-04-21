INCLUDES= -I$(top_srcdir)/include
AM_CFLAGS=-g

TESTS=test_rl test_sg
EXTRA_DIST=saturate_deployment.xml $(TESTS)

noinst_PROGRAMS=saturate_simulator saturate_sensor saturate_maestro

saturate_simulator_SOURCES=_saturate_simulator.c saturate.c
saturate_simulator_LDADD=	$(top_builddir)/src/libsimgrid.la

saturate_sensor_SOURCES=          _saturate_sensor.c saturate.c
saturate_maestro_SOURCES=         _saturate_maestro.c saturate.c

saturate_sensor_LDADD=            $(top_builddir)/src/libgras.la 
saturate_maestro_LDADD=       	  $(top_builddir)/src/libgras.la 

# Take care of generatated sources
NAME=saturate
PROCESSES=sensor maestro
include $(top_srcdir)/examples/temps-gras-stub.mk

# Cruft
include $(top_srcdir)/acmacro/dist-files.mk
