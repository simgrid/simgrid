EXTRA_DIST=comm_time_deployment.xml comm_time_platform.xml

# Declare sources:
JAVA_SRC=Master.java Slave.java CommTimeTest.java CommTimeTask.java
JAVA_TESTS=CommTimeTest.class

EXTRA_DIST+=$(JAVA_SRC)

# Declare test class
if HAVE_JAVA
  noinst_JAVA=$(JAVA_SRC)
  TESTS=$(JAVA_TESTS)
endif

include $(srcdir)/../java_test.mk

# Cruft
include $(top_srcdir)/acmacro/dist-files.mk
