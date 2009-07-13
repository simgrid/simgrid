# Copyright (c) 2004-2007. The SimGrid team. All right reserved.

# This file is part of the SimGrid project. This is free software:
# You can redistribute and/or modify it under the terms of the
# GNU LGPL (v2.1) licence.


INCLUDES  = -I$(top_srcdir)/include
AM_CFLAGS = -g

EXTRA_DIST = msg_platform.xml \
             small_platform.xml \
             small_platform_with_failures.xml \
             jupiter_state.trace link5_state.trace \
             sendrecv/platform_sendrecv.xml \
             sendrecv/deployment_sendrecv.xml \
             suspend/deployment_suspend.xml \
             masterslave/deployment_masterslave_forwarder.xml \
             masterslave/deployment_masterslave.xml \
             migration/migration.deploy \
             gtnets/r-n200-f50-s4-2-d.xml \
             gtnets/onelink-p.xml \
             gtnets/onelink-d.xml \
             gtnets/r-n200-f50-s4-2-p.xml \
             gtnets/dogbone-d.xml \
             gtnets/dogbone-p.xml \
             parallel_task/test_ptask_deployment.xml \
             parallel_task/test_ptask_platform.xml \
             priority/deployment_priority.xml \
             properties/deployment_properties.xml


TESTS_ENVIRONMENT = $(top_builddir)/tools/tesh/tesh


TESTS = sendrecv/sendrecv_CLM03.tesh \
        sendrecv/sendrecv_Vegas.tesh \
        sendrecv/sendrecv_Reno.tesh \
        suspend/suspend.tesh \
	masterslave/masterslave.tesh \
	masterslave/masterslave_forwarder.tesh \
	masterslave/masterslave_failure.tesh \
        masterslave/masterslave_bypass.tesh \
        migration/migration.tesh \
        parallel_task/parallel_task.tesh \
        priority/priority.tesh \
	properties/msg_prop.tesh

if HAVE_GTNETS
  TESTS += gtnets/gtnets.tesh
endif

CLEANFILES = sendrecv/*~ \
             sendrecv/sendrecv \
             suspend/*~ \
             suspend/suspend \
             masterslave/*~ \
             masterslave/masterslave_forwarder \
             masterslave/masterslave_failure \
             masterslave/masterslave_bypass \
             parallel_task/parallel_task \
             parallel_task/test_ptask \
             priority/priority \
             properties/msg_prop

if HAVE_GTNETS
             CLEANFILES += gtnets/gtnets gtnets/gtnets_kayo
endif




noinst_PROGRAMS = sendrecv/sendrecv \
                  suspend/suspend \
                  masterslave/masterslave_forwarder \
                  masterslave/masterslave_failure \
                  masterslave/masterslave_bypass \
                  migration/migration \
                  parallel_task/parallel_task \
                  parallel_task/test_ptask \
                  priority/priority \
                  properties/msg_prop \
                  actions/actions

if HAVE_GTNETS
  noinst_PROGRAMS += gtnets/gtnets gtnets/gtnets_kayo
endif


# property handle example
properties_msg_prop_SOURCES = properties/msg_prop.c
properties_msg_prop_LDADD   = $(top_builddir)/src/libsimgrid.la

# actions example
actions_actions_SOURCES = actions/actions.c
actions_actions_LDADD   = $(top_builddir)/src/libsimgrid.la

# migration example
migration_migration_SOURCES = migration/migration.c
migration_migration_LDADD   = $(top_builddir)/src/libsimgrid.la


# sendrecv simple example
sendrecv_sendrecv_SOURCES = sendrecv/sendrecv.c
sendrecv_sendrecv_LDADD   = $(top_builddir)/src/libsimgrid.la

# suspend/resume simple example
suspend_suspend_SOURCES = suspend/suspend.c
suspend_suspend_LDADD   = $(top_builddir)/src/libsimgrid.la

# parallel task example
parallel_task_parallel_task_SOURCES = parallel_task/parallel_task.c
parallel_task_parallel_task_LDADD   = $(top_builddir)/src/libsimgrid.la
parallel_task_test_ptask_SOURCES = parallel_task/test_ptask.c
parallel_task_test_ptask_LDADD   = $(top_builddir)/src/libsimgrid.la

# playing with priorities example
priority_priority_SOURCES = priority/priority.c
priority_priority_LDADD   = $(top_builddir)/src/libsimgrid.la

# master/slave application example using a forwarder module
masterslave_masterslave_forwarder_SOURCES = masterslave/masterslave_forwarder.c
masterslave_masterslave_forwarder_LDADD   = $(top_builddir)/src/libsimgrid.la

# master/slave application example with failures
masterslave_masterslave_failure_SOURCES = masterslave/masterslave_failure.c
masterslave_masterslave_failure_LDADD   = $(top_builddir)/src/libsimgrid.la

# bypass the surf xml parser
masterslave_masterslave_bypass_SOURCES = masterslave/masterslave_bypass.c
masterslave_masterslave_bypass_LDADD   = $(top_builddir)/src/libsimgrid.la

# verify if the GTNETS feature is working
if HAVE_GTNETS
gtnets_gtnets_SOURCES  = gtnets/gtnets.c
gtnets_gtnets_kayo_SOURCES  = gtnets/gtnets_kayo.c
gtnets_gtnets_LDADD   = $(top_builddir)/src/libsimgrid.la
gtnets_gtnets_kayo_LDADD   = $(top_builddir)/src/libsimgrid.la
else
  EXTRA_DIST+= gtnets/gtnets.c gtnets/gtnets_kayo.c
endif

EXTRA_DIST+=$(TESTS)
include $(top_srcdir)/acmacro/dist-files.mk
