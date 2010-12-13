/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid_config.h"

#ifdef HAVE_TRACING

#include "instr/instr_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_api, instr, "API");

xbt_dict_t created_categories;

void TRACE_category(const char *category)
{
  TRACE_category_with_color (category, NULL);
}

void TRACE_category_with_color (const char *category, const char *color)
{
  if (!TRACE_is_active())
    return;
//
//  {
//    //check if hosts have been created
//    xbt_assert1 (allHostTypes != NULL && xbt_dict_length(allHostTypes) != 0,
//        "%s must be called after environment creation", __FUNCTION__);
//  }

  //check if category is already created
  char *created = xbt_dict_get_or_null(created_categories, category);
  if (created) return;
  xbt_dict_set (created_categories, category, xbt_strdup("1"), xbt_free);

  //define final_color
  char final_color[INSTR_DEFAULT_STR_SIZE];
  if (!color){
    //generate a random color
    double red = drand48();
    double green = drand48();
    double blue = drand48();
    snprintf (final_color, INSTR_DEFAULT_STR_SIZE, "%f %f %f", red, green, blue);
  }else{
    snprintf (final_color, INSTR_DEFAULT_STR_SIZE, "%s", color);
  }

  DEBUG2("CAT,declare %s, %s", category, final_color);

//FIXME
//  -  if (final) {
//  -    //for m_process_t
//  -    if (TRACE_msg_process_is_enabled())
//  -      pajeDefineContainerType("process", type, "process");
//  -    if (TRACE_msg_process_is_enabled())
//  -      pajeDefineStateType("process-state", "process", "process-state");
//  -
//  -    if (TRACE_msg_task_is_enabled())
//  -      pajeDefineContainerType("task", type, "task");
//  -    if (TRACE_msg_task_is_enabled())
//  -      pajeDefineStateType("task-state", "task", "task-state");
//  -  }

  //define the type of this category on top of hosts and links
  if (TRACE_categorized ()){
    instr_new_user_variable_type (category, final_color);
  }
}

void TRACE_declare_mark(const char *mark_type)
{
  if (!TRACE_is_active())
    return;
  if (!mark_type)
    return;

  DEBUG1("MARK,declare %s", mark_type);
  pajeDefineEventType(mark_type, "0", mark_type);
}

void TRACE_mark(const char *mark_type, const char *mark_value)
{
  if (!TRACE_is_active())
    return;
  if (!mark_type || !mark_value)
    return;

  DEBUG2("MARK %s %s", mark_type, mark_value);
  pajeNewEvent(MSG_get_clock(), mark_type, "0", mark_value);
}

#endif /* HAVE_TRACING */
