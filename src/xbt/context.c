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

/* #define WARNING(format, ...) (fprintf(stderr, "[%s , %s : %d] ", __FILE__, __FUNCTION__, __LINE__),\ */
/*                               fprintf(stderr, format, ## __VA_ARGS__), \ */
/*                               fprintf(stderr, "\n")) */
/* #define VOIRP(expr) WARNING("  {" #expr " = %p }", expr) */

#ifndef HAVE_UCONTEXT_H
/* don't want to play with conditional compilation in automake tonight, sorry.
   include directly the c file from here when needed. */
# include "context_win32.c" 
#endif

static xbt_context_t current_context = NULL;
static xbt_context_t init_context = NULL;
static xbt_swag_t context_to_destroy = NULL;
static xbt_swag_t context_living = NULL;

static void __xbt_context_yield(xbt_context_t context)
{
  int return_value = 0;

  xbt_assert0(current_context,"You have to call context_init() first.");
  
/*   WARNING("--------- current_context (%p) is yielding to context(%p) ---------",current_context,context); */
/*   VOIRP(current_context); */
/*   if(current_context) VOIRP(current_context->save); */
/*   VOIRP(context); */
/*   if(context) VOIRP(context->save); */

  if (context) {
    if(context->save==NULL) {
/*       WARNING("**** Yielding to somebody else ****"); */
/*       WARNING("Saving current_context value (%p) to context(%p)->save",current_context,context); */
      context->save = current_context ;
/*       WARNING("current_context becomes  context(%p) ",context); */
      current_context = context ;
/*       WARNING("Current position memorized (context->save). Jumping to context (%p)",context); */
      return_value = swapcontext (&(context->save->uc), &(context->uc));
      xbt_assert0((return_value==0),"Context swapping failure");
/*       WARNING("I am (%p). Coming back\n",context); */
    } else {
      xbt_context_t old_context = context->save ;
/*       WARNING("**** Back ! ****"); */
/*       WARNING("Setting current_context (%p) to context(%p)->save",current_context,context); */
      current_context = context->save ;
/*       WARNING("Setting context(%p)->save to NULL",context); */
      context->save = NULL ;
/*       WARNING("Current position memorized (%p). Jumping to context (%p)",context,old_context); */
      return_value = swapcontext (&(context->uc), &(old_context->uc));
      xbt_assert0((return_value==0),"Context swapping failure");
/*       WARNING("I am (%p). Coming back\n",context); */
    }
  }

  return;
}

static void xbt_context_destroy(xbt_context_t context)
{
  xbt_free(context);

  return;
}

void xbt_context_init(void)
{
  if(!current_context) {
    current_context = init_context = xbt_new0(s_xbt_context_t,1);
    context_to_destroy = xbt_swag_new(xbt_swag_offset(*current_context,hookup));
    context_living = xbt_swag_new(xbt_swag_offset(*current_context,hookup));
    xbt_swag_insert(init_context, context_living);
  }
}

void xbt_context_empty_trash(void)
{
  xbt_context_t context=NULL;

  while((context=xbt_swag_extract(context_to_destroy)))
    xbt_context_destroy(context);
}

static void *__context_wrapper(void *c)
{
  xbt_context_t context = c;
  int i;

/*   msg_global->current_process = process; */

  if(context->startup_func)
    context->startup_func(context->startup_arg);

  /*  WARNING("Calling the main function"); */
  /*   xbt_context_yield(context); */
  (context->code) (context->argc,context->argv);

  for(i=0;i<context->argc; i++) 
    if(context->argv[i]) xbt_free(context->argv[i]);
  if(context->argv) xbt_free(context->argv);

  if(context->cleanup_func)
    context->cleanup_func(context->cleanup_arg);

  xbt_swag_remove(context, context_living);
  xbt_swag_insert(context, context_to_destroy);

  __xbt_context_yield(context);
  xbt_assert0(0,"You're cannot be here!");
  return NULL;
}

void xbt_context_start(xbt_context_t context) 
{
/*   xbt_fifo_insert(msg_global->process, process); */
/*   xbt_fifo_insert(msg_global->process_to_run, process); */

  makecontext (&(context->uc), (void (*) (void)) __context_wrapper,
	       1, context);
  return;
}

xbt_context_t xbt_context_new(xbt_context_function_t code, 
			      void_f_pvoid_t startup_func, void *startup_arg,
			      void_f_pvoid_t cleanup_func, void *cleanup_arg,
			      int argc, char *argv[])
{
  xbt_context_t res = NULL;

  res = xbt_new0(s_xbt_context_t,1);

  xbt_assert0(getcontext(&(res->uc))==0,"Error in context saving.");

  res->code = code;
  res->uc.uc_link = NULL;
  res->argc = argc;
  res->argv = argv;
/*   res->uc.uc_link = &(current_context->uc); */
  /* WARNING : when this context is over, the current_context (i.e. the 
     father), is awaken... Theorically, the wrapper should prevent using 
     this feature. */
  res->uc.uc_stack.ss_sp = res->stack;
  res->uc.uc_stack.ss_size = STACK_SIZE;
  res->startup_func = startup_func;
  res->startup_arg = startup_arg;
  res->cleanup_func = cleanup_func;
  res->cleanup_arg = cleanup_arg;

  xbt_swag_insert(res, context_living);

  return res;
}


void xbt_context_yield(void)
{
  __xbt_context_yield(current_context);
}

void xbt_context_schedule(xbt_context_t context)
{
  xbt_assert0((current_context==init_context),
	      "You are not supposed to run this function here!");
  __xbt_context_yield(context);
}

void xbt_context_exit(void) {
  xbt_context_t context=NULL;

  xbt_context_empty_trash();
  xbt_swag_free(context_to_destroy);

  while((context=xbt_swag_extract(context_living)))
    xbt_context_free(context);

  xbt_swag_free(context_living);

  init_context = current_context = NULL ;
}

void xbt_context_free(xbt_context_t context)
{
  int i ;

  xbt_swag_remove(context, context_living);  
  for(i=0;i<context->argc; i++) 
    if(context->argv[i]) xbt_free(context->argv[i]);
  if(context->argv) xbt_free(context->argv);
  
  if(context->cleanup_func)
    context->cleanup_func(context->cleanup_arg);
  xbt_context_destroy(context);

  return;
}
