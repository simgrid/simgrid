SUBDIRS=. Tests
DISTCLEANFILES=Makefile.in
INCLUDES= -I$(top_srcdir)/src/include -I../nws_portability/Include  \
          @CFLAGS_XML@ @CFLAGS_SimGrid@
EXTRA_DIST=gs/parse.yy.l


lib_LIBRARIES= libgrasrl.a libgrassg.a

noinst_LIBRARIES=libgrasutils.a

#                     core/dict_multi.c

COMMON_S=\
  Common/gras.c      Common/gras_datadesc.c        Common/gras_msg.c            \
  \
  core/log.c         core/log_default_appender.c   core/error.c                 \
  core/dynar.c                                                                  \
  core/dict.c        core/dict_elm.c               core/dict_cursor.c           \
  \
  core/config.c                               \
  \
  gs/datadesc.c                               \
  gs/tools.c                                  \
  gs/categories.h                             \
  gs/connection.h                             \
  gs/sequence.h                               \
  gs/net_driver.c      gs/net_driver.h        \
  gs/net_driver_fd.c   gs/net_interface_fd.h  \
  gs/type_driver_rl.c  gs/type_interface_rl.h \
  gs/type_driver.c     gs/type_driver.h       \
  gs/message.c         gs/message.h           \
  gs/type.c            gs/type.h              \
  gs/parse.c           gs/parse.h             gs/parse.yy.c \
  gs/vars.c

gs/parse.yy.c: gs/parse.yy.l
	flex -o$@ -Pgs_parse_ $^

COMMON_L=\
 $(foreach file,\
  diagnostic formatutil strutil osutil, \
  $(top_srcdir)/src/nws_portability/build-@host@/obj/$(file).o)

libgrasutils_a_SOURCES = $(COMMON_S) Tests/gras_dummy.c
libgrasutils_a_LIBADD = $(COMMON_L)


libgrasrl_a_SOURCES= $(COMMON_S) RL/gras_rl.c RL/gras_rl.h
libgrasrl_a_LIBADD= \
 $(foreach file,\
  exp_smooth forc median mse_forc protocol \
  dnsutil fbuff forecast_api last_value run_mean timeouts, \
  $(top_srcdir)/src/nws_portability/build-@host@/obj/$(file).o) \
 $(COMMON_L)

	
libgrassg_a_SOURCES= $(COMMON_S) SG/gras_sg.c SG/gras_sg.h
libgrassg_a_LIBADD= \
 $(COMMON_L)


