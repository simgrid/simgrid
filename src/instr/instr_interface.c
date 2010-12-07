/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid_config.h"

#ifdef HAVE_TRACING

#include "instr/instr_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_api, instr, "API");

xbt_dict_t defined_types;
xbt_dict_t created_categories;

int TRACE_category(const char *category)
{
  return TRACE_category_with_color (category, NULL);
}

int TRACE_category_with_color (const char *category, const char *color)
{
  static int first_time = 1;
  if (!TRACE_is_active())
    return 1;

  if (first_time) {
    TRACE_define_type("user_type", "0", 1);
    first_time = 0;
  }
  return TRACE_create_category_with_color(category, "user_type", "0", color);
}

void TRACE_define_type(const char *type,
                       const char *parent_type, int final)
{
  char *val_one = NULL;
  if (!TRACE_is_active())
    return;

  char *defined;
  //type must not exist
  defined = xbt_dict_get_or_null(defined_types, type);
  xbt_assert1 (defined == NULL, "Type %s is already defined", type);
  //parent_type must exist or be different of 0
  defined = xbt_dict_get_or_null(defined_types, parent_type);
  xbt_assert1 (defined != NULL || strcmp(parent_type, "0") == 0,
      "Type (used as parent) %s is not defined", parent_type);

  pajeDefineContainerType(type, parent_type, type);
  if (final) {
    //for m_process_t
    if (TRACE_msg_process_is_enabled())
      pajeDefineContainerType("process", type, "process");
    if (TRACE_msg_process_is_enabled())
      pajeDefineStateType("process-state", "process", "process-state");

    if (TRACE_msg_task_is_enabled())
      pajeDefineContainerType("task", type, "task");
    if (TRACE_msg_task_is_enabled())
      pajeDefineStateType("task-state", "task", "task-state");
  }
  val_one = xbt_strdup("1");
  xbt_dict_set(defined_types, type, &val_one, xbt_free);
}

int TRACE_create_category(const char *category,
                          const char *type, const char *parent_category)
{
  return TRACE_create_category_with_color (category, type, parent_category, NULL);
}

int TRACE_create_category_with_color(const char *category,
                          const char *type,
                          const char *parent_category,
                          const char *color)
{
  char state[100];
  char *val_one = NULL;
  if (!TRACE_is_active())
    return 1;

  char *defined = xbt_dict_get_or_null(defined_types, type);
  //check if type is defined
  xbt_assert1 (defined != NULL, "Type %s is not defined", type);
  //check if parent_category exists
  defined = xbt_dict_get_or_null(created_categories, parent_category);
  xbt_assert1 (defined != NULL || strcmp(parent_category, "0") == 0,
      "Category (used as parent) %s is not created", parent_category);
  //check if category is created
  char *created = xbt_dict_get_or_null(created_categories, category);
  if (created) return 0;

  pajeCreateContainer(MSG_get_clock(), category, type, parent_category,
                      category);

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

  /* for registering application categories on top of platform */
  snprintf(state, 100, "b%s", category);
  if (TRACE_platform_is_enabled())
    pajeDefineVariableTypeWithColor(state, "LINK", state, final_color);
  snprintf(state, 100, "p%s", category);
  if (TRACE_platform_is_enabled())
    pajeDefineVariableTypeWithColor(state, "HOST", state, final_color);

  val_one = xbt_strdup("1");
  xbt_dict_set(created_categories, category, &val_one, xbt_free);
  return 0;
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
