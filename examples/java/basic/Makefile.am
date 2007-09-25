EXTRA_DIST=basic_deployment.xml basic_platform.xml

# Declare sources:
JAVA_SRC=Master.java Forwarder.java Slave.java BasicTest.java BasicTask.java
JAVA_TESTS=BasicTest.class

# Declare test class
if HAVE_JAVA
noinst_JAVA=$(JAVA_SRC)
TESTS=$(JAVA_TESTS)
else
EXTRA_DIST+=$(JAVA_SRC)
EXTRA_DIST+=$(JAVA_TESTS)
endif

include $(srcdir)/../java_test.mk

# Cruft
include $(top_srcdir)/acmacro/dist-files.mk
