SUBDIRS= . amok

AM_CFLAGS= -g
#AM_CFLAGS= -DNDEBUG 

# -DNLOG   cuts absolutely all logs at compilation time.
# -DNDEBUG cuts asserts and logs at "trace" and "debug" levels.

# -g -ffast-math -funroll-loops -O3 -fno-strict-aliasing
# Those should be added by configure when using gcc
# fast-math is nasty when using IEEE floating point semantic
# strict-aliasing breaks my type-punning bad habit.


MAINTAINERCLEANFILES=Makefile.in
INCLUDES= -I$(top_srcdir)/include 
EXTRA_DIST= \
        \
	portable.h \
	\
        xbt/dict_private.h \
        xbt/heap_private.h \
        xbt/fifo_private.h \
        xbt/context_private.h xbt/context_win32.h xbt/context_win32.c\
        \
	surf/maxmin_private.h \
	surf/trace_mgr_private.h \
	surf/surf_private.h \
	surf/cpu_private.h \
	surf/workstation_private.h \
	surf/surf_parse.c \
	surf/surfxml.l \
	surf/surfxml.c \
	surf/network_private.h \
	\
	msg/private.h \
	\
	gras/Transport/transport_interface.h \
	gras/Virtu/virtu_interface.h \
	gras/Virtu/virtu_rl.h                 gras/Virtu/virtu_sg.h \
	gras/DataDesc/ddt_parse.yy.l          gras/DataDesc/ddt_parse.yy.c

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

VERSION_INFO= -release 20040722 -version-info 0:0:0
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

COMMON_S=\
  \
  xbt_modinter.h    gras_modinter.h                                            \
  \
  xbt/sysdep.c                                                                 \
  xbt/log.c         xbt/log_default_appender.c   xbt/error.c                 \
  xbt/dynar.c                                                                  \
  xbt/dict.c        xbt/dict_elm.c               xbt/dict_cursor.c           \
  xbt/heap.c                                                                    \
  xbt/fifo.c                                                                    \
  xbt/swag.c                                                                   \
  xbt/set.c                                                                    \
  xbt/module.c                                                                 \
  xbt/config.c                                                                 \
  xbt/context.c                                                                \
  \
  surf/maxmin.c                                                              \
  surf/trace_mgr.c                                                           \
  surf/surf.c                                                                \
  surf/surf_parse.c                                                          \
  surf/cpu.c   surf/network.c   surf/workstation.c                           \
  \
  msg/task.c msg/host.c msg/m_process.c msg/gos.c \
  msg/global.c msg/environment.c msg/deployment.c  \
  \
  gras/gras.c \
  \
  gras/Transport/transport.c          gras/Transport/transport_private.h   gras/Transport/transport_plugin_buf.c  \
  \
  gras/DataDesc/ddt_create.c          \
  gras/DataDesc/ddt_convert.c         gras/DataDesc/ddt_exchange.c     \
  gras/DataDesc/cbps.c                gras/DataDesc/datadesc.c         \
  gras/DataDesc/datadesc_interface.h  gras/DataDesc/datadesc_private.h \
  gras/DataDesc/ddt_parse.c           gras/DataDesc/ddt_parse.yy.c         gras/DataDesc/ddt_parse.yy.h \
  \
  gras/Msg/msg.c                      gras/Msg/msg_interface.h             gras/Msg/msg_private.h \
  \
  gras/Virtu/process.c

gras/DataDesc/ddt_parse.yy.c: gras/DataDesc/ddt_parse.yy.l
	@LEX@ -o$@ -Pgras_ddt_parse_ $^

surf/surf_parse.c: surf/surfxml.c

surf/surfxml.c: surf/surfxml.l
	@LEX@ -o$@ -Psurf_parse_ $^

surf/surfxml.l: surf/surfxml.dtd
	cd surf; flexml -b 10000000 -S -L surfxml.dtd ; \
	mv surfxml.h surfxml.h.bak ; \
	sed 's/surfxml\.h/surf\/surfxml\.h/g' surfxml.h.bak > surfxml.h


libgras_la_SOURCES= $(COMMON_S) \
  gras/Transport/rl_transport.c  gras/Transport/transport_plugin_tcp.c  gras/Transport/transport_plugin_file.c      \
  \
  gras/Virtu/rl_process.c        gras/Virtu/rl_time.c                   gras/Virtu/rl_conditional.c
libgras_la_LDFLAGS = $(VERSION_INFO) @GRAS_DEP@

libsimgrid_la_SOURCES= $(COMMON_S) \
  gras/Transport/sg_transport.c  gras/Transport/transport_plugin_sg.c             \
  \
  gras/Virtu/sg_process.c        gras/Virtu/sg_time.c                 gras/Virtu/sg_conditional.c
libsimgrid_la_LDFLAGS = $(VERSION_INFO) @SIMGRID_DEP@
