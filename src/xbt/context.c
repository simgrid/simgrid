/* 	$Id$	 */

/* a fast and simple context switching library                              */

/* Copyright (c) 2004 Arnaud Legrand.                                       */
/* Copyright (c) 2004 Martin Quinson.                                       */
/* All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "portable.h"
#include "context_private.h"
#include "xbt/error.h"
#include "xbt/dynar.h"
#include "gras_config.h"
XBT_LOG_NEW_DEFAULT_SUBCATEGORY(context, xbt, "Context");

#ifndef HAVE_UCONTEXT_H
/* don't want to play with conditional compilation in automake tonight, sorry.
   include directly the c file from here when needed. */
# include "context_win32.c" 
#endif

static context_t current_context = NULL;
static xbt_dynar_t context_to_destroy = NULL;

void context_init(void)
{
  if(!current_context) {
    current_context = xbt_new0(s_context_t,1);
    context_to_destroy = xbt_dynar_new(sizeof(context_t),xbt_free);
  }
}

void context_empty_trash(void)
{
  xbt_dynar_reset(context_to_destroy);
}

static void *__context_wrapper(void *c)
{
  context_t context = c;
  int i;

/*   msg_global->current_process = process; */

  /*  WARNING("Calling the main function"); */
  (context->code) (context->argc,context->argv);

  for(i=0;i<context->argc; i++) 
    if(context->argv[i]) xbt_free(context->argv[i]);
  if(context->argv) xbt_free(context->argv);

  xbt_dynar_push(context_to_destroy, &context);

  context_yield(context);

  return NULL;
}

void context_start(context_t context) 
{

/*   TBX_FIFO_insert(msg_global->process, process); */
/*   TBX_FIFO_insert(msg_global->process_to_run, process); */

  /*  WARNING("Assigning __MSG_process_launcher to context (%p)",context); */
  makecontext (&(context->uc), (void (*) (void)) __context_wrapper,
	       1, context);

  return;
}

context_t context_create(context_function_t code,
			 int argc, char *argv[])
{
  context_t res = NULL;

  res = xbt_new0(s_context_t,1);

  /*  WARNING("Initializing context (%p)",res); */

  xbt_assert0(getcontext(&(res->uc))==0,"Error in context saving.");

  /*  VOIRP(res->uc); */
  res->code = code;
  res->uc.uc_link = &(current_context->uc); /* FIXME LATER */
  /* WARNING : when this context is over, the current_context (i.e. the 
     father), is awaken... May result in bugs later.*/
  res->uc.uc_stack.ss_sp = res->stack;
  res->uc.uc_stack.ss_size = STACK_SIZE;
  return res;
}

static void context_destroy(context_t context)
{
  xbt_free(context);

  return;
}

void context_yield(context_t context)
{

  xbt_assert0(current_context,"You have to call context_init() first.");

  /*   __MSG_context_init(); */
  /*  fprintf(stderr,"\n"); */
  /*  WARNING("--------- current_context (%p) is yielding to context(%p) ---------",current_context,context); */
  /*  VOIRP(current_context); */
  /*  if(current_context) VOIRP(current_context->save); */
  /*  VOIRP(context); */
  /*  if(context) VOIRP(context->save); */

  if (context) {
/*     m_process_t self = msg_global->current_process; */
    if(context->save==NULL) {
      /*      WARNING("**** Yielding to somebody else ****"); */
      /*      WARNING("Saving current_context value (%p) to context(%p)->save",current_context,context); */
      context->save = current_context ;
      context->uc.uc_link = &(current_context->uc);
      /*      WARNING("current_context becomes  context(%p) ",context); */
      current_context = context ;
      /*      WARNING("Current position memorized (context->save). Jumping to context (%p)",context); */
      if(!swapcontext (&(context->save->uc), &(context->uc))) 
	xbt_assert0(0,"Context swapping failure");
      /*      WARNING("I am (%p). Coming back\n",context); */
    } else {
      context_t old_context = context->save ;
      /*      WARNING("**** Back ! ****"); */
      /*      WARNING("Setting current_context (%p) to context(%p)->save",current_context,context); */
      current_context = context->save ;
      /*      WARNING("Setting context(%p)->save to NULL",current_context,context); */
      context->save = NULL ;
      /*      WARNING("Current position memorized (%p). Jumping to context (%p)",context,old_context); */
      if(!swapcontext (&(context->uc), &(old_context->uc)) ) 
	xbt_assert0(0,"Context swapping failure");
      /*      WARNING("I am (%p). Coming back\n",context); */
    }
/*     msg_global->current_process = self; */
  }

  return;
}


/****************************** PTHREADS ***************************/

/* FIXME: this is dead code as we don't really need it.
   If you have pthreads, you have contexts (which are better) */

#if 0 
#ifdef HAVE_LIBPTHREAD  /* Nevermind, let's use pthreads... */

static void *__MSG_process_launcher(void *p)
{
  m_process_t process = (m_process_t) p;
  sim_data_process_t sim_data = process->simdata;
  context_t context = NULL;
  int i;
  msg_global->current_process = process;
  TBX_FIFO_insert(msg_global->process, process);

  TBX_FIFO_insert(msg_global->process_to_run, process);
/*   pthread_mutex_lock(&(sim_data->context->mutex)); */
  __MSG_context_yield(sim_data->context);

  (*sim_data->context->code) (sim_data->argc,sim_data->argv);

  TBX_FIFO_remove(msg_global->process, process);
  TBX_FIFO_remove(msg_global->process_to_run, process);

  /* Free blocked process if any */
/*   pthread_mutex_lock(&(sim_data->context->mutex)); */
/*   pthread_cond_signal(&(sim_data->context->cond)); */
/*   pthread_mutex_unlock(&(sim_data->context->mutex)); */

  context = sim_data->context;

  for(i=0;i<sim_data->argc; i++) 
    if(sim_data->argv[i]) FREE(sim_data->argv[i]);
  if(sim_data->argv) FREE(sim_data->argv);
  if(process->name) FREE(process->name);
  FREE(sim_data);
  FREE(process);

  TBX_FIFO_insert(msg_global->context_to_destroy,context);
  __MSG_context_yield(context);
/*   __MSG_context_destroy(&context); */

  return NULL;
}

MSG_error_t __MSG_context_start(m_process_t process) 
{
  sim_data_process_t sim_data = NULL;

  sim_data = process->simdata;

  pthread_mutex_lock(&(sim_data->context->mutex));

  /* Launch the thread */
  if (pthread_create(sim_data->context->thread, NULL, __MSG_process_launcher,
		     process) != 0) {
    WARNING("Unable to create a thread.");
    return MSG_FATAL;
  }
  pthread_cond_wait(&(sim_data->context->cond), &(sim_data->context->mutex));
  pthread_mutex_unlock(&(sim_data->context->mutex));
/*   __MSG_context_yield(sim_data->context); */

  return MSG_OK;
}

context_t __MSG_context_create(m_process_code_t code)
{
  context_t res = NULL;

  res = CALLOC(1, sizeof(struct s_context));

  res->code = code;
  res->thread = CALLOC(1, sizeof(pthread_t));
  if (pthread_mutex_init(&(res->mutex), NULL) != 0)
    FAILURE("Mutex initialization error");
  if (pthread_cond_init(&(res->cond), NULL) != 0)
    FAILURE("Condition initialization error");

  return res;
}

MSG_error_t __MSG_context_destroy(context_t * context)
{
  ASSERT(((context != NULL) && (*context != NULL)), "Invalid parameters");

  FREE((*context)->thread);
  pthread_mutex_destroy(&((*context)->mutex));
  pthread_cond_destroy(&((*context)->cond));

  FREE(*context);
  *context = NULL;

  return MSG_OK;
}

MSG_error_t __MSG_context_yield(context_t context)
{
  if (context) {
    m_process_t self = msg_global->current_process;
    pthread_mutex_lock(&(context->mutex));
    pthread_cond_signal(&context->cond);
    pthread_cond_wait(&context->cond, &context->mutex);
    pthread_mutex_unlock(&(context->mutex));
    msg_global->current_process = self;
  }
  return MSG_OK;
}
#endif /* HAVE_LIBPTHREAD */
#endif /* 0 */
