DISTCLEANFILES=Makefile.in
INCLUDES= -I$(top_srcdir)/src/include

noinst_PROGRAMS=saturate_simulator saturate_sensor saturate_maestro 

saturate_simulator_SOURCES=	  _saturate_simulator.c saturate.c
saturate_sensor_SOURCES=          _saturate_sensor.c saturate.c
saturate_maestro_SOURCES=         _saturate_maestro.c saturate.c

saturate_simulator_LDADD=	  $(top_srcdir)/src/Core/libgrassg.a $(top_srcdir)/src/modules/libgrasmodules.a @LIBS_SimGrid@ @LIBS_XML@
saturate_sensor_LDADD=            $(top_srcdir)/src/Core/libgrasrl.a $(top_srcdir)/src/modules/libgrasmodules.a
saturate_maestro_LDADD=       	  $(top_srcdir)/src/Core/libgrasrl.a $(top_srcdir)/src/modules/libgrasmodules.a

# cleanup templates
CLEANFILES= _saturate_simulator.c _saturate_sensor.c _saturate_maestro.c 

# generate templates 
_saturate_sensor.c _saturate_maestro.c _saturate_simulator.c: saturate_deployment.txt
	../gras_stub_generator saturate saturate_deployment.txt >/dev/null




