
#pragma hdrstop

#include <TTestRunner.h>

TestRunner_t test_runner = NULL;

void terminate(void);

#pragma argsused

int main(int argc, char *argv[])
{
  errno_t e;
  initializeErrno();

  /* Create a test runner. */
  test_runner = TestRunner_new();

  if (NULL == test_runner)
    terminate();

  /* Initialize the test runner. */
  if (E_SUCCESS != TestRunner_initialize(test_runner, argc, argv))
    terminate();

  /* Launch the test runner. */
  TestRunner_run(test_runner);

  e = getErrno();

  terminate();

  return e;
}


void terminate(void)
{
  errno_t e = getErrno();

  if (NULL != test_runner)
    TestRunner_free(test_runner);

  printf("\n Program terminated with the exit code : %3d (%s)\n",
         getErrno(), __errlist[getErrno()]);

  terminateErrno();

  exit(e);
}
