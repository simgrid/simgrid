# Copyright (c) 2004-2007. The SimGrid team. All right reserved.

# This file is part of the SimGrid project. This is free software:
# You can redistribute and/or modify it under the terms of the
# GNU LGPL (v2.1) licence.


#INCLUDES  = -I$(top_srcdir)/include
#AM_CFLAGS = -g

# For each test and/or example:
#  - add the binary name into the noinst_PROGRAMS variable
#  - add a rule to compile the program, inspired from the existing ones

noinst_PROGRAMS = bcast bcbench compute compute2 compute3 first second mvmul ring_c split
#reduce Not working yet (FIXME)

bcast: bcast.c
	$(top_builddir)/src/smpi/smpicc $^ -o $@
bcbench: bcbench.c
	$(top_builddir)/src/smpi/smpicc $^ -o $@
compute: compute.c
	$(top_builddir)/src/smpi/smpicc $^ -o $@
compute2: compute2.c
	$(top_builddir)/src/smpi/smpicc $^ -o $@
compute3: compute3.c
	$(top_builddir)/src/smpi/smpicc $^ -o $@
first: mvmul.c
	$(top_builddir)/src/smpi/smpicc $^ -o $@
second: second.c
	$(top_builddir)/src/smpi/smpicc $^ -o $@
reduce: reduce.c
	$(top_builddir)/src/smpi/smpicc $^ -o $@
ring_c: ring_c.c
	$(top_builddir)/src/smpi/smpicc $^ -o $@
split: split.c
	$(top_builddir)/src/smpi/smpicc $^ -o $@
mvmul: mvmul.c
	$(top_builddir)/src/smpi/smpicc $^ -o $@

# List the tesh files below
TESTS = 

# No need to change anything after that line
TESTS_ENVIRONMENT = $(top_builddir)/tools/tesh/tesh
EXTRA_DIST = $(TESTS)
CLEANFILES = *~

include $(top_srcdir)/acmacro/dist-files.mk
