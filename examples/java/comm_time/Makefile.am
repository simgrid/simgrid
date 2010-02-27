# Copyright (c) 2004-2007. The SimGrid team. All right reserved.

# This file is part of the SimGrid project. This is free software:
# You can redistribute and/or modify it under the terms of the
# GNU LGPL (v2.1) licence.


EXTRA_DIST=comm_time_deployment.xml comm_time_platform.xml

# Declare sources:
JAVA_SRC=Master.java Slave.java CommTimeTest.java \
         MyTask.java FinalizeTask.java
JAVA_TESTS=CommTimeTest.class
TESTS=$(JAVA_TESTS)

EXTRA_DIST+=$(JAVA_SRC)

# Declare test class
if HAVE_JAVA
  noinst_JAVA=$(JAVA_SRC)
endif

include $(srcdir)/../java_test.mk

# Cruft
include $(top_srcdir)/acmacro/dist-files.mk
