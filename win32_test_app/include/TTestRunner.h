#ifndef __TEST_RUNNER_H__
#define __TEST_RUNNER_H__

#include <TTestSuite.h>


/*
 * Declaration of the s_TestRunner structure, which represents
 * a test runner used to run suites of tests.
 */
typedef struct s_TestRunner {
  Buffer_t buffer;              /* a single buffer                                              */
  TestSuite_t testSuite;        /* the test suite to run                                */

} s_TestRunner_t, *TestRunner_t;


/* 
 * s_TestRunner structure connected functions.
 */

/* 
 * Create an new s_TestRunner struct and 
 * returns a pointer to self.
 */
TestRunner_t TestRunner_new(void);

/* 
 * Initialize the s_TestRunner struct.
 */
errno_t TestRunner_initialize(TestRunner_t runner, int argc, char *argv[]);

/* 
 * Launch the test runner.
 */
void TestRunner_run(TestRunner_t runner);

/* 
 * Free the s_TestRunner.
 */
void TestRunner_free(TestRunner_t runner);







#endif                          /* #ifndef __TestRunner_H__ */
