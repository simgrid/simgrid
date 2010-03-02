/*
 * Copyright 2010. The SimGrid Team. All right reserved.
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package.
 */
#include "bindings/ruby_bindings.h"
#include "surf/surfxml_parse.h"

static VALUE application_handler_class; // The Current Instance of ApplicationHandler Class ,is it the same as Current Thread ??!!


// #define MY_DEBUG 

static void r_init() {
  ruby_init();
  ruby_init_loadpath();
  rb_require("ApplicationHandler.rb");
  
} 

void rb_application_handler_on_start_document(void) {
  
   r_init();
   application_handler_class = rb_funcall3(rb_const_get(rb_cObject, rb_intern("ApplicationHandler")),  rb_intern("new"), 0, 0);
   rb_funcall(application_handler_class,rb_intern("onStartDocument"),0);
 #ifdef MY_DEBUG
   printf ("application_handler_on_start_document ...Done\n" );
 #endif
  
}

void rb_application_handler_on_end_document(void) {
  rb_funcall(application_handler_class,rb_intern("onEndDocument"),0); 
}

void rb_application_handler_on_begin_process(void) {
  VALUE hostName = rb_str_new2(A_surfxml_process_host);
  VALUE function = rb_str_new2(A_surfxml_process_function);
#ifdef MY_DEBUG
   printf ("On_Begin_Process: %s : %s \n",RSTRING(hostName)->ptr,RSTRING(function)->ptr);
#endif 
   rb_funcall(application_handler_class,rb_intern("onBeginProcess"),2,hostName,function); 
}

void rb_application_handler_on_process_arg(void) {
   VALUE arg = rb_str_new2(A_surfxml_argument_value);
#ifdef MY_DEBUG
   printf ("On_Process_Args >> Sufxml argument value : %s\n",RSTRING(arg)->ptr);
#endif
   rb_funcall(application_handler_class,rb_intern("onProcessArg"),1,arg); 
}

void rb_application_handler_on_property(void) {
   VALUE id = rb_str_new2(A_surfxml_prop_id);
   VALUE val =  rb_str_new2(A_surfxml_prop_value);
   rb_funcall(application_handler_class,rb_intern("onProperty"),2,id,val);
}


void rb_application_handler_on_end_process(void) {
 rb_funcall(application_handler_class,rb_intern("onEndProcess"),0);
}
