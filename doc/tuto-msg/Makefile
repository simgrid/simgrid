# This Makefile is specifically tailored for the binaries of this tutorial.

# For your own project, you should use the one provided at 
# http://simgrid.gforge.inria.fr/simgrid/latest/doc/install_yours.html

# Some configuration
SIMGRID_INSTALL_PATH ?= ../.. # Where you installed simgrid 
CC ?= gcc                     # Your compiler (on Mac, use clang instead)

# No change needed bellow for this tutorial.
############################################################################

all: masterworker

WARNINGS = -Wall -Wextra -Wshadow -Wcast-align -Waggregate-return -Wmissing-prototypes \
          -Wmissing-declarations -Wmissing-prototypes -Wpointer-arith -Wwrite-strings \
          -Wmissing-noreturn -Wredundant-decls \
	  -Wno-nested-externs  -Wno-strict-prototypes -Wno-unused-parameter
WARNINGS += -Werror # Comment that line to not be in paranoid mode

# CFLAGS = -std=gnu99 -g -O0 $(WARNINGS) $(EXTRA_CFLAGS) # Use this line to make debugging easier
CFLAGS = -std=gnu99 -g -O2 $(WARNINGS) $(EXTRA_CFLAGS) # Use this line to get better performance

# No change should be mandated past that line
#############################################
# The following are implicit rules, used by default to actually build
# the targets for which you listed the dependencies above.

.SUFFIXES: .c
# The blanks before the $(CC) must be a Tab char, not spaces
.c:
	$(CC) -I$(strip $(SIMGRID_INSTALL_PATH))/include -L$(strip $(SIMGRID_INSTALL_PATH))/lib/ $(CFLAGS) $< -lsimgrid -o $@

clean:
	rm -f *.o *~ masterworker masterworker-sol1 masterworker-sol2 masterworker-sol3 masterworker-sol4
.PHONY: clean

