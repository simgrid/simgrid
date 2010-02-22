/*
 * $Id$
 *
 * Copyright 2010 Martin Quinson, Mehdi Fekari           
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 */

#include "rb_msg_process.h"

#define DEBUG
// Init Ruby
static void initRuby()
{
  
  ruby_init();
  ruby_init_loadpath();
  rb_require("RubyProcess.rb");
  
}


/***********************************************

Functions for Ruby Process Management ( Up Call)

Idependant Methods

************************************************/


// get Ruby Process Name
static VALUE process_getName( VALUE ruby_process )
{
  
   initRuby();
  // instance = rb_funcall3(rb_const_get(rb_cObject, rb_intern("RbProcess")),  rb_intern("new"), 0, 0);
   return rb_funcall(ruby_process,rb_intern("getName"),0);
  
}

// Get  Process ID
static VALUE process_getID(VALUE ruby_process)
{

  initRuby();
  return rb_funcall(ruby_process,rb_intern("getID"),0);
  
}

// Get Bind
static VALUE process_getBind(VALUE ruby_process)
{
 
  initRuby();
  return rb_funcall(ruby_process,rb_intern("getBind"),0);
  
}


// Set Bind
static void process_setBind(VALUE ruby_process,long bind)
{

  initRuby();
  VALUE r_bind = LONG2FIX(bind);
  rb_funcall(ruby_process,rb_intern("setBind"),1,r_bind);
  
}

// isAlive
static VALUE process_isAlive(VALUE ruby_process)
{
  
 initRuby();
 return rb_funcall(ruby_process,rb_intern("alive?"),0);
  
}

// Kill Process
static void process_kill(VALUE ruby_process)
{
  
  initRuby();  
  rb_funcall(ruby_process,rb_intern("kill"),0);
  
}

// join Process
static void process_join( VALUE ruby_process )
{
  
 initRuby();
 rb_funcall(ruby_process,rb_intern("join"),0);
  
}

// unschedule Process
static void process_unschedule( VALUE ruby_process )
{
 
  initRuby();
  rb_funcall(ruby_process,rb_intern("unschedule"),0);
  
}

// schedule Process
static void process_schedule( VALUE ruby_process )
{
   
 initRuby();
 rb_funcall(ruby_process,rb_intern("schedule"),0);
  
}

/***************************************************

Function for Native Process ( Bound ) Management

Methods Belong to MSG Module

****************************************************/

// Process To Native

static m_process_t process_to_native(VALUE ruby_process)
{
  
  VALUE id = process_getBind(ruby_process);
  if (!id)
  {
   rb_raise(rb_eRuntimeError,"Process Not Bound >>> id_Bind Null");
   return NULL;
  }
  long l_id= FIX2LONG(id);
  return (m_process_t)l_id;
  
}

// Bind Process

static void processBind(VALUE ruby_process,m_process_t process)
{
  
  long bind = (long)(process);
  process_setBind(ruby_process,bind);
  
}


// processCreate

static void processCreate(VALUE class,VALUE ruby_process,VALUE host)
{
  
 VALUE rbName;      // Name of Java Process instance
 m_process_t process; // Native Process to Create
 const char * name ; // Name of C Native Process
 char alias[MAX_ALIAS_NAME + 1 ] = {0};
 msg_mailbox_t mailbox;
 rbName = process_getName(ruby_process);

 if(!rbName)
 {
  rb_raise(rb_eRuntimeError,"Internal error : Process Name Cannot be NULL");
  return; 
 } 
 // Allocate the data for the simulation
 process = xbt_new0(s_m_process_t,1);
 process->simdata = xbt_new0(s_simdata_process_t,1);
 // Do we Really Need to Create Ruby Process Instance , >> process is already a Ruby Process !! So..Keep on ;)
 // Bind The Ruby Process instance to The Native Process
 processBind(ruby_process,process); 
 name = RSTRING(rbName)->ptr;
 process->name = xbt_strdup(name);
 Data_Get_Struct(host,m_host_t,process->simdata->m_host);

 if(!(process->simdata->m_host)) // Not Binded
 {
   free(process->simdata);
   free(process->data);
   free(process);
   rb_raise(rb_eRuntimeError,"Host not bound...while creating native process");
   return;
 }
 process->simdata->PID = msg_global->PID++; //  msg_global ??
 /*DEBUG 
 ("fil in process %s/%s (pid=%d) %p (sd=%p, host=%p, host->sd=%p) ",
  process->name ,process->simdata->m_host->name,process->simdata->PID,
  process,process->simdata, process->simdata->m_host,
  process->simdata->m_host->simdata);*/
  
 #ifdef DEBUG
 printf("fill in process %s/%s (pid=%d) %p (sd%=%p , host=%p, host->sd=%p)\n",
	process->name , process->simdata->m_host->name,process->simdata->PID,
	process,process->simdata, process->simdata->m_host,
	process->simdata->m_host->simdata);
 #endif
  process->simdata->s_process =
  SIMIX_process_create(process->name,
		       (xbt_main_func_t)ruby_process,
		       (void *) process,
		       process->simdata->m_host->simdata->smx_host->name,
		       0,NULL,NULL);

 //DEBUG ( "context created (s_process=%p)",process->simdata->s_process);
 #ifdef DEBUG
 printf("context created (s_process=%p)\n",process->simdata->s_process);
 #endif
 if (SIMIX_process_self()) { // SomeOne Created Me !!
   process->simdata->PPID = MSG_process_get_PID(SIMIX_process_self()->data);
 }
 else
 {
   process->simdata->PPID = -1;
 }
  process->simdata->last_errno = MSG_OK;
  // let's Add the Process to the list of the Simulation's Processes
  xbt_fifo_unshift(msg_global->process_list,process);
  sprintf(alias,"%s:%s",(process->simdata->m_host->simdata->smx_host)->name,
	  process->name);
	  
  mailbox = MSG_mailbox_new(alias);

}


// Process Management

static void processSuspend(VALUE class,VALUE ruby_process)
{
  
  m_process_t process = process_to_native(ruby_process);
  
  if (!process)
  {
    rb_raise(rb_eRuntimeError,"Process Not Bound...while suspending process");
    return;  
  }
  
  // Trying to suspend The Process
  
  if ( MSG_OK != MSG_process_suspend(process))
      rb_raise(rb_eRuntimeError,"MSG_process_suspend() failed");
  
    
}

static void processResume(VALUE class,VALUE ruby_process)
{
  m_process_t process = process_to_native(ruby_process);
  if (!process)
  {
    rb_raise(rb_eRuntimeError,"Process not Bound...while resuming process");
    return ;
  }
  // Trying to resume the process
  if ( MSG_OK != MSG_process_resume(process))
    rb_raise(rb_eRuntimeError,"MSG_process_resume() failed");
  
}

static VALUE processIsSuspend(VALUE class,VALUE ruby_process)
{
  
  m_process_t process = process_to_native(ruby_process);
  if (!process)
  {
    rb_raise (rb_eRuntimeError,"Process not Bound...while testing if suspended");
    return;
  }
  
  // 1 is The Process is Suspended , 0 Otherwise
  if(MSG_process_is_suspended(process))
    return Qtrue;
  
  return Qfalse;
  
}

static void processKill(VALUE class,VALUE ruby_process)
{
 m_process_t process = process_to_native(ruby_process);
 
 if(!process)
 {
  rb_raise (rb_eRuntimeError,"Process Not Bound...while killing process");
  return ;
 }
  // Delete The Global Reference / Ruby Process
  process_kill(ruby_process);
  // Delete the Native Process
  MSG_process_kill(process);
  
}

static VALUE processGetHost(VALUE class,VALUE ruby_process)
{
  
  m_process_t process = process_to_native(ruby_process);
  m_host_t host;
  
  if (!process)
  {
  rb_raise(rb_eRuntimeError,"Process Not Bound...while getting Host");
  return Qnil; // NULL
  }
  
  host = MSG_process_get_host(process);
  
  if(!host->data)
  {
   rb_raise (rb_eRuntimeError,"MSG_process_get_host() failed");
   return Qnil;
  }
  
   return Data_Wrap_Struct(class, 0, host_free, host);
  
}

static void processExit(VALUE class,VALUE ruby_process)
{
  
  m_process_t process = process_to_native(ruby_process);
  if(!process)
  {
   rb_raise(rb_eRuntimeError,"Process Not Bound...while exiting process");
   return;
  }
  SIMIX_context_stop(SIMIX_process_self()->context);
  
}