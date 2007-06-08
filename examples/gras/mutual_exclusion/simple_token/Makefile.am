INCLUDES= -I$(top_srcdir)/include
EXTRA_DIST=simple_token.xml 
include $(top_srcdir)/examples/gras/tests.mk

# AUTOMAKE variable definition
noinst_PROGRAMS=simple_token_node simple_token_simulator

simple_token_simulator_SOURCES=  _simple_token_simulator.c simple_token.c
simple_token_simulator_LDADD=    $(top_builddir)/src/libsimgrid.la

simple_token_node_SOURCES=	    _simple_token_node.c simple_token.c
simple_token_node_LDADD=	    $(top_builddir)/src/libgras.la

# generate temps
NAME=simple_token
PROCESSES=node
include $(top_srcdir)/examples/temps-gras-stub.mk

# Cruft
include $(top_srcdir)/acmacro/dist-files.mk
