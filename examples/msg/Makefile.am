INCLUDES  = -I$(top_srcdir)/include
AM_CFLAGS = -g

EXTRA_DIST = msg_platform.xml \
             small_platform.xml \
             sendrecv/platform_sendrecv.xml \
             sendrecv/deployment_sendrecv.xml \
             suspend/deployment_suspend.xml \
             masterslave/deployment_masterslave_forwarder.xml \
             masterslave/deployment_masterslave.xml \
             gtnets/r-n200-f50-s4-2-d.xml \
             gtnets/onelink-p.xml \
             gtnets/onelink-d.xml \
             gtnets/r-n200-f50-s4-2-p.xml \
             gtnets/dogbone-d.xml \
             gtnets/dogbone-p.xml


TESTS_ENVIRONMENT = $(top_builddir)/tools/tesh/tesh


TESTS = sendrecv/sendrecv_KCCFLN05.tesh \
        sendrecv/sendrecv_Vegas.tesh \
        sendrecv/sendrecv_Reno.tesh \
        suspend/suspend.tesh \
	masterslave/masterslave.tesh \
	masterslave/masterslave_forwarder.tesh
        #masterslave/masterslave_bypass.tesh

if HAVE_SDP
  BROKEN_TESTS = sendrecv/sendrecv_SDP.tesh
endif
if HAVE_GTNETS
endif

CLEANFILES = sendrecv/*~ \
             sendrecv/sendrecv \
             suspend/*~ \
             suspend/suspend \
             masterslave/*~ \
             masterslave/masterslave \
             masterslave/masterslave_forwarder 
             #masterslave/masterslave_bypass

if HAVE_GTNETS
             CLEANFILES += gtnets/gtnets 
endif




bin_PROGRAMS = sendrecv/sendrecv \
               suspend/suspend \
               masterslave/masterslave \
               masterslave/masterslave_forwarder \
               masterslave/masterslave_failure
               #masterslave/masterslave_bypass

if HAVE_GTNETS
               bin_PROGRAMS += gtnets/gtnets 
endif



# sendrecv simple example
sendrecv_sendrecv_SOURCES = sendrecv/sendrecv.c
sendrecv_sendrecv_LDADD   = $(top_builddir)/src/libsimgrid.la

# suspend/resume simple example
suspend_suspend_SOURCES = suspend/suspend.c
suspend_suspend_LDADD   = $(top_builddir)/src/libsimgrid.la

# master/slave application example
masterslave_masterslave_SOURCES = masterslave/masterslave.c
masterslave_masterslave_LDADD   = $(top_builddir)/src/libsimgrid.la

# master/slave application example using a forwarder module
masterslave_masterslave_forwarder_SOURCES = masterslave/masterslave_forwarder.c
masterslave_masterslave_forwarder_LDADD   = $(top_builddir)/src/libsimgrid.la

# master/slave application example with failures
masterslave_masterslave_failure_SOURCES = masterslave/masterslave_failure.c
masterslave_masterslave_failure_LDADD   = $(top_builddir)/src/libsimgrid.la

# bypass the surf xml parser
#masterslave_masterslave_bypass_SOURCES = masterslave/masterslave_bypass.c
#masterslave_masterslave_bypass_LDADD   = $(top_builddir)/src/libsimgrid.la

# verify if the GTNETS feature is working
if HAVE_GTNETS
gtnets_gtnets_SOURCES = gtnets/gtnets.c
gtnets_gtnets_LDADD   = $(top_builddir)/src/libsimgrid.la
else
  EXTRA_DIST+= gtnets/gtnets.c
endif

include $(top_srcdir)/acmacro/dist-files.mk
