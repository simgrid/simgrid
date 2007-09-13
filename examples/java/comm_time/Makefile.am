EXTRA_DIST=comm_time_deployment.xml comm_time_platform.xml

# Declare sources:
noinst_JAVA= Master.java Slave.java CommTimeTest.java CommTimeTask.java

#if HAVE_JAVA
 # Declare test class
 TESTS=CommTimeTest.class
#endif

include $(srcdir)/../java_test.mk

# Cruft
include $(top_srcdir)/acmacro/dist-files.mk
