/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

#include <stdio.h>

#include "xbt.h"

extern xbt_test_unit_t _xbt_current_unit;

/* SGU: BEGIN PROTOTYPES */
  /* SGU: BEGIN FILE xbt/cunit.c */
    void  test_expected_failure(void);
  /* SGU: END FILE */

  /* SGU: BEGIN FILE xbt/ex.c */
    void  test_controlflow(void);
    void  test_value(void);
    void  test_variables(void);
    void  test_cleanup(void);
  /* SGU: END FILE */

  /* SGU: BEGIN FILE xbt/dynar.c */
    void  test_dynar_int(void);
    void test_dynar_insert(void);
    void  test_dynar_double(void);
    void  test_dynar_string(void);
    void  test_dynar_sync_int(void);
  /* SGU: END FILE */

  /* SGU: BEGIN FILE xbt/dict.c */
    void  test_dict_basic(void);
    void  test_dict_remove(void);
    void  test_dict_nulldata(void);
    void  test_dict_scalar(void);
    void  test_dict_crash(void);
    void  test_dict_multicrash(void);
  /* SGU: END FILE */

  /* SGU: BEGIN FILE xbt/set.c */
    void  test_set_basic(void);
    void  test_set_change(void);
    void  test_set_retrieve(void);
    void  test_set_remove(void);
  /* SGU: END FILE */

  /* SGU: BEGIN FILE xbt/swag.c */
    void  test_swag_basic(void);
  /* SGU: END FILE */

  /* SGU: BEGIN FILE xbt/xbt_str.c */
    void  test_split_quoted(void);
    void  test_split_str(void);
  /* SGU: END FILE */

  /* SGU: BEGIN FILE xbt/xbt_strbuff.c */
    void  test_strbuff_substitute(void);
  /* SGU: END FILE */

  /* SGU: BEGIN FILE xbt/xbt_sha.c */
    void  test_crypto_sha(void);
  /* SGU: END FILE */

  /* SGU: BEGIN FILE xbt/config.c */
    void  test_config_memuse(void);
    void  test_config_validation(void);
    void  test_config_use(void);
  /* SGU: END FILE */

  /* SGU: BEGIN FILE xbt/xbt_synchro.c */
    void  test_dynar_dopar(void);
  /* SGU: END FILE */


/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

int main(int argc, char *argv[]) {
  xbt_test_suite_t suite; 
  char selection[1024];
  int i;

  int res;

  /* SGU: BEGIN SUITES DECLARATION */
    /* SGU: BEGIN FILE xbt/cunit.c */
      suite = xbt_test_suite_by_name("cunit","Testsuite mechanism autotest");
      xbt_test_suite_push(suite, "expect",  test_expected_failure,  "expected failures");
    /* SGU: END FILE */

    /* SGU: BEGIN FILE xbt/ex.c */
      suite = xbt_test_suite_by_name("xbt_ex","Exception Handling");
      xbt_test_suite_push(suite, "controlflow",  test_controlflow,  "basic nested control flow");
      xbt_test_suite_push(suite, "value",  test_value,  "exception value passing");
      xbt_test_suite_push(suite, "variables",  test_variables,  "variable value preservation");
      xbt_test_suite_push(suite, "cleanup",  test_cleanup,  "cleanup handling");
    /* SGU: END FILE */

    /* SGU: BEGIN FILE xbt/dynar.c */
      suite = xbt_test_suite_by_name("dynar","Dynar data container");
      xbt_test_suite_push(suite, "int",  test_dynar_int,  "Dynars of integers");
      xbt_test_suite_push(suite, "insert", test_dynar_insert, "Using the xbt_dynar_insert and xbt_dynar_remove functions");
      xbt_test_suite_push(suite, "double",  test_dynar_double,  "Dynars of doubles");
      xbt_test_suite_push(suite, "string",  test_dynar_string,  "Dynars of strings");
      xbt_test_suite_push(suite, "synchronized int",  test_dynar_sync_int,  "Synchronized dynars of integers");
    /* SGU: END FILE */

    /* SGU: BEGIN FILE xbt/dict.c */
      suite = xbt_test_suite_by_name("dict","Dict data container");
      xbt_test_suite_push(suite, "basic",  test_dict_basic,  "Basic usage: change, retrieve, traverse");
      xbt_test_suite_push(suite, "remove",  test_dict_remove,  "Removing some values");
      xbt_test_suite_push(suite, "nulldata",  test_dict_nulldata,  "NULL data management");
      xbt_test_suite_push(suite, "dicti",  test_dict_scalar,  "Scalar data and key management");
      xbt_test_suite_push(suite, "crash",  test_dict_crash,  "Crash test");
      xbt_test_suite_push(suite, "multicrash",  test_dict_multicrash,  "Multi-dict crash test");
    /* SGU: END FILE */

    /* SGU: BEGIN FILE xbt/set.c */
      suite = xbt_test_suite_by_name("set","Set data container");
      xbt_test_suite_push(suite, "basic",  test_set_basic,  "Basic usage");
      xbt_test_suite_push(suite, "change",  test_set_change,  "Changing some values");
      xbt_test_suite_push(suite, "retrieve",  test_set_retrieve,  "Retrieving some values");
      xbt_test_suite_push(suite, "remove",  test_set_remove,  "Removing some values");
    /* SGU: END FILE */

    /* SGU: BEGIN FILE xbt/swag.c */
      suite = xbt_test_suite_by_name("swag","Swag data container");
      xbt_test_suite_push(suite, "basic",  test_swag_basic,  "Basic usage");
    /* SGU: END FILE */

    /* SGU: BEGIN FILE xbt/xbt_str.c */
      suite = xbt_test_suite_by_name("xbt_str","String Handling");
      xbt_test_suite_push(suite, "xbt_str_split_quoted",  test_split_quoted,  "test the function xbt_str_split_quoted");
      xbt_test_suite_push(suite, "xbt_str_split_str",  test_split_str,  "test the function xbt_str_split_str");
    /* SGU: END FILE */

    /* SGU: BEGIN FILE xbt/xbt_strbuff.c */
      suite = xbt_test_suite_by_name("xbt_strbuff","String Buffers");
      xbt_test_suite_push(suite, "xbt_strbuff_substitute",  test_strbuff_substitute,  "test the function xbt_strbuff_substitute");
    /* SGU: END FILE */

    /* SGU: BEGIN FILE xbt/xbt_sha.c */
      suite = xbt_test_suite_by_name("hash","Various hash functions");
      xbt_test_suite_push(suite, "sha",  test_crypto_sha,  "Test of the sha algorithm");
    /* SGU: END FILE */

    /* SGU: BEGIN FILE xbt/config.c */
      suite = xbt_test_suite_by_name("config","Configuration support");
      xbt_test_suite_push(suite, "memuse",  test_config_memuse,  "Alloc and free a config set");
      xbt_test_suite_push(suite, "validation",  test_config_validation,  "Validation tests");
      xbt_test_suite_push(suite, "use",  test_config_use,  "Data retrieving tests");
    /* SGU: END FILE */

    /* SGU: BEGIN FILE xbt/xbt_synchro.c */
      suite = xbt_test_suite_by_name("synchro","Advanced synchronization mecanisms");
      xbt_test_suite_push(suite, "dopar",  test_dynar_dopar,  "do parallel on dynars of integers");
    /* SGU: END FILE */

  /* SGU: END SUITES DECLARATION */
      
  xbt_init(&argc,argv);
    
  /* Search for the tests to do */
    selection[0]='\0';
    for (i=1;i<argc;i++) {
      if (!strncmp(argv[i],"--tests=",strlen("--tests="))) {
        char *p=strchr(argv[i],'=')+1;
        if (selection[0] == '\0') {
          strcpy(selection, p);
        } else {
          strcat(selection, ",");
          strcat(selection, p);
        }
      } else if (!strncmp(argv[i],"--dump-only",strlen("--dump-only"))||
 	         !strncmp(argv[i],"--dump",     strlen("--dump"))) {
        xbt_test_dump(selection);
        return 0;
      } else if (!strncmp(argv[i],"--help",strlen("--help"))) {
	  printf(
	      "Usage: testall [--help] [--tests=selection] [--dump-only]\n\n"
	      "--help: display this help\n"
	      "--dump-only: don't run the tests, but display some debuging info about the tests\n"
	      "--tests=selection: Use argument to select which suites/units/tests to run\n"
	      "                   --tests can be used more than once, and selection may be a comma\n"
	      "                   separated list of directives.\n\n"
	      "Directives are of the form:\n"
	      "   [-]suitename[:unitname]\n\n"
	      "If the first char is a '-', the directive disables its argument instead of enabling it\n"
	      "suitename/unitname is the set of tests to en/disable. If a unitname is not specified,\n"
	      "it applies on any unit.\n\n"
	      "By default, everything is enabled.\n\n"
	      "'all' as suite name apply to all suites.\n\n"
	      "Example 1: \"-toto,+toto:tutu\"\n"
	      "  disables the whole toto testsuite (any unit in it),\n"
	      "  then reenables the tutu unit of the toto test suite.\n\n"
	      "Example 2: \"-all,+toto\"\n"
	      "  Run nothing but the toto suite.\n");
	  return 0;
      } else {
        printf("testall: Unknown option: %s\n",argv[i]);
        return 1;
      }
    }
  /* Got all my tests to do */
      
  res = xbt_test_run(selection);
  xbt_test_exit();
  return res;
}
/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

