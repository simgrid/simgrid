INCLUDES= -I$(top_srcdir)/include
AM_CFLAGS=-g

TESTS=test_rl test_sg
EXTRA_DIST=saturate_deployment.txt $(TESTS)

if HAVE_SG
 check_PROGRAMS=saturate_simulator saturate_sensor saturate_maestro

 saturate_simulator_SOURCES=_saturate_simulator.c saturate.c
 saturate_simulator_LDADD=	$(top_builddir)/src/libgrassg.la @LIBS_SimGrid@ # $(top_srcdir)/src/modules/libgrasmodules.la
else
 check_PROGRAMS=saturate_sensor saturate_maestro
endif

saturate_sensor_SOURCES=          _saturate_sensor.c saturate.c
saturate_maestro_SOURCES=         _saturate_maestro.c saturate.c

saturate_sensor_LDADD=            $(top_srcdir)/src/libgrasrl.la # $(top_srcdir)/src/modules/libgrasmodules.a
saturate_maestro_LDADD=       	  $(top_srcdir)/src/libgrasrl.la # $(top_srcdir)/src/modules/libgrasmodules.a

# cleanup templates
CLEANFILES= _saturate_simulator.c _saturate_sensor.c _saturate_maestro.c 

# generate templates 
_saturate_sensor.c _saturate_maestro.c _saturate_simulator.c: saturate_deployment.txt
	$(top_srcdir)/examples/gras_stub_generator saturate saturate_deployment.txt >/dev/null




