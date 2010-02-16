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
#include "rb_application_handler.h"
#include "surf/surfxml_parse.h"
#include <stdio.h>

// #define DEBUG 

static void  r_init()
{
  
  ruby_init();
  ruby_init_loadpath();
  rb_require("ApplicationHandler.rb");
  
} 

static void  application_handler_on_start_document(void)
{
  
   r_init();
   //current One
   current = rb_funcall3(rb_const_get(rb_cObject, rb_intern("ApplicationHandler")),  rb_intern("new"), 0, 0);
   rb_funcall(current,rb_intern("onStartDocument"),0);
 #ifdef DEBUG
   printf ("application_handler_on_start_document ...Done\n" );
 #endif
  
}

static void  application_handler_on_end_document(void)
{
  //r_init();
  rb_funcall(current,rb_intern("onEndDocument"),0); 

}

static void application_handler_on_begin_process(void) 
{
  //r_init();
  VALUE hostName = rb_str_new2(A_surfxml_process_host);
  VALUE function = rb_str_new2(A_surfxml_process_function);
#ifdef DEBUG
   printf ("On_Begin_Process: %s : %s \n",RSTRING(hostName)->ptr,RSTRING(function)->ptr);
#endif 
   rb_funcall(current,rb_intern("onBeginProcess"),2,hostName,function); 
}

static void  application_handler_on_process_arg(void)
{
  //r_init();
   VALUE arg = rb_str_new2(A_surfxml_argument_value);
#ifdef DEBUG
   printf ("On_Process_Args >> Sufxml argument value : %s\n",RSTRING(arg)->ptr);
#endif
   rb_funcall(current,rb_intern("onProcessArg"),1,arg); 
}

static void  application_handler_on_property(void)
{
 
  //r_init()
   VALUE id = rb_str_new2(A_surfxml_prop_id);
   VALUE val =  rb_str_new2(A_surfxml_prop_value);
   rb_funcall(current,rb_intern("onProperty"),2,id,val);
   
}


static void application_handler_on_end_process(void)
{
  
 //r_init()
 rb_funcall(current,rb_intern("onEndProcess"),0);
    
}
