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
#include "rb_msg_task.h"

// Free Method
static void task_free(m_task_t tk) {
  MSG_task_destroy(tk);
}

// New Method
static VALUE task_new(VALUE class, VALUE name,VALUE comp_size,VALUE comm_size)
{
  
  //char * t_name = RSTRING(name)->ptr;
  m_task_t task = MSG_task_create(RSTRING(name)->ptr,NUM2INT(comp_size),NUM2INT(comm_size),NULL);
  // Wrap m_task_t to a Ruby Value
  return Data_Wrap_Struct(class, 0, task_free, task);

}

//Get Computation Size
static VALUE task_comp(VALUE class,VALUE task)
{
  double size;
  m_task_t tk;
  // Wrap Ruby Value to m_task_t struct
  Data_Get_Struct(task, m_task_t, tk);
  size = MSG_task_get_compute_duration(tk);
  return rb_float_new(size);
}


//Get Name

static VALUE task_name(VALUE class,VALUE task)
{
  
  // Wrap Ruby Value to m_task_t struct
  
  m_task_t tk;
  Data_Get_Struct(task, m_task_t, tk);
  return rb_str_new2(MSG_task_get_name(tk));
   
}




// Execute Task

static VALUE task_execute(VALUE class,VALUE task)
{
  
  // Wrap Ruby Value to m_task_t struct
  m_task_t tk;
  Data_Get_Struct(task, m_task_t, tk);
  return INT2NUM(MSG_task_execute(tk));
  
  
}

// Sending Task

static void task_send(VALUE class,VALUE task,VALUE mailbox)
{
  
    // Wrap Ruby Value to m_task_t struct
  
  m_task_t tk;
  Data_Get_Struct(task, m_task_t, tk);
  int res = MSG_task_send(tk,RSTRING(mailbox)->ptr);
 
  if(res != MSG_OK)
   rb_raise(rb_eRuntimeError,"MSG_task_send failed");
  
  return;
}

// Recieving Task 

/**
*It Return a Task 
*/

static VALUE task_receive(VALUE class,VALUE mailbox)
{
  m_task_t tk; 
  MSG_task_receive(tk,RSTRING(mailbox)->ptr); 
  return Data_Wrap_Struct(class, 0, task_free, tk);
}

// Recieve Task 2
// Not Appreciated 
static VALUE task_receive2(VALUE class,VALUE task,VALUE mailbox)
{
  m_task_t tk;
  Data_Get_Struct(task, m_task_t, tk);
  return INT2NUM(MSG_task_receive(tk,RSTRING(mailbox)->ptr)); 
  
}


// It Return a Native Process ( m_process_t )
static VALUE task_sender(VALUE class,VALUE task)
{
  m_task_t tk;
  Data_Get_Struct(task,m_task_t,tk);
  return MSG_task_get_sender(tk);
  
}

// it return a Host 
static VALUE task_source(VALUE class,VALUE task)
{
  m_task_t tk;
  Data_Get_Struct(task,m_task_t,tk);
  
  m_host_t host = MSG_task_get_source(tk);
  if(!host->data)
  {
    rb_raise(rb_eRuntimeError,"MSG_task_get_source() failed");
    return Qnil;
  }
  return host;
  
}

// Return Boolean
static VALUE task_listen(VALUE class,VALUE task,VALUE alias)
{
 m_task_t tk;
 const char *p_alias;
 int rv;
 
 Data_Get_Struct(task,m_task_t,tk);
 p_alias = RSTRING(alias)->ptr;
 
 rv = MSG_task_listen(p_alias);
 
 if(rv) return Qtrue;
 
 return Qfalse;

}

// return Boolean
static VALUE task_listen_host(VALUE class,VALUE task,VALUE alias,VALUE host)
{
  
 m_task_t tk;
 m_host_t ht;
 const char *p_alias;
 int rv;
 
 Data_Get_Struct(task,m_task_t,tk);
 Data_Get_Struct(host,m_host_t,ht);
 p_alias = RSTRING(alias)->ptr;
 
 rv = MSG_task_listen_from_host(p_alias,ht);
 
 if (rv) return Qtrue;
 
 return Qfalse;
 
 
  
}