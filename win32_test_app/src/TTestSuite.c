#include <TTestSuite.h>


/*
 * Create a new s_TestSuite an returns a pointer to self.
 */
TestSuite_t TestSuite_new(void)
{
  TestSuite_t testSuite = calloc(1, sizeof(s_TestSuite_t));

  if (NULL == testSuite) {
    setErrno(E_TEST_SUITE_ALLOCATION_FAILED);
    TestSuite_free(testSuite);
    return NULL;
  }

  testSuite->stream = Stream_new();

  if (NULL == testSuite->stream) {
    TestSuite_free(testSuite);
    return NULL;
  }

  testSuite->test_case_context = TestCaseContext_new();

  if (NULL == testSuite->test_case_context) {
    TestSuite_free(testSuite);
    return NULL;
  }

  testSuite->test_case_context->hConsole = GetStdHandle(STD_OUTPUT_HANDLE);

  testSuite->threads = ThreadDynarray_new(15);

  if (NULL == testSuite->threads) {
    TestSuite_free(testSuite);
    return NULL;
  }

  testSuite->successCount = 0;
  testSuite->failureCount = 0;

  return testSuite;
}

/*
 * Initialize the s_TestSuite structure.
 */
errno_t TestSuite_initialize(TestSuite_t ptr, int argc, char *argv[])
{
  switch (argc) {
  case 1:
    TestSuite_print("Run the test case from the console.\n");

    ptr->stream->file = stdin;
    return E_SUCCESS;

  case 2:

    if (E_SUCCESS != Stream_isValidFile(argv[1]))
      return getErrno();

    printf("\n\n Test runner : %s\n\n", argv[1]);

    if (E_SUCCESS != Stream_openFile(ptr->stream, argv[1]))
      return getErrno();

    return E_SUCCESS;

  default:
    {
      setErrno(E_BAD_USAGE);
      return getErrno();
    }
  }
}

/* 
 * Launch the test suite. 
 */
void TestSuite_run(TestSuite_t testSuite)
{
  Stream_t stream = testSuite->stream;

  /* Handle all lines in the testsuite file (or from stdin) */
  while ((Stream_getLine(stream) != -1) && (E_SUCCESS == getErrno())) {
    /* Don't process blank lines. */
    if (Stream_lineIsBlank(stream))
      continue;

    /* Check if the current text line contains a invalid token. */
    if (Stream_lineContainsInvalidToken(stream))
      return;

    /* Check if the text line contains a meta command. */
    if (Stream_lineIsMetaCommand(stream)) {
      /* Check if the current text line contains a unknown meta command. */
      if (Stream_lineIsUnknwnMetaCommand(stream))
        return;

      /* Check the meta command validity. */
      if (Stream_lineIsInvalidMetaCommand(stream))
        return;

      /* We have a valid meta command, process it */
      if (E_SUCCESS != TestSuite_processMetaCommand(testSuite))
        return;

      continue;
    }

    /* Handle the comment. */
    if (Stream_lineIsComment(stream)) {
      Stream_printLine(testSuite->stream, comment_line_type);
      continue;
    }

    /* Handle expected child output. */
    if (Stream_lineIsExpectedChildOutput(stream)) {
      if (E_SUCCESS != TestSuite_processExpectedChildOutput(testSuite))
        return;

      continue;
    }

    /* Handle expected child input. */
    if (Stream_lineIsChildInput(stream)) {
      if (E_SUCCESS != TestSuite_processChildInput(testSuite))
        return;

      continue;
    }

    if (Stream_lineIsChangeDir(stream)) {
      if (E_SUCCESS != TestSuite_changeDir(testSuite))
        return;

      continue;
    }

    /* Handle synchrone synchrone test case. */
    if (Stream_lineIsSyncTestCase(stream)) {
      TestCaseContext_setName(testSuite->test_case_context,
                              stream->line + 2);

      TestSuite_runSyncTestCase(testSuite->test_case_context);


      if (TestSuite_iSPostOutputCheckingEnabled
          (testSuite->test_case_context)) {
        TestSuite_checkChildOutput(testSuite->test_case_context);
      }

      if (TestSuite_iSExitCodeCheckingEnabled
          (testSuite->test_case_context)) {
        if (E_SUCCESS !=
            TestSuite_checkChildExitCode(testSuite->test_case_context))
          return;
      }


      if (E_SUCCESS != getErrno())
        return;
    }
    /* Handle asynchrone synchrone test case. */
    else if (Stream_lineIsAsyncTestCase(stream)) {
      TestCaseContext_setName(testSuite->test_case_context,
                              stream->line + 2);

      if (E_SUCCESS != TestSuite_runAsyncTestCase(testSuite))
        return;
    } else {
      ASSERT(false);
    }

    /* Clear the child input stream. */
    Buffer_clear(testSuite->test_case_context->inputBuffer);
    /* Clear the command line buffer. */
    Buffer_clear(testSuite->test_case_context->commandLineBuffer);
  }
}

/* 
 * Meta command processing.
 */
errno_t TestSuite_processMetaCommand(TestSuite_t testSuite)
{
  Stream_t stream = testSuite->stream;

  if (!strncmp("set timeout ", stream->line + 2, strlen("set timeout "))) {
    TestSuite_setTimeout(testSuite);
  } else
      if (!strncmp
          ("command line ", stream->line + 2, strlen("command line"))) {
    TestSuite_setCommandLine(testSuite);
  } else
      if (!strncmp
          ("enable output checking", stream->line + 2,
           strlen("enable output checking"))) {
    TestSuite_enableOutputChecking(testSuite);
  } else
      if (!strncmp
          ("disable output checking", stream->line + 2,
           strlen("disable output checking"))) {
    TestSuite_disableOutputChecking(testSuite);
  } else
      if (!strncmp
          ("enable post output checking", stream->line + 2,
           strlen("enable post output checking"))) {
    TestSuite_enablePostOutputChecking(testSuite);
  } else
      if (!strncmp
          ("disable post output checking", stream->line + 2,
           strlen("disable post output checking"))) {
    TestSuite_disablePostOutputChecking(testSuite);
  } else
      if (!strncmp
          ("expect exit code ", stream->line + 2,
           strlen("expect exit code "))) {
    TestSuite_setExpectedExitCode(testSuite);
  } else if (!strncmp("export ", stream->line + 2, strlen("export "))) {
    TestSuite_export(testSuite);
  } else if (!strncmp("unset ", stream->line + 2, strlen("unset "))) {
    TestSuite_unset(testSuite);
  } else
      if (!strncmp
          ("create console", stream->line + 2, strlen("create console"))) {
    TestSuite_createConsole(testSuite);
  } else
      if (!strncmp
          ("create no console", stream->line + 2,
           strlen("create no console"))) {
    TestSuite_createNoConsole(testSuite);
  } else
      if (!strncmp
          ("enable exit code checking", stream->line + 2,
           strlen("enable exit code checking"))) {
    TestSuite_enableExitCodeChecking(testSuite);
  } else
      if (!strncmp
          ("disable exit code checking", stream->line + 2,
           strlen("disable exit code checking"))) {
    TestSuite_disableExitCodeChecking(testSuite);
  } else {
    /* TODO */
    ASSERT(false);
  }

  return E_SUCCESS;

}

/* 
 * Set the timeout of the test case context of the
 * test suite.
 */
void TestSuite_setTimeout(TestSuite_t testSuite)
{

  int timeout = atoi(testSuite->stream->line + 2 + strlen("set timeout "));
  TestCaseContext_setTimeout(testSuite->test_case_context, timeout);
}

/*
 * Enable output checking for the current test case context.
 */
void TestSuite_enableOutputChecking(TestSuite_t testSuite)
{
  TestCaseContext_enableOutputChecking(testSuite->test_case_context);
}

void TestSuite_setCommandLine(TestSuite_t testSuite)
{
  TestCaseContext_setCommandLine(testSuite->test_case_context,
                                 testSuite->stream->line + 2 +
                                 strlen("command line "));
}

/*
 * Disable output checking for the current test case context.
 */
void TestSuite_disableOutputChecking(TestSuite_t testSuite)
{
  TestCaseContext_disableOutputChecking(testSuite->test_case_context);
}

/*
 * Enable post output checking for the current test case context.
 */
void TestSuite_enablePostOutputChecking(TestSuite_t testSuite)
{
  TestCaseContext_enable_post_output_checking(testSuite->
                                              test_case_context);
}

void TestSuite_createConsole(TestSuite_t testSuite)
{
  TestCaseContext_createConsole(testSuite->test_case_context);
}

void TestSuite_createNoConsole(TestSuite_t testSuite)
{
  TestCaseContext_createNoConsole(testSuite->test_case_context);
}

/*
 * Disable post output checking for the current test case context.
 */
void TestSuite_disablePostOutputChecking(TestSuite_t testSuite)
{
  TestCaseContext_disablePostOutputChecking(testSuite->test_case_context);
}

/*
 * Set the expected exit code of the current test case context of the test suite.
 */
void TestSuite_setExpectedExitCode(TestSuite_t testSuite)
{
  int expectedExitCode =
      atoi(testSuite->stream->line + 2 + strlen("expect exit code "));
  TestCaseContext_setExpectedExitCode(testSuite->test_case_context,
                                      expectedExitCode);
}

void TestSuite_enableExitCodeChecking(TestSuite_t testSuite)
{
  TestCaseContext_enableExitCodeChecking(testSuite->test_case_context);
}

void TestSuite_disableExitCodeChecking(TestSuite_t testSuite)
{
  TestCaseContext_disableExitCodeChecking(testSuite->test_case_context);
}


/*
 * Export a variable in the environment of the current test_runner.exe process.
 */
errno_t TestSuite_export(TestSuite_t testSuite)
{
  /* TODO trim */
  const char *ptr;
  const char *pos;
  char __buffer[50] = { 0 };
  char *line = testSuite->stream->line + strlen("export ");

  line[strlen(line) - 1] = '\0';

  ptr = strchr(line, ' ');
  pos = ++ptr;
  ptr = strchr(line, '=');
  strncpy(__buffer, pos, ptr - pos);
  if (!SetEnvironmentVariable(__buffer, ++ptr)) {
    setErrno(E_EXPORT_FAILED);
    Stream_printLine(testSuite->stream, export_failed_line_type);
    return getErrno();

  }

  return E_SUCCESS;
}

errno_t TestSuite_unset(TestSuite_t testSuite)
{
  char line[128] = { 0 };
  const char *ptr;
  strcpy(line, testSuite->stream->line + 2);
  ptr = strchr(line, ' ');
  line[strlen(line) - 1] = '\0';

  if (!SetEnvironmentVariable(++ptr, NULL)) {

    setErrno(E_UNSET_FAILED);
    Stream_printLine(testSuite->stream, unset_failed_line_type);
    return getErrno();
  }

  return E_SUCCESS;
}

/*
 * Expected child output processing.
 */
errno_t TestSuite_processExpectedChildOutput(TestSuite_t testSuite)
{
  /* TODO : logic error */
  if (!TestCaseContext_isOutputCheckingEnabled
      (testSuite->test_case_context))
    return E_SUCCESS;

  /* TODO : trim */
  TestCaseContext_appendExpectedOutput(testSuite->test_case_context,
                                       testSuite->stream->line + 2);

  return E_SUCCESS;
}

/*
 * Child input processing.
 */
errno_t TestSuite_processChildInput(TestSuite_t testSuite)
{
  /* TODO : trim */
  TestCaseContext_appendChildInput(testSuite->test_case_context,
                                   testSuite->stream->line + 2);

  return E_SUCCESS;
}

/* 
 * Free the s_TestSuite pointed to by ptr.
 */
void TestSuite_free(TestSuite_t testSuite)
{
  ThreadEntry_t entry;
  unsigned long count;
  unsigned long i;
  DWORD dwWaitResult;
  bool steel_running;
  bool last_async_process_error = false;
  DWORD ExitCode = 0;
  errno_t e = getErrno();

  if (NULL == testSuite)
    return;

  count = ThreadDynarray_getCount(testSuite->threads);

  /* Wait for all asynchrone process */
  if (NULL != testSuite->threads && count) {
    while (true) {
      steel_running = false;

      for (i = 0; i < count; i++) {
        entry = ThreadDynarray_at(testSuite->threads, i);

        GetExitCodeThread(entry->hThread, &ExitCode);

        if (STILL_ACTIVE == ExitCode) {
          Sleep(0);
          steel_running = true;
        }
      }

      if (!steel_running)
        break;
    }

    for (i = 0; i < count; i++) {
      entry = ThreadDynarray_at(testSuite->threads, i);

      if (entry->context->pi.hProcess) {
        dwWaitResult = WaitForSingleObject(entry->hThread, INFINITE);

        if ((WAIT_FAILED == dwWaitResult))
          TerminateThread(entry->hThread, 0);
        else
          CloseHandle(entry->hThread);
      }

      /*if(((E_SUCCESS == e) || (E_EXIT_CODE_DONT_MATCH == e) || (E_OUTPUT_DONT_MATCH == e)) && !last_async_process_error)
         { */
      /* Child output and exit code checking */
      if (TestSuite_iSPostOutputCheckingEnabled(entry->context)) {
        if (E_SUCCESS != TestSuite_checkChildOutput(entry->context))
          last_async_process_error = true;
      }

      if (TestSuite_iSExitCodeCheckingEnabled(entry->context)) {
        if (E_SUCCESS != TestSuite_checkChildExitCode(entry->context))
          last_async_process_error = true;
      }
    }

    TestCaseContext_free(entry->context);
    /*} */

    ThreadDynarray_destroy(testSuite->threads);
  }

  if (NULL != testSuite->test_case_context)
    TestCaseContext_free(testSuite->test_case_context);

  if (NULL != testSuite->stream)
    Stream_free(testSuite->stream);

  free(testSuite);
}

/*
 * Check the child output.     
 */
errno_t TestSuite_checkChildOutput(TestCaseContext_t context)
{
  bool are_equals = false;
  char str[256] = { 0 };


  if (context->expectedOutputBuffer->size == 0
      && context->outputBuffer->size == 0)
    return E_SUCCESS;

  Buffer_chomp(context->outputBuffer);
  Buffer_chomp(context->expectedOutputBuffer);


  if (context->outputBuffer->size != context->expectedOutputBuffer->size
      || strcmp(context->outputBuffer->data,
                context->expectedOutputBuffer->data)) {
    strcpy(str, "<OUTPUT NOT MATCH            >  \n");
    TestSuite_print(str);

  } else {
    are_equals = true;
    strcpy(str, "<OUTPUT MATCH                >  \n");
    TestSuite_print(str);


  }

  memset(str, 0, 256);

  if (context->expectedOutputBuffer->size) {
    sprintf(str,
            "<EXPECTED                    >      SIZE (%4d) DATA (%s)\n",
            context->expectedOutputBuffer->size,
            context->expectedOutputBuffer->data);
    TestSuite_print(str);
  } else {
    scanf(str,
          "<EXPECTED                    >      SIZE (%4d) DATA (%s)\n",
          context->expectedOutputBuffer->size, "empty");
    TestSuite_print(str);
  }

  memset(str, 0, 256);

  if (context->outputBuffer->size) {
    sprintf(str,
            "<RECEIVED                    >      SIZE (%4d) DATA (%s)\n",
            context->outputBuffer->size, context->outputBuffer->data);
    TestSuite_print(str);
  } else {
    sprintf(str,
            "<RECEIVED                    >      SIZE (%4d) DATA (%s)\n",
            context->outputBuffer->size, "empty");
    TestSuite_print(str);
  }

  Buffer_clear(context->expectedOutputBuffer);
  Buffer_clear(context->outputBuffer);

  if (!are_equals) {
    setErrno(E_OUTPUT_DONT_MATCH);
    return getErrno();
  }

  return E_SUCCESS;
}

/*
 * Check the child process exit code.
 */
errno_t TestSuite_checkChildExitCode(TestCaseContext_t context)
{
  bool __success = false;
  char str[256] = { 0 };

  sprintf(str, "<TEST CASE TERMINATED        >      %s %3ld\n",
          context->name, context->exitCode);
  TestSuite_print(str);

  memset(str, 0, 256);

  /* if a expected exit code was signaled, compare it with the real. */
  if (context->expectedExitCode != INVALID_EXIT_CODE) {
    if (context->expectedExitCode != context->exitCode) {

      TestSuite_print("<EXIT CODE DON'T MATCH       >\n");
    } else {
      __success = true;
      TestSuite_print("<EXIT CODE MATCH             >\n");
    }
    sprintf(str, "<EXIT CODE EXPECTED          >      (%3d)\n",
            context->expectedExitCode);
    TestSuite_print(str);

    memset(str, 0, 256);

    sprintf(str, "<EXIT CODE RETURNED          >      (%3d)\n",
            context->exitCode);
    TestSuite_print(str);

    context->expectedExitCode = INVALID_EXIT_CODE;
  }

  if (!__success) {
    setErrno(E_EXIT_CODE_DONT_MATCH);
    return getErrno();
  }

  return E_SUCCESS;
}

/* 
 * Terminate the test suite.
 */
void TestSuite_terminate(TestSuite_t testSuite)
{
  TestCaseContext_t context = testSuite->test_case_context;

  /* cleanup the child_input_stream/output buffers.       */
  if (NULL != context->inputBuffer)
    Buffer_free(context->inputBuffer);

  if (NULL != context->outputBuffer)
    Buffer_free(context->outputBuffer);

  if (NULL != context->expectedOutputBuffer)
    Buffer_free(context->expectedOutputBuffer);

  /* close the file stream.                               */
  if (NULL != testSuite->stream)
    Stream_free(testSuite->stream);


}


/*
 * Print message
 */
void TestSuite_print(const char *str)
{
  char *t = (char *) calloc(1, 20);

  __time(t);

  EnterCriticalSection(&cs);
  printf("%s   %s", t, str);
  LeaveCriticalSection(&cs);

  free(t);

}

unsigned long WINAPI TestSuite_asyncReadChildOutput(void *param)
{
  char str[1024] = { 0 };
  char __buffer[1024] = { 0 };

  DWORD nBytesRead;
  DWORD nCharsWritten;
  TestCaseContext_t context = (TestCaseContext_t) param;
  HANDLE hPipeRead = context->hOutputRead;


  while (context->runThread) {
    if (!ReadFile(hPipeRead, str, sizeof(str), &nBytesRead, NULL)
        || !nBytesRead) {
      if (GetLastError() == ERROR_BROKEN_PIPE) {
        break;
      } else {
        /* TODO */
        context->threadExitCode = 1;
        exit(1);
      }
    }

    if (nBytesRead) {
      if (context->isOutputCheckingEnabled) {
        if (!Buffer_empty(context->outputBuffer))
          Buffer_clear(context->outputBuffer);

        TestSuite_print(str);


        Buffer_append(context->outputBuffer, str);
      }

      memset(str, 0, 1024);
      memset(__buffer, 0, 1024);
    }

  }
  context->threadExitCode = 0;
  return 0;
}

errno_t TestSuite_runAsyncTestCase(TestSuite_t testSuite)
{
  DWORD ThreadId;
  s_ThreadEntry_t entry;
  /* = (ThreadEntry_t)calloc(1,sizeof(s_ThreadEntry_t)); */

  TestCaseContext_t context = testSuite->test_case_context;
  memset(&entry, 0, sizeof(s_ThreadEntry_t));
  entry.context = TestCaseContext_new();

  Buffer_append(entry.context->inputBuffer, context->inputBuffer->data);
  Buffer_append(entry.context->outputBuffer, context->outputBuffer->data);
  Buffer_append(entry.context->expectedOutputBuffer,
                context->expectedOutputBuffer->data);
  Buffer_append(entry.context->commandLineBuffer,
                context->commandLineBuffer->data);
  entry.context->name = strdup(context->name);
  entry.context->timeoutValue = context->timeoutValue;
  entry.context->isOutputCheckingEnabled =
      context->isOutputCheckingEnabled;
  entry.context->isPostOutputCheckingEnabled =
      context->isPostOutputCheckingEnabled;
  entry.context->expectedExitCode = context->expectedExitCode;
  entry.context->createConsole = context->createConsole;
  entry.context->exitCodeCheckingEnabled =
      context->exitCodeCheckingEnabled;
  entry.context->hConsole = context->hConsole;
  Buffer_clear(context->inputBuffer);
  Buffer_clear(context->outputBuffer);
  Buffer_clear(context->expectedOutputBuffer);
  memset(&(entry.context->pi), 0, sizeof(PROCESS_INFORMATION));
  entry.context->runThread = true;

  entry.hThread =
      CreateThread(NULL, 0, TestSuite_runSyncTestCase,
                   (LPVOID) entry.context, CREATE_SUSPENDED, &ThreadId);
  entry.threadId = ThreadId;
  ThreadDynarray_pushBack(testSuite->threads, &entry);
  ResumeThread(entry.hThread);
  Sleep(0);
  setErrno(E_SUCCESS);

  return getErrno();
}

unsigned long WINAPI TestSuite_runSyncTestCase(void *param)
{
  STARTUPINFO si = { 0 };
  SECURITY_ATTRIBUTES sa = { 0 };
  DWORD dwWaitResult = 0;
  DWORD dwExitCode = 0;
  DWORD ThreadId;
  DWORD nBytes = 0;
  DWORD dwCreationMode = CREATE_NO_WINDOW;
  char cmdLine[4098] = { 0 };

  TestCaseContext_t context = (TestCaseContext_t) param;
  context->started = true;


  sa.nLength = sizeof(SECURITY_ATTRIBUTES);
  sa.lpSecurityDescriptor = NULL;
  /* The pipe handes can be inherited by the child. */
  sa.bInheritHandle = TRUE;

  /* Create a write pipe handle for the child std output */
  if (!CreatePipe
      (&(context->hChildStdoutReadTmp), &(context->hChildStdOutWrite), &sa,
       0)) {
    setErrno(E_CANNOT_CREATE_CHILD_STDOUT_READ_HANDLE);
    return getErrno();
  }

  /* 
   * Create a duplicate of the output write handle for the std error
   * write handle. This is necessary in case the child application closes
   * one of its std output handles.
   */
  if (!DuplicateHandle
      (GetCurrentProcess(), (context->hChildStdOutWrite),
       GetCurrentProcess(), &(context->hChildStderr), 0, TRUE,
       DUPLICATE_SAME_ACCESS)) {
    setErrno(E_CANNOT_CREATE_CHILD_STDERR_READ_HANDLE);
    return getErrno();
  }

  /* Create a read pipe handle for the child std input */
  if (!CreatePipe
      (&(context->hChildStdInRead), &(context->hChildStdinWriteTmp), &sa,
       0)) {
    setErrno(E_CANNOT_CREATE_CHILD_STDIN_WRITE_HANDLE);
    return getErrno();
  }


  /* Create new output read handle and the input write handle use by 
   * the parent process to communicate with his child. Set the Properties  
   * to FALSE. Otherwise, the child inherits the properties and, as a 
   * result, non-closeable  handles to the pipes are created. 
   */

  /* Read handle for read operations on the child std output. */
  if (!DuplicateHandle
      (GetCurrentProcess(), (context->hChildStdoutReadTmp),
       GetCurrentProcess(), &(context->hOutputRead), 0, FALSE,
       DUPLICATE_SAME_ACCESS)) {
    setErrno(E_CANNOT_CREATE_STDOUT_READ_HANDLE);
    return getErrno();
  }


  /* Write handle for write operations on the child std input. */
  if (!DuplicateHandle
      (GetCurrentProcess(), (context->hChildStdinWriteTmp),
       GetCurrentProcess(), &(context->hInputWrite), 0, FALSE,
       DUPLICATE_SAME_ACCESS)) {
    setErrno(E_CANNOT_CREATE_STDIN_WRITE_HANDLE);
    return getErrno();
  }


  /* Close inheritable copies of the handles you do not want to be inherited. */
  if (!CloseHandle((context->hChildStdoutReadTmp))) {
    setErrno(E_CANNOT_CLOSE_CHILD_STDIN_TEMPORY_HANDLE);
    return getErrno();
  }

  context->hChildStdoutReadTmp = NULL;

  if (!CloseHandle((context->hChildStdinWriteTmp))) {
    setErrno(E_CANNOT_CLOSE_CHILD_STDOUT_TEMPORY_HANDLE);
    return getErrno();
  }


  context->hChildStdinWriteTmp = NULL;

  si.cb = sizeof(STARTUPINFO);
  /* Set the child std handles. */
  si.dwFlags = STARTF_USESTDHANDLES;
  si.hStdOutput = context->hChildStdOutWrite;
  si.hStdInput = context->hChildStdInRead;
  si.hStdError = context->hChildStderr;

  if (context->createConsole)
    dwCreationMode = CREATE_NEW_CONSOLE;

  if (!Buffer_empty(context->commandLineBuffer)) {
    Buffer_chomp(context->commandLineBuffer);
    sprintf(cmdLine, "%s %s", context->name,
            context->commandLineBuffer->data);
  } else
    strcpy(cmdLine, context->name);


  /* Create the child process. */
  if (!CreateProcess
      (NULL, cmdLine, NULL, NULL, TRUE, dwCreationMode, NULL, NULL, &si,
       &(context->pi))) {
    setErrno(E_CANNOT_CREATE_CHILD_PROCESS);
    return getErrno();
  }

  if (!CloseHandle(context->pi.hThread)) {
    setErrno(E_CANNOT_CLOSE_PROCESS_THREAD_HANDLE);
    return getErrno();
  }


  context->pi.hThread = NULL;

  /* close unnessary pipe handles. */
  if (!CloseHandle(context->hChildStdOutWrite)) {
    setErrno(E_CANNOT_CLOSE_CHILD_STDOUT_HANDLE);
    return getErrno();
  }

  context->hChildStdOutWrite = NULL;

  if (!CloseHandle(context->hChildStdInRead)) {
    setErrno(E_CANNOT_CLOSE_CHILD_STDIN_HANDLE);
    return getErrno();
  }

  context->hChildStdInRead = NULL;

  if (!CloseHandle(context->hChildStderr)) {
    setErrno(E_CANNOT_CLOSE_CHILD_STDERR_HANDLE);
    return getErrno();
  }

  context->hChildStderr = NULL;

  if (!Buffer_empty(context->inputBuffer)) {
    if (!WriteFile
        (context->hInputWrite, context->inputBuffer->data,
         context->inputBuffer->size, &nBytes, NULL)) {
      setErrno(E_CANNOT_WRITE_ON_CHILD_STDIN);
      return getErrno();
    }
  }

  context->hThread =
      CreateThread(&sa, 0, TestSuite_asyncReadChildOutput,
                   (LPVOID) context, 0, &ThreadId);
  Sleep(0);

  if (NULL == context->hThread) {
    setErrno(E_CANNOT_CREATE_READ_CHILD_OUTPUT_THREAD);
    return getErrno();
  }


  dwWaitResult =
      WaitForSingleObject(context->pi.hProcess, context->timeoutValue);

  if (WAIT_FAILED == dwWaitResult) {
    TerminateProcess(context->pi.hProcess, 0);
    context->pi.hProcess = NULL;
    context->runThread = false;

    if (WAIT_FAILED == WaitForSingleObject(context->hThread, INFINITE)) {
      setErrno(E_WAIT_THREAD_FAILED);
      return getErrno();
    }

    if (!CloseHandle(context->hThread)) {
      setErrno(E_CANNOT_CLOSE_THREAD_HANDLE);
      return getErrno();
    }

    context->hThread = NULL;

    if (!CloseHandle(context->hOutputRead)) {
      setErrno(E_CANNOT_CLOSE_READ_HANDLE);
      return getErrno();
    }

    context->hOutputRead = NULL;

    if (!CloseHandle(context->hInputWrite)) {
      setErrno(E_CANNOT_CLOSE_WRITE_HANDLE);
      return getErrno();
    }

    context->hInputWrite = NULL;
    setErrno(E_WAIT_FAILURE);
    return getErrno();
  }

  if (WAIT_TIMEOUT == dwWaitResult) {
    TerminateProcess(context->pi.hProcess, 0);
    context->pi.hProcess = NULL;
    context->runThread = false;

    if (WAIT_FAILED == WaitForSingleObject(context->hThread, INFINITE)) {
      setErrno(E_WAIT_THREAD_FAILED);
      return getErrno();
    }

    if (!CloseHandle(context->hThread)) {
      setErrno(E_CANNOT_CLOSE_THREAD_HANDLE);
      return getErrno();
    }

    context->hThread = NULL;

    if (!CloseHandle(context->hOutputRead)) {
      setErrno(E_CANNOT_CLOSE_READ_HANDLE);
      return getErrno();
    }


    context->hOutputRead = NULL;

    if (!CloseHandle(context->hInputWrite)) {
      setErrno(E_CANNOT_CLOSE_WRITE_HANDLE);
      return getErrno();
    }

    context->hInputWrite = NULL;
    setErrno(E_WAIT_TIMEOUT);
    return getErrno();
  }

  /* all is ok . */

  context->runThread = false;

  if (WAIT_FAILED == WaitForSingleObject(context->hThread, INFINITE)) {
    setErrno(E_WAIT_THREAD_FAILED);
    return getErrno();
  }

  if (!CloseHandle(context->hThread)) {
    setErrno(E_CANNOT_CLOSE_THREAD_HANDLE);
    return getErrno();
  }

  context->hThread = NULL;

  if (!CloseHandle(context->hOutputRead)) {
    setErrno(E_CANNOT_CLOSE_READ_HANDLE);
    return getErrno();
  }

  context->hOutputRead = NULL;

  if (!CloseHandle(context->hInputWrite)) {
    setErrno(E_CANNOT_CLOSE_WRITE_HANDLE);
    return getErrno();
  }

  context->hInputWrite = NULL;


  /* Get the child exit code before close it. */
  GetExitCodeProcess(context->pi.hProcess, &dwExitCode);

  context->exitCode = (int) dwExitCode;

  if (!CloseHandle(context->pi.hProcess)) {
    setErrno(E_CANNOT_CLOSE_PROCESS_HANDLE);
    return getErrno();
  }

  context->runThread = true;

  if (TestSuite_iSPostOutputCheckingEnabled(context)) {
    if (context->expectedOutputBuffer->size != 0
        || context->outputBuffer->size != 0) {
      Buffer_chomp(context->outputBuffer);
      Buffer_chomp(context->expectedOutputBuffer);

      if (context->outputBuffer->size !=
          context->expectedOutputBuffer->size
          || strcmp(context->outputBuffer->data,
                    context->expectedOutputBuffer->data)) {
        setErrno(E_OUTPUT_DONT_MATCH);
      }
    }
  }

  if (TestSuite_iSExitCodeCheckingEnabled(context)) {
    if (context->expectedExitCode != INVALID_EXIT_CODE) {
      if (context->expectedExitCode != context->exitCode) {
        setErrno(E_EXIT_CODE_DONT_MATCH);
      }
    }
  }

  context->pi.hProcess = NULL;
  return getErrno();
}

bool TestSuite_iSPostOutputCheckingEnabled(TestCaseContext_t context)
{
  if (!context->isPostOutputCheckingEnabled
      && context->isOutputCheckingEnabled) {
    return true;
  }

  return false;
}

bool TestSuite_iSExitCodeCheckingEnabled(TestCaseContext_t context)
{
  return context->exitCodeCheckingEnabled;
}

errno_t TestSuite_changeDir(TestSuite_t testSuite)
{
  char *line = testSuite->stream->line + 5;
  size_t size = strlen(line);

  while ((line[size - 1] == '\n') || (line[size - 1] == '\r')) {
    line[size - 1] = '\0';

    if (size)
      (size)--;
  }

  if (!SetCurrentDirectory(line)) {
    setErrno(E_CHANGE_DIRECTORY_FAILED);
    return E_CHANGE_DIRECTORY_FAILED;
  }

  Stream_printLine(testSuite->stream, change_directory_line_type);

  return E_SUCCESS;
}
