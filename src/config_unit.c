/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

#include <stdio.h>
#include "xbt.h"
/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

# 1296 "/home/navarrop/Developments/simgrid/src/xbt/config.c" 
#include "xbt.h"
#include "xbt/ex.h"

XBT_LOG_EXTERNAL_CATEGORY(xbt_cfg);


static xbt_cfg_t make_set()
{
  xbt_cfg_t set = NULL;

  xbt_log_threshold_set(&_XBT_LOGV(xbt_cfg), xbt_log_priority_critical);
  xbt_cfg_register_str(&set, "speed:1_to_2_int");
  xbt_cfg_register_str(&set, "peername:1_to_1_string");
  xbt_cfg_register_str(&set, "user:1_to_10_string");

  return set;
}                               /* end_of_make_set */

XBT_TEST_UNIT("memuse", test_config_memuse, "Alloc and free a config set")
{
  xbt_cfg_t set = make_set();
  xbt_test_add0("Alloc and free a config set");
  xbt_cfg_set_parse(set,
                    "peername:veloce user:mquinson\nuser:oaumage\tuser:alegrand");
  xbt_cfg_free(&set);
  xbt_cfg_free(&set);
}

XBT_TEST_UNIT("validation", test_config_validation, "Validation tests")
{
  xbt_cfg_t set = set = make_set();
  xbt_ex_t e;

  xbt_test_add0("Having too few elements for speed");
  xbt_cfg_set_parse(set,
                    "peername:veloce user:mquinson\nuser:oaumage\tuser:alegrand");
  TRY {
    xbt_cfg_check(set);
  }
  CATCH(e) {
    if (e.category != mismatch_error ||
        strncmp(e.msg, "Config elem speed needs",
                strlen("Config elem speed needs")))
      xbt_test_fail1("Got an exception. msg=%s", e.msg);
    xbt_ex_free(e);
  }
  xbt_cfg_free(&set);
  xbt_cfg_free(&set);



  xbt_test_add0("Having too much values of 'speed'");
  set = make_set();
  xbt_cfg_set_parse(set, "peername:toto:42 user:alegrand");
  TRY {
    xbt_cfg_set_parse(set, "speed:42 speed:24 speed:34");
  }
  CATCH(e) {
    if (e.category != mismatch_error ||
        strncmp(e.msg, "Cannot add value 34 to the config elem speed",
                strlen("Config elem speed needs")))
      xbt_test_fail1("Got an exception. msg=%s", e.msg);
    xbt_ex_free(e);
  }
  xbt_cfg_check(set);
  xbt_cfg_free(&set);
  xbt_cfg_free(&set);

}

XBT_TEST_UNIT("use", test_config_use, "Data retrieving tests")
{

  xbt_test_add0("Get a single value");
  {
    /* get_single_value */
    int ival;
    xbt_cfg_t myset = make_set();

    xbt_cfg_set_parse(myset, "peername:toto:42 speed:42");
    ival = xbt_cfg_get_int(myset, "speed");
    if (ival != 42)
      xbt_test_fail1("Speed value = %d, I expected 42", ival);
    xbt_cfg_free(&myset);
  }

  xbt_test_add0("Get multiple values");
  {
    /* get_multiple_value */
    xbt_dynar_t dyn;
    xbt_cfg_t myset = make_set();

    xbt_cfg_set_parse(myset, "peername:veloce user:foo\nuser:bar\tuser:toto");
    xbt_cfg_set_parse(myset, "speed:42");
    xbt_cfg_check(myset);
    dyn = xbt_cfg_get_dynar(myset, "user");

    if (xbt_dynar_length(dyn) != 3)
      xbt_test_fail1("Dynar length = %lu, I expected 3",
                     xbt_dynar_length(dyn));

    if (strcmp(xbt_dynar_get_as(dyn, 0, char *), "foo"))
        xbt_test_fail1("Dynar[0] = %s, I expected foo",
                       xbt_dynar_get_as(dyn, 0, char *));

    if (strcmp(xbt_dynar_get_as(dyn, 1, char *), "bar"))
        xbt_test_fail1("Dynar[1] = %s, I expected bar",
                       xbt_dynar_get_as(dyn, 1, char *));

    if (strcmp(xbt_dynar_get_as(dyn, 2, char *), "toto"))
        xbt_test_fail1("Dynar[2] = %s, I expected toto",
                       xbt_dynar_get_as(dyn, 2, char *));
    xbt_cfg_free(&myset);
  }

  xbt_test_add0("Access to a non-existant entry");
  {
    /* non-existant_entry */
    xbt_cfg_t myset = make_set();
    xbt_ex_t e;

    TRY {
      xbt_cfg_set_parse(myset, "color:blue");
    } CATCH(e) {
      if (e.category != not_found_error)
        xbt_test_exception(e);
      xbt_ex_free(e);
    }
    xbt_cfg_free(&myset);
  }
}
/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

