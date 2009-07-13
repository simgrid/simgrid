# Copyright (c) 2004-2007. The SimGrid team. All right reserved.

# This file is part of the SimGrid project. This is free software:
# You can redistribute and/or modify it under the terms of the
# GNU LGPL (v2.1) licence.


#AM_CFLAGS= -DNDEBUG 

# -DNLOG   cuts absolutely all logs at compilation time.
# -DNDEBUG cuts asserts and logs at "trace" and "debug" levels.



MAINTAINERCLEANFILES=Makefile.in
INCLUDES= -I$(top_srcdir)/include -I$(top_srcdir)/src/include

bin_SCRIPTS= smpi/smpicc smpi/smpirun

EXTRA_DIST= \
        \
	portable.h \
	\
        xbt/mallocator_private.h \
	xbt/dynar_private.h \
        xbt/dict_private.h \
        xbt/heap_private.h \
        xbt/fifo_private.h \
        xbt/graph_private.h \
	xbt/graphxml_parse.c \
	xbt/graphxml.l \
	xbt/graphxml.c \
	xbt/graphxml.dtd \
        xbt/xbt_context_private.h \
        xbt/xbt_context_java.h \
	xbt/log_private.h \
	xbt/ex_interface.h \
        xbt/backtrace_linux.c xbt/backtrace_windows.c xbt/backtrace_dummy.c \
        \
	surf/maxmin_private.h \
	surf/trace_mgr_private.h \
	surf/surf_private.h \
	surf/surfxml_parse.c \
	surf/simgrid_dtd.l \
	surf/simgrid_dtd.c \
	surf/simgrid.dtd \
	surf/network_private.h \
	surf/network_gtnets_private.h \
	surf/gtnets/gtnets_interface.h \
	surf/gtnets/gtnets_simulator.h \
	surf/gtnets/gtnets_topology.h  \
	\
        include/surf/surf_resource.h  \
	include/surf/datatypes.h \
	include/surf/maxmin.h \
	include/surf/trace_mgr.h \
	include/surf/surf.h \
	include/surf/surfxml_parse_private.h \
	include/surf/random_mgr.h \
	\
	include/xbt/wine_dbghelp.h \
	include/xbt/context.h \
	include/xbt/xbt_os_time.h \
	include/xbt/xbt_os_thread.h \
	\
	msg/private.h \
	msg/mailbox.h \
	\
	simdag/private.h \
	\
	gras/DataDesc/ddt_parse.yy.l \
	gras/Virtu/virtu_rl.h \
	gras/Virtu/virtu_sg.h \
	gras/Virtu/virtu_interface.h \
	gras/Virtu/virtu_private.h \
	gras/Transport/transport_interface.h \
	amok/Bandwidth/bandwidth_private.h \
	amok/amok_modinter.h \
	\
	include/simix/simix.h include/simix/datatypes.h \
	simix/private.h \
	\
	smpi/private.h \
	smpi/smpi_coll_private.h \
	smpi/README

#LIBRARY_VERSION= 0:0:0
#                 | | |
#          +------+ | +---+
#          |        |     |
#       current:revision:age
#          |        |     |
#          |        |     +- Increment if interfaces have been added
#          |        |        Set to zero if interfaces have been removed or
#          |        |        changed
#          |        +- Increment if source code has changed
#          |           Set to zero if current is incremented
#          +- Increment if interfaces have been added, removed or changed

VERSION_INFO= -version-info 2:0:0
# from `info libtool "Updating version info"` 
# and  `info libtool "Release numbers"` 
#
# A) For stable library (interface wise), you should use --version-info:
#
# - Begin with C:R:A = 0:0:0 (ie here, VERSION_INFO= -version-info 0:0:0)
# - Do not update it before public release (keep numbers small)
#
# ----------------------------------------------------------------------+
# +   Interface     |  code of existing  | Interface | New version info |
# | removal/change? | interface changed? | addition? |                  |
# +-----------------+--------------------+-----------+------------------+
# |      yes        |   must be yes ;)   |           |  C++ : 0   : 0   |
# |      no         |        yes         |    yes    |  C   : R++ : A++ |
# |      no         |        yes         |    no     |  C   : R++ : A   |
# |      no         |        no          |    yes    |  C   : R   : A++ |
# |      no         |        no          |    no     |  C   : R   : A   |
# +-----------------+--------------------+-----------+------------------+
#
# B) For rapidely changing library, you should go for the -release flag
#
#  It modifies the library name, and you thus cannot say that a library
#   using this trick is ready for a "stable" release (say, in Debian).


################################################
# Declaration of the different modules content #
################################################

XBT_SRC=\
  \
  xbt/snprintf.c    xbt/xbt_str.c xbt/xbt_strbuff.c  xbt/xbt_sha.c           \
  xbt/ex.c                                                                   \
  \
  xbt_modinter.h    gras_modinter.h                                          \
  xbt/xbt_virtu.c                                                            \
  \
  xbt/xbt_os_time.c                                                          \
  xbt/asserts.c                                                              \
  xbt/log.c                    xbt/xbt_log_appender_file.c                   \
  xbt/xbt_log_layout_simple.c  xbt/xbt_log_layout_format.c                   \
  xbt/mallocator.c                                                           \
  xbt/dynar.c                                                                \
  xbt/dict.c        xbt/dict_elm.c               xbt/dict_cursor.c           \
  xbt/dict_multi.c                                                           \
  xbt/heap.c                                                                 \
  xbt/fifo.c                                                                 \
  xbt/swag.c                                                                 \
  xbt/graph.c                                                                \
  xbt/set.c                                                                  \
  xbt/xbt_matrix.c                                                           \
  \
  xbt/xbt_queue.c                                                            \
  \
  xbt/xbt_peer.c                                                             \
  \
  xbt/xbt_main.c                                                             \
  \
  xbt/config.c                                                               \
  xbt/cunit.c                                                                \
  xbt/graphxml_parse.c

XBT_RL_SRC = \
  xbt/xbt_rl_synchro.c		\
  xbt/xbt_rl_time.c
XBT_SG_SRC = \
  xbt/xbt_sg_synchro.c		\
  xbt/xbt_sg_time.c     

SURF_SRC= \
  surf/surf_model.c                                                          \
  surf/surf_action.c                                                         \
  surf/surf_routing.c                                                        \
  surf/surf_config.c                                                         \
  surf/maxmin.c                                                              \
  surf/fair_bottleneck.c                                                     \
  surf/lagrange.c                                                            \
  surf/trace_mgr.c                                                           \
  surf/random_mgr.c                                                          \
  surf/surf.c                                                                \
  surf/surfxml_parse.c                                                       \
  surf/cpu.c   surf/network.c surf/network_constant.c surf/workstation.c     \
  surf/surf_model_timer.c                                                    \
  surf/workstation_ptask_L07.c                                               \
  xbt/xbt_sg_stubs.c

if CONTEXT_THREADS
  SURF_SRC +=  xbt/xbt_os_thread.c xbt/xbt_context_thread.c  
else  
  SURF_SRC += xbt/xbt_context_sysv.c
endif

GTNETS_SRC= \
  surf/gtnets/gtnets_simulator.cc \
  surf/gtnets/gtnets_topology.cc  \
  surf/gtnets/gtnets_interface.cc \
  surf/network_gtnets.c

# Separated because src/gras/rl_stubs also define the function xbt_context_mod_init() and xbt_context_mod_exit()
# so there is an implementation of these functions in the simgrid library and an implementatin in the gras library 
CTX_SRC= xbt/xbt_context.c

SIMIX_SRC= \
  simix/smx_global.c \
  simix/smx_deployment.c \
  simix/smx_environment.c \
  simix/smx_host.c \
  simix/smx_process.c \
  simix/smx_action.c \
  simix/smx_synchro.c

SMPI_SRC= \
  smpi/smpi_base.c \
  smpi/smpi_bench.c \
  smpi/smpi_global.c \
  smpi/smpi_mpi.c \
  smpi/smpi_sender.c \
  smpi/smpi_receiver.c \
  smpi/smpi_util.c \
  smpi/smpi_coll.c

MSG_SRC=  msg/msg_config.c \
  msg/task.c msg/host.c msg/m_process.c msg/gos.c \
  msg/global.c msg/environment.c msg/deployment.c msg/msg_mailbox.c \
  msg/msg_actions.c

JMSG_C_SRC = \
  xbt/xbt_context_java.c \
  java/jxbt_utilities.c     java/jxbt_utilities.h      \
  java/jmsg.c               java/jmsg.h                \
  java/jmsg_channel.c       java/jmsg_channel.h        \
  java/jmsg_host.c          java/jmsg_host.h           \
  java/jmsg_process.c       java/jmsg_process.h        \
  java/jmsg_task.c          java/jmsg_task.h           \
  java/jmsg_application_handler.c java/jmsg_application_handler.h

JMSG_JAVA_SRC = \
  java/simgrid/msg/MsgException.java                 \
  java/simgrid/msg/JniException.java                 \
  java/simgrid/msg/NativeException.java              \
  java/simgrid/msg/HostNotFoundException.java        \
  java/simgrid/msg/ProcessNotFoundException.java     \
  \
  java/simgrid/msg/Msg.java                          \
  java/simgrid/msg/Process.java                      \
  java/simgrid/msg/Host.java                         \
  java/simgrid/msg/Task.java                         \
  \
  java/simgrid/msg/MsgNative.java                    \
  java/simgrid/msg/DTDResolver.java                  \
  java/simgrid/msg/ApplicationHandler.java           \
  \
  java/simgrid/msg/Sem.java


SIMDAG_SRC= simdag/sd_global.c simdag/sd_link.c simdag/sd_task.c simdag/sd_workstation.c

GRAS_COMMON_SRC= \
  gras/gras.c  \
  gras/Transport/transport.c          gras/Transport/transport_private.h  \
  gras/Msg/gras_msg_mod.c             gras/Msg/gras_msg_types.c           \
  gras/Msg/gras_msg_exchange.c        gras/Msg/gras_msg_listener.c        \
  gras/Msg/rpc.c                      gras/Msg/timer.c                    \
  gras/Msg/msg_interface.h            gras/Msg/msg_private.h              \
  \
  gras/Virtu/process.c gras/Virtu/gras_module.c  \
  \
  gras/DataDesc/ddt_create.c          \
  gras/DataDesc/ddt_convert.c         gras/DataDesc/ddt_exchange.c     \
  gras/DataDesc/cbps.c                gras/DataDesc/datadesc.c         \
  gras/DataDesc/datadesc_interface.h  gras/DataDesc/datadesc_private.h \
  gras/DataDesc/ddt_parse.c           gras/DataDesc/ddt_parse.yy.c         gras/DataDesc/ddt_parse.yy.h

GRAS_RL_SRC= \
  gras/rl_stubs.c                     xbt/xbt_os_thread.c                    \
  \
  gras/Transport/rl_transport.c          \
  gras/Transport/transport_plugin_file.c   gras/Transport/transport_plugin_tcp.c  \
  \
  gras/Virtu/rl_emul.c \
  gras/Virtu/rl_process.c \
  gras/Virtu/rl_dns.c\
  \
  gras/Msg/rl_msg.c

GRAS_RL_SRC+=$(XBT_RL_SRC)

GRAS_SG_SRC= gras/Transport/sg_transport.c  gras/Transport/transport_plugin_sg.c        \
  \
  gras/Virtu/sg_emul.c \
  gras/Virtu/sg_process.c   \
  gras/Virtu/sg_dns.c\
  \
  gras/Msg/sg_msg.c

GRAS_SG_SRC+=$(XBT_SG_SRC)

AMOK_SRC= \
  amok/amok_base.c \
  amok/Bandwidth/bandwidth.c amok/Bandwidth/saturate.c \
  amok/PeerManagement/peermanagement.c

##############################
# Deal with optional modules #
##############################

if HAVE_GTNETS
  GTNETS_USED=$(GTNETS_SRC)
else
  GTNETS_USED=
  EXTRA_DIST+=$(GTNETS_SRC)
endif

###
### Declare the library content
###

lib_LTLIBRARIES= libsimgrid.la libgras.la libsmpi.la

libgras_la_SOURCES=  $(XBT_SRC) $(GRAS_COMMON_SRC) $(GRAS_RL_SRC) $(AMOK_SRC)
libgras_la_LDFLAGS = -no-undefined $(VERSION_INFO) @GRAS_DEP@ @LD_DYNAMIC_FLAGS@ -lm 

libsimgrid_la_SOURCES = $(XBT_SRC) $(SURF_SRC) $(GTNETS_USED) \
                        $(SIMIX_SRC) $(CTX_SRC) \
                        $(MSG_SRC) $(SIMDAG_SRC) \
			$(GRAS_COMMON_SRC) $(GRAS_SG_SRC) $(AMOK_SRC)
libsimgrid_la_LDFLAGS = -no-undefined $(VERSION_INFO) @SIMGRID_DEP@ @LD_DYNAMIC_FLAGS@ -lm
# -Wl,--entry -Wl,simgrid_version

# Please do not add -no-undefined to libsmpi_la_LDFLAGS:
# smpi_simulated_main is indeed defined in user code, and undef in the
# library (no idea why it works under linux, but it fails under windows)
libsmpi_la_LIBADD = libsimgrid.la
libsmpi_la_SOURCES = $(SMPI_SRC)
libsmpi_la_LDFLAGS = $(VERSION_INFO) @SMPI_DEP@ @LD_DYNAMIC_FLAGS@ -lm

# Windows users (at least, MSVC ones) need a def file.
if IS_WINDOWS
libgras_la_LDFLAGS    += -Wl,--output-def,libgras.def
libsimgrid_la_LDFLAGS += -Wl,--output-def,libsimgrid.def
libsmpi_la_LDFLAGS    += -Wl,--output-def,libsmpi.def
endif

##
## Java cruft

EXTRA_DIST+=$(JMSG_JAVA_SRC)

if HAVE_JAVA
libsimgrid_la_SOURCES += $(JMSG_C_SRC) # add the binding support to the library

clean-local:
	-rm -rf .classes

jardir = $(prefix)/jar
jar_DATA =  simgrid.jar

simgrid.jar: $(JMSG_JAVA_SRC) $(srcdir)/surf/simgrid.dtd
	[ -e .classes ] || mkdir .classes
	$(JAVAC) -d .classes $(foreach file,$(JMSG_JAVA_SRC),$(srcdir)/$(file))
	[ -e .classes/surf ] || mkdir .classes/surf
	cp $(srcdir)/surf/simgrid.dtd .classes/surf
	$(JAR) cvf simgrid.jar -C .classes .
else
EXTRA_DIST+=$(JMSG_C_SRC) $(MSG_SRC)
endif

if GRAMINE_MODE
else 
###
### Testing infrastructure
###

# Test files must be added both to the TEST_CFILES and TEST_UNITS because
# if I compute TEST_UNITS from TEST_CFILES, automake fails to note that they
# are generated. Sorry about that.

# If you add a test unit, you should regenerate simgrid_units_main.c from scratch:
# make clean-units ; make testall

# Suites and tests run in the given order.

noinst_PROGRAMS=testall
TEST_CFILES=xbt/cunit.c  xbt/ex.c          \
            xbt/dynar.c xbt/dict.c xbt/set.c xbt/swag.c \
	    xbt/xbt_str.c  xbt/xbt_sha.c                \
            xbt/config.c 
TEST_UNITS= @builddir@/cunit_unit.c   @builddir@/ex_unit.c         \
            @builddir@/dynar_unit.c   @builddir@/dict_unit.c @builddir@/set_unit.c @builddir@/swag_unit.c \
	    @builddir@/xbt_str_unit.c @builddir@/xbt_strbuff_unit.c @builddir@/xbt_sha_unit.c\
            @builddir@/config_unit.c 

BUILT_SOURCES=../include/surf/simgrid_dtd.h surf/simgrid_dtd.c \
              ../include/xbt/graphxml.h xbt/graphxml.c \
              gras/DataDesc/ddt_parse.yy.c \
	      $(TEST_UNITS) @builddir@/simgrid_units_main.c

testall_SOURCES= $(TEST_UNITS) @builddir@/simgrid_units_main.c
testall_LDADD=libgras.la
TESTS=testall
TESTS_ENVIRONMENT=$(SG_TEST_EXENV)

EXTRA_DIST+=$(testall_SOURCES)

if MAINTAINER_MODE
CLEANFILES=$(TEST_UNITS)

$(TEST_UNITS): xbt/cunit.c xbt/ex.c xbt/xbt_str.c xbt/xbt_strbuff.c xbt/xbt_sha.c\
               xbt/dynar.c xbt/dict.c xbt/set.c xbt/swag.c xbt/config.c
	@top_srcdir@/tools/sg_unit_extractor.pl $^

@builddir@/simgrid_units_main.c: $(TEST_UNITS)
	@top_srcdir@/tools/sg_unit_extractor.pl @srcdir@/xbt/cunit.c

clean-units:
	rm -f simgrid_units_main.c *_unit.c @srcdir@/simgrid_units_main.c @srcdir@/*_unit.c
	for n in $(TEST_UNITS) ; do \
	  dep=`echo $$n|sed -e 's|.c$$|.Po|' -e 's|[^/]*/||' -e 's|^|.deps/|'`; \
          echo "Removing dependency tracker of $$n ($$dep @srcdir@/$$dep)"; \
	  rm -vf $$dep @srcdir@/$$dep; touch $$dep; \
        done
else

if BOTBUILD_MODE
$(TEST_UNITS) simgrid_units_main.c clean-units: 
	@echo "WARNING: Test units not generated, but touched instead since this is a bot build."
	touch $@
else 
$(TEST_UNITS) simgrid_units_main.c clean-units: 
	@echo "ERROR: Test units not generated."
	@echo "ERROR: If you are using a SVN checkout, configure with the --enable-maintainer-mode flag"
	@echo "ERROR: If not, please report the bug to the simgrid-devel mailing list."
	@exit 1
endif
endif

###
### Regenerate what needs to with flex & flexml
###


gras/DataDesc/ddt_parse.yy.c: gras/DataDesc/ddt_parse.yy.l
	set -e;@LEX@ -o$@ -Pgras_ddt_parse_ --noline $^

if MAINTAINER_MODE
surf/simgrid_dtd.c: surf/simgrid_dtd.l
	set -e;mkdir -p `dirname $@`; @LEX@ -o$@ -Psurf_parse_ --noline $^
	sed 's/#include <unistd.h>/#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32) || defined(__TOS_WIN__)\n#  ifndef __STRICT_ANSI__\n#    include <io.h>\n#    include <process.h>\n#  endif\n#else\n#  include <unistd.h>\n#endif/g' -i $@
xbt/graphxml.c: xbt/graphxml.l
	set -e;mkdir -p `dirname $@`; @LEX@ -o$@ -Pxbt_graph_parse_ --noline $^
	sed 's/#include <unistd.h>/#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32) || defined(__TOS_WIN__)\n#  ifndef __STRICT_ANSI__\n#    include <io.h>\n#    include <process.h>\n#  endif\n#else\n#  include <unistd.h>\n#endif/g' -i $@

if HAVE_FLEXML
surf/simgrid_dtd.l: $(srcdir)/surf/simgrid.dtd
	set -e; mkdir -p surf; flexml --root-tags platform -b 1000000 -P surfxml --sysid=simgrid.dtd -S $@ -L $^ 
$(top_srcdir)/include/surf/simgrid_dtd.h: $(srcdir)/surf/simgrid.dtd
	rm -f $(top_srcdir)/include/surf/simgrid.h 
	set -e;                flexml --root-tags platform            -P surfxml --sysid=simgrid.dtd -H $@ -L $^ 
	if test -e $(top_srcdir)/include/surf/simgrid.h ; then mv $(top_srcdir)/include/surf/simgrid.h $@ ; fi
	sed 's/extern  *\([^ ]*[ \*]*\)/XBT_PUBLIC_DATA(\1) /' -i $@
	sed 's/XBT_PUBLIC_DATA(\([^)]*\)) *\([^(]*\)(/XBT_PUBLIC(\1) \2(/' -i $@

xbt/graphxml.l: $(srcdir)/xbt/graphxml.dtd
	set -e; flexml -b 1000000 -P graphxml --sysid=graphxml.dtd -S $@ -L $^
$(top_srcdir)/include/xbt/graphxml.h: $(srcdir)/xbt/graphxml.dtd
	set -e; flexml            -P graphxml --sysid=graphxml.dtd -H $@ -L $^
	sed 's/extern  *\([^ ]*[ \*]*\)/XBT_PUBLIC_DATA(\1) /' -i $@
	sed 's/XBT_PUBLIC_DATA(\([^)]*\)) *\([^(]*\)(/XBT_PUBLIC(\1) \2(/' -i $@
else

$(top_srcdir)/include/surf/simgrid_dtd.h surf/simgrid_dtd.l: $(top_srcdir)/src/surf/simgrid.dtd
	@echo "ERROR: src/surf/simgrid.dtd was modified, but the flexml program was not detected"
	@echo "ERROR: Please install it, or if you didn't modify this file, try this:"
	@echo "ERROR:   touch include/surf/simgrid_dtd.h src/surf/simgrid_dtd.l"
	@exit 1
$(top_srcdir)/include/xbt/graphxml.h xbt/graphxml.l: $(top_srcdir)/src/xbt/graphxml.dtd
	@echo "ERROR: src/xbt/graphxml.dtd was modified, but the flexml program was not detected"
	@echo "ERROR: Please install it, or if you didn't modify this file, try this:"
	@echo "ERROR:   touch include/xbt/graphxml.h src/xbt/graphxml.l"
	@exit 1
endif
endif
endif

include $(top_srcdir)/acmacro/dist-files.mk
