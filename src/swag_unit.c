/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

#include <stdio.h>
#include "xbt.h"
/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

#line 224 "xbt/swag.c" 


typedef struct {
  s_xbt_swag_hookup_t setA;
  s_xbt_swag_hookup_t setB;
  const char *name;
} shmurtz, s_shmurtz_t, *shmurtz_t;


XBT_TEST_UNIT("basic", test_swag_basic, "Basic usage")
{
  shmurtz_t obj1, obj2, obj;
  xbt_swag_t setA, setB;

  obj1 = xbt_new0(s_shmurtz_t, 1);
  obj2 = xbt_new0(s_shmurtz_t, 1);

  obj1->name = "Obj 1";
  obj2->name = "Obj 2";

  xbt_test_add0("Basic usage");
  xbt_test_log3("%p %p %ld\n", obj1, &(obj1->setB),
                (long) ((char *) &(obj1->setB) - (char *) obj1));

  setA = xbt_swag_new(xbt_swag_offset(*obj1, setA));
  setB = xbt_swag_new(xbt_swag_offset(*obj1, setB));

  xbt_swag_insert(obj1, setA);
  xbt_swag_insert(obj1, setB);
  xbt_swag_insert(obj2, setA);
  xbt_swag_insert(obj2, setB);

  xbt_swag_remove(obj1, setB);
  /*  xbt_swag_remove(obj2, setB); */

  xbt_test_add0("Traverse set A");
  xbt_swag_foreach(obj, setA) {
    xbt_test_log1("Saw: %s", obj->name);
  }

  xbt_test_add0("Traverse set B");
  xbt_swag_foreach(obj, setB) {
    xbt_test_log1("Saw: %s", obj->name);
  }

  xbt_test_add0("Ensure set content and length");
  xbt_test_assert(xbt_swag_belongs(obj1, setA));
  xbt_test_assert(xbt_swag_belongs(obj2, setA));

  xbt_test_assert(!xbt_swag_belongs(obj1, setB));
  xbt_test_assert(xbt_swag_belongs(obj2, setB));

  xbt_test_assert(xbt_swag_size(setA) == 2);
  xbt_test_assert(xbt_swag_size(setB) == 1);

  xbt_swag_free(setA);
  xbt_swag_free(setB);
}

/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

