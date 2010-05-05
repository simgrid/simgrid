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
             gtnets/onelink-p.xml \
             gtnets/onelink-d.xml \
             gtnets/dogbone-d.xml \
             gtnets/dogbone-p.xml \
             gtnets/waxman-d.xml \
             gtnets/waxman-p.xml \
             parallel_task/test_ptask_deployment.xml \
             parallel_task/test_ptask_platform.xml \
             priority/deployment_priority.xml \
             properties/deployment_properties.xml \
             trace/test_trace_integration.xml \
             trace/file.trace \
             trace/test1.xml \
             trace/test2.xml \
             trace/test3.xml \
             trace/test4.xml \
             trace/test5.xml \
             trace/test6.xml \
             trace/test7.xml \
             trace/test8.xml \
             trace/test9.xml \
             trace/test10.xml \
             trace/test11.xml \
	     fafard_state.trace jupiter_state.trace link5_state.trace


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
	properties/msg_prop.tesh \
    trace/trace.tesh \
	masterslave/masterslave_cpu_ti.tesh

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
             properties/msg_prop \
             trace/test_trace_integration

if HAVE_GTNETS
             CLEANFILES += gtnets/gtnets
endif




noinst_PROGRAMS = sendrecv/sendrecv \
                  suspend/suspend \
                  masterslave/masterslave_mailbox \
                  masterslave/masterslave_forwarder \
                  masterslave/masterslave_failure \
                  masterslave/masterslave_bypass \
                  migration/migration \
                  parallel_task/parallel_task \
                  parallel_task/test_ptask \
                  priority/priority \
                  properties/msg_prop \
                  actions/actions \
                  trace/test_trace_integration \
		  mc/centralized \
		  mc/bugged1 \
		  mc/bugged2

if HAVE_GTNETS
  noinst_PROGRAMS += gtnets/gtnets
endif


# property handle example
properties_msg_prop_SOURCES = properties/msg_prop.c
properties_msg_prop_LDADD   = $(top_builddir)/src/libsimgrid.la

# actions example
actions_actions_SOURCES = actions/actions.c
actions_actions_LDADD   = $(top_builddir)/src/libsimgrid.la
actions_actions_CFLAGS   = -I$(top_builddir)/src/include

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

# master/slave application example using a mailboxes
masterslave_masterslave_mailbox_SOURCES = masterslave/masterslave_mailbox.c
masterslave_masterslave_mailbox_LDADD   = $(top_builddir)/src/libsimgrid.la

# master/slave application example with failures
masterslave_masterslave_failure_SOURCES = masterslave/masterslave_failure.c
masterslave_masterslave_failure_LDADD   = $(top_builddir)/src/libsimgrid.la

# bypass the surf xml parser
masterslave_masterslave_bypass_SOURCES = masterslave/masterslave_bypass.c
masterslave_masterslave_bypass_LDADD   = $(top_builddir)/src/libsimgrid.la

# bypass the surf xml parser
trace_test_trace_integration_SOURCES = trace/test_trace_integration.c
trace_test_trace_integration_LDADD   = $(top_builddir)/src/libsimgrid.la

# Model-checking examples
mc_centralized_SOURCES = mc/centralized_mutex.c
mc_centralized_LDADD = $(top_builddir)/src/libsimgrid.la

# bugged example 1
mc_bugged1_SOURCES = mc/bugged1.c
mc_bugged1_LDADD = $(top_builddir)/src/libsimgrid.la

# bugged example 2
mc_bugged2_SOURCES = mc/bugged2.c
mc_bugged2_LDADD = $(top_builddir)/src/libsimgrid.la


# verify if the GTNETS feature is working
if HAVE_GTNETS
gtnets_gtnets_SOURCES  = gtnets/gtnets.c
gtnets_gtnets_LDADD   = $(top_builddir)/src/libsimgrid.la
else
  EXTRA_DIST+= gtnets/gtnets.c
endif

EXTRA_DIST+=$(TESTS)
include $(top_srcdir)/acmacro/dist-files.mk
