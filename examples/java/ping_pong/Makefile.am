EXTRA_DIST=ping_pong_deployment.xml ping_pong_platform.xml surfxml.dtd

# Declare sources:
noinst_JAVA= Sender.java Receiver.java PingPongTask.java PingPongTest.java

# Declare test class
TESTS=PingPongTest.class

include $(srcdir)/../java_test.mk

# Cruft
include $(top_srcdir)/acmacro/dist-files.mk
