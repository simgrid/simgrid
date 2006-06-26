INCLUDES= -I$(top_srcdir)/include
TESTS= test_sg test_rl
EXTRA_DIST=all2all_deployment.xml test_sg.in test_rl.in

# AUTOMAKE variable definition
noinst_PROGRAMS=all2all_sender all2all_receiver all2all_simulator

all2all_simulator_SOURCES=	_all2all_simulator.c all2all.c 
all2all_simulator_LDADD=	$(top_builddir)/src/libsimgrid.la

all2all_sender_SOURCES=	_all2all_sender.c all2all.c
all2all_sender_LDADD=	$(top_builddir)/src/libgras.la

all2all_receiver_SOURCES=	_all2all_receiver.c all2all.c
all2all_receiver_LDADD=	$(top_builddir)/src/libgras.la

# Take care of generatated sources
NAME=all2all
PROCESSES= sender receiver
include $(top_srcdir)/examples/temps-gras-stub.mk

# Cruft
include $(top_srcdir)/acmacro/dist-files.mk
