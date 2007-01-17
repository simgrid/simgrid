AM_CFLAGS= -g
#AM_CFLAGS= -DNDEBUG 

# -DNLOG   cuts absolutely all logs at compilation time.
# -DNDEBUG cuts asserts and logs at "trace" and "debug" levels.

# -g -ffast-math -funroll-loops -O3 -fno-strict-aliasing
# Those should be added by configure when using gcc
# fast-math is nasty when using IEEE floating point semantic
# strict-aliasing breaks my type-punning bad habit.


MAINTAINERCLEANFILES=Makefile.in
INCLUDES= -I$(top_srcdir)/include -I$(top_srcdir)/src/include 
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
        xbt/context_private.h xbt/context_win32.h xbt/context_win32.c\
	xbt/ex_interface.h \
        \
	surf/maxmin_private.h \
	surf/trace_mgr_private.h \
	surf/surf_private.h \
	surf/cpu_private.h \
	surf/workstation_private.h \
	surf/surf_timer_private.h \
	surf/surfxml_parse.c \
	surf/surfxml.l \
	surf/surfxml.c \
	surf/surfxml.dtd \
	surf/network_private.h \
	surf/network_dassf_private.h \
	surf/workstation_KCCFLN05_private.h \
	\
	include/surf/maxmin.h \
	include/surf/trace_mgr.h \
	include/surf/surf.h \
	include/surf/surfxml_parse_private.h \
	\
	include/xbt/xbt_portability.h \
	include/xbt/context.h \
	\
	msg/private.h \
	\
	simdag/private.h \
	\
	gras/Transport/transport_interface.h \
	gras/Virtu/virtu_interface.h          gras/Virtu/virtu_private.h\
	gras/Virtu/virtu_rl.h                 gras/Virtu/virtu_sg.h \
	gras/DataDesc/ddt_parse.yy.l          gras/DataDesc/ddt_parse.yy.c \
	\
	amok/Bandwidth/bandwidth_private.h \
	amok/amok_modinter.h

#        gras_private.h

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

VERSION_INFO= -version-info 0:0:0
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

lib_LTLIBRARIES= libsimgrid.la libgras.la

COMMON_SRC=\
  \
  xbt/snprintf.c                                                             \
  xbt/ex.c                                                                   \
  \
  xbt_modinter.h    gras_modinter.h                                          \
  \
  xbt/sysdep.c                                                               \
  xbt/asserts.c                                                              \
  xbt/log.c         xbt/log_default_appender.c                               \
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
  xbt/xbt_peer.c                                                             \
  xbt/xbt_main.c                                                             \
  xbt/config.c                                                               \
  xbt/cunit.c                                                                \
  xbt/graphxml_parse.c                                                       \
  \
  gras/gras.c \
  \
  gras/Transport/transport.c          gras/Transport/transport_private.h  \
  \
  gras/DataDesc/ddt_create.c          \
  gras/DataDesc/ddt_convert.c         gras/DataDesc/ddt_exchange.c     \
  gras/DataDesc/cbps.c                gras/DataDesc/datadesc.c         \
  gras/DataDesc/datadesc_interface.h  gras/DataDesc/datadesc_private.h \
  gras/DataDesc/ddt_parse.c           gras/DataDesc/ddt_parse.yy.c         gras/DataDesc/ddt_parse.yy.h \
  \
  gras/Msg/msg.c                      gras/Msg/rpc.c                       gras/Msg/timer.c                 \
  gras/Msg/msg_interface.h            gras/Msg/msg_private.h           \
  \
  gras/Virtu/process.c gras/Virtu/gras_module.c

RL_SRC= \
  gras/Transport/rl_transport.c          \
  gras/Transport/transport_plugin_file.c   gras/Transport/transport_plugin_tcp.c  \
  \
  gras/Virtu/rl_emul.c \
  gras/Virtu/rl_process.c        gras/Virtu/rl_time.c \
  gras/Virtu/rl_dns.c\
  \
  gras/Msg/rl_msg.c

SG_SRC=  \
  xbt/context.c                                                              \
  \
  surf/maxmin.c                                                              \
  surf/sdp.c                                                                 \
  surf/trace_mgr.c                                                           \
  surf/surf.c                                                                \
  surf/surfxml_parse.c                                                       \
  surf/cpu.c   surf/network.c   surf/workstation.c                           \
  surf/surf_timer.c                                                          \
  surf/network_dassf.c                                                       \
  surf/workstation_KCCFLN05.c  \
  \
  msg/msg_config.c \
  msg/task.c msg/host.c msg/m_process.c msg/gos.c \
  msg/global.c msg/environment.c msg/deployment.c  \
  \
  simdag/sd_global.c simdag/sd_link.c simdag/sd_task.c                       \
  simdag/sd_workstation.c                                                    \
  \
  gras/Transport/sg_transport.c  gras/Transport/transport_plugin_sg.c        \
  \
  gras/Virtu/sg_emul.c \
  gras/Virtu/sg_process.c        gras/Virtu/sg_time.c     \
  gras/Virtu/sg_dns.c\
  \
  gras/Msg/sg_msg.c

AMOK_SRC= \
  amok/amok_base.c \
  amok/Bandwidth/bandwidth.c amok/Bandwidth/saturate.c \
  amok/PeerManagement/peermanagement.c

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
            xbt/config.c
TEST_UNITS= cunit_unit.c ex_unit.c         \
            dynar_unit.c dict_unit.c set_unit.c swag_unit.c \
            config_unit.c

BUILT_SOURCES=../include/surf/surfxml.h surf/surfxml.c \
              ../include/xbt/graphxml.h xbt/graphxml.c \
              gras/DataDesc/ddt_parse.yy.c \
              $(TEST_UNITS) simgrid_units_main.c

testall_SOURCES= $(TEST_UNITS) simgrid_units_main.c
testall_LDADD=libgras.la
TESTS=testall

EXTRA_DIST+=$(testall_SOURCES)

if MAINTAINER_MODE
cunit_unit.c: xbt/cunit.c
	@top_srcdir@/tools/sg_unit_extractor.pl $^
ex_unit.c: xbt/ex.c
	@top_srcdir@/tools/sg_unit_extractor.pl $^
dynar_unit.c: xbt/dynar.c
	@top_srcdir@/tools/sg_unit_extractor.pl $^
dict_unit.c: xbt/dict.c
	@top_srcdir@/tools/sg_unit_extractor.pl $^
set_unit.c: xbt/set.c
	@top_srcdir@/tools/sg_unit_extractor.pl $^
swag_unit.c: xbt/swag.c
	@top_srcdir@/tools/sg_unit_extractor.pl $^
config_unit.c: xbt/config.c
	@top_srcdir@/tools/sg_unit_extractor.pl $^

simgrid_units_main.c: $(TEST_UNITS)
	@top_srcdir@/tools/sg_unit_extractor.pl xbt/cunit.c

clean-units:
	rm -f simgrid_units_main.c *_unit.c

else

$(TEST_UNITS) simgrid_units_main.c clean-units: 
	@echo "ERROR: Test units not generated."
	@echo "ERROR: If you are using a CVS checkout, configure with the --enable-maintainer-mode flag"
	@echo "ERROR: If not, please report the bug to the simgrid-devel mailing list."
	@exit 1

endif

###
### Regenerate what needs to with flex & flexml
###


gras/DataDesc/ddt_parse.yy.c: gras/DataDesc/ddt_parse.yy.l
	set -e;@LEX@ -o$@ -Pgras_ddt_parse_ --noline $^

if MAINTAINER_MODE
surf/surfxml.c: surf/surfxml.l
	set -e;mkdir -p `dirname $@`; @LEX@ -o$@ -Psurf_parse_ --noline $^
xbt/graphxml.c: xbt/graphxml.l
	set -e;mkdir -p `dirname $@`; @LEX@ -o$@ -Pxbt_graph_parse_ --noline $^

if HAVE_FLEXML
surf/surfxml.l: $(srcdir)/surf/surfxml.dtd
	set -e; mkdir -p surf; flexml -b 1000000 -P surfxml --sysid=surfxml.dtd -S $@ -L $^
$(top_srcdir)/include/surf/surfxml.h: $(srcdir)/surf/surfxml.dtd
	set -e;                flexml            -P surfxml --sysid=surfxml.dtd -H $@ -L $^

xbt/graphxml.l: $(srcdir)/xbt/graphxml.dtd
	set -e; flexml -b 1000000 -P graphxml --sysid=graphxml.dtd -S $@ -L $^
$(top_srcdir)/include/xbt/graphxml.h: $(srcdir)/xbt/graphxml.dtd
	set -e; flexml            -P graphxml --sysid=graphxml.dtd -H $@ -L $^
else

$(top_srcdir)/include/surf/surfxml.h surf/surfxml.l: $(top_srcdir)/src/surf/surfxml.dtd
	@echo "ERROR: src/surf/surfxml.dtd was modified, but the flexml program was not detected"
	@echo "ERROR: Please install it, or if you didn't modify this file, try this:"
	@echo "ERROR:   touch include/surf/surfxml.h src/surf/surfxml.l"
	@exit 1
$(top_srcdir)/include/xbt/graphxml.h xbt/graphxml.l: $(top_srcdir)/src/xbt/graphxml.dtd
	@echo "ERROR: src/xbt/graphxml.dtd was modified, but the flexml program was not detected"
	@echo "ERROR: Please install it, or if you didn't modify this file, try this:"
	@echo "ERROR:   touch include/xbt/graphxml.h src/xbt/graphxml.l"
	@exit 1
endif
endif
endif

###
### Declare the library content
###

libgras_la_SOURCES= $(COMMON_SRC) $(RL_SRC) $(AMOK_SRC)
libgras_la_LDFLAGS = -no-undefined $(VERSION_INFO) @GRAS_DEP@ @LD_DYNAMIC_FLAGS@ -lm 

libsimgrid_la_SOURCES= $(COMMON_SRC) $(SG_SRC) $(AMOK_SRC)
libsimgrid_la_LDFLAGS = -no-undefined $(VERSION_INFO) @SIMGRID_DEP@ @LD_DYNAMIC_FLAGS@ -lm



include $(top_srcdir)/acmacro/dist-files.mk
