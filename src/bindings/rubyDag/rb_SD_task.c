/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "rb_SD_task.h"

// Free Method
static void SD_task_free(SD_task_t tk) {
//   SD_task_destroy(tk);
}

static void rb_SD_task_destroy(VALUE class,VALUE task)
{
 SD_task_t tk ;
 Data_Get_Struct(task, SD_task_t, tk);
 SD_task_destroy(tk); 
}
// New Method
static VALUE rb_SD_task_new(VALUE class, VALUE name,VALUE amount)
{
  //data Set to NULL
  SD_task_t task = SD_task_create(RSTRING(name)->ptr,NULL,NUM2DBL(amount));
  // Wrap m_task_t to a Ruby Value
  return Data_Wrap_Struct(class, 0, SD_task_free, task);

}

//Get Name
static VALUE rb_SD_task_name(VALUE class,VALUE task)
{
  // Wrap Ruby Value to m_task_t struct
  SD_task_t tk;
  Data_Get_Struct(task, SD_task_t, tk);
  return rb_str_new2(SD_task_get_name(tk));
   
}

// Schedule Task
static void rb_SD_task_schedule(VALUE class,VALUE task,VALUE workstation_nb,VALUE workstation_list,VALUE computation_amount,VALUE communication_amount,VALUE rate)
{
  // Wrap Ruby Value to m_task_t struct
  int i,wrk_nb,comp_nb,comm_nb;
  double *comp_amount,*comm_amount;
  double rt;
  VALUE *ptr_wrk,*ptr_comp,*ptr_comm;
  SD_task_t tk;
  Data_Get_Struct(task, SD_task_t, tk);
  wrk_nb = NUM2INT(workstation_nb);
  comp_nb = RARRAY(computation_amount)->len;
  comm_nb = RARRAY(communication_amount)->len;
  rt = NUM2DBL(rate);
  SD_workstation_t *wrk_list;
  
  ptr_wrk = RARRAY(workstation_list)->ptr;
  ptr_comp = RARRAY(computation_amount)->ptr;
  ptr_comm = RARRAY(communication_amount)->ptr;
  
  wrk_list = xbt_new0(SD_workstation_t,wrk_nb);
  comp_amount = xbt_new0(double,wrk_nb);
  comm_amount = xbt_new0(double,wrk_nb);
  
  // wrk_nb == comp_nb == comm_nb ???
  for (i=0;i<wrk_nb;i++)
  {
    Data_Get_Struct(ptr_wrk[i],SD_workstation_t,wrk_list[i]); 
    comp_amount[i] = NUM2DBL(ptr_comp[i]);
    comm_amount[i] = NUM2DBL(ptr_comm[i]);
  }
  /*for (i=0;i<comp_nb;i++)
    comp_amount[i] = NUM2DBL(ptr_comp[i]);
  
  for (i=0;i<comm_nb;i++)
    comm_amount[i] = NUM2DBL(ptr_comm[i]);*/
  
  SD_task_schedule(tk,wrk_nb,wrk_list,comp_amount,comm_amount,rt);
  
  xbt_free(wrk_list);
  xbt_free(comp_amount);
  xbt_free(comm_amount);
  

}

//unschedule
static void rb_SD_task_unschedule(VALUE class,VALUE task)
{
  SD_task_t tk;
  Data_Get_Struct(task,SD_task_t,tk);
  SD_task_unschedule (tk);
  
}

// task dependency add
static void rb_SD_task_add_dependency(VALUE Class,VALUE task_src,VALUE task_dst)
{
  SD_task_t tk_src,tk_dst;
  Data_Get_Struct(task_src, SD_task_t ,tk_src);
  Data_Get_Struct(task_dst, SD_task_t ,tk_dst);
  SD_task_dependency_add(NULL,NULL,tk_src,tk_dst);
  
}

//task execution time
static VALUE rb_SD_task_execution_time(VALUE class,VALUE task,VALUE workstation_nb,VALUE workstation_list,VALUE computation_amount,VALUE communication_amount,VALUE rate)
{
  
  int i,wrk_nb;
  double *comp_amount,*comm_amount;
  double rt;
  VALUE *ptr_wrk,*ptr_comp,*ptr_comm;
  SD_task_t tk;
  Data_Get_Struct(task, SD_task_t, tk);
  wrk_nb = NUM2INT(workstation_nb);
  rt = NUM2DBL(rate);
  SD_workstation_t *wrk_list;
  
  ptr_wrk = RARRAY(workstation_list)->ptr;
  ptr_comp = RARRAY(computation_amount)->ptr;
  ptr_comm = RARRAY(communication_amount)->ptr;
  
  wrk_list = xbt_new0(SD_workstation_t,wrk_nb);
  comp_amount = xbt_new0(double,wrk_nb);
  comm_amount = xbt_new0(double,wrk_nb);
  
  for (i=0;i<wrk_nb;i++)
  {
    Data_Get_Struct(ptr_wrk[i],SD_workstation_t,wrk_list[i]); 
    comp_amount[i] = NUM2DBL(ptr_comp[i]);
    comm_amount[i] = NUM2DBL(ptr_comm[i]);
  }
  
  return rb_float_new(SD_task_get_execution_time (tk,wrk_nb,wrk_list,comp_amount,comm_amount,rt));
  
}

//task start time
static VALUE rb_SD_task_start_time(VALUE class,VALUE task)
{

  SD_task_t tk;
  Data_Get_Struct(task,SD_task_t,tk);
  double time=SD_task_get_start_time(tk);
  return rb_float_new(time);
  
}

//task fininsh time
static VALUE rb_SD_task_finish_time(VALUE class,VALUE task)
{
  SD_task_t tk;
  Data_Get_Struct(task,SD_task_t,tk);
  double time=SD_task_get_finish_time(tk);
  return rb_float_new(time);
  
}

// Simulate : return a SD_task
static VALUE rb_SD_simulate(VALUE class,VALUE how_long)
{
  int i;
  double hl = NUM2DBL(how_long);
  SD_task_t * tasks = SD_simulate(hl); 
  VALUE rb_tasks = rb_ary_new();
  for (i = 0; tasks[i] != NULL; i++)
    {
      VALUE tk = Qnil;
      tk = Data_Wrap_Struct(class, 0, SD_task_free, tasks[i]);
      rb_ary_push(rb_tasks,tk);
      
    }
  return  rb_tasks;
 
}
