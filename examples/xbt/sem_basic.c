/* Copyright (c) 2007. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>


#include "xbt/xbt_os_thread.h"
#include "xbt.h"
#include "xbt/log.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(sem_basic,"Messages specific for this sem example");



#define THREAD_THREADS_MAX			((unsigned int)10)

/*
 * the thread funtion.
 */
void*
thread_routine(void* param);

/* an entry of the table of threads */
typedef struct s_thread_entry
{
	xbt_os_thread_t thread;
	unsigned int thread_index;	/* the index of the thread	*/
}s_thread_entry_t,* thread_entry_t;


static xbt_os_sem_t 
sem = NULL;

static
int value = 0;
int
main(int argc, char* argv[])
{
	s_thread_entry_t threads_table[THREAD_THREADS_MAX] = {0};	
	unsigned int i,j;
	int exit_code = 0;
	
	xbt_init(&argc,argv);
	
	sem = xbt_os_sem_init(1);
	
	i = 0;
	
	while(i < THREAD_THREADS_MAX)
	{
		threads_table[i].thread_index = i;

		if(NULL == (threads_table[i].thread = xbt_os_thread_create("thread",thread_routine,&(threads_table[i].thread_index))))
			break;
	
		i++;
	}
	
	/* close the thread handles */
	for(j = 0; j < THREAD_THREADS_MAX; j++)
		xbt_os_thread_join(threads_table[j].thread,NULL);
	
	xbt_os_sem_destroy(sem);
	
	INFO1("sem_basic terminated with exit code %d (success)",EXIT_SUCCESS);

	return EXIT_SUCCESS;
		
}

void*
thread_routine(void* param)
{
	int thread_index = *((int*)param);
	int exit_code = 0;
	
	xbt_os_sem_acquire(sem);
	INFO1("Hello i'm the thread %d",thread_index);
	value++;
	INFO1("The new value of the global variable is %d, bye",value);
	xbt_os_sem_release(sem);
	
	xbt_os_thread_exit(&exit_code);

	return (void*)(NULL);
}



