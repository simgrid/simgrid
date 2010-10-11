#ifndef __TestSuite_H__
#define __TestSuite_H__

#include <TStream.h>
#include <TThreadDynarray.h>

/*
 * Declaration of the s_TestSuite, which represents
 * a suite of tests.
 */
typedef struct s_TestSuite {
  TestCaseContext_t test_case_context;  /* the context of the current test case */
  Stream_t stream;              /* stdin or file stream                                 */
  size_t testCaseCount;         /* test case count                                              */
  size_t successCount;          /* test case success count                              */
  size_t failureCount;          /* test case failure count                              */
#ifdef __VERBOSE
  char currentTime[30];         /* the current time                                             */
#endif                          /* #ifdef __VERBOSE */
  ThreadDynarray_t threads;
} s_TestSuite_t, *TestSuite_t;

/* 
 * s_TestSuite connected functions.
 */

/*
 * Create a new s_TestSuite an returns a pointer to self.
 */
TestSuite_t TestSuite_new(void);

/* 
 * Free the s_TestSuite pointed to by ptr.
 */
void TestSuite_free(TestSuite_t ptr);

 /*
  * Initialize the s_TestSuite structure.
  */
errno_t TestSuite_initialize(TestSuite_t ptr, int argc, char *argv[]);


/*
 * This function reads an entire line, storing 
 * the address of the buffer containing the  text into  
 * s_TestSuite.current_line. 
 */
ssize_t TestSuite_getline(TestSuite_t ptr, size_t * len);

/* 
 * Launch the test suite. 
 */
void TestSuite_run(TestSuite_t ptr);

/* 
 * Meta command processing.
 */
errno_t TestSuite_processMetaCommand(TestSuite_t testSuite);

/* 
 * Set the timeout of the test case context of the
 * test suite.
 */
void TestSuite_setTimeout(TestSuite_t testSuite);

/*
 * Enable output checking for the current test case context.
 */
void TestSuite_enableOutputChecking(TestSuite_t testSuite);

/*
 * Disable output checking for the current test case context.
 */
void TestSuite_disableOutputChecking(TestSuite_t testSuite);

/*
 * Enable post output checking for the current test case context.
 */
void TestSuite_enablePostOutputChecking(TestSuite_t testSuite);

/*
 * Disable post output checking for the current test case context.
 */
void TestSuite_disablePostOutputChecking(TestSuite_t testSuite);

/*
 * Set the expected exit code of the current test case context of the test suite.
 */
void TestSuite_setExpectedExitCode(TestSuite_t testSuite);

/*
 * Export a variable in the environment of the current test_runner.exe process.
 */
errno_t TestSuite_export(TestSuite_t testSuite);

/*
 * Expected child output processing.
 */
errno_t TestSuite_processExpectedChildOutput(TestSuite_t testSuite);

/*
 * Child input processing.
 */
errno_t TestSuite_processChildInput(TestSuite_t testSuite);

/*
 * Check the child output.
 */
errno_t TestSuite_checkChildOutput(TestCaseContext_t context);

/*
 * Print message
 */
void TestSuite_print(const char *str);

/*
 * Check the child process exit code.
 */
errno_t TestSuite_checkChildExitCode(TestCaseContext_t context);

errno_t TestSuite_unset(TestSuite_t testSuite);

void TestSuite_createConsole(TestSuite_t testSuite);

void TestSuite_createNoConsole(TestSuite_t testSuite);

void TestSuite_enableExitCodeChecking(TestSuite_t testSuite);

void TestSuite_disableExitCodeChecking(TestSuite_t testSuite);

unsigned long WINAPI TestSuite_runSyncTestCase(void *param);

errno_t TestSuite_runAsyncTestCase(TestSuite_t testSuite);

/* 
 * Terminate the test suite.
 */
void TestSuite_terminate(TestSuite_t testSuite);

unsigned long WINAPI TestSuite_asyncReadChildOutput(void *param);

bool TestSuite_iSPostOutputCheckingEnabled(TestCaseContext_t context);

bool TestSuite_iSExitCodeCheckingEnabled(TestCaseContext_t context);

errno_t TestSuite_changeDir(TestSuite_t testSuite);

void TestSuite_setCommandLine(TestSuite_t testSuite);


#ifdef __VERBOSE
/* 
 * Update the current time. 
 */
void TestSuite_update_current_time(TestSuite_t ptr);
#endif                          /* #ifdef __VERBOSE */



#endif                          /* #ifndef __TestSuite_H__ */
