# Copyright (c) 2004-2009. The SimGrid team. All right reserved.

# This file is part of the SimGrid project. This is free software:
# You can redistribute and/or modify it under the terms of the
# GNU LGPL (v2.1) licence.


# HOW TO ADD A NEW EXAMPLE:
#
#  - add the binary name into the noinst_PROGRAMS variable
#  - add a rule to compile the program, inspired from the existing ones
#  - add a tesh file running it in several conditions. 
#    Write the file taking the others as example, and after reading
#    <simgrid>/tools/tesh/README.tesh
#  - List the tesh file in the TESTS variable so that it gets tested on make check

noinst_PROGRAMS = allreduce bcast bcbench compute compute2 compute3 first pingpong second sendrecv mvmul ring_c split scatter reduce 

alltoall2: alltoall2.c
	$(top_builddir)/src/smpi/smpicc $^ -o $@
alltoall_basic: alltoall_basic.c
	$(top_builddir)/src/smpi/smpicc $^ -o $@
alltoallv: alltoallv.c
	$(top_builddir)/src/smpi/smpicc $^ -o $@
allreduce: allreduce.c
	$(top_builddir)/src/smpi/smpicc $^ -o $@
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
pingpong: pingpong.c
	$(top_builddir)/src/smpi/smpicc $^ -o $@
second: second.c
	$(top_builddir)/src/smpi/smpicc $^ -o $@
scatter: scatter.c
	$(top_builddir)/src/smpi/smpicc $^ -o $@
reduce: reduce.c
	$(top_builddir)/src/smpi/smpicc $^ -o $@
ring_c: ring_c.c
	$(top_builddir)/src/smpi/smpicc $^ -o $@
split: split.c
	$(top_builddir)/src/smpi/smpicc $^ -o $@
mvmul: mvmul.c
	$(top_builddir)/src/smpi/smpicc $^ -o $@
sendrecv: sendrecv.c
	$(top_builddir)/src/smpi/smpicc $^ -o $@

# List the tesh files below
TESTS = bcast.tesh reduce.tesh

# No need to change anything after that line
TESTS_ENVIRONMENT = $(top_builddir)/tools/tesh/tesh
EXTRA_DIST = $(TESTS)
CLEANFILES = *~

include $(top_srcdir)/acmacro/dist-files.mk
