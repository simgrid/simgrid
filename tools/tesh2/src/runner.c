#include <runner.h>
#include <units.h>
#include <error.h>

#include <errno.h>	/* for error code	*/
#include <stdlib.h>	/* for calloc()		*/
#include <stdio.h>	

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(tesh);

/* the unique tesh runner */
static runner_t
runner = NULL;

/* wait for the tesh runner terminaison	*/
static void
runner_wait(void);

static void*
runner_start_routine(void* p);


/* check the syntax of the tesh files if 
 * the want_check_syntax is specified. Returns
 * 0 if the syntax is clean.
 */
static int
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
runner_init(int want_check_syntax, int timeout, fstreams_t fstreams)
{
	
	if(runner)
	{
		ERROR0("Runner is already initialized");
		return EEXIST;
	}
		
		
	runner = xbt_new0(s_runner_t, 1);
	
	if(!(runner->units = units_new(runner, fstreams)))
	{
		ERROR0("Runner initialization failed");
		
		free(runner);
		runner = NULL;
		return errno;
	}

	runner->timeout = timeout;
	runner->timeouted = 0;
	runner->interrupted = 0;
	runner->number_of_ended_units = 0;
	runner->number_of_runned_units = 0;
	runner->waiting = 0;
	
	if(want_check_syntax)
	{
		if((errno = check_syntax()))
			return errno;		
	}
		
	return 0;
		
}

void
runner_destroy(void)
{
	units_free((void**)(&(runner->units)));

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
	xbt_os_mutex_t mutex;

	mutex = xbt_os_mutex_init();
	
	units_run_all(runner->units, mutex);
	
	if(!interrupted)
		runner_wait();
	
	/* if the runner is timeouted or receive a interruption request
	 * , interrupt all the active units.
	 */
	if(runner->timeouted || interrupted)
		runner_interrupt();
	
	units_join_all(runner->units);

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
runner_display_status(void)
{
	if(!want_dry_run)
	{
		
		/*unit_t unit;*/
	
		printf("Runner\n");
		printf("Status informations :\n");
	
		printf("    number of units     %d\n",units_get_size(runner->units));
		printf("    exit code           %d (%s)\n",exit_code, exit_code ? error_to_string(exit_code) : "success");
		
		units_verbose(runner->units);
	}
	else
	{
		if(exit_code)
			ERROR0("Syntax error detected");
		else if(exit_code == 0)
			INFO0("Syntax 0K");
	}
}

static int
check_syntax(void)
{
	if(!want_dry_run)
	{
		want_dry_run = 1;
		
		runner_run();
	
		want_dry_run = 0;
		
		if(0 == exit_code)
		{
			if(!want_silent)
				INFO0("syntax checked (OK)");
			
			units_reset_all(runner->units);
		
		}
		
	}
	else
	{
		WARN0("mismatch in the syntax : --just-check-syntax and --check-syntax options at same time");
	}

	return exit_code;
}
