#include <TTestCaseContext.h>

/*
 * Implementation of s_TestCaseContext connected functions.
 */

/* 
 * Create a new s_TestCaseContext and returns a pointer to self.
 */
TestCaseContext_t TestCaseContext_new(void)
{
  TestCaseContext_t context = calloc(1, sizeof(s_TestCaseContext_t));

  if (NULL == context) {
    setErrno(E_TEST_CASE_CONTEXT_ALLOCATION_FAILED);
    return NULL;
  }

  context->inputBuffer = Buffer_new();

  if (NULL == context->inputBuffer) {
    TestCaseContext_free(context);
    return NULL;
  }

  context->outputBuffer = Buffer_new();

  if (NULL == context->outputBuffer) {
    TestCaseContext_free(context);
    return NULL;
  }

  context->expectedOutputBuffer = Buffer_new();

  if (NULL == context->expectedOutputBuffer) {
    TestCaseContext_free(context);
    return NULL;
  }

  context->commandLineBuffer = Buffer_new();

  if (NULL == context->commandLineBuffer) {
    TestCaseContext_free(context);
    return NULL;
  }


  context->isOutputCheckingEnabled = DEFAULT_OUTPUT_CHECKING_MODE;
  context->isPostOutputCheckingEnabled = DEFAULT_POST_OUTPUT_CHECKING_MODE;
  context->timeoutValue = DEFAULT_TIMEOUT_VALUE;
  context->expectedExitCode = INVALID_EXIT_CODE;
  context->exitCode = INVALID_EXIT_CODE;
  context->name = NULL;

  context->runThread = true;
  context->hThread = NULL;
  context->hOutputRead = NULL;
  context->hInputWrite = NULL;
  context->hChildStdInRead = NULL;
  context->hChildStdOutWrite = NULL;
  context->hChildStderr = NULL;
  context->hChildStdoutReadTmp = NULL;
  context->hChildStdinWriteTmp = NULL;
  context->hConsole = NULL;

  context->createConsole = false;
  context->exitCodeCheckingEnabled = false;

  context->started = false;

  memset(&(context->pi), 0, sizeof(PROCESS_INFORMATION));

  return context;
}

/* 
 * Destroy the s_TestCaseContext referenced by context. 
 */
void TestCaseContext_free(TestCaseContext_t context)
{
  if (NULL == context)
    return;

  if (NULL != context->inputBuffer)
    Buffer_free(context->inputBuffer);

  if (NULL != context->outputBuffer)
    Buffer_free(context->outputBuffer);

  if (NULL != context->expectedOutputBuffer)
    Buffer_free(context->expectedOutputBuffer);

  if (NULL != context->commandLineBuffer)
    Buffer_free(context->commandLineBuffer);

  if (NULL == context->name)
    free(context->name);


  /* Close all pipe handles. */
  if (context->hChildStdoutReadTmp)
    CloseHandle(context->hChildStdoutReadTmp);

  if (context->hChildStdInRead)
    CloseHandle(context->hChildStdInRead);

  if (context->hChildStdinWriteTmp)
    CloseHandle(context->hChildStdinWriteTmp);

  if (context->hChildStdOutWrite)
    CloseHandle(context->hChildStdOutWrite);

  if (context->hOutputRead)
    CloseHandle(context->hOutputRead);

  if (context->pi.hThread)
    CloseHandle(context->pi.hThread);

  /* Use some violence, no choice. */
  if (context->pi.hProcess) {
    /* Kill the child process. */
    TerminateProcess(context->pi.hProcess, 0);
  }

  if (context->hThread) {
    /* Terminate the thread */
    TerminateThread(context->hThread, 0);
  }

  if (context->hInputWrite)
    CloseHandle(context->hInputWrite);

  if (context->hChildStderr)
    CloseHandle(context->hChildStderr);

  free(context);
  context = NULL;
}

/* 
 * Set the timeout of the test case context.
 */
void TestCaseContext_setTimeout(TestCaseContext_t context, int timeout)
{
  context->timeoutValue = timeout;
}

/*
 * Enable the output checking of the test case context.
 */
void TestCaseContext_enableOutputChecking(TestCaseContext_t context)
{
  context->isOutputCheckingEnabled = true;
}

/*
 * Enable the output checking of the test case context.
 */
void TestCaseContext_disableOutputChecking(TestCaseContext_t context)
{
  /* If the post output checking mode is enable, disable it */
  context->isPostOutputCheckingEnabled = false;
  context->isOutputCheckingEnabled = false;
}

/*
 * Enable the post output checking of the test case context.
 */
void TestCaseContext_enable_post_output_checking(TestCaseContext_t context)
{
  /* enable the post output checking mode if the output checking mode is enabled */
  if (context->isOutputCheckingEnabled)
    context->isPostOutputCheckingEnabled = true;
}

/*
 * Disable the post output checking of the test case context.
 */
void TestCaseContext_disablePostOutputChecking(TestCaseContext_t context)
{
  context->isPostOutputCheckingEnabled = false;
}

void TestCaseContext_createConsole(TestCaseContext_t context)
{
  context->createConsole = true;
}

void TestCaseContext_createNoConsole(TestCaseContext_t context)
{
  context->createConsole = false;
}

/*
 * Set the expected exit code of the test case context.
 */
void TestCaseContext_setExpectedExitCode(TestCaseContext_t context,
                                         int expected_code)
{
  context->expectedExitCode = expected_code;
}

/*
 * Return true if the output checking mode is enabled for this
 * test case context. Otherwise the functions returns false.
 */
bool TestCaseContext_isOutputCheckingEnabled(TestCaseContext_t context)
{
  return context->isOutputCheckingEnabled;
}

void TestCaseContext_enableExitCodeChecking(TestCaseContext_t context)
{
  context->exitCodeCheckingEnabled = true;
}

void TestCaseContext_disableExitCodeChecking(TestCaseContext_t context)
{
  context->exitCodeCheckingEnabled = false;
}

void TestCaseContext_setCommandLine(TestCaseContext_t context,
                                    char *cmdLine)
{
  Buffer_append(context->commandLineBuffer, cmdLine);
}


/*
 * Append a child output to check in the 
 * test case context.
 */
void TestCaseContext_appendExpectedOutput(TestCaseContext_t context,
                                          char *expected_output)
{
  Buffer_append(context->expectedOutputBuffer, expected_output);
}

/*
 * Append a child output to check in the 
 * test case context.
 */
void TestCaseContext_appendChildInput(TestCaseContext_t context,
                                      char *input)
{
  Buffer_append(context->inputBuffer, input);
}

/*
 * Set the name of the test case name.
 */
void TestCaseContext_setName(TestCaseContext_t context, char *name)
{
  size_t size;

  if (NULL != context->name) {
    free(context->name);
  }

  context->name = strdup(name);
  size = strlen(name);

  while ((context->name[size - 1] == '\n')
         || (context->name[size - 1] == '\r')) {
    context->name[size - 1] = '\0';

    if (size)
      size--;
  }

  /*context->name[strlen(context->name) - 1] ='\0'; */
}

/* 
 * Clear the s_TestCaseContext referenced by context.
 */
void TestCaseContext_clear(TestCaseContext_t context)
{
  if (!Buffer_empty(context->inputBuffer))
    Buffer_clear(context->inputBuffer);

  if (!Buffer_empty(context->outputBuffer))
    Buffer_clear(context->outputBuffer);

  if (!Buffer_empty(context->expectedOutputBuffer))
    Buffer_clear(context->expectedOutputBuffer);

  if (!Buffer_empty(context->commandLineBuffer))
    Buffer_clear(context->commandLineBuffer);

  if (NULL == context->name) {
    free(context->name);
    context->name = NULL;
  }

  context->isOutputCheckingEnabled = DEFAULT_OUTPUT_CHECKING_MODE;
  context->isPostOutputCheckingEnabled = DEFAULT_POST_OUTPUT_CHECKING_MODE;
  context->timeoutValue = DEFAULT_TIMEOUT_VALUE;
  context->expectedExitCode = INVALID_EXIT_CODE;
  context->exitCode = INVALID_EXIT_CODE;
}
