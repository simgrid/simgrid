INCLUDES  = -I$(top_srcdir)/include
AM_CFLAGS = -g

EXTRA_DIST = msg_platform.xml \
             sendrecv/platform_sendrecv.xml \
             sendrecv/deployment_sendrecv.xml

TESTS_ENVIRONMENT = $(top_builddir)/tools/tesh/tesh

TESTS = sendrecv/sendrecv_KCCFLN05.tesh
if HAVE_SDP
  TESTS += sendrecv/sendrecv_KCCFLN05_proportional.tesh
endif
TESTS+= sendrecv/sendrecv_KCCFLN05_Vegas.tesh \
        sendrecv/sendrecv_KCCFLN05_Reno.tesh \
        suspend/suspend.tesh \
	masterslave/masterslave.tesh \
	masterslave/masterslave_forwarder.tesh
        #masterslave/masterslave_bypass.tesh

CLEANFILES = sendrecv/*~ \
             sendrecv/sendrecv \
             suspend/*~ \
             suspend/suspend \
             masterslave/*~ \
             masterslave/masterslave \
             masterslave/masterslave_forwarder
             #masterslave/masterslave_bypass

bin_PROGRAMS = sendrecv/sendrecv \
               suspend/suspend \
               masterslave/masterslave \
               masterslave/masterslave_forwarder
               #masterslave/masterslave_bypass

# sendrecv simple example
sendrecv_sendrecv_SOURCES = sendrecv/sendrecv.c
sendrecv_sendrecv_LDADD   = $(top_builddir)/src/libsimgrid.la

# suspend/resume simple example
suspend_suspend_SOURCES = suspend/suspend.c
suspend_suspend_LDADD   = $(top_builddir)/src/libsimgrid.la

# master/slave application example using a forwarder module
masterslave_masterslave_SOURCES = masterslave/masterslave.c
masterslave_masterslave_LDADD   = $(top_builddir)/src/libsimgrid.la

# verify the communication time of a simple simulation
masterslave_masterslave_forwarder_SOURCES = masterslave/masterslave_forwarder.c
masterslave_masterslave_forwarder_LDADD   = $(top_builddir)/src/libsimgrid.la

# bypass the surf xml parser
#masterslave_masterslave_bypass_SOURCES = masterslave/masterslave_bypass.c
#masterslave_masterslave_bypass_LDADD   = $(top_builddir)/src/libsimgrid.la

include $(top_srcdir)/acmacro/dist-files.mk
