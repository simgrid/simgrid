DISTCLEANFILES=Makefile.in
INCLUDES= -I$(top_srcdir)/src/include

noinst_PROGRAMS=bandwidth_simulator bandwidth_maestro bandwidth_sensor

bandwidth_simulator_SOURCES=	_bandwidth_simulator.c bandwidth.c
bandwidth_maestro_SOURCES=	_bandwidth_maestro.c bandwidth.c
bandwidth_sensor_SOURCES=	_bandwidth_sensor.c bandwidth.c


bandwidth_simulator_LDADD=	$(top_srcdir)/src/Core/libgrassg.a $(top_srcdir)/src/modules/libgrasmodules.a @LIBS_SimGrid@ @LIBS_XML@
bandwidth_maestro_LDADD=	$(top_srcdir)/src/Core/libgrasrl.a $(top_srcdir)/src/modules/libgrasmodules.a
bandwidth_sensor_LDADD= 	$(top_srcdir)/src/Core/libgrasrl.a $(top_srcdir)/src/modules/libgrasmodules.a

# cleanup temps
CLEANFILES=_bandwidth_simulator.c _bandwidth_maestro.c _bandwidth_sensor.c

# generate temps
_bandwidth_maestro.c _bandwidth_sensor.c _bandwidth_simulator.c: bandwidth_deployment.txt
	$(top_srcdir)/src/examples/gras_stub_generator bandwidth bandwidth_deployment.txt >/dev/null
