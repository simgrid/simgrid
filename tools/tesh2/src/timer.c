#include <timer.h>
#include <command.h>

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(tesh);

static void*
timer_start_routine(void* p);

ttimer_t
timer_new(command_t command)
{
	ttimer_t timer = xbt_new0(s_timer_t, 1);
	
	timer->command = command;
	timer->thread = NULL;
	timer->timeouted = 0;
	timer->started = xbt_os_sem_init(0);

	return timer;
}

void
timer_free(ttimer_t* timer)
{
	free(*timer);
	*timer = NULL;
}

void
timer_time(ttimer_t timer)
{
	timer->thread = xbt_os_thread_create("", timer_start_routine, timer);
}

static void*
timer_start_routine(void* p)
{
	ttimer_t timer = (ttimer_t)p;
	command_t command = timer->command;
	
	int now = (int)time(NULL);
	int lead_time = now + command->context->timeout;
	
	xbt_os_sem_release(timer->started);
	
	while(!command->failed && !command->interrupted && !command->successeded && !timer->timeouted) 
	{
		if(lead_time >= now)
		{
			xbt_os_thread_yield();
			/*usleep(100);*/
			now = (int)time(NULL);
		}
		else
		{
			/*printf("the timer is timeouted\n");*/
			timer->timeouted = 1;
		}
	}

	if(timer->timeouted && !command->failed && !command->successeded  && !command->interrupted)
	{
		command_handle_failure(command, csr_timeout);
		command_kill(command);
		exit_code = ETIMEOUT;
		
	}
	
  	return NULL;
}

void
timer_wait(ttimer_t timer)
{
	xbt_os_thread_join(timer->thread, NULL);
}
