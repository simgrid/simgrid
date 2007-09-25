EXTRA_DIST=ping_pong_deployment.xml ping_pong_platform.xml

# Declare sources:
JAVA_SRC=Sender.java Receiver.java PingPongTask.java PingPongTest.java
JAVA_TESTS=PingPongTest.class

EXTRA_DIST+=$(JAVA_SRC)

# Declare test class
if HAVE_JAVA
  noinst_JAVA=$(JAVA_SRC)
  TESTS=$(JAVA_TESTS)
endif

include $(srcdir)/../java_test.mk

# Cruft
include $(top_srcdir)/acmacro/dist-files.mk
