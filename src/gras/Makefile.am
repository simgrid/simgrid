SUBDIRS=. Tests
AM_CFLAGS= -DNDEBUG 

# -DNLOG   cuts absolutely all logs at compilation time.
# -DNDEBUG cuts asserts and logs at "trace" and "debug" levels.

# -g -ffast-math -funroll-loops -O3 -fno-strict-aliasing
# Those should be added by configure when using gcc
# fast-math is nasty when using IEEE floating point semantic
# strict-aliasing breaks my type-punning bad habit.


MAINTAINERCLEANFILES=Makefile.in
INCLUDES= @CFLAGS_SimGrid@

EXTRA_DIST= ./gras_private.h    \
	Core/dict_private.h \
	Transport/transport_interface.h \
	Virtu/virtu_interface.h \
	Virtu/virtu_rl.h \
	Virtu/virtu_sg.h \
	DataDesc/ddt_parse.yy.l DataDesc/ddt_parse.yy.c

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

#  Core/config.c                               

COMMON_S=\
  \
  Core/module.c      Core/core_interface.h                                      \
  Core/log.c         Core/log_default_appender.c   Core/error.c                 \
  Core/dynar.c                                                                  \
  Core/dict.c        Core/dict_elm.c               Core/dict_cursor.c           \
  Core/set.c                                                                    \
  \
  \
  Transport/transport.c          Transport/transport_private.h   Transport/transport_plugin_buf.c  \
  \
  DataDesc/ddt_create.c          \
  DataDesc/ddt_convert.c         DataDesc/ddt_exchange.c     \
  DataDesc/cbps.c                DataDesc/datadesc.c         \
  DataDesc/datadesc_interface.h  DataDesc/datadesc_private.h \
  DataDesc/ddt_parse.c  DataDesc/ddt_parse.yy.c DataDesc/ddt_parse.yy.h \
  \
  Msg/msg.c   Msg/msg_interface.h   Msg/msg_private.h \
  \
  Virtu/process.c

# DataDesc/ddt_declare.c 

DataDesc/ddt_parse.yy.c: DataDesc/ddt_parse.yy.l
	@LEX@ -o$@ -Pgras_ddt_parse_ $^

libgrasrl_la_SOURCES= $(COMMON_S) \
  Transport/rl_transport.c  Transport/transport_plugin_tcp.c  Transport/transport_plugin_file.c      \
  \
  Virtu/rl_process.c        Virtu/rl_time.c                   Virtu/rl_conditional.c
libgrasrl_la_LDFLAGS = $(VERSION_INFO)

if HAVE_SG
  libgrassg_la_SOURCES= $(COMMON_S) \
    Transport/sg_transport.c  Transport/transport_plugin_sg.c             \
    \
    Virtu/sg_process.c        Virtu/sg_time.c                   Virtu/sg_conditional.c
  libgrassg_la_LDFLAGS = $(VERSION_INFO)
endif
