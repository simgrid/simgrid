INCLUDES= -I$(top_srcdir)/include
AM_CFLAGS=-g
TESTS=test_rl test_sg
EXTRA_DIST=bandwidth_deployment.txt $(TESTS)

if HAVE_SG
  check_PROGRAMS=bandwidth_maestro bandwidth_sensor bandwidth_simulator
  
  bandwidth_simulator_SOURCES=	_bandwidth_simulator.c bandwidth.c
  bandwidth_simulator_LDADD=	$(top_builddir)/src/libgrassg.la $(top_builddir)/src/amok/libamok.la @LIBS_SimGrid@ 
else
  check_PROGRAMS=bandwidth_maestro bandwidth_sensor
endif

bandwidth_maestro_SOURCES=	_bandwidth_maestro.c bandwidth.c
bandwidth_sensor_SOURCES=	_bandwidth_sensor.c bandwidth.c

bandwidth_maestro_LDADD=	$(top_builddir)/src/libgrasrl.la $(top_builddir)/src/amok/libamok.la
bandwidth_sensor_LDADD= 	$(top_builddir)/src/libgrasrl.la $(top_builddir)/src/amok/libamok.la

# cleanup temps
MAINTAINERCLEANFILES=_bandwidth_simulator.c _bandwidth_maestro.c _bandwidth_sensor.c

# generate temps
_bandwidth_maestro.c _bandwidth_sensor.c _bandwidth_simulator.c: bandwidth_deployment.txt
	$(top_srcdir)/examples/gras_stub_generator bandwidth bandwidth_deployment.txt >/dev/null
