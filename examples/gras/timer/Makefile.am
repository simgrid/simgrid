INCLUDES= -I$(top_srcdir)/include
AM_CFLAGS=-g
NAME=timer
PROCESSES= client

TESTS= test_rl test_sg
EXTRA_DIST=$(NAME)_deployment.xml $(TESTS)

# AUTOMAKE variable definition
noinst_PROGRAMS=timer_client timer_simulator

timer_simulator_SOURCES=	_timer_simulator.c timer.c
timer_simulator_LDADD=	$(top_builddir)/src/libsimgrid.la

timer_client_SOURCES=	_timer_client.c timer.c
timer_client_LDADD=	$(top_builddir)/src/libgras.la

# cleanup temps
CLEANFILES = _$(NAME)_simulator.c _$(NAME)_client.c _$(NAME)_server.c \
             $(NAME).Makefile.local $(NAME).Makefile.remote $(NAME).deploy.sh 

MOSTLYCLEANFILES = $(NAME).trace

# generate temps

$(foreach proc, $(PROCESSES), _$(NAME)_$(proc).c) _$(NAME)_simulator.c: $(srcdir)/$(NAME)_deployment.xml ../../../tools/gras/gras_stub_generator@EXEEXT@
	[ x@EXEEXT@ = x ] || exenv=wine; $$exenv ../../../tools/gras/gras_stub_generator $(NAME) $(srcdir)/$(NAME)_deployment.xml >/dev/null

../../../tools/gras/gras_stub_generator@EXEEXT@:
	make -C   ../../../tools/gras/ gras_stub_generator@EXEEXT@

include $(top_srcdir)/acmacro/dist-files.mk
