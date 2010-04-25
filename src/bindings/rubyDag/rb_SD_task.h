/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef RB_SD_TASK_H
#define RB_SD_TASK_H

#include <ruby.h>
#include <simdag/simdag.h>
#include "xbt/sysdep.h" 


// Free Method
static void SD_task_free(SD_task_t tk); // needed by ruby while wrapping

//destroy
static void rb_SD_task_destory(VALUE Class,VALUE task);

// New Method  
static VALUE rb_SD_task_new(VALUE Class, VALUE name,VALUE amount); //data set to NULL

//Get Name
static VALUE rb_SD_task_name(VALUE Class,VALUE task);

// Schedule Task
static void rb_SD_task_schedule(VALUE Class,VALUE task,VALUE workstation_nb,VALUE workstation_list,VALUE comp_amount,VALUE comm_amount,VALUE rate);

// unschedule Task
static void rb_SD_task_unschedule(VALUE Class,VALUE task);

// task dependency Add ( name & data set to NULL)
static void rb_SD_task_add_dependency(VALUE Class,VALUE task_src,VALUE task_dst);

// task execution time
static VALUE rb_SD_task_execution_time(VALUE Class,VALUE task,VALUE workstation_nb,VALUE workstation_list,VALUE comp_amount,VALUE comm_amount,VALUE rate);

// task start time
static VALUE rb_SD_task_start_time(VALUE Class,VALUE task);

// task finish time
static VALUE rb_SD_task_finish_time(VALUE Class,VALUE task);

// simulation
static VALUE rb_SD_simulate(VALUE Class,VALUE how_long);

#endif