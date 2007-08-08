CLASSPATH=../../../src/simgrid.jar
EXTRA_DIST=basic_deployment.xml basic_platform.xml surfxml.dtd

# Declare sources:
noinst_JAVA= Master.java Forwarder.java Slave.java BasicTest.java BasicTask.java

# Declare test class
TESTS=BasicTest.class

include $(srcdir)/../java_test.mk

# Cruft
include $(top_srcdir)/acmacro/dist-files.mk
