INCLUDES= -I$(top_srcdir)/include
TESTS= test_rl test_sg
EXTRA_DIST=simple_token.xml $(TESTS)

# AUTOMAKE variable definition
noinst_PROGRAMS=simple_token_node simple_token_simulator

simple_token_simulator_SOURCES=  _simple_token_simulator.c simple_token.c
simple_token_simulator_LDADD=    $(top_builddir)/src/libsimgrid.la

simple_token_node_SOURCES=	    _simple_token_node.c simple_token.c
simple_token_node_LDADD=	    $(top_builddir)/src/libgras.la

# cleanup temps
CLEANFILES = _simple_token_simulator.c _simple_token_node.c _simple_token_node.c \
             simple_token.Makefile.local simple_token.Makefile.remote simple_token.deploy.sh 

MOSTLYCLEANFILES = simple_token.trace

# generate temps
NAME=simple_token
PROCESSES=node

$(foreach proc, $(PROCESSES), _$(NAME)_$(proc).c) _$(NAME)_simulator.c: $(srcdir)/$(NAME).xml $(top_srcdir)/tools/gras/gras_stub_generator@EXEEXT@
	[ x@EXEEXT@ = x ] || exenv=wine; $$exenv $(top_srcdir)/tools/gras/gras_stub_generator@EXEEXT@ $(NAME) $(srcdir)/$(NAME).xml >/dev/null

$(top_srcdir)/tools/gras/gras_stub_generator@EXEEXT@:
	make -C   $(top_srcdir)/tools/gras/ gras_stub_generator@EXEEXT@

include $(top_srcdir)/acmacro/dist-files.mk
