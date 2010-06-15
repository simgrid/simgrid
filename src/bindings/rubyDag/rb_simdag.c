/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

/* 
 * $Id$
 *
 * Copyright 2010 Simgrid Team         
 * All right reserved. 
 *
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the license 
 *(GNU LGPL) which comes with this package. 
 */ 

//FIXME #include "ruby_simdag.h"
#include "rb_SD_task.c"
#include "rb_SD_workstation.c"
#include <simdag/simdag.h>
#define MY_DEBUG

// XBT_LOG_EXTERNAL_DEFAULT_CATEGORY(ruby);

// MSG Module
VALUE rb_dag;
// MSG Classes
VALUE rb_task;
VALUE rb_workstation;

//Init Msg_Init From Ruby
static void sd_init(VALUE Class,VALUE args)
{ 
  char **argv=NULL;
  const char *tmp;
  int argc,type,i;
  VALUE *ptr;
  type = TYPE(args);
  if ( type != T_ARRAY )
  {
   rb_raise(rb_eRuntimeError,"Bad Argument to SD_init");
   return;
  }
  ptr = RARRAY(args)->ptr;
  argc= RARRAY(args)->len;
  argc++;
  argv= xbt_new0(char*,argc);
  argv[0] = strdup("ruby");
  for (i=0;i<argc-1;i++)
  {
   VALUE value = ptr[i];
   type = TYPE(value);
   tmp = RSTRING(value)->ptr;
   argv[i+1] = strdup(tmp); 
  }
  SD_init(&argc,argv);
  free(argv);
//   INFO0("SD Init...Done");
  printf("SD Init...Done\n");
  return;
  
}

//Init Msg_Run From Ruby
static void sd_reinit(VALUE class)
{
  SD_application_reinit();
}
 
//deploy Application
static void sd_createEnvironment(VALUE class,VALUE plateformFile )
{   
   int type = TYPE(plateformFile);
   if ( type != T_STRING )
      rb_raise(rb_eRuntimeError,"Bad Argument's Type");  
   const char * platform =  RSTRING(plateformFile)->ptr;
   SD_create_environment(platform);
   printf("Create Environment...Done\n");

 return;  
}
 

// INFO
static void sd_info(VALUE class,VALUE msg)
{
 const char *s = RSTRING(msg)->ptr;
//  INFO1("%s",s);
 printf("%s\n",s);
}

// Get Clock
static VALUE sd_get_clock(VALUE class)
{
  return rb_float_new(SD_get_clock());
}   

void Init_dag()
{
   // Modules
   rb_dag = rb_define_module("DAG");
    
   // Classes
   //Associated Environment Methods!
   rb_define_method(rb_dag,"init",sd_init,1);
   rb_define_method(rb_dag,"reinit",sd_reinit,0);
   rb_define_method(rb_dag,"createEnvironment",sd_createEnvironment,1);
   rb_define_method(rb_dag,"info",sd_info,1);
   rb_define_method(rb_dag,"getClock",sd_get_clock,0);
   
   //Classes       
   rb_task = rb_define_class_under(rb_dag,"SdTask",rb_cObject);
   rb_workstation = rb_define_class_under(rb_dag,"SdWorkstation",rb_cObject);
//    TODO Link Class
    
   //Task Methods    
   rb_define_module_function(rb_task,"new",rb_SD_task_new,2);
//    rb_define_module_function(rb_task,"free",rb_SD_task_destroy,1);
   rb_define_module_function(rb_task,"name",rb_SD_task_name,1);
   rb_define_module_function(rb_task,"schedule",rb_SD_task_schedule,6);
   rb_define_module_function(rb_task,"unschedule",rb_SD_task_unschedule,1); 
   rb_define_module_function(rb_task,"addDependency",rb_SD_task_add_dependency,2);
   rb_define_module_function(rb_task,"executionTime",rb_SD_task_execution_time,6);
   rb_define_module_function(rb_task,"startTime",rb_SD_task_start_time,1);
   rb_define_module_function(rb_task,"finishTime",rb_SD_task_finish_time,1);
   rb_define_module_function(rb_task,"simulate",rb_SD_simulate,1);
   
   //Worstation Methods  
   rb_define_module_function(rb_workstation,"getList",rb_SD_workstation_list,0);
   rb_define_module_function(rb_workstation,"getNumber",rb_SD_workstation_number,0);
   rb_define_module_function(rb_workstation,"name",rb_SD_workstation_name,1);
}  
