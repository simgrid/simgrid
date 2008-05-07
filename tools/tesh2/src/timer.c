/*
 * src/timer.c - type representing a timer.
 *
 * Copyright 2008,2009 Martin Quinson, Malek Cherier All right reserved. 
 *
 * This program is free software; you can redistribute it and/or modify it 
 * under the terms of the license (GNU LGPL) which comes with this package.
 *
 * Purpose:
 * 		This file contains all the definitions of the functions related with
 * 		the tesh timer type.
 *
 */
 
#include <timer.h>
#include <command.h>



XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(tesh);

static void*
timer_start_routine(void* p);

ttimer_t
timer_new(command_t command)
{
	ttimer_t timer;
	
	/* TODO : check the parameter */
	
	timer = xbt_new0(s_timer_t, 1);
	
	timer->command = command;
	timer->thread = NULL;
	timer->timeouted = 0;
	timer->started = xbt_os_sem_init(0);

	return timer;
}

int
timer_free(ttimer_t* ptr)
{
	/* TODO : check the parameter */
	
	free(*ptr);
	*ptr = NULL;
	
	return 0;
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
			now = (int)time(NULL);
		}
		else
		{
			timer->timeouted = 1;
		}
	}

	if(timer->timeouted && !command->failed && !command->successeded  && !command->interrupted)
	{
		error_register("Command timed out", ECMDTIMEDOUT, command->context->command_line, command->unit->fstream->name);
		command_kill(command);
		command_handle_failure(command, csr_timeout);
		
		
	}
	
  	return NULL;
}

void
timer_wait(ttimer_t timer)
{
	xbt_os_thread_join(timer->thread, NULL);
}
