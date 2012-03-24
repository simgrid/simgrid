/* Copyright (c) 2010. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
  * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid_config.h"

#ifdef HAVE_TRACING
#include "instr/instr_private.h"
#include "surf/network_private.h"

typedef enum {
  INSTR_US_DECLARE,
  INSTR_US_SET,
  INSTR_US_ADD,
  INSTR_US_SUB
} InstrUserVariable;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_api, instr, "API");

/** \ingroup TRACE_category
 *  \brief Declare a new category with a random color.
 *
 *  This function should be used to define a user category. The
 *  category can be used to differentiate the tasks that are created
 *  during the simulation (for example, tasks from server1, server2,
 *  or request tasks, computation tasks, communication tasks). All
 *  resource utilization (host power and link bandwidth) will be
 *  classified according to the task category. Tasks that do not
 *  belong to a category are not traced. The color for the category
 *  that is being declared is random. This function has no effect
 *  if a category with the same name has been already declared.
 *
 * See \ref tracing_tracing for details on how to trace
 * the (categorized) resource utilization.
 *
 *  \param category The name of the new tracing category to be created.
 *
 *  \see TRACE_category_with_color, MSG_task_set_category
 */
void TRACE_category(const char *category)
{
  TRACE_category_with_color (category, NULL);
}

/** \ingroup TRACE_category
 *  \brief Declare a new category with a color.
 *
 *  Same as #TRACE_category, but let user specify a color encoded as a
 *  RGB-like string with three floats from 0 to 1. So, to specify a
 *  red color, pass "1 0 0" as color parameter. A light-gray color
 *  can be specified using "0.7 0.7 0.7" as color. This function has
 *  no effect if a category with the same name has been already declared.
 *
 * See \ref tracing_tracing for details on how to trace
 * the (categorized) resource utilization.
 *
 *  \param category The name of the new tracing category to be created.
 *  \param color The color of the category (see \ref tracing_tracing to
 *  know how to correctly specify the color)
 *
 *  \see MSG_task_set_category
 */
void TRACE_category_with_color (const char *category, const char *color)
{
  /* safe switch */
  if (!TRACE_is_enabled()) return;

  if (!(TRACE_categorized() && category != NULL)) return;

  /* if platform is not traced, we can't deal with categories */
  if (!TRACE_needs_platform()) return;

  //check if category is already created
  char *created = xbt_dict_get_or_null(created_categories, category);
  if (created) return;
  xbt_dict_set (created_categories, category, xbt_strdup("1"), NULL);

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

  //define the type of this category on top of hosts and links
  instr_new_variable_type (category, final_color);
}

/** \ingroup TRACE_mark
 * \brief Declare a new type for tracing mark.
 *
 * This function declares a new Paje event
 * type in the trace file that can be used by
 * simulators to declare application-level
 * marks. This function is independent of
 * which API is used in SimGrid.
 *
 * \param mark_type The name of the new type.
 *
 * \see TRACE_mark
 */
void TRACE_declare_mark(const char *mark_type)
{
  /* safe switch */
  if (!TRACE_is_enabled()) return;

  if (!mark_type) return;

  XBT_DEBUG("MARK,declare %s", mark_type);
  PJ_type_event_new(mark_type, NULL, PJ_type_get_root());
}

/**
 * \ingroup TRACE_mark
 * \brief Create a new instance of a tracing mark type.
 *
 * This function creates a mark in the trace file. The
 * first parameter had to be previously declared using
 * #TRACE_declare_mark, the second is the identifier
 * for this mark instance. We recommend that the
 * mark_value is a unique value for the whole simulation.
 * Nevertheless, this is not a strong requirement: the
 * trace will be valid even if there are multiple mark
 * identifiers for the same trace.
 *
 * \param mark_type The name of the type for which the new instance will belong.
 * \param mark_value The name of the new instance mark.
 *
 * \see TRACE_declare_mark
 */
void TRACE_mark(const char *mark_type, const char *mark_value)
{
  /* safe switch */
  if (!TRACE_is_enabled()) return;

  if (!mark_type || !mark_value) return;

  XBT_DEBUG("MARK %s %s", mark_type, mark_value);
  type_t type = PJ_type_get (mark_type, PJ_type_get_root());
  if (type == NULL){
    THROWF (tracing_error, 1, "mark_type with name (%s) not declared before", mark_type);
  }
  val_t value = PJ_value_get (mark_value, type);
  if (value == NULL){
    value = PJ_value_new (mark_value, NULL, type);
  }
  new_pajeNewEvent (MSG_get_clock(), PJ_container_get_root(), type, value);
}

static void instr_user_variable(double time,
                         const char *resource,
                         const char *variable,
                         const char *father_type,
                         double value,
                         InstrUserVariable what,
                         const char *color)
{
  /* safe switch */
  if (!TRACE_is_enabled()) return;

  /* if platform is not traced, we can't deal user variables */
  if (!TRACE_needs_platform()) return;

  char valuestr[100];
  snprintf(valuestr, 100, "%g", value);

  switch (what){
  case INSTR_US_DECLARE:
    instr_new_user_variable_type (father_type, variable, color);
    break;
  case INSTR_US_SET:
  {
    container_t container = PJ_container_get(resource);
    type_t type = PJ_type_get (variable, container->type);
    new_pajeSetVariable(time, container, type, value);
    break;
  }
  case INSTR_US_ADD:
  {
    container_t container = PJ_container_get(resource);
    type_t type = PJ_type_get (variable, container->type);
    new_pajeAddVariable(time, container, type, value);
    break;
  }
  case INSTR_US_SUB:
  {
    container_t container = PJ_container_get(resource);
    type_t type = PJ_type_get (variable, container->type);
    new_pajeSubVariable(time, container, type, value);
    break;
  }
  default:
    //TODO: launch exception
    break;
  }
}

static void instr_user_srcdst_variable(double time,
                              const char *src,
                              const char *dst,
                              const char *variable,
                              const char *father_type,
                              double value,
                              InstrUserVariable what)
{
  xbt_dynar_t route=NULL;
  network_element_t src_elm = xbt_lib_get_or_null(host_lib,src,ROUTING_HOST_LEVEL);
  if(!src_elm) src_elm = xbt_lib_get_or_null(as_router_lib,src,ROUTING_ASR_LEVEL);
  if(!src_elm) xbt_die("Element '%s' not found!",src);

  network_element_t dst_elm = xbt_lib_get_or_null(host_lib,dst,ROUTING_HOST_LEVEL);
  if(!dst_elm) dst_elm = xbt_lib_get_or_null(as_router_lib,dst,ROUTING_ASR_LEVEL);
  if(!dst_elm) xbt_die("Element '%s' not found!",dst);

  routing_get_route_and_latency (src_elm, dst_elm, &route,NULL);
  unsigned int i;
  void *link;
  xbt_dynar_foreach (route, i, link) {
    char *link_name = ((link_CM02_t)link)->lmm_resource.generic_resource.name;
    instr_user_variable (time, link_name, variable, father_type, value, what, NULL);
  }
}

int TRACE_platform_graph_export_graphviz (const char *filename)
{
  /* returns 1 if successful, 0 otherwise */
  if (!TRACE_is_enabled()) return 0;
  xbt_graph_t g = instr_routing_platform_graph();
  if (g == NULL) return 0;
  instr_routing_platform_graph_export_graphviz (g, filename);
  xbt_graph_free_graph (g, xbt_free, xbt_free, NULL);
  return 1;
}

/*
 * Derived functions that use instr_user_variable and TRACE_user_srcdst_variable.
 * They were previously defined as pre-processors directives, but were transformed
 * into functions so the user can track them using gdb.
 */

/* for host variables */
void TRACE_host_variable_declare (const char *var)
{
  instr_user_variable(0, NULL, var, "HOST", 0, INSTR_US_DECLARE, NULL);
}

void TRACE_host_variable_declare_with_color (const char *var, const char *color)
{
  instr_user_variable(0, NULL, var, "HOST", 0, INSTR_US_DECLARE, color);
}

void TRACE_host_variable_set (const char *host, const char *variable, double value)
{
  TRACE_host_variable_set_with_time (MSG_get_clock(), host, variable, value);
}

void TRACE_host_variable_add (const char *host, const char *variable, double value)
{
  TRACE_host_variable_add_with_time (MSG_get_clock(), host, variable, value);
}

void TRACE_host_variable_sub (const char *host, const char *variable, double value)
{
  TRACE_host_variable_sub_with_time (MSG_get_clock(), host, variable, value);
}

void TRACE_host_variable_set_with_time (double time, const char *host, const char *variable, double value)
{
  instr_user_variable(time, host, variable, "HOST", value, INSTR_US_SET, NULL);
}

void TRACE_host_variable_add_with_time (double time, const char *host, const char *variable, double value)
{
  instr_user_variable(time, host, variable, "HOST", value, INSTR_US_ADD, NULL);
}

void TRACE_host_variable_sub_with_time (double time, const char *host, const char *variable, double value)
{
  instr_user_variable(time, host, variable, "HOST", value, INSTR_US_SUB, NULL);
}

/* for link variables */
void TRACE_link_variable_declare (const char *var)
{
  instr_user_variable (0, NULL, var, "LINK", 0, INSTR_US_DECLARE, NULL);
}

void TRACE_link_variable_declare_with_color (const char *var, const char *color)
{
  instr_user_variable (0, NULL, var, "LINK", 0, INSTR_US_DECLARE, color);
}

void TRACE_link_variable_set (const char *link, const char *variable, double value)
{
  TRACE_link_variable_set_with_time (MSG_get_clock(), link, variable, value);
}

void TRACE_link_variable_add (const char *link, const char *variable, double value)
{
  TRACE_link_variable_add_with_time (MSG_get_clock(), link, variable, value);
}

void TRACE_link_variable_sub (const char *link, const char *variable, double value)
{
  TRACE_link_variable_sub_with_time (MSG_get_clock(), link, variable, value);
}

void TRACE_link_variable_set_with_time (double time, const char *link, const char *variable, double value)
{
  instr_user_variable (time, link, variable, "LINK", value, INSTR_US_SET, NULL);
}

void TRACE_link_variable_add_with_time (double time, const char *link, const char *variable, double value)
{
  instr_user_variable (time, link, variable, "LINK", value, INSTR_US_ADD, NULL);
}

void TRACE_link_variable_sub_with_time (double time, const char *link, const char *variable, double value)
{
  instr_user_variable (time, link, variable, "LINK", value, INSTR_US_SUB, NULL);
}

/* for link variables, but with src and dst used for get_route */
void TRACE_link_srcdst_variable_set (const char *src, const char *dst, const char *variable, double value)
{
  TRACE_link_srcdst_variable_set_with_time (MSG_get_clock(), src, dst, variable, value);
}

void TRACE_link_srcdst_variable_add (const char *src, const char *dst, const char *variable, double value)
{
  TRACE_link_srcdst_variable_add_with_time (MSG_get_clock(), src, dst, variable, value);
}

void TRACE_link_srcdst_variable_sub (const char *src, const char *dst, const char *variable, double value)
{
  TRACE_link_srcdst_variable_sub_with_time (MSG_get_clock(), src, dst, variable, value);
}

void TRACE_link_srcdst_variable_set_with_time (double time, const char *src, const char *dst, const char *variable, double value)
{
  instr_user_srcdst_variable (time, src, dst, variable, "LINK", value, INSTR_US_SET);
}

void TRACE_link_srcdst_variable_add_with_time (double time, const char *src, const char *dst, const char *variable, double value)
{
  instr_user_srcdst_variable (time, src, dst, variable, "LINK", value, INSTR_US_ADD);
}

void TRACE_link_srcdst_variable_sub_with_time (double time, const char *src, const char *dst, const char *variable, double value)
{
  instr_user_srcdst_variable (time, src, dst, variable, "LINK", value, INSTR_US_SUB);
}

#endif /* HAVE_TRACING */
