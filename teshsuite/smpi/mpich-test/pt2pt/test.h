/* Header for testing procedures */

#ifndef _INCLUDED_TEST_H_
#define _INCLUDED_TEST_H_

#include "mpi.h"

#if defined(NEEDS_STDLIB_PROTOTYPES)
#include "protofix.h"
#endif

void Test_Init (const char *, int);
#ifdef USE_STDARG
void Test_Printf (const char *, ...);
void Test_Errors_warn ( MPI_Comm *, int *, ... );
#else
/* No prototype */
void Test_Printf();
void Test_Errors_warn();
#endif
void Test_Message (const char *);
void Test_Failed (const char *);
void Test_Passed (const char *);
int Summarize_Test_Results (void);
void Test_Finalize (void);
void Test_Waitforall (void);

extern MPI_Errhandler TEST_ERRORS_WARN;
#endif
