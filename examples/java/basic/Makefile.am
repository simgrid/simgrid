# Copyright (c) 2004-2007. The SimGrid team. All right reserved.

# This file is part of the SimGrid project. This is free software:
# You can redistribute and/or modify it under the terms of the
# GNU LGPL (v2.1) licence.


EXTRA_DIST=basic_deployment.xml basic_platform.xml

# Declare sources:
JAVA_SRC=Master.java Forwarder.java Slave.java BasicTest.java \
         FinalizeTask.java
JAVA_TESTS=BasicTest.class
TESTS=$(JAVA_TESTS)

EXTRA_DIST+=$(JAVA_SRC)

# Declare test class
if HAVE_JAVA
  noinst_JAVA=$(JAVA_SRC)
endif

include $(srcdir)/../java_test.mk

# Cruft
include $(top_srcdir)/acmacro/dist-files.mk
