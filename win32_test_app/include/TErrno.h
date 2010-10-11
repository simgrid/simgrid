#ifndef __ERRNO_H__
#define __ERRNO_H__

#include <TDefs.h>
#include <string.h>
#include <windows.h>

#define E_SUCCESS 											((errno_t)0)    /* Success                                                                                                                      */
#define E_TEST_RUNNER_ALLOCATION_FAILED 					((errno_t)1)    /* Test runner allocation failed                                                                        */
#define E_TEST_CASE_CONTEXT_ALLOCATION_FAILED				((errno_t)2)    /* Test case context allocation failed                                                          */
#define E_BUFFER_ALLOCATION_FAILED							((errno_t)3)    /* Buffer allocation failed                                                                                     */
#define E_BUFFER_DATA_ALLOCATION_FAILED						((errno_t)4)    /* Buffer data allocation failed                                                                        */
#define E_TEST_SUITE_ALLOCATION_FAILED						((errno_t)5)    /* Test suite allocation failed                                                                         */
#define E_FILE_NOT_FOUND									((errno_t)6)    /* Ffile not found                                                                                                              */
#define E_BAD_USAGE											((errno_t)7)    /* Bad usage                                                                                                            */
#define E_INVALID_FILE_Stream								((errno_t)8)    /* Invalid file stream                  */
#define E_STREAM_ALLOCATION_FAILED							((errno_t)9)    /* Stream allocation failed                                     */
#define E_Buffer_DATA_REALLOCATION_FAILED					((errno_t)10)   /* Buffer data reallocation failed                                                                      */
#define E_STREAM_LINE_ALLOCATION_FAILED						((errno_t)11)   /* Stream line allocation failed */
#define E_STREAM_LINE_REALLOCATION_FAILED					((errno_t)12)   /* Stream line reallocation failed */
#define E_STREAM_EMPTY										((errno_t)13)   /* File empty   */
#define E_STREAM_ERROR										((errno_t)14)   /* File error   */
#define E_UNKWN_META_COMMAND								((errno_t)15)   /* Unknown meta command detected */
#define E_INVALID_TIMEOUT_VALUE								((errno_t)16)   /* Invalid timeout value                        */
#define E_INVALID_EXIT_CODE_VALUE							((errno_t)17)   /* Invalid exit code                            */
#define E_INVALID_EXPORT									((errno_t)18)   /* Invalid export meta command          */
#define E_INVALID_UNSET										((errno_t)19)   /* Invalid unset meta command           */
#define E_EXPORT_FAILED										((errno_t)20)   /* Export failed */
#define E_UNSET_FAILED										((errno_t)21)   /* Unset failed */
#define E_SYNC_TEST_CASE_ALLOCATION_FAILED					((errno_t)22)   /* Synchrone test case allocation failed        */
#define E_CANNOT_CREATE_CHILD_STDOUT_READ_HANDLE			((errno_t)23)   /* Can't create the child std output read handle */
#define E_CANNOT_CREATE_CHILD_STDERR_READ_HANDLE			((errno_t)24)   /* Can't create the child std error read handle */
#define E_CANNOT_CREATE_CHILD_STDIN_WRITE_HANDLE			((errno_t)25)   /* Can't create the child std input write handle */
#define E_CANNOT_CREATE_STDOUT_READ_HANDLE					((errno_t)26)   /* Can't create the std output handle                   */
#define E_CANNOT_CREATE_STDIN_WRITE_HANDLE					((errno_t)27)   /* Can't create the std input handle                    */
#define E_CANNOT_CLOSE_CHILD_STDIN_TEMPORY_HANDLE			((errno_t)28)   /* Can't close the tempory child std input handle */
#define E_CANNOT_CLOSE_CHILD_STDOUT_TEMPORY_HANDLE			((errno_t)29)   /* Can't close the tempory child std output handle */
#define E_CANNOT_CREATE_CHILD_PROCESS						((errno_t)30)   /* Can't create the child process                                       */
#define E_CANNOT_CLOSE_PROCESS_THREAD_HANDLE				((errno_t)31)   /* Can't close the child process handle                         */
#define E_CANNOT_CLOSE_CHILD_STDOUT_HANDLE					((errno_t)32)   /* Can't close the child std output handle                      */
#define E_CANNOT_CLOSE_CHILD_STDIN_HANDLE					((errno_t)33)   /* Can't close the child std input handle                       */
#define E_CANNOT_CLOSE_CHILD_STDERR_HANDLE					((errno_t)34)   /* Can't close the child std error handle                       */
#define E_CANNOT_WRITE_ON_CHILD_STDIN						((errno_t)35)   /* Can't write on child std output                                      */
#define E_CANNOT_CREATE_READ_CHILD_OUTPUT_THREAD			((errno_t)36)   /* Can't create the read child output thread            */
#define E_WAIT_THREAD_FAILED								((errno_t)37)   /* Wait thread failed                                                           */
#define E_CANNOT_CLOSE_THREAD_HANDLE						((errno_t)38)   /* Can't close thread handle                                            */
#define E_CANNOT_CLOSE_READ_HANDLE							((errno_t)39)   /* Can't close the read handle                                          */
#define E_CANNOT_CLOSE_WRITE_HANDLE							((errno_t)40)   /* Can't close the write handle                                         */
#define E_WAIT_FAILURE										((errno_t)41)   /* Wait failure                                                                         */
#define E_CANNOT_CLOSE_PROCESS_HANDLE						((errno_t)42)   /* Can't close the process handle                                       */
#define E_OUTPUT_DONT_MATCH									((errno_t)43)   /* Output don't match                                                           */
#define E_OPEN_FILE_FAILED                                  ((errno_t)44)       /* Open file failed */
#define E_INVALID_TOKEN                                     ((errno_t)45)       /* Invalid token */
#define E_WAIT_TIMEOUT                                      ((errno_t)46)       /* Wait timeout detected  */
#define E_EXIT_CODE_DONT_MATCH                              ((errno_t)47)       /* Exit code don't match    */
#define E_CHANGE_DIRECTORY_FAILED                           ((errno_t)48)       /* Change directory failed  */

/* error message list */
static const char *__errlist[] = {
  "Success",
  "Test runner allocation failed",
  "Test case context allocation failed",
  "Buffer allocation failed",
  "Buffer data allocation failed",
  "Test suite allocation failed",
  "File not found",
  "Bad usage",
  "Invalid file stream",
  "Stream allocation failed",
  "Buffer data reallocation failed",
  "Stream line allocation failed",
  "Stream line reallocation failed",
  "File empty",
  "File error",
  "Unknown meta command detected",
  "Invalid timeout value",
  "Invalid exit code",
  "Invalid export meta command",
  "Invalid unset meta command",
  "Export failed",
  "Unset failed",
  "Synchrone test case allocation failed",
  "Can't create the child std output read handle",
  "Can't create the child std error read handle",
  "Can't create the child std input write handle",
  "Can't create the std output handle",
  "Can't create the std input handle",
  "Can't close the tempory child std input handle",
  "Can't close the tempory child std output handle",
  "Can't create the child process",
  "Can't close the child process handle",
  "Can't close the child std output handle",
  "Can't close the child std input handle",
  "Can't close the child std error handle",
  "Can't write on child std output",
  "Can't create the read child output thread",
  "Wait thread failed",
  "Can't close thread handle",
  "Can't close the read handle",
  "Can't close the write handle",
  "Wait failure",
  "Can't close the process handle",
  "Output don't match",
  "Open file failed",
  "Invalid token",
  "Wait timeout detected",
  "Exit code don't match",
  "Change directory failed"
};

extern void initializeErrno(void);

extern void terminateErrno(void);

extern void setErrno(errno_t e);

extern errno_t getErrno(void);

#endif                          /* #ifndef __ERRNO_H__ */
