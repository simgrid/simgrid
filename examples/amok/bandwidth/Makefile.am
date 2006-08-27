INCLUDES= -I$(top_srcdir)/include
AM_CFLAGS=-g
TESTS=test_rl test_sg
EXTRA_DIST=bandwidth.xml $(TESTS)

noinst_PROGRAMS=bandwidth_maestro bandwidth_sensor bandwidth_simulator
  
bandwidth_simulator_SOURCES=	_bandwidth_simulator.c bandwidth.c
bandwidth_simulator_LDADD=	$(top_builddir)/src/libsimgrid.la 

bandwidth_maestro_SOURCES=	_bandwidth_maestro.c bandwidth.c
bandwidth_maestro_LDADD=	$(top_builddir)/src/libgras.la 

bandwidth_sensor_SOURCES=	_bandwidth_sensor.c bandwidth.c
bandwidth_sensor_LDADD= 	$(top_builddir)/src/libgras.la 

# Take care of generatated sources
NAME=bandwidth
PROCESSES= maestro sensor
include $(top_srcdir)/examples/temps-gras-stub.mk

# Cruft 
include $(top_srcdir)/acmacro/dist-files.mk
