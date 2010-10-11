/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

#include <stdio.h>
#include "xbt.h"
/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

#line 294 "xbt/xbt_strbuff.c" 
#include "xbt/strbuff.h"

/* buffstr have 512 chars by default. Adding 1000 chars like this will force a resize, allowing us to test that b->used and b->size are consistent */
#define force_resize \
  "1.........1.........2.........3.........4.........5.........6.........7.........8.........9........." \
  "2.........1.........2.........3.........4.........5.........6.........7.........8.........9........." \
  "3.........1.........2.........3.........4.........5.........6.........7.........8.........9........." \
  "4.........1.........2.........3.........4.........5.........6.........7.........8.........9........." \
  "5.........1.........2.........3.........4.........5.........6.........7.........8.........9........." \
  "6.........1.........2.........3.........4.........5.........6.........7.........8.........9........." \
  "7.........1.........2.........3.........4.........5.........6.........7.........8.........9........." \
  "8.........1.........2.........3.........4.........5.........6.........7.........8.........9........." \
  "9.........1.........2.........3.........4.........5.........6.........7.........8.........9........." \
  "0.........1.........2.........3.........4.........5.........6.........7.........8.........9........."

static void mytest(const char *input, const char *patterns,
                   const char *expected)
{
  xbt_dynar_t dyn_patterns;     /* splited string */
  xbt_dict_t p;                 /* patterns */
  unsigned int cpt;
  char *str;                    /*foreach */
  xbt_strbuff_t sb;             /* what we test */

  p = xbt_dict_new();
  dyn_patterns = xbt_str_split(patterns, " ");
  xbt_dynar_foreach(dyn_patterns, cpt, str) {
    xbt_dynar_t keyvals = xbt_str_split(str, "=");
    char *key = xbt_dynar_get_as(keyvals, 0, char *);
    char *val = xbt_dynar_get_as(keyvals, 1, char *);
    xbt_str_subst(key, '_', ' ', 0);    // to put space in names without breaking the enclosing dynar_foreach
    xbt_dict_set(p, key, xbt_strdup(val), free);
    xbt_dynar_free(&keyvals);
  }
  xbt_dynar_free(&dyn_patterns);
  sb = xbt_strbuff_new();
  xbt_strbuff_append(sb, input);
  xbt_strbuff_varsubst(sb, p);
  xbt_dict_free(&p);
  xbt_test_assert4(!strcmp(sb->data, expected),
                   "Input (%s) with patterns (%s) leads to (%s) instead of (%s)",
                   input, patterns, sb->data, expected);
  xbt_strbuff_free(sb);
}

XBT_TEST_UNIT("xbt_strbuff_substitute", test_strbuff_substitute, "test the function xbt_strbuff_substitute")
{
  xbt_test_add0("Empty");
  mytest("", "", "");

  xbt_test_add0("Value shorter, no braces, only variable");
  mytest("$tutu", "tutu=t", "t");
  xbt_test_add0("Value shorter, braces, only variable");
  mytest("${tutu}", "tutu=t", "t");
  xbt_test_add0("Value shorter, no braces, data after");
  mytest("$tutu toto", "tutu=t", "t toto");
  xbt_test_add0("Value shorter, braces, data after");
  mytest("${tutu} toto", "tutu=t", "t toto");
  xbt_test_add0("Value shorter, no braces, data before");
  mytest("toto $tutu", "tutu=t", "toto t");
  xbt_test_add0("Value shorter, braces, data before");
  mytest("toto ${tutu}", "tutu=t", "toto t");
  xbt_test_add0("Value shorter, no braces, data before and after");
  mytest("toto $tutu tata", "tutu=t", "toto t tata");
  xbt_test_add0("Value shorter, braces, data before and after");
  mytest("toto ${tutu} tata", "tutu=t", "toto t tata");

  xbt_test_add0("Value as long, no braces, only variable");
  mytest("$tutu", "tutu=12345", "12345");
  xbt_test_add0("Value as long, braces, only variable");
  mytest("${tutu}", "tutu=1234567", "1234567");
  xbt_test_add0("Value as long, no braces, data after");
  mytest("$tutu toto", "tutu=12345", "12345 toto");
  xbt_test_add0("Value as long, braces, data after");
  mytest("${tutu} toto", "tutu=1234567", "1234567 toto");
  xbt_test_add0("Value as long, no braces, data before");
  mytest("toto $tutu", "tutu=12345", "toto 12345");
  xbt_test_add0("Value as long, braces, data before");
  mytest("toto ${tutu}", "tutu=1234567", "toto 1234567");
  xbt_test_add0("Value as long, no braces, data before and after");
  mytest("toto $tutu tata", "tutu=12345", "toto 12345 tata");
  xbt_test_add0("Value as long, braces, data before and after");
  mytest("toto ${tutu} tata", "tutu=1234567", "toto 1234567 tata");

  xbt_test_add0("Value longer, no braces, only variable");
  mytest("$t", "t=tututu", "tututu");
  xbt_test_add0("Value longer, braces, only variable");
  mytest("${t}", "t=tututu", "tututu");
  xbt_test_add0("Value longer, no braces, data after");
  mytest("$t toto", "t=tututu", "tututu toto");
  xbt_test_add0("Value longer, braces, data after");
  mytest("${t} toto", "t=tututu", "tututu toto");
  xbt_test_add0("Value longer, no braces, data before");
  mytest("toto $t", "t=tututu", "toto tututu");
  xbt_test_add0("Value longer, braces, data before");
  mytest("toto ${t}", "t=tututu", "toto tututu");
  xbt_test_add0("Value longer, no braces, data before and after");
  mytest("toto $t tata", "t=tututu", "toto tututu tata");
  xbt_test_add0("Value longer, braces, data before and after");
  mytest("toto ${t} tata", "t=tututu", "toto tututu tata");

  xbt_test_add0("Value much longer, no braces, only variable");
  mytest("$t", "t=" force_resize, force_resize);
  xbt_test_add0("Value much longer, no braces, data after");
  mytest("$t toto", "t=" force_resize, force_resize " toto");
  xbt_test_add0("Value much longer, braces, data after");
  mytest("${t} toto", "t=" force_resize, force_resize " toto");
  xbt_test_add0("Value much longer, no braces, data before");
  mytest("toto $t", "t=" force_resize, "toto " force_resize);
  xbt_test_add0("Value much longer, braces, data before");
  mytest("toto ${t}", "t=" force_resize, "toto " force_resize);
  xbt_test_add0("Value much longer, no braces, data before and after");
  mytest("toto $t tata", "t=" force_resize, "toto " force_resize " tata");
  xbt_test_add0("Value much longer, braces, data before and after");
  mytest("toto ${t} tata", "t=" force_resize,
         "toto " force_resize " tata");

  xbt_test_add0("Escaped $");
  mytest("\\$tutu", "tutu=t", "\\$tutu");
  xbt_test_add0("Space in var name (with braces)");
  mytest("${tu ti}", "tu_ti=t", "t");

  xbt_test_add0("Two variables");
  mytest("$toto $tutu", "toto=1 tutu=2", "1 2");

  // Commented: I'm too lazy to do a memmove in var name to remove the backslash after use.
  // Users should use braces.
  //  xbt_test_add0("Escaped space in var name", "$tu\\ ti", "tu_ti=t", "t");

  xbt_test_add0("Default value");
  mytest("${t:-toto}", "", "toto");
  xbt_test_add0("Useless default value (variable already defined)");
  mytest("${t:-toto}", "t=TRUC", "TRUC");

}

/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

