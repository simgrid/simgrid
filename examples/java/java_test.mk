#########################
# How to compile the code
# All this to only get a dumb javac *java... Automake still have issues with java

JAVAROOT=.

AM_JAVACFLAGS=-classpath $(top_srcdir)/src/simgrid.jar


##########################
# What to do on make check
# We need to rely on an external script (../runtest) because automake adds a ./ in front of the test name
# We need to override LD_LIBRARY_PATH so that the VM finds the libsimgrid4java.so
# We need to override the CLASSPATH to add simgrid.jar and current dir to the picture
# We also need to express dependencies manually (in each test dir)
# Damn...
TESTS_ENVIRONMENT=LD_LIBRARY_PATH="$(top_srcdir)/src/.libs:$$LD_LIBRARY_PATH" \
                  CLASSPATH=".:$(top_srcdir)/src/simgrid.jar:$$CLASSPATH" \
                  $(srcdir)/../runtest 

# declare that we must recompile everything before lauching tests
$(TESTS): classnoinst.stamp
