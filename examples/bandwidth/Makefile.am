INCLUDES= -I$(top_srcdir)/include
AM_CFLAGS=-g
TESTS=test_rl test_sg
EXTRA_DIST=bandwidth_deployment.txt $(TESTS)

noinst_PROGRAMS=bandwidth_maestro bandwidth_sensor bandwidth_simulator
  
bandwidth_simulator_SOURCES=	_bandwidth_simulator.c bandwidth.c
bandwidth_simulator_LDADD=	$(top_builddir)/src/libsimgrid.la $(top_builddir)/src/amok/libamok.la

bandwidth_maestro_SOURCES=	_bandwidth_maestro.c bandwidth.c
bandwidth_maestro_LDADD=	$(top_builddir)/src/libgras.la $(top_builddir)/src/amok/libamok.la

bandwidth_sensor_SOURCES=	_bandwidth_sensor.c bandwidth.c
bandwidth_sensor_LDADD= 	$(top_builddir)/src/libgras.la $(top_builddir)/src/amok/libamok.la

# cleanup temps
CLEANFILES=_bandwidth_simulator.c _bandwidth_maestro.c _bandwidth_sensor.c \
	   bandwidth.Makefile.local bandwidth.Makefile.remote bandwidth.deploy.sh
	   
MOSTLYCLEANFILES = bandwidth.trace

# generate temps
NAME=bandwidth
PROCESSES= maestro sensor

$(foreach proc, $(PROCESSES), _$(NAME)_$(proc).c) _$(NAME)_simulator.c: $(srcdir)/$(NAME)_deployment.xml ../../../tools/gras/gras_stub_generator@EXEEXT@
	[ x@EXEEXT@ = x ] || exenv=wine; $$exenv ../../../tools/gras/gras_stub_generator@EXEEXT@ $(NAME) $(srcdir)/$(NAME)_deployment.xml >/dev/null

../../../tools/gras/gras_stub_generator@EXEEXT@:
	make -C   ../../../tools/gras/ gras_stub_generator@EXEEXT@

include $(top_srcdir)/acmacro/dist-files.mk
