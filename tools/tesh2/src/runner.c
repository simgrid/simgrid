/*
 * src/runner.c - type representing the runner.
 *
 * Copyright 2008,2009 Martin Quinson, Malek Cherier All right reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the license (GNU LGPL) which comes with this package.
 *
 * Purpose:
 * 		This file contains all the definitions of the functions related with
 * 		the tesh runner type.
 *
 */
 
#include <runner.h>
#include <units.h>
#include <unit.h>
#include <xerrno.h>
#include <variable.h>

#include <errno.h>	/* for error code	*/
#include <stdlib.h>	/* for calloc()		*/
#include <stdio.h>

#ifndef WIN32
#include <sys/resource.h>
#endif

#define _RUNNER_HASHCODE		0xFEFEAAAA	

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(tesh);

#ifndef WIN32
extern char**
environ;
#endif

/* the unique tesh runner */
static runner_t
runner = NULL;

/* wait for the tesh runner terminaison	*/
static void
runner_wait(void);

static void*
runner_start_routine(void* p);


/* check the syntax of the tesh files if 
 * the check_syntax_flag is specified. Returns
 * 0 if the syntax is clean.
 */
static void
check_syntax(void);

#ifdef WIN32

static HANDLE 
timer_handle = NULL;

static void*
runner_start_routine(void* p)
{
	
    LARGE_INTEGER li;

    li.QuadPart=- runner->timeout * 10000000;	/* 10000000 = 10 000 000 * 100 nanoseconds = 1 second */

    /* create the waitable timer */
    timer_handle = CreateWaitableTimer(NULL, TRUE, NULL);

    /* set a timer to wait for timeout seconds */
    SetWaitableTimer(timer_handle, &li, 0, NULL, NULL, 0);
    
    /* wait for the timer */
    WaitForSingleObject(timer_handle, INFINITE);
	
	if(runner->waiting)
	{
		exit_code = ELEADTIME;
		runner->timeouted = 1;
		xbt_os_sem_release(units_sem);
	}

	return NULL;
}

#else
static void*
runner_start_routine(void* p)
{
	struct timespec ts;

	ts.tv_sec = runner->timeout;
	ts.tv_nsec = 0L;

	do
	{
		nanosleep(&ts, &ts);
	}while(EINTR == errno);
	
	if(errno)
	{
		/* TODO process the error */
	}
	else
	{
		if(runner->waiting)
		{
			exit_code = ELEADTIME;
			runner->timeouted = 1;
			xbt_os_sem_release(units_sem);
		}
	}
	
	return NULL;
}
#endif


int
runner_init(int check_syntax_flag, int timeout, fstreams_t fstreams)
{
	
	int i;
	char* val;
	char buffer[PATH_MAX + 1] = {0};
	int code;
	const char* cstr;
	variable_t variable;
	
	if(runner)
		return EALREADY;
		
	runner = xbt_new0(s_runner_t, 1);
	
	if(!(runner->units = units_new(runner, fstreams)))
	{
		free(runner);
		runner = NULL;
		return -1;
	}

	runner->timeout = timeout;
	runner->timeouted = 0;
	runner->interrupted = 0;
	runner->number_of_ended_units = 0;
	runner->number_of_runned_units = 0;
	runner->waiting = 0;
	
	runner->total_of_tests = 0;
	runner->total_of_successeded_tests = 0;
	runner->total_of_failed_tests = 0;
	runner->total_of_interrupted_tests = 0;
	
	runner->total_of_units = 0;
	runner->total_of_successeded_units = 0;
	runner->total_of_failed_units = 0;
	runner->total_of_interrupted_units = 0;
	
	runner->total_of_suites = 0;
	runner->total_of_successeded_suites = 0;
	runner->total_of_failed_suites = 0;
	runner->total_of_interrupted_suites = 0;
	
	/* initialize the vector of variables */
	runner->variables = xbt_dynar_new(sizeof(variable_t), (void_f_pvoid_t)variable_free);
	
	/* add the environment variables in the vector */
	for(i = 0; environ[i] != NULL; i++)
	{
		val = strchr(environ[i], '=');
		
		if(val)
		{
			val++;
				
			if(val[0] != '\0')
				strncpy(buffer, environ[i], (val - environ[i] -1));
				
			if(!strcmp("TESH_PPID", buffer))
				is_tesh_root = 0;
			
			variable = variable_new(buffer, val);
			variable->env = 1;
			xbt_dynar_push(runner->variables, &variable);
		}
	}
	
	if(is_tesh_root)
	{
		sprintf(buffer,"%d",getpid());
		
		#ifndef WIN32
		setenv("TESH_PPID", buffer, 0);
		#else
		SetEnvironmentVariable("TESH_PPID", buffer);
		#endif
		
		variable = variable_new("TESH_PPID", buffer);
		variable->env = 1;
			
		xbt_dynar_push(runner->variables, &variable);
	}
	
	i = 0;
	
	/* add the errors variables */
	while((cstr = error_get_at(i++, &code)))
	{
		sprintf(buffer,"%d",code);
		variable = variable_new(cstr, buffer);
		variable->err = 1;
		xbt_dynar_push(runner->variables, &variable);
	}
	
	/* if the user want check the syntax, check it */
	if(check_syntax_flag)
		check_syntax();
		
	return exit_code ? -1 : 0;
		
}

void
runner_destroy(void)
{
	units_free((void**)(&(runner->units)));

	xbt_dynar_free(&runner->variables);
	
	#ifdef WIN32
	CloseHandle(timer_handle);
	#endif

	if(runner->thread)
		xbt_os_thread_join(runner->thread, NULL);

	free(runner);
	

	runner = NULL;
}

void
runner_run(void)
{
	/* allocate the mutex used by the units to asynchronously access 
	 * to the properties of the runner.
	 */
	xbt_os_mutex_t mutex = xbt_os_mutex_init();
	
	/* run all the units */
	units_run_all(runner->units, mutex);
	
	if(!interrupted)
		runner_wait();
	
	/* if the runner is timeouted or receive a interruption request
	 * , interrupt all the active units.
	 */
	if(runner->timeouted || interrupted)
		runner_interrupt();
	
	/* joins all the units */
	units_join_all(runner->units);
	
	/* release the mutex resource */
	xbt_os_mutex_destroy(mutex);

}

static void
runner_wait(void)
{
	if(runner->timeout > 0)
		runner->thread = xbt_os_thread_create("", runner_start_routine, NULL);
	
	/* signal that the runner is waiting */
	runner->waiting = 1;
	
	/* wait for the end of all the units */
	xbt_os_sem_acquire(units_sem);
	
	runner->waiting = 0;
}



/*
 * interrupt all the active units.
 * this function is called when the lead time of the execution is reached
 * or when a failed unit requests an interruption of the execution.
 */
void
runner_interrupt(void)
{
	units_interrupt_all(runner->units);
}

void
runner_summarize(void)
{
	
	if(!dry_run_flag)
	{
		#ifndef WIN32
		struct rusage r_usage;
		#else
		FILETIME start_time;
		FILETIME exit_time;
		FILETIME kernel_time;
		FILETIME user_time;
		SYSTEMTIME si;
		#endif
		
		printf("\n  TEst SHell utility - mini shell specialized in running test units.\n");
		printf(" =============================================================================\n");
		
		units_summuarize(runner->units);
		
		printf(" =====================================================================%s\n",
		runner->total_of_failed_tests ? "== FAILED": (runner->total_of_interrupted_tests || runner->total_of_interrupted_units) ? "==== INTR" : "====== OK");
		
		printf(" TOTAL : Suite(s): %.0f%% ok (%d suite(s): %d ok",
		(runner->total_of_suites ? (1-((double)runner->total_of_failed_suites + (double)runner->total_of_interrupted_suites)/(double)runner->total_of_suites)*100.0 : 100.0),
		runner->total_of_suites, runner->total_of_successeded_suites);
		
		if(runner->total_of_failed_suites > 0)
			printf(", %d failed", runner->total_of_failed_suites);
		
		if(runner->total_of_interrupted_suites > 0)
			printf(", %d interrupted)", runner->total_of_interrupted_suites);
		
		printf(")\n");	
		
		printf("         Unit(s):  %.0f%% ok (%d unit(s): %d ok",
		(runner->total_of_units ? (1-((double)runner->total_of_failed_units + (double)runner->total_of_interrupted_units)/(double)runner->total_of_units)*100.0 : 100.0),
		runner->total_of_units, runner->total_of_successeded_units);
		
		if(runner->total_of_failed_units > 0)
			printf(", %d failed", runner->total_of_failed_units);
		
		if(runner->total_of_interrupted_units > 0)
			printf(", %d interrupted)", runner->total_of_interrupted_units);
		
		printf(")\n");
		
		printf("         Test(s):  %.0f%% ok (%d test(s): %d ok",
		(runner->total_of_tests ? (1-((double)runner->total_of_failed_tests + (double)runner->total_of_interrupted_tests)/(double)runner->total_of_tests)*100.0 : 100.0),
		runner->total_of_tests, runner->total_of_successeded_tests); 
		
		if(runner->total_of_failed_tests > 0)
			printf(", %d failed", runner->total_of_failed_tests);
		
		if(runner->total_of_interrupted_tests > 0)
			printf(", %d interrupted)", runner->total_of_interrupted_tests);
	 	
		printf(")\n\n");
		
		#ifndef WIN32
		if(!getrusage(RUSAGE_SELF, &r_usage))
		{
		
			printf("         Total tesh user time used:       %ld second(s) %ld microsecond(s)\n", r_usage.ru_utime.tv_sec, r_usage.ru_utime.tv_usec);
			printf("         Total tesh system time used:     %ld second(s) %ld microsecond(s)\n\n", r_usage.ru_stime.tv_sec, r_usage.ru_stime.tv_usec);
		
			if(!getrusage(RUSAGE_CHILDREN, &r_usage))
			{
				printf("         Total children user time used:   %ld second(s) %ld microsecond(s)\n", r_usage.ru_utime.tv_sec, r_usage.ru_utime.tv_usec);
				printf("         Total children system time used: %ld second(s) %ld microsecond(s)\n\n", r_usage.ru_stime.tv_sec, r_usage.ru_stime.tv_usec);
		
			}	
		}
		#else
	
		if(GetProcessTimes(GetCurrentProcess(), &start_time, &exit_time, &kernel_time, &user_time))
		{
			FileTimeToSystemTime(&user_time, &si);
			
			printf("         Total tesh user time used:       %uhour(s) %uminute(s) %usecond(s) %millisecond(s)\n", si.wHour, si.wMinute, si.wSecond, si.wMilliseconds );
			
			FileTimeToSystemTime(&kernel_time, &si);
			
			printf("         Total tesh kernel time used:     %uhour(s) %uminute(s) %usecond(s) %millisecond(s)\n", si.wHour, si.wMinute, si.wSecond, si.wMilliseconds );
		}



		#endif
	}
	else
	{
		if(exit_code)
			ERROR0("Syntax error detected");
		else if(!exit_code)
			INFO0("Syntax 0K");
	}
}

static void
check_syntax(void)
{
	if(!dry_run_flag)
	{
		dry_run_flag = 1;
		
		runner_run();
	
		dry_run_flag = 0;
		
		if(!exit_code)
		{
			if(!silent_flag)
				INFO0("syntax checked (OK)");
			
			units_reset_all(runner->units);
		
		}
		else
			errno = exit_code;
		
	}
	else
	{
		WARN0("mismatch in the syntax : --just-check-syntax and --check-syntax options at same time");
	}

}
