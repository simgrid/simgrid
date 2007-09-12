/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

#include <stdio.h>

#include "xbt.h"

extern xbt_test_unit_t _xbt_current_unit;

/* SGU: BEGIN PROTOTYPES */
  /* SGU: BEGIN FILE ./xbt/cunit.c */
    void test_expected_failure(void);
  /* SGU: END FILE */

/* SGU: END PROTOTYPES */

/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

int main(int argc, char *argv[]) {
  xbt_test_suite_t suite; 
  char selection[1024];
  int i;

  /* SGU: BEGIN SUITES DECLARATION */
    /* SGU: BEGIN FILE ./xbt/cunit.c */
      suite = xbt_test_suite_by_name("cunit","Testsuite mechanism autotest");
      xbt_test_suite_push(suite, "expect", test_expected_failure, "expected failures");
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
      
  return xbt_test_run(selection);
}
/*******************************/
/* GENERATED FILE, DO NOT EDIT */
/*******************************/

