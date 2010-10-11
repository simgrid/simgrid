#include <TTestRunner.h>


/* 
 * Create an new s_TestRunner struct and 
 * returns a pointer to self.
 */
TestRunner_t TestRunner_new(void)
{
  TestRunner_t ptr = calloc(1, sizeof(s_TestRunner_t));

  if (NULL == ptr) {
    setErrno(E_TEST_RUNNER_ALLOCATION_FAILED);
    return NULL;
  }

  ptr->buffer = Buffer_new();

  if (NULL == ptr->buffer) {
    TestRunner_free(ptr);
    return NULL;
  }

  ptr->testSuite = TestSuite_new();

  if (NULL == ptr->testSuite) {
    TestRunner_free(ptr);
    return NULL;
  }

  setErrno(E_SUCCESS);
  return ptr;
}

/* 
 * Initialize the s_TestRunner struct 
 * by processing the command line.
 */
errno_t TestRunner_initialize(TestRunner_t runner, int argc, char *argv[])
{
  if (E_SUCCESS != TestSuite_initialize(runner->testSuite, argc, argv))
    return getErrno();

  return E_SUCCESS;
}

/* 
 * Launch the test runner.
 */
void TestRunner_run(TestRunner_t runner)
{
  TestSuite_run(runner->testSuite);
}

/* 
 * Free the s_TestRunner.
 */
void TestRunner_free(TestRunner_t runner)
{
  if (NULL == runner)
    return;

  if (NULL != runner->buffer)
    Buffer_free(runner->buffer);

  if (NULL != runner->testSuite)
    TestSuite_free(runner->testSuite);
}
