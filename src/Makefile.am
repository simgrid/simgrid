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
INCLUDES= -I$(top_srcdir)/include \
          @CFLAGS_SimGrid@
EXTRA_DIST= \
        \
        xbt/dict_private.h \
        \
	gras/Transport/transport_interface.h \
	gras/Virtu/virtu_interface.h \
	gras/Virtu/virtu_rl.h \
	gras/Virtu/virtu_sg.h \
	gras/DataDesc/ddt_parse.yy.l gras/DataDesc/ddt_parse.yy.c

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

if HAVE_SG
 lib_LTLIBRARIES= libgrasrl.la libgrassg.la
else
 lib_LTLIBRARIES= libgrasrl.la
endif


COMMON_S=\
  \
  xbt_modinter.h    gras_modinter.h                                            \
  \
  xbt/sysdep.c                                                                 \
  xbt/log.c         xbt/log_default_appender.c   xbt/error.c                 \
  xbt/dynar.c                                                                  \
  xbt/dict.c        xbt/dict_elm.c               xbt/dict_cursor.c           \
  xbt/set.c                                                                    \
  xbt/module.c                                                                 \
  xbt/config.c                                                                 \
  \
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

libgrasrl_la_SOURCES= $(COMMON_S) \
  gras/Transport/rl_transport.c  gras/Transport/transport_plugin_tcp.c  gras/Transport/transport_plugin_file.c      \
  \
  gras/Virtu/rl_process.c        gras/Virtu/rl_time.c                   gras/Virtu/rl_conditional.c
libgrasrl_la_LDFLAGS = $(VERSION_INFO)

if HAVE_SG
  libgrassg_la_SOURCES= $(COMMON_S) \
    gras/Transport/sg_transport.c  gras/Transport/transport_plugin_sg.c             \
    \
    gras/Virtu/sg_process.c        gras/Virtu/sg_time.c                 gras/Virtu/sg_conditional.c
  libgrassg_la_LDFLAGS = $(VERSION_INFO)
endif
