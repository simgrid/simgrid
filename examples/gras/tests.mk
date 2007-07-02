# This file factorize all the testing infrastructure for the GRAS examples
#
# it's made complicated by the facts that:
#  - we use tesh, we need to find it back
#  - we don't want to generate the test_* files with configure, so we
#    have to declare some variables in front of the shell command
#    running the test
#  - we want to fully test the surf on the way. So, we have to ask for
#    full precision timestamps in the tests. On the other hand,
#    message size differ when we are on 32bits and when we are on
#    64bits (obviously), so we have to maintain 2 tests per directory
#    and pick the right one here.

if GRAS_ARCH_32_BITS
  TESTS= test_sg_32 test_rl
test-sg: 
	$(TESTS_ENVIRONMENT) test_sg_32
else
  TESTS= test_sg_64 test_rl
test-sg: 
	$(TESTS_ENVIRONMENT) test_sg_64
endif

test-rl: force
	$(TESTS_ENVIRONMENT) test_rl
TESTS_ENVIRONMENT=srcdir=$(srcdir) EXEEXT=$(EXEEXT) @top_builddir@/tools/tesh/tesh

force:

EXTRA_DIST+=test_rl test_sg_32 test_sg_64

.PHONY: test_sg_32 test_SG_64 test_rl 
