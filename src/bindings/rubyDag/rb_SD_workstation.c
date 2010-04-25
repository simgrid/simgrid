/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "rb_SD_workstation.h"

static void SD_workstation_free(SD_workstation_t wrk)
{
  //NOTHING TO DO
}
// Workstation list
static VALUE rb_SD_workstation_list(VALUE class)
{
 
  int i,nb;
  nb = SD_workstation_get_number();
  VALUE workstation_list = rb_ary_new2(nb);
  for (i=0;i<nb;i++)
  {
   VALUE wrk = Qnil;
   wrk = Data_Wrap_Struct(class, 0, SD_workstation_free, SD_workstation_get_list()[i]);
   rb_ary_push(workstation_list,wrk);
    
  }
  return workstation_list;
}

// Workstation number
static VALUE rb_SD_workstation_number(VALUE class)
{
  int nb = SD_workstation_get_number();
  return INT2NUM(nb);
}

// Workstation name
static VALUE rb_SD_workstation_name(VALUE class,VALUE workstation)
{
 SD_workstation_t wk;
 Data_Get_Struct(workstation, SD_workstation_t, wk);
 return rb_str_new2(SD_workstation_get_name(wk));
 
}