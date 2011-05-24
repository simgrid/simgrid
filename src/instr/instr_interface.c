/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid_config.h"

#ifdef HAVE_TRACING

#include "instr/instr_private.h"
#include "surf/network_private.h"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_api, instr, "API");

void TRACE_category(const char *category)
{
  TRACE_category_with_color (category, NULL);
}

void TRACE_category_with_color (const char *category, const char *color)
{
  if (!(TRACE_is_active() && category != NULL))
    return;

  xbt_assert (instr_platform_traced(),
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

  XBT_DEBUG("CAT,declare %s, %s", category, final_color);

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
    instr_new_variable_type (category, final_color);
  }
}

void TRACE_declare_mark(const char *mark_type)
{
  if (!TRACE_is_active())
    return;
  if (!mark_type)
    return;

  XBT_DEBUG("MARK,declare %s", mark_type);
  getEventType(mark_type, NULL, getRootType());
}

void TRACE_mark(const char *mark_type, const char *mark_value)
{
  if (!TRACE_is_active())
    return;
  if (!mark_type || !mark_value)
    return;

  XBT_DEBUG("MARK %s %s", mark_type, mark_value);
  type_t type = getEventType (mark_type, NULL, getRootContainer()->type);
  val_t value = getValue (mark_value, NULL, type);
  new_pajeNewEvent (MSG_get_clock(), getRootContainer(), type, value);
}

void TRACE_user_variable(double time,
                         const char *resource,
                         const char *variable,
                         const char *father_type,
                         double value,
                         InstrUserVariable what)
{
  if (!TRACE_is_active())
    return;

  xbt_assert (instr_platform_traced(),
      "%s must be called after environment creation", __FUNCTION__);

  char valuestr[100];
  snprintf(valuestr, 100, "%g", value);

  switch (what){
  case INSTR_US_DECLARE:
    instr_new_user_variable_type (father_type, variable, NULL);
    break;
  case INSTR_US_SET:
  {
    container_t container = getContainerByName(resource);
    type_t type = getVariableType (variable, NULL, container->type);
    new_pajeSetVariable(time, container, type, value);
    break;
  }
  case INSTR_US_ADD:
  {
    container_t container = getContainerByName(resource);
    type_t type = getVariableType (variable, NULL, container->type);
    new_pajeAddVariable(time, container, type, value);
    break;
  }
  case INSTR_US_SUB:
  {
    container_t container = getContainerByName(resource);
    type_t type = getVariableType (variable, NULL, container->type);
    new_pajeSubVariable(time, container, type, value);
    break;
  }
  default:
    //TODO: launch exception
    break;
  }
}

void TRACE_user_srcdst_variable(double time,
                              const char *src,
                              const char *dst,
                              const char *variable,
                              const char *father_type,
                              double value,
                              InstrUserVariable what)
{
  xbt_dynar_t route = global_routing->get_route (src, dst);
  unsigned int i;
  void *link;
  xbt_dynar_foreach (route, i, link) {
    char *link_name = ((link_CM02_t)link)->lmm_resource.generic_resource.name;
    TRACE_user_variable (time, link_name, variable, father_type, value, what);
  }
}

const char *TRACE_node_name (xbt_node_t node)
{
  void *data = xbt_graph_node_get_data(node);
  char *str = (char*)data;
  return str;
}

xbt_graph_t TRACE_platform_graph (void)
{
  if (!TRACE_is_active())
    return NULL;

  return instr_routing_platform_graph ();
}

void TRACE_platform_graph_export_graphviz (xbt_graph_t g, const char *filename)
{
  instr_routing_platform_graph_export_graphviz (g, filename);
}

#endif /* HAVE_TRACING */
