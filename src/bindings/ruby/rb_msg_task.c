/* Task-related bindings to ruby  */

/* Copyright 2010. The SimGrid Team. All right reserved. */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */


#include "bindings/ruby_bindings.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(ruby);

// Free Method
void rb_task_free(m_task_t tk) {
  MSG_task_destroy(tk);
}

// New Method
VALUE rb_task_new(VALUE class, VALUE name,VALUE comp_size,VALUE comm_size) {
  //char * t_name = RSTRING(name)->ptr;
  m_task_t task = MSG_task_create(RSTRING(name)->ptr,NUM2INT(comp_size),NUM2INT(comm_size),NULL);
  // Wrap m_task_t to a Ruby Value
  return Data_Wrap_Struct(class, 0, rb_task_free, task);

}

//Get Computation Size
VALUE rb_task_comp(VALUE class,VALUE task) {
  double size;
  m_task_t tk;
  // Wrap Ruby Value to m_task_t struct
  Data_Get_Struct(task, s_m_task_t, tk);
  size = MSG_task_get_compute_duration(tk);
  return rb_float_new(size);
}

//Get Name
VALUE rb_task_name(VALUE class,VALUE task) {

  // Wrap Ruby Value to m_task_t struct
  m_task_t tk;
  Data_Get_Struct(task, s_m_task_t, tk);
  return rb_str_new2(MSG_task_get_name(tk));
}

// Execute Task
VALUE rb_task_execute(VALUE class,VALUE task) {

  // Wrap Ruby Value to m_task_t struct
  m_task_t tk;
  Data_Get_Struct(task, s_m_task_t, tk);
  return INT2NUM(MSG_task_execute(tk));
}

// Sending Task
void rb_task_send(VALUE class,VALUE task,VALUE mailbox) {

  // Wrap Ruby Value to m_task_t struct
  m_task_t tk;
  Data_Get_Struct(task, s_m_task_t, tk);
  int res = MSG_task_send(tk,RSTRING(mailbox)->ptr);
  if(res != MSG_OK)
    rb_raise(rb_eRuntimeError,"MSG_task_send failed");
}

// Receiving Task (returns a Task)
VALUE rb_task_receive(VALUE class, VALUE mailbox) {
  // Task
  m_task_t task = NULL;
  INFO1("Receiving a task on mailbox '%s'",RSTRING(mailbox)->ptr);
  MSG_task_receive(&task,RSTRING(mailbox)->ptr);
  INFO2("XXXXXXXXReceived a task %p %s",task,task->name);
  return Data_Wrap_Struct(class, 0, rb_task_free, task);
}

// It Return a Native Process ( m_process_t )
VALUE rb_task_sender(VALUE class,VALUE task) {
  m_task_t tk;
  Data_Get_Struct(task,s_m_task_t,tk);
  THROW_UNIMPLEMENTED;
  return 0;//MSG_task_get_sender(tk);
}

// it return a Host 
VALUE rb_task_source(VALUE class,VALUE task) {
  m_task_t tk;
  Data_Get_Struct(task,s_m_task_t,tk);

  m_host_t host = MSG_task_get_source(tk);
  if(!host->data) {
    rb_raise(rb_eRuntimeError,"MSG_task_get_source() failed");
    return Qnil;
  }
  THROW_UNIMPLEMENTED;
  return 0;//host;
}

// Return Boolean
VALUE rb_task_listen(VALUE class,VALUE task,VALUE alias) {
  m_task_t tk;
  const char *p_alias;
  int rv;

  Data_Get_Struct(task,s_m_task_t,tk);
  p_alias = RSTRING(alias)->ptr;

  rv = MSG_task_listen(p_alias);

  if(rv) return Qtrue;

  return Qfalse;
}

// return Boolean
VALUE rb_task_listen_host(VALUE class,VALUE task,VALUE alias,VALUE host) {

  m_task_t tk;
  m_host_t ht;
  const char *p_alias;
  int rv;

  Data_Get_Struct(task,s_m_task_t,tk);
  Data_Get_Struct(host,s_m_host_t,ht);
  p_alias = RSTRING(alias)->ptr;

  rv = MSG_task_listen_from_host(p_alias,ht);

  if (rv)
    return Qtrue;
  return Qfalse;
}
