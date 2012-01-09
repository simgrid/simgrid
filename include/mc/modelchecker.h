#include "xbt.h"

XBT_PUBLIC(void) MC_assert(int);
XBT_PUBLIC(void) MC_assert_stateful(int);
XBT_PUBLIC(void) MC_assert_pair_stateful(int);
XBT_PUBLIC(void) MC_assert_pair_stateless(int);
XBT_PUBLIC(int) MC_random(int min, int max);
