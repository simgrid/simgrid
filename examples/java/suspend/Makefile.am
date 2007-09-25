EXTRA_DIST=suspend_deployment.xml suspend_platform.xml

# Declare sources:
JAVA_SRC=DreamMaster.java LazyGuy.java SuspendTest.java
JAVA_TESTS=SuspendTest.class

EXTRA_DIST+=$(JAVA_SRC)

# Declare test class
if HAVE_JAVA
noinst_JAVA=$(JAVA_SRC)
TESTS=$(JAVA_TESTS)
else
EXTRA_DIST+=$(JAVA_TESTS)
endif

include $(srcdir)/../java_test.mk

# Cruft
include $(top_srcdir)/acmacro/dist-files.mk
