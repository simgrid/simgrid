/* $Id$ */

/* context_Ruby - implementation of context switching with lua coroutines */

/* Copyright (c) 2004-2008 the SimGrid team. All right reserved */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */
#include <ruby.h>
#include "private.h"
#include "xbt/function_types.h"
#include "xbt/sysdep.h"
#include "xbt/log.h"
#include "xbt/asserts.h"
#include "context_sysv_config.h"
#include "bindings/ruby/rb_msg_process.c"
// #include "bindings/ruby/rb_msg.c"

// #define MY_DEBUG


typedef struct s_smx_ctx_ruby

{
  SMX_CTX_BASE_T;
  VALUE process;   // The  Ruby Process Instance 
  //...
}s_smx_ctx_ruby_t,*smx_ctx_ruby_t;
  
static smx_context_t
smx_ctx_ruby_create_context(xbt_main_func_t code,int argc,char** argv,
			    void_f_pvoid_t cleanup_func,void *cleanup_arg);

static int smx_ctx_ruby_factory_finalize(smx_context_factory_t *factory);

static void smx_ctx_ruby_free(smx_context_t context);

static void smx_ctx_ruby_start(smx_context_t context);

static void smx_ctx_ruby_stop(smx_context_t context);

static void smx_ctx_ruby_suspend(smx_context_t context);

static void 
  smx_ctx_ruby_resume(smx_context_t old_context,smx_context_t new_context);

static void smx_ctx_ruby_wrapper(void); 



void SIMIX_ctx_ruby_factory_init(smx_context_factory_t *factory)
{
  
 *factory = xbt_new0(s_smx_context_factory_t,1);
 
 (*factory)->create_context = smx_ctx_ruby_create_context;
 (*factory)->finalize = smx_ctx_ruby_factory_finalize;
 (*factory)->free = smx_ctx_ruby_free;
 (*factory)->start = smx_ctx_ruby_start;
 (*factory)->stop = smx_ctx_ruby_stop;
 (*factory)->suspend = smx_ctx_ruby_suspend;
 (*factory)->resume = smx_ctx_ruby_resume;
 (*factory)->name = "smx_ruby_context_factory";
  ruby_init();
  ruby_init_loadpath();
  #ifdef MY_DEBUG
  printf("SIMIX_ctx_ruby_factory_init...Done\n");
  #endif 
}
  
static int smx_ctx_ruby_factory_finalize(smx_context_factory_t *factory)
{
  
 free(*factory);
 *factory = NULL;
 return 0;
 
}

static smx_context_t 
  smx_ctx_ruby_create_context(xbt_main_func_t code,int argc,char** argv,
			      void_f_pvoid_t cleanup_func,void* cleanup_arg)
{
  
  smx_ctx_ruby_t context = xbt_new0(s_smx_ctx_ruby_t,1);
  
  /*if the user provided a function for the process , then use it 
  Otherwise it's the context for maestro */
  if( code )
  {
   context->cleanup_func = cleanup_func;
   context->cleanup_arg = cleanup_arg;
   context->process = (VALUE)code;
   
  #ifdef MY_DEBUG
  printf("smx_ctx_ruby_create_context...Done\n");
  #endif
   
  }
  return (smx_context_t) context;
  
}
  
static void smx_ctx_ruby_free(smx_context_t context)
{
 /* VALUE process;
  if (context)
  {
   smx_ctx_ruby_t ctx_ruby = (smx_ctx_ruby_t) context;
   
   if (ctx_ruby->process){
     // if the Ruby Process is Alive , Join it   
   // if ( process_isAlive(ctx_ruby->process))
    {
      process = ctx_ruby->process;
      ctx_ruby->process = Qnil;
      process_join(process);
    } 
    
  }
  free(context);
  context = NULL; 
  }*/
  free (context);
  context = NULL;
    #ifdef MY_DEBUG
  printf("smx_ctx_ruby_free_context...Done\n");
  #endif
  
}

static void smx_ctx_ruby_start(smx_context_t context)
{
   
 /* Already Done .. Since a Ruby Process is launched within initialization
 We Start it Within the Initializer ... We Use the Semaphore To Keep 
 The Thread Alive Waitin' For Mutex Signal to Execute The Main*/
 
}

static void smx_ctx_ruby_stop(smx_context_t context)
{
  
  
  VALUE process = Qnil;
  smx_ctx_ruby_t ctx_ruby,current;
  
  if ( context->cleanup_func)
    (*(context->cleanup_func)) (context->cleanup_arg);
  
  ctx_ruby = (smx_ctx_ruby_t) context;
  
  // Well , Let's Do The Same as JNI Stoppin' Process
  if ( simix_global->current_process->iwannadie )
  {
   if( ctx_ruby->process )
   {
    //if the Ruby Process still Alive ,let's Schedule it
    if ( process_isAlive( ctx_ruby->process ) )
    {
     current = (smx_ctx_ruby_t)simix_global->current_process->context;
     process_schedule(current->process); 
     process = ctx_ruby->process;     
     // interupt/kill The Ruby Process
     process_kill(process);
    }
   }    
  }else {
   
    process = ctx_ruby->process;
    ctx_ruby->process = Qnil;
    
  }
  #ifdef MY_DEBUG
  printf("smx_ctx_ruby_stop...Done\n");
  #endif
}

static void smx_ctx_ruby_suspend(smx_context_t context)
{

if (context)
{
smx_ctx_ruby_t ctx_ruby = (smx_ctx_ruby_t) context;
  if (ctx_ruby->process)
    process_unschedule( ctx_ruby->process ) ;
#ifdef MY_DEBUG
  printf("smx_ctx_ruby_unschedule...Done\n");
#endif
}
  
  else
    rb_raise(rb_eRuntimeError,"smx_ctx_ruby_suspend failed");
  
}

static void smx_ctx_ruby_resume(smx_context_t old_context,smx_context_t new_context)
{
  
 smx_ctx_ruby_t ctx_ruby = (smx_ctx_ruby_t) new_context;
 process_schedule(ctx_ruby->process);
 
  #ifdef MY_DEBUG
    printf("smx_ctx_ruby_schedule...Done\n");
  #endif
 
}
