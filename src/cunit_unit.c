/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

#include <stdio.h>
#include "xbt.h"
/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

#line 826 "xbt/cunit.c" 


XBT_TEST_UNIT("expect", test_expected_failure, "expected failures")
{
  xbt_test_add0("Skipped test");
  xbt_test_skip();

  xbt_test_add2("%s %s", "EXPECTED", "FAILURE");
  xbt_test_expect_failure();
  xbt_test_log2("%s %s", "Test", "log");
  xbt_test_fail0("EXPECTED FAILURE");
}

/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

