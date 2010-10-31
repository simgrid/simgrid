/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "bindings/ruby_bindings.h"
#include "surf/surfxml_parse.h"

XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(ruby);

// Used to instanciate the Process 
static VALUE args;
static VALUE prop;
static VALUE function_name;
static VALUE host_name;

static VALUE rb_process_instance(VALUE fct_name, VALUE arguments,
                                 VALUE properties)
{
  ruby_init();
  ruby_init_loadpath();
  char *p_className = RSTRING(fct_name)->ptr;   // name of process is the one of the class
  return rb_funcall(rb_const_get(rb_cObject, rb_intern(p_className)),
                    rb_intern("new"), 3, fct_name, arguments, properties);
}

// FIXME: don't mess with MSG internals here, use MSG_process_create_with_arguments()
static void rb_process_create_with_args(VALUE fct_name, VALUE arguments,
                                        VALUE properties, VALUE ht_name)
{

  VALUE ruby_process =
      rb_process_instance(fct_name, arguments, properties);
  m_process_t process;          // Native Process to Create
  const char *name;             // Name of C Native Processs


  if (!fct_name)
    rb_raise(rb_eRuntimeError,
             "Internal error: Process name cannot be NULL");
  name = RSTRING(fct_name)->ptr;
  DEBUG1("Create native process %s", name);

  char **argv = xbt_new(char *, 2);
  argv[0] = bprintf("%s@%s", name, RSTRING(ht_name)->ptr);
  argv[1] = NULL;

  // Allocate the data for the simulation
  process = MSG_process_create_with_arguments(name,
      (xbt_main_func_t) ruby_process,
      process,
      MSG_get_host_by_name(RSTRING(ht_name)->ptr),
      1, argv);

  // Bind The Ruby Process instance to The Native Process
  rb_process_bind(ruby_process, process);
}


void rb_application_handler_on_start_document(void)
{


  args = rb_ary_new();          // Max length = 16 !!
  prop = rb_ary_new();

}

void rb_application_handler_on_end_document(void)
{

  args = Qnil;
  prop = Qnil;
  function_name = Qnil;
  host_name = Qnil;
}

void rb_application_handler_on_begin_process(void)
{

  host_name = rb_str_new2(A_surfxml_process_host);
  function_name = rb_str_new2(A_surfxml_process_function);

  args = rb_ary_new();          // Max length = 16 ?!
  prop = rb_ary_new();

}

void rb_application_handler_on_process_arg(void)
{

  VALUE arg = rb_str_new2(A_surfxml_argument_value);
  rb_ary_push(args, arg);
}

void rb_application_handler_on_property(void)
{

  VALUE id = rb_str_new2(A_surfxml_prop_id);
  VALUE val = rb_str_new2(A_surfxml_prop_value);
  int i_id = NUM2INT(id);
  rb_ary_store(prop, i_id, val);

}

void rb_application_handler_on_end_process(void)
{

  rb_process_create_with_args(function_name, args, prop, host_name);

}
