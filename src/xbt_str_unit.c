/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

#include <stdio.h>
#include "xbt.h"
/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

#line 423 "xbt/xbt_str.c" 
#include "xbt/str.h"

#define mytest(name, input, expected) \
  xbt_test_add0(name); \
  d=xbt_str_split_quoted(input); \
  s=xbt_str_join(d,"XXX"); \
  xbt_test_assert3(!strcmp(s,expected),\
                   "Input (%s) leads to (%s) instead of (%s)", \
                   input,s,expected);\
                   free(s); \
                   xbt_dynar_free(&d);

XBT_TEST_UNIT("xbt_str_split_quoted", test_split_quoted, "test the function xbt_str_split_quoted")
{
  xbt_dynar_t d;
  char *s;

  mytest("Empty", "", "");
  mytest("Basic test", "toto tutu", "totoXXXtutu");
  mytest("Useless backslashes", "\\t\\o\\t\\o \\t\\u\\t\\u",
         "totoXXXtutu");
  mytest("Protected space", "toto\\ tutu", "toto tutu");
  mytest("Several spaces", "toto   tutu", "totoXXXtutu");
  mytest("LTriming", "  toto tatu", "totoXXXtatu");
  mytest("Triming", "  toto   tutu  ", "totoXXXtutu");
  mytest("Single quotes", "'toto tutu' tata", "toto tutuXXXtata");
  mytest("Double quotes", "\"toto tutu\" tata", "toto tutuXXXtata");
  mytest("Mixed quotes", "\"toto' 'tutu\" tata", "toto' 'tutuXXXtata");
  mytest("Backslashed quotes", "\\'toto tutu\\' tata",
         "'totoXXXtutu'XXXtata");
  mytest("Backslashed quotes + quotes", "'toto \\'tutu' tata",
         "toto 'tutuXXXtata");

}

#define mytest_str(name, input, separator, expected) \
  xbt_test_add0(name); \
  d=xbt_str_split_str(input, separator); \
  s=xbt_str_join(d,"XXX"); \
  xbt_test_assert3(!strcmp(s,expected),\
                   "Input (%s) leads to (%s) instead of (%s)", \
                   input,s,expected);\
                   free(s); \
                   xbt_dynar_free(&d);

XBT_TEST_UNIT("xbt_str_split_str", test_split_str, "test the function xbt_str_split_str")
{
  xbt_dynar_t d;
  char *s;

  mytest_str("Empty string and separator", "", "", "");
  mytest_str("Empty string", "", "##", "");
  mytest_str("Empty separator", "toto", "", "toto");
  mytest_str("String with no separator in it", "toto", "##", "toto");
  mytest_str("Basic test", "toto##tutu", "##", "totoXXXtutu");
}
/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

