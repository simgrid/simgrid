# This Makefile is specifically tailored for the binaries of this tutorial.

# For your own project, you should use the one provided at 
# http://simgrid.gforge.inria.fr/simgrid/latest/doc/install_yours.html

# Some configuration
SIMGRID_INSTALL_PATH = /opt/simgrid # Where you installed simgrid
CC = gcc                            # Your compiler (on Mac, use clang instead)

# No change needed bellow for this tutorial.
############################################################################

all: masterworker0 masterworker1 masterworker2 masterworker3 masterworker4
masterworker0: masterworker0.o
masterworker1: masterworker1.o
masterworker2: masterworker2.o
masterworker3: masterworker3.o
masterworker4: masterworker4.o

WARNING = -Wshadow -Wcast-align -Waggregate-return -Wmissing-prototypes \
          -Wmissing-declarations -Wstrict-prototypes -Wmissing-prototypes \
          -Wmissing-declarations -Wmissing-noreturn -Wredundant-decls \
	  -Wnested-externs -Wpointer-arith -Wwrite-strings

# CFLAGS = -g -O0 $(WARNINGS) # Use this line to make debugging easier
CFLAGS = -g -O2 $(WARNINGS) # Use this line to get better performance

# No change should be mandated past that line
#############################################
# The following are implicit rules, used by default to actually build
# the targets for which you listed the dependencies above.

# The blanks before the $(CC) must be a Tab char, not spaces
%: %.o
	$(CC) -L$(strip $(SIMGRID_INSTALL_PATH))/lib/    $(CFLAGS) $^ -lsimgrid -o $@
%.o: %.c
	$(CC) -I$(strip $(SIMGRID_INSTALL_PATH))/include $(CFLAGS) -c -o $@ $<

clean:
	rm -f *.o *~ masterworker0 masterworker1 masterworker2 masterworker3 masterworker4
.PHONY: clean

