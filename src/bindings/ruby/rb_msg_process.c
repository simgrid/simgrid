/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "msg/private.h"        /* s_simdata_process_t */
#include "bindings/ruby_bindings.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(ruby, bindings, "Ruby Bindings");

/*
 * Functions for Ruby Process Management (Up Calls)
 */

// get Ruby Process Name
VALUE rb_process_getName(VALUE ruby_process)
{
  return rb_funcall(ruby_process, rb_intern("getName"), 0);
}

// Get  Process ID
VALUE rb_process_getID(VALUE ruby_process)
{
  return rb_funcall(ruby_process, rb_intern("getID"), 0);
}

// Get Bind
VALUE rb_process_getBind(VALUE ruby_process)
{
  return rb_funcall(ruby_process, rb_intern("getBind"), 0);
}

// Set Bind
void rb_process_setBind(VALUE ruby_process, long bind)
{
  VALUE r_bind = LONG2FIX(bind);
  rb_funcall(ruby_process, rb_intern("setBind"), 1, r_bind);
}

// isAlive
VALUE rb_process_isAlive(VALUE ruby_process)
{
  return rb_funcall(ruby_process, rb_intern("alive?"), 0);
}

// Kill Process
void rb_process_kill_up(VALUE ruby_process)
{
  rb_funcall(ruby_process, rb_intern("kill"), 0);
}

// join Process
void rb_process_join(VALUE ruby_process)
{
  rb_funcall(ruby_process, rb_intern("join"), 0);
}

// FIXME: all this calls must be manually inlined I guess
// unschedule Process
void rb_process_unschedule(VALUE ruby_process)
{
  rb_funcall(ruby_process, rb_intern("unschedule"), 0);
}

// schedule Process
void rb_process_schedule(VALUE ruby_process)
{
  rb_funcall(ruby_process, rb_intern("schedule"), 0);
}

/***************************************************
Function for Native Process ( Bound ) Management

Methods Belong to MSG Module
****************************************************/

// Process To Native
m_process_t rb_process_to_native(VALUE ruby_process)
{
  VALUE id = rb_process_getBind(ruby_process);
  if (!id) {
    rb_raise(rb_eRuntimeError, "Process Not Bound >>> id_Bind Null");
    return NULL;
  }
  long l_id = FIX2LONG(id);
  return (m_process_t) l_id;
}

// Bind Process
void rb_process_bind(VALUE ruby_process, m_process_t process)
{
  long bind = (long) (process);
  rb_process_setBind(ruby_process, bind);
}


// Process Management
void rb_process_suspend(VALUE class, VALUE ruby_process)
{

  m_process_t process = rb_process_to_native(ruby_process);

  if (!process) {
    rb_raise(rb_eRuntimeError,
             "Process Not Bound...while suspending process");
    return;
  }
  // Trying to suspend The Process

  if (MSG_OK != MSG_process_suspend(process))
    rb_raise(rb_eRuntimeError, "MSG_process_suspend() failed");
}

void rb_process_resume(VALUE class, VALUE ruby_process)
{
  m_process_t process = rb_process_to_native(ruby_process);
  if (!process) {
    rb_raise(rb_eRuntimeError,
             "Process not Bound...while resuming process");
    return;
  }
  // Trying to resume the process
  if (MSG_OK != MSG_process_resume(process))
    rb_raise(rb_eRuntimeError, "MSG_process_resume() failed");
}

VALUE rb_process_isSuspended(VALUE class, VALUE ruby_process)
{
  m_process_t process = rb_process_to_native(ruby_process);
  if (!process) {
    rb_raise(rb_eRuntimeError,
             "Process not Bound...while testing if suspended");
    return Qfalse;
  }
  if (MSG_process_is_suspended(process))
    return Qtrue;
  return Qfalse;
}

void rb_process_kill_down(VALUE class, VALUE ruby_process)
{
  m_process_t process = rb_process_to_native(ruby_process);

  if (!process) {
    rb_raise(rb_eRuntimeError,
             "Process Not Bound...while killing process");
    return;
  }
  // Delete The Global Reference / Ruby Process
  rb_process_kill_up(ruby_process);
  // Delete the Native Process
  MSG_process_kill(process);
}

VALUE rb_process_getHost(VALUE class, VALUE ruby_process)
{
  m_process_t process = rb_process_to_native(ruby_process);
  m_host_t host;


  if (!process) {
    rb_raise(rb_eRuntimeError, "Process Not Bound...while getting Host");
    return Qnil;                // NULL
  }

  host = MSG_process_get_host(process);

  return Data_Wrap_Struct(class, 0, rb_host_free, host);
  /*if(host->data) printf("Ok\n"); 

     if(!host->data) {
     rb_raise (rb_eRuntimeError,"MSG_process_get_host() failed");
     return Qnil;
     }
     printf("Houuuuuuuuuuuuuuna3!!\n");
     return Data_Wrap_Struct(class, 0, rb_host_free, host); */
}

void rb_process_exit(VALUE class, VALUE ruby_process)
{
  m_process_t process = rb_process_to_native(ruby_process);
  if (!process) {
    rb_raise(rb_eRuntimeError,
             "Process Not Bound...while exiting process");
    return;
  }
  SIMIX_context_stop(SIMIX_process_self()->context);
}
