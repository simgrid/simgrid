INCLUDES= -I$(top_srcdir)/include
TESTS= test_rl test_sg
EXTRA_DIST=tokenS_deployment.xml $(TESTS)

# AUTOMAKE variable definition
noinst_PROGRAMS=tokenS_node tokenS_simulator

tokenS_simulator_SOURCES=  _tokenS_simulator.c tokenS.c
tokenS_simulator_LDADD=    $(top_builddir)/src/libsimgrid.la

tokenS_node_SOURCES=	    _tokenS_node.c tokenS.c
tokenS_node_LDADD=	    $(top_builddir)/src/libgras.la

# cleanup temps
CLEANFILES = _tokenS_simulator.c _tokenS_node.c _tokenS_node.c \
             tokenS.Makefile.local tokenS.Makefile.remote tokenS.deploy.sh 

MOSTLYCLEANFILES = tokenS.trace

# generate temps
NAME=tokenS
PROCESSES=node

$(foreach proc, $(PROCESSES), _$(NAME)_$(proc).c) _$(NAME)_simulator.c: $(srcdir)/$(NAME)_deployment.xml ../../../tools/gras/gras_stub_generator@EXEEXT@
	[ x@EXEEXT@ = x ] || exenv=wine; $$exenv ../../../tools/gras/gras_stub_generator@EXEEXT@ $(NAME) $(srcdir)/$(NAME)_deployment.xml >/dev/null

../../../tools/gras/gras_stub_generator@EXEEXT@:
	make -C   ../../../tools/gras/ gras_stub_generator@EXEEXT@

include $(top_srcdir)/acmacro/dist-files.mk
