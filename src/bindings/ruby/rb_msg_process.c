/*
 * Copyright 2010, The SimGrid Team. All right reserved.
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 */

#include "msg/private.h" /* s_simdata_process_t */
#include "bindings/ruby_bindings.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(ruby,bindings,"Ruby Bindings");

// Init Ruby
void initRuby(void) {
  ruby_init();
  ruby_init_loadpath();
//  KILLME  rb_require("RubyProcess.rb");
}


/*
 * Functions for Ruby Process Management (Up Calls)
 */


// get Ruby Process Name
VALUE rb_process_getName(VALUE ruby_process) {
  initRuby();
  // instance = rb_funcall3(rb_const_get(rb_cObject, rb_intern("RbProcess")),  rb_intern("new"), 0, 0);
  return rb_funcall(ruby_process,rb_intern("getName"),0);

}

// Get  Process ID
VALUE rb_process_getID(VALUE ruby_process) {
  initRuby();
  return rb_funcall(ruby_process,rb_intern("getID"),0);
}

// Get Bind
VALUE rb_process_getBind(VALUE ruby_process) {
  initRuby();
  return rb_funcall(ruby_process,rb_intern("getBind"),0);
}


// Set Bind
void rb_process_setBind(VALUE ruby_process,long bind) {
  initRuby();
  VALUE r_bind = LONG2FIX(bind);
  rb_funcall(ruby_process,rb_intern("setBind"),1,r_bind);
}

// isAlive
VALUE rb_process_isAlive(VALUE ruby_process) {
  initRuby();
  return rb_funcall(ruby_process,rb_intern("alive?"),0);
}

// Kill Process
void rb_process_kill_up(VALUE ruby_process) {
  initRuby();  
  rb_funcall(ruby_process,rb_intern("kill"),0);
}

// join Process
void rb_process_join( VALUE ruby_process ) {
  initRuby();
  rb_funcall(ruby_process,rb_intern("join"),0);
}

// unschedule Process
void rb_process_unschedule( VALUE ruby_process ) {
  initRuby();
  rb_funcall(ruby_process,rb_intern("unschedule"),0);
}

// schedule Process
void rb_process_schedule( VALUE ruby_process ) {
  initRuby();
  rb_funcall(ruby_process,rb_intern("schedule"),0);
}

/***************************************************

Function for Native Process ( Bound ) Management

Methods Belong to MSG Module

 ****************************************************/

// Process To Native
m_process_t rb_process_to_native(VALUE ruby_process) {
  VALUE id = rb_process_getBind(ruby_process);
  if (!id) {
    rb_raise(rb_eRuntimeError,"Process Not Bound >>> id_Bind Null");
    return NULL;
  }
  long l_id= FIX2LONG(id);
  return (m_process_t)l_id;
}

// Bind Process
void rb_process_bind(VALUE ruby_process,m_process_t process) {
  long bind = (long)(process);
  rb_process_setBind(ruby_process,bind);
}


// processCreate

void rb_process_create(VALUE class,VALUE ruby_process,VALUE host) {
  VALUE rbName;      // Name of Java Process instance
  m_process_t process; // Native Process to Create
  const char * name ; // Name of C Native Process
  char alias[MAX_ALIAS_NAME + 1 ] = {0};
  msg_mailbox_t mailbox;
  rbName = rb_process_getName(ruby_process);

  if(!rbName) {
    rb_raise(rb_eRuntimeError,"Internal error : Process Name Cannot be NULL");
    return;
  }
  // Allocate the data for the simulation
  process = xbt_new0(s_m_process_t,1);
  process->simdata = xbt_new0(s_simdata_process_t,1);
  // Do we Really Need to Create Ruby Process Instance , >> process is already a Ruby Process !! So..Keep on ;)
  // Bind The Ruby Process instance to The Native Process
  rb_process_bind(ruby_process,process);
  name = RSTRING(rbName)->ptr;
  process->name = xbt_strdup(name);
  Data_Get_Struct(host,s_m_host_t,process->simdata->m_host);

  if(!(process->simdata->m_host)) // Not Binded
  {
    free(process->simdata);
    free(process->data);
    free(process);
    rb_raise(rb_eRuntimeError,"Host not bound...while creating native process");
    return;
  }
  process->simdata->PID = msg_global->PID++; //  msg_global ??

  DEBUG7("fill in process %s/%s (pid=%d) %p (sd=%p , host=%p, host->sd=%p)\n",
      process->name , process->simdata->m_host->name,process->simdata->PID,
      process,process->simdata, process->simdata->m_host,
      process->simdata->m_host->simdata);

  process->simdata->s_process =
      SIMIX_process_create(process->name,
          (xbt_main_func_t)ruby_process,
          (void *) process,
          process->simdata->m_host->simdata->smx_host->name,
          0,NULL,NULL);

 DEBUG1("context created (s_process=%p)\n",process->simdata->s_process);

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
void rb_process_suspend(VALUE class,VALUE ruby_process) {

  m_process_t process = rb_process_to_native(ruby_process);

  if (!process) {
    rb_raise(rb_eRuntimeError,"Process Not Bound...while suspending process");
    return;  
  }

  // Trying to suspend The Process

  if ( MSG_OK != MSG_process_suspend(process))
    rb_raise(rb_eRuntimeError,"MSG_process_suspend() failed");
}

void rb_process_resume(VALUE class,VALUE ruby_process) {
  m_process_t process = rb_process_to_native(ruby_process);
  if (!process) {
    rb_raise(rb_eRuntimeError,"Process not Bound...while resuming process");
    return ;
  }

  // Trying to resume the process
  if ( MSG_OK != MSG_process_resume(process))
    rb_raise(rb_eRuntimeError,"MSG_process_resume() failed");
}

VALUE rb_process_isSuspended(VALUE class,VALUE ruby_process) {
  m_process_t process = rb_process_to_native(ruby_process);
  if (!process) {
    rb_raise (rb_eRuntimeError,"Process not Bound...while testing if suspended");
    return Qfalse;
  }

  if(MSG_process_is_suspended(process))
    return Qtrue;
  return Qfalse;
}

void rb_process_kill_down(VALUE class,VALUE ruby_process) {
  m_process_t process = rb_process_to_native(ruby_process);

  if(!process) {
    rb_raise (rb_eRuntimeError,"Process Not Bound...while killing process");
    return;
  }
  // Delete The Global Reference / Ruby Process
  rb_process_kill_up(ruby_process);
  // Delete the Native Process
  MSG_process_kill(process);
}

VALUE rb_process_getHost(VALUE class,VALUE ruby_process) {
  m_process_t process = rb_process_to_native(ruby_process);
  m_host_t host;

  if (!process) {
    rb_raise(rb_eRuntimeError,"Process Not Bound...while getting Host");
    return Qnil; // NULL
  }

  host = MSG_process_get_host(process);

  if(!host->data) {
    rb_raise (rb_eRuntimeError,"MSG_process_get_host() failed");
    return Qnil;
  }

  return Data_Wrap_Struct(class, 0, rb_host_free, host);
}

void rb_process_exit(VALUE class,VALUE ruby_process) {
  m_process_t process = rb_process_to_native(ruby_process);
  if(!process) {
    rb_raise(rb_eRuntimeError,"Process Not Bound...while exiting process");
    return;
  }
  SIMIX_context_stop(SIMIX_process_self()->context);
}
