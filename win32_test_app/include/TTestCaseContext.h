#ifndef __TestCaseContext_H__
#define __TestCaseContext_H__

#include <TBuffer.h>
#include <windows.h>

/* 
 * Declaration a the s_TestCaseContext structure 
 * which represents the context of a test case during his
 * execution.
 */
typedef struct s_TestCaseContext {
  char *name;                   /* the test case name                                                                   */
  int timeoutValue;             /* the timeout value                                                                    */
  bool isOutputCheckingEnabled; /* if true, output checking is enable                                   */
  bool isPostOutputCheckingEnabled;     /* if true, the post output checking mode is enable             */
  Buffer_t inputBuffer;         /* buffer that contains child input                                             */
  Buffer_t outputBuffer;        /* the child output buffer                                                              */
  Buffer_t expectedOutputBuffer;        /* buffer that contains child expected output                   */
  int expectedExitCode;         /* the child expected exit code                                                 */
  int threadExitCode;           /* the thread exit code                                                                                         */
  int exitCode;                 /* the child process exit code                                                                          */
  bool runThread;               /* false if the thread of the test case must terminate                          */
  HANDLE hThread;               /* the handle of the thread                                                                                     */
  HANDLE hOutputRead;           /* handle to the read pipe                                                                                      */
  HANDLE hInputWrite;           /* handle to the write pipe                                                                                     */
  HANDLE hChildStdInRead;       /* handle to the pipe used to read the child std input                          */
  HANDLE hChildStdOutWrite;     /* handle to the pipe used to write to the chil std output                      */
  HANDLE hChildStderr;          /* handle to the pipe used to read the child std error                          */
  PROCESS_INFORMATION pi;       /* this structure contains child process informations                           */
  HANDLE hChildStdoutReadTmp;   /* tempory handle                                                                                                       */
  HANDLE hChildStdinWriteTmp;   /* tempory handle                                                                                                       */
  bool createConsole;           /* true if we can create a console for the child process            */
  bool exitCodeCheckingEnabled; /* true if we want to check the child exit code                     */
  HANDLE hConsole;              /* handle to the console                                            */
  bool started;                 /* true if the child process started                                */
  Buffer_t commandLineBuffer;   /* command line buffer                                              */

} s_TestCaseContext_t, *TestCaseContext_t;

/* Output checking is disabled by default*/
#define DEFAULT_OUTPUT_CHECKING_MODE		false

/* Post output checking mode is disabled by default*/
#define DEFAULT_POST_OUTPUT_CHECKING_MODE	false

/* The default timeout value is 5 seconds*/
#define DEFAULT_TIMEOUT_VALUE				((int)120000)

/* Invalid exit code value (default value)*/
#define INVALID_EXIT_CODE 					((int)0xFFFFFF)

/* 
 * s_TestCaseContext struct connected functions.
 */

/* 
 * Create a new s_TestCaseContext and returns a pointer to self.
 */
TestCaseContext_t TestCaseContext_new(void);

/* 
 * Destroy the s_TestCaseContext referenced by context. 
 */
void TestCaseContext_free(TestCaseContext_t context);

/* 
 * Clear the s_TestCaseContext referenced by context.
 */
void TestCaseContext_clear(TestCaseContext_t context);

/* 
 * Set the timeout of the test case context.
 */
void TestCaseContext_setTimeout(TestCaseContext_t context, int timeout);

/*
 * Enable the output checking of the test case context.
 */
void TestCaseContext_enableOutputChecking(TestCaseContext_t context);

/*
 * Disable the output checking of the test case context.
 */
void TestCaseContext_disableOutputChecking(TestCaseContext_t context);

/*
 * Enable the post output checking of the test case context.
 */
void TestCaseContext_enable_post_output_checking(TestCaseContext_t
                                                 context);

/*
 * Disable the post output checking of the test case context.
 */
void TestCaseContext_disablePostOutputChecking(TestCaseContext_t context);

/*
 * Set the expected exit code of the test case context.
 */
void TestCaseContext_setExpectedExitCode(TestCaseContext_t context,
                                         int expected_code);

/*
 * Return true if the output checking mode is enabled for this
 * test case context. Otherwise the functions returns false.
 */
bool TestCaseContext_isOutputCheckingEnabled(TestCaseContext_t context);

/*
 * Append a child output to check in the 
 * test case context.
 */
void TestCaseContext_appendExpectedOutput(TestCaseContext_t context,
                                          char *expected_output);

/*
 * Append a child output to check in the 
 * test case context.
 */
void TestCaseContext_appendChildInput(TestCaseContext_t context,
                                      char *input);

/*
 * Set the name of the test case name.
 */
void TestCaseContext_setName(TestCaseContext_t context, char *name);

void TestCaseContext_createConsole(TestCaseContext_t context);

void TestCaseContext_createNoConsole(TestCaseContext_t context);

void TestCaseContext_enableExitCodeChecking(TestCaseContext_t context);

void TestCaseContext_disableExitCodeChecking(TestCaseContext_t context);


void TestCaseContext_setCommandLine(TestCaseContext_t context,
                                    char *cmdLine);




#endif                          /* #ifndef __TestCaseContext_H__ */
