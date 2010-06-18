/* $Id: signal.c 3483 2007-05-07 11:18:56Z mquinson $ */

/* signal -- what TESH needs to know about signals                          */

/* Copyright (c) 2007 Martin Quinson.                                       */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <xsignal.h>

#ifdef _XBT_WIN32
int
is_an_unhandled_exception(DWORD exit_code);

typedef struct s_exception_entry
{
	DWORD value;
	const char* signal;
}s_exception_entry_t,* exception_entry_t;
   
static const s_exception_entry_t exceptions[] =
{
	{EXCEPTION_ACCESS_VIOLATION, "SIGSEGV"},
	{EXCEPTION_ARRAY_BOUNDS_EXCEEDED, "SIGSEGV"},
	{EXCEPTION_BREAKPOINT, "SIGTRAP"},
	{EXCEPTION_DATATYPE_MISALIGNMENT, "SIGBUS"},
	{EXCEPTION_FLT_DENORMAL_OPERAND, "SIGFPE"},
	{EXCEPTION_FLT_DIVIDE_BY_ZERO, "SIGFPE"},
	{EXCEPTION_FLT_INEXACT_RESULT, "SIGFPE"},
	{EXCEPTION_FLT_INVALID_OPERATION, "SIGFPE"},
	{EXCEPTION_FLT_OVERFLOW, "SIGFPE"},
	{EXCEPTION_FLT_STACK_CHECK, "SIGFPE"},
	{EXCEPTION_FLT_UNDERFLOW, "SIGFPE"},
	{EXCEPTION_ILLEGAL_INSTRUCTION, "SIGILL"},
	{EXCEPTION_IN_PAGE_ERROR, "SIGSEGV"},
	{EXCEPTION_INT_DIVIDE_BY_ZERO, "SIGFPE"},
	{EXCEPTION_INT_OVERFLOW, "SIGFPE"},
	{EXCEPTION_STACK_OVERFLOW, "SIGILL"},
	{EXCEPTION_SINGLE_STEP, "SIGTRAP"},
	{EXCEPTION_NONCONTINUABLE_EXCEPTION, "SIGILL"},
	{EXCEPTION_PRIV_INSTRUCTION, "SIGILL"}
};
/* number of the entries in the table of exceptions */
#define MAX_EXECPTION			((unsigned int)19)

#endif

typedef struct s_signal_entry {
  const char* name;
  int number;
} s_signal_entry_t,* signal_entry_t;

static const s_signal_entry_t signals[] = {
	{"SIGHUP"	,SIGHUP},
	{"SIGINT"	,SIGINT},
	{"SIGQUIT"	,SIGQUIT},
	{"SIGILL"	,SIGILL},
	{"SIGTRAP"	,SIGTRAP},
	{"SIGABRT"	,SIGABRT},
	{"SIGFPE"	,SIGFPE},
	{"SIGKILL"	,SIGKILL},
	{"SIGBUS"	,SIGBUS},
	{"SIGSEGV"	,SIGSEGV},
	{"SIGSYS"	,SIGSYS},
	{"SIGPIPE"	,SIGPIPE},
	{"SIGALRM"	,SIGALRM},
	{"SIGTERM"	,SIGTERM},
	{"SIGURG"	,SIGURG},
	{"SIGSTOP"	,SIGSTOP},
	{"SIGTSTP"	,SIGTSTP},
	{"SIGCONT"	,SIGCONT},
	{"SIGCHLD"	,SIGCHLD},
	{"SIGTTIN"	,SIGTTIN},
	{"SIGTTOU"	,SIGTTOU},
	{"SIGIO"	,SIGIO},
	{"SIGXCPU"	,SIGXCPU},
	{"SIGXFSZ"	,SIGXFSZ},
	{"SIGVTALRM",SIGVTALRM},
	{"SIGPROF"	,SIGPROF},
	{"SIGWINCH"	,SIGWINCH},
	{"SIGUSR1"	,SIGUSR1},
	{"SIGUSR2"	,SIGUSR2},
	{"SIG UNKNOWN"  ,-1}
};

#ifdef _XBT_WIN32
const char* signal_name(DWORD got, const char* expected) 
#else
const char* signal_name(unsigned int got, char *expected) 
#endif
{
	int i;
	
	#ifdef _XBT_WIN32

	for (i=0; i < MAX_EXECPTION; i++)
		if (exceptions[i].value == got)
			return (exceptions[i].signal);
	#else
	if((got == SIGBUS) && !strcmp("SIGSEGV",expected))
		got = SIGSEGV;

	for (i=0; signals[i].number != -1; i++)
		if (signals[i].number == got)
			return (signals[i].name);
	
	#endif
	return bprintf("SIG UNKNOWN (%d)", got);
}

int
sig_exists(const char* sig_name)
{
	int i;

	for (i=0; signals[i].number != -1; i++)
		if (!strcmp(signals[i].name, sig_name))
			return 1; 

	/* not found */
	return 0;
}


#ifdef _XBT_WIN32
int
is_an_unhandled_exception(DWORD exit_code)
{
 	unsigned int i;

 	for(i = 0; i < MAX_EXECPTION; i++)
 		if(exceptions[i].value == exit_code)
 			return 1;
 	
 	return 0;	
}
#endif
