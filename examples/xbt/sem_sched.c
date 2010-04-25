/* Copyright (c) 2007, 2008. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>


#include "xbt/xbt_os_thread.h"
#include "xbt.h"
#include "xbt/log.h"
XBT_LOG_NEW_DEFAULT_CATEGORY(sem_sched,"Messages specific for this sem example");

#ifndef ENOBUFS
#define ENOBUFS		1024
#endif

#define CTX_MAX			((unsigned int)1000)


#define MAX_ARG						30
#define MAX_ARGS					10

typedef int (*pfn_func_t)(int, char**);

static int
__next_ctx_ID = 0;

typedef struct s_job
{
	pfn_func_t func;
	int argc;
	char** argv;
}s_job_t,* job_t;


job_t
job_new(pfn_func_t func, int argc, char** argv);

int
job_execute(job_t job);

int
job_free(job_t* ref);

/* an entry of the table of threads */
typedef struct s_ctx
{
	xbt_os_sem_t begin;
	xbt_os_sem_t end;
	int failure;
	job_t job;
	xbt_os_thread_t imp;
	int index;
}s_ctx_t,* ctx_t;

typedef struct s_shed
{
	ctx_t* ctxs;
	int size;
	int capacity;
}s_sched_t,* sched_t;


void 
schedule(ctx_t c);
void 
unschedule(ctx_t c);

void*
ctx_function(void* param);

ctx_t 
ctx_new(job_t job);

int 
ctx_free(ctx_t* ref);

sched_t
sched_new(int size);

int
sched_add_job(sched_t sched, job_t job);

int
sched_init(sched_t sched);

int
sched_schedule(sched_t sched);

int
sched_clean(sched_t sched);

int
sched_free(sched_t* ref);

int
job(int argc, char* argv[]);

int
main(int argc, char* argv[])
{
	sched_t sched;
	int i, size;
	char** __argv;
	char arg[MAX_ARG] = {0};
	
	
	xbt_init(&argc, argv);
	
	if(argc != 2) 
	{
		INFO1("Usage: %s job count",argv[0]);
		exit(EXIT_FAILURE);
	}  
	
	
	size = atoi(argv[1]);
	
	/* create a new scheduler */
	sched = sched_new(size);
	
	if(!sched)
	{
		INFO1("sched_new() failed : errno %d",errno);
		xbt_exit();
		exit(EXIT_FAILURE);
	}
	
	__argv = xbt_new0(char*,MAX_ARGS);
	
	for(i = 0; i < MAX_ARGS; i++)
	{
		sprintf(arg,"arg_%d",i);
		__argv[i] = strdup(arg);
		memset(arg,0,MAX_ARG);
		
	}
	
	for(i = 0; i < size; i++)
		sched_add_job(sched,job_new(job,(i < MAX_ARGS) ? i : MAX_ARGS,__argv));
	
	/* initialize the scheduler */
	if(sched_init(sched) < 0)
	{
		sched_free(&sched);
		INFO1("sched_init() failed : errno %d\n",errno);
		xbt_exit();
		exit(EXIT_FAILURE);
	}
	
	/* schedule the jobs */
	if(sched_schedule(sched) < 0)
	{
		sched_free(&sched);
		INFO1("sched_init() failed : errno %d",errno);
		xbt_exit();
		exit(EXIT_FAILURE);
	}
	
	/* cleanup */
	if(sched_clean(sched) < 0)
	{
		sched_free(&sched);
		INFO1("sched_init() failed : errno %d",errno);
		xbt_exit();
		exit(EXIT_FAILURE);
	}
	
	/* destroy the scheduler */
	sched_free(&sched);
	
	INFO1("sem_sched terminated with exit code %d (success)",EXIT_SUCCESS);

	xbt_exit();
	
	return EXIT_SUCCESS;
		
}

void*
ctx_function(void* param)
{
	int i = 0;
	int exit_code = 1;
	ctx_t ctx = (ctx_t)param;
	
	INFO1("Hello i'm the owner of the context %d, i'm waiting for starting",ctx->index);
	
	unschedule(ctx);
	
	if(ctx->failure)
	{
		INFO1("0ups the scheduler initialization failed bye {%d}.",ctx->index);
		xbt_os_thread_exit(&exit_code);
	}
	
	INFO1("I'm the owner of the context %d : I'm started",ctx->index);
	INFO0("Wait a minute, I do my job");
	
	/* do its job */
	exit_code = job_execute(ctx->job);
	
	INFO1("Have finished my job, bye {%d}\n",ctx->index);
	
	xbt_os_sem_release(ctx->end);
	
	xbt_os_thread_exit(&exit_code);
}

void schedule(ctx_t c) 
{
	xbt_os_sem_release(c->begin);	/* allow C to go 		*/
	xbt_os_sem_acquire(c->end);	/* wait C's end 		*/
}

void unschedule(ctx_t c) 
{
   xbt_os_sem_release(c->end);		/* I'm done, dude 		*/
   xbt_os_sem_acquire(c->begin);	/* can I start again? 	*/
}

ctx_t 
ctx_new(job_t job)
{
	ctx_t ctx = xbt_new0(s_ctx_t,1);
	ctx->index = ++__next_ctx_ID;
	ctx->begin = xbt_os_sem_init(0);
	ctx->end = xbt_os_sem_init(0);
	ctx->failure = 0;
	ctx->job = job;
	
	return ctx;
}

int 
ctx_free(ctx_t* ref)
{
	ctx_t ctx;
	if(!(*ref))
		return EINVAL;
	
	ctx = *ref;
	
	xbt_os_sem_destroy(ctx->begin);
	xbt_os_sem_destroy(ctx->end);
	job_free(&(ctx->job));
	free(ctx);
	*ref = NULL;
	
	return 0;
}

sched_t
sched_new(int size)
{
	sched_t sched;
	
	if(size <= 0)
	{
		errno = EINVAL;
		return NULL;
	}
	
	sched = xbt_new0(s_sched_t,1);
	
	if(!sched)
	{
		errno = ENOMEM;
		return NULL;
	}
		
	sched->ctxs = xbt_new0(ctx_t,size);
	
	if(!(sched->ctxs))
	{
		errno = ENOMEM;
		free(sched);
		return NULL;
	}
	
	sched->size = 0;
	sched->capacity = size;
	
	return sched;
}

int
sched_add_job(sched_t sched, job_t job)
{
	if(!sched || !job)
		return EINVAL;
	
	if(sched->capacity < sched->size)
		return ENOBUFS;
		
	sched->ctxs[(sched->size)++] = ctx_new(job);
	
	return 0;
}

int
sched_init(sched_t sched)
{
	int i,j;
	int success = 1;
	
	if(!sched)
		return EINVAL;
	
	for(i = 0; i < sched->size; i++)
	{	
		sched->ctxs[i]->imp = xbt_os_thread_create("thread",ctx_function,(void*)sched->ctxs[i]);
		
		xbt_os_sem_acquire(sched->ctxs[i]->end);
	}
	
	if(!success)
	{
		for(j = 0; j < i; j++)
		{
			sched->ctxs[j]->failure = 1;
			xbt_os_sem_release(sched->ctxs[j]->begin);
		}
			
		for(j = 0; j < i; j++)
		{
			xbt_os_thread_join(sched->ctxs[j]->imp,0);
			
			ctx_free(&(sched->ctxs[j]));
		}
		
		return -1;
					
	}
	
	return 0;
}

int
sched_schedule(sched_t sched)
{
	int i;
	
	if(!sched)
		return EINVAL;
		
	for(i = 0; i < sched->size; i++)
		schedule(sched->ctxs[i]);
		
	return 0;
}

int
sched_clean(sched_t sched)
{
	int i;
	
	if(!sched)
		return EINVAL;
		
	for(i = 0; i < sched->size; i++)
	{
		xbt_os_thread_join(sched->ctxs[i]->imp,NULL);
		
		ctx_free(&(sched->ctxs[i]));
	}
	
	return 0;
}

int
sched_free(sched_t* ref)
{
	if(*ref)
		return EINVAL;
		
	free(((sched_t)(*ref))->ctxs);
	
	*ref = NULL;
	
	return 0;
}


int
job(int argc, char** argv)
{
	int i = 0;
	
	INFO0("I'm the job : I'm going to print all the args of my commande line");
	
	INFO1("-- Arguments (%d):",argc);
	
	for(i = 0; i < argc; i++)
		INFO2("  ---- [%i] %s",i,argv[i]);
		
	return 0;
}

job_t
job_new(pfn_func_t func, int argc, char** argv)
{
	job_t job;
	int i;
	
	/* todo check the parameters */
	job = xbt_new0(s_job_t,1);
	
	if(!job)
	{
		errno = ENOMEM;
		return NULL;
	}
	
	job->argv = xbt_new0(char*,argc);
	
	if(!(job->argv))
	{
		free(job);
		errno = ENOMEM;
		return NULL;
	}
	
	for(i = 0; i < argc; i++)
		job->argv[i] = strdup(argv[i]);
	
	job->func = func;	
	job->argc = argc;
	
	return job;
}

int
job_execute(job_t job)
{
	if(!job)
		return EINVAL;
		
	return (*(job->func))(job->argc, job->argv);
}

int
job_free(job_t* ref)
{
	job_t job;
	int i;
	
	if(!(*ref))
		return EINVAL;
		
	job = *ref;
	
	for(i = 0; i < job->argc; i++)
		free(job->argv[i]);
	
	free(job->argv);
	free(*ref);
	*ref = NULL;
	
	return 0;
}



