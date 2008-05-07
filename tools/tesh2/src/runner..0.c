#include <runner.h>
#include <unit.h>

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
		exit_code = E_GLOBAL_TIMEOUT;
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
			exit_code = E_GLOBAL_TIMEOUT;
			runner->timeouted = 1;
			xbt_os_sem_release(units_sem);
		}
	}
	
	return NULL;
}
#endif


int
runner_init(
					int want_check_syntax, 
					int timeout,
					int number_of_files, 
					strings_t files, 
					FILE** streams)
{
	int i, rv;
	int j = 0;
	

	runner = xbt_new0(s_runner_t, 1);
	
	runner->units = xbt_new0(unit_t, number_of_files);

	if(!(runner->units))
	{
		/* TODO : display the error */
		free(runner);
		runner = NULL;
	}
	
	
	runner->number_of_units = number_of_files;

	runner->timeout = timeout;
	runner->timeouted = 0;
	runner->interrupted = 0;
	runner->number_of_ended_units = 0;
	runner->number_of_runned_units = 0;
	runner->waiting = 0;
	
	for(i = 0; i < files->number; i++)
	{
		if(streams[i])
			(runner->units)[j++] = unit_new(runner, NULL, files->items[i], streams[i]);
	}
	
	
	if(want_check_syntax)
	{
		if((rv = check_syntax()))
			return rv;		
	}
		
	return 0;
		
}

void
runner_destroy(void)
{
	int i, size;

	size = runner->number_of_units;

	for(i = 0; i < size; i++)
	{
		unit_free(&(runner->units[i]));
	}

	free(runner->units);

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
	int i;
	xbt_os_mutex_t mutex;
	xbt_os_thread_t thread;

	mutex = xbt_os_mutex_init();
	
	for(i = 0; i < runner->number_of_units; i++)
	{
		if(!unit_run(runner->units[i], mutex))
		{
			ERROR1("Can't run the unit %s",runner->units[i]->file_name);
			interrupted = 1;
			break;
		}
	}
	
	if(!interrupted)
		runner_wait();
	
	/* if the runner is timeouted or receive a interruption request
	 * , interrupt all the active units.
	 */

	if(runner->timeouted || interrupted)
		runner_interrupt();
	
	
	/*printf("the runner try to join all the units\n");*/
	
	for(i = 0; i < runner->number_of_units; i++)
	{
		thread = runner->units[i]->thread;
		
		if(thread)
			xbt_os_thread_join(thread, NULL);
	}
	
	/*printf("the runner has joined all the units\n");*/

	xbt_os_mutex_destroy(mutex);

}

static void
runner_wait(void)
{
	if(runner->timeout > 0)
		runner->thread = xbt_os_thread_create("", runner_start_routine, NULL);

	/* signal that the runner is waiting */
	runner->waiting = 1;

	/*printf("the runner try to acquire the units sem\n");*/
	/* wait for the end of all the units */
	xbt_os_sem_acquire(units_sem);
	/*printf("the runner has acquired the units sem\n");*/
	
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
	int i;
	int size;
	
	size = runner->number_of_units;

	for(i = 0; i < size; i++)
		if(!(runner->units[i]->successeded) && !(runner->units[i]->interrupted))
			unit_interrupt(runner->units[i]);
}

void
runner_display_status(void)
{
	if(!want_dry_run)
	{
		int i, size;
		
		size = runner->number_of_units;
	
		printf("Runner\n");
		printf("Status informations :\n");
	
		printf("    number of units     %d\n",size);
		
		if(exit_code)
			printf("    exit code           %d (failure)\n",exit_code);
		else
			printf("    exit code           %d (success)\n",exit_code);
		
		
			for(i = 0; i < size; i++)
				unit_display_status(runner->units[i]); 
	}
	else
	{
		if(exit_code == E_SYNTAX)
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
			int i;
			unit_t unit;
			
			if(!want_silent)
				INFO0("syntax checked (OK)");
			
			for(i = 0; i < runner->number_of_units; i++)
			{
				unit = runner->units[i];
				
				fseek(unit->stream,0L, SEEK_SET);
				unit->parsed = 0;
				unit->number_of_commands = 0;
			}
		
		}
		
	}
	else
	{
		WARN0("mismatch in the syntax : --just-check-syntax and --check-syntax options at same time");
	}

	return exit_code;
}
