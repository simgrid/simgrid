/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid_config.h"

#ifdef HAVE_TRACING

#include "instr/instr_private.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(tracing, "Tracing Interface");

static xbt_dict_t defined_types;
static xbt_dict_t created_categories;

int TRACE_start()
{
  // tracing system must be:
  //    - enabled (with --cfg=tracing:1)
  //    - already configured (TRACE_global_init already called)
  if (!(TRACE_is_enabled() && TRACE_is_configured())){
    return 0;
  }

  /* open the trace file */
  TRACE_paje_start();

  /* activate trace */
  TRACE_activate ();

  /* base type hierarchy:
   * --cfg=tracing
   */
  pajeDefineContainerType("PLATFORM", "0", "platform");
  pajeDefineContainerType("HOST", "PLATFORM", "HOST");
  pajeDefineContainerType("LINK", "PLATFORM", "LINK");
  pajeDefineVariableType("power", "HOST", "power");
  pajeDefineVariableType("bandwidth", "LINK", "bandwidth");
  pajeDefineVariableType("latency", "LINK", "latency");
  pajeDefineEventType("source", "LINK", "source");
  pajeDefineEventType("destination", "LINK", "destination");

  /* type hierarchy for:
   * --cfg=tracing/uncategorized
   */
  if (TRACE_platform_is_enabled()) {
    if (TRACE_uncategorized()){
      pajeDefineVariableType("power_used", "HOST", "power_used");
      pajeDefineVariableType("bandwidth_used", "LINK", "bandwidth_used");
    }
  }

  /* type hierarchy for:
   * --cfg=tracing/msg/process
   * --cfg=tracing/msg/volume
   */
  if (TRACE_msg_process_is_enabled() || TRACE_msg_volume_is_enabled()) {
    //processes grouped by host
    pajeDefineContainerType("PROCESS", "HOST", "PROCESS");
  }

  if (TRACE_msg_process_is_enabled()) {
    pajeDefineStateType("category", "PROCESS", "category");
    pajeDefineStateType("presence", "PROCESS", "presence");
  }

  if (TRACE_msg_volume_is_enabled()) {
    pajeDefineLinkType("volume", "0", "PROCESS", "PROCESS", "volume");
  }

  /* type hierarchy for:
   * --cfg=tracing/msg/task
   */
  if (TRACE_msg_task_is_enabled()) {
    //tasks grouped by host
    pajeDefineContainerType("TASK", "HOST", "TASK");
    pajeDefineStateType("category", "TASK", "category");
    pajeDefineStateType("presence", "TASK", "presence");
  }

  /* type hierarchy for
   * --cfg=tracing/smpi
   * --cfg=tracing/smpi/group
   */
  if (TRACE_smpi_is_enabled()) {
    if (TRACE_smpi_is_grouped()){
      pajeDefineContainerType("MPI_PROCESS", "HOST", "MPI_PROCESS");
    }else{
      pajeDefineContainerType("MPI_PROCESS", "PLATFORM", "MPI_PROCESS");
    }
    pajeDefineStateType("MPI_STATE", "MPI_PROCESS", "MPI_STATE");
    pajeDefineLinkType("MPI_LINK", "0", "MPI_PROCESS", "MPI_PROCESS",
                       "MPI_LINK");
  }

  /* creating the platform */
  pajeCreateContainer(MSG_get_clock(), "platform", "PLATFORM", "0",
                      "simgrid-platform");

  /* other trace initialization */
  defined_types = xbt_dict_new();
  created_categories = xbt_dict_new();
  TRACE_msg_task_alloc();
  TRACE_category_alloc();
  TRACE_surf_alloc();
  TRACE_msg_process_alloc();
  TRACE_smpi_alloc();
  return 0;
}

int TRACE_end()
{
  if (!TRACE_is_active())
    return 1;

  /* close the trace file */
  TRACE_paje_end();

  /* activate trace */
  TRACE_desactivate ();
  return 0;
}

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

  //check if type is already defined
  if (xbt_dict_get_or_null(defined_types, type)) {
    THROW1(tracing_error, TRACE_ERROR_TYPE_ALREADY_DEFINED,
           "Type %s is already defined", type);
  }
  //check if parent_type is already defined
  if (strcmp(parent_type, "0")
      && !xbt_dict_get_or_null(defined_types, parent_type)) {
    THROW1(tracing_error, TRACE_ERROR_TYPE_NOT_DEFINED,
           "Type (used as parent) %s is not defined", parent_type);
  }

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

  //check if type is defined
  if (!xbt_dict_get_or_null(defined_types, type)) {
    THROW1(tracing_error, TRACE_ERROR_TYPE_NOT_DEFINED,
           "Type %s is not defined", type);
    return 1;
  }
  //check if parent_category exists
  if (strcmp(parent_category, "0")
      && !xbt_dict_get_or_null(created_categories, parent_category)) {
    THROW1(tracing_error, TRACE_ERROR_CATEGORY_NOT_DEFINED,
           "Category (used as parent) %s is not created", parent_category);
    return 1;
  }
  //check if category is created
  if (xbt_dict_get_or_null(created_categories, category)) {
    return 1;
  }

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

  pajeDefineEventType(mark_type, "0", mark_type);
}

void TRACE_mark(const char *mark_type, const char *mark_value)
{
  if (!TRACE_is_active())
    return;
  if (!mark_type || !mark_value)
    return;

  pajeNewEvent(MSG_get_clock(), mark_type, "0", mark_value);
}

#endif /* HAVE_TRACING */
