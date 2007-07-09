EXTRA_DIST=msg_test_suspend_deployment.xml msg_test_suspend_platform.xml surfxml.dtd

# Declare sources:
noinst_JAVA= DreamMaster.java LazyGuy.java SuspendTest.java

# Declare test class
#if HAVE_JAVA
TESTS=SuspendTest.class
#endif

include $(srcdir)/../java_test.mk

# Cruft
include $(top_srcdir)/acmacro/dist-files.mk
