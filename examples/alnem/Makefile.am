DISTCLEANFILES=Makefile.in
INCLUDES= -I$(top_srcdir)/src/include
#-I../../src -I../../nws_portability/Include @CFLAGS_XML@ @CFLAGS_SimGrid@

noinst_PROGRAMS=alnem_simulator alnem_sensor alnem_maestro 

alnem_simulator_SOURCES=      _alnem_simulator.c alnem.c
alnem_sensor_SOURCES=         _alnem_sensor.c    alnem.c
alnem_maestro_SOURCES=        _alnem_maestro.c   alnem.c


alnem_simulator_LDADD=	  $(top_srcdir)/src/core/libgrassg.a $(top_srcdir)/src/modules/libgrasmodules.a @LIBS_SimGrid@ @LIBS_XML@
alnem_sensor_LDADD=       $(top_srcdir)/src/core/libgrasrl.a $(top_srcdir)/src/modules/libgrasmodules.a @LIBS_SimGrid@
alnem_maestro_LDADD=      $(top_srcdir)/src/core/libgrasrl.a $(top_srcdir)/src/modules/libgrasmodules.a @LIBS_SimGrid@

bin_PROGRAMS=alnem_builder
alnem_builder_LDADD=  @LIBS_SimGrid@ @LIBS_XML@

# cleanup temps
CLEANFILES= _alnem_simulator.c _alnem_sensor.c _alnem_maestro.c 

# generate temps 
_alnem_sensor.c _alnem_maestro.c _alnem_simulator.c: alnem_deployment.txt
	../gras_stub_generator alnem alnem_deployment.txt >/dev/null




