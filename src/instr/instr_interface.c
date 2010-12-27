/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid_config.h"

#ifdef HAVE_TRACING

#include "instr/instr_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_api, instr, "API");

void TRACE_category(const char *category)
{
  TRACE_category_with_color (category, NULL);
}

void TRACE_category_with_color (const char *category, const char *color)
{
  if (!(TRACE_is_active() && category != NULL))
    return;

  xbt_assert1 (instr_platform_traced(),
      "%s must be called after environment creation", __FUNCTION__);

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
  getEventType(mark_type, NULL, getRootType());
}

void TRACE_mark(const char *mark_type, const char *mark_value)
{
  if (!TRACE_is_active())
    return;
  if (!mark_type || !mark_value)
    return;

  DEBUG2("MARK %s %s", mark_type, mark_value);
  type_t type = getEventType (mark_type, NULL, getRootContainer()->type);
  new_pajeNewEvent (MSG_get_clock(), getRootContainer(), type, mark_value);
}


void TRACE_user_link_variable(double time, const char *resource,
                              const char *variable,
                              double value, const char *what)
{
  if (!TRACE_is_active())
    return;

  xbt_assert1 (instr_platform_traced(),
      "%s must be called after environment creation", __FUNCTION__);

  char valuestr[100];
  snprintf(valuestr, 100, "%g", value);

  if (strcmp(what, "declare") == 0) {
    instr_new_user_link_variable_type (variable, NULL);
  } else{
    container_t container = getContainerByName (resource);
    type_t type = getVariableType (variable, NULL, container->type);
    if (strcmp(what, "set") == 0) {
      new_pajeSetVariable(time, container, type, value);
    } else if (strcmp(what, "add") == 0) {
      new_pajeAddVariable(time, container, type, value);
    } else if (strcmp(what, "sub") == 0) {
      new_pajeSubVariable(time, container, type, value);
    }
  }
}

void TRACE_user_host_variable(double time, const char *variable,
                              double value, const char *what)
{
  if (!TRACE_is_active())
    return;

  xbt_assert1 (instr_platform_traced(),
      "%s must be called after environment creation", __FUNCTION__);

  char valuestr[100];
  snprintf(valuestr, 100, "%g", value);

  if (strcmp(what, "declare") == 0) {
    instr_new_user_host_variable_type (variable, NULL);
  } else{
    char *host_name = MSG_host_self()->name;
    container_t container = getContainerByName(host_name);
    type_t type = getVariableType (variable, NULL, container->type);
    if (strcmp(what, "set") == 0) {
      new_pajeSetVariable(time, container, type, value);
    } else if (strcmp(what, "add") == 0) {
      new_pajeAddVariable(time, container, type, value);
    } else if (strcmp(what, "sub") == 0) {
      new_pajeSubVariable(time, container, type, value);
    }
  }
}

#endif /* HAVE_TRACING */
