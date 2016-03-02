/* Copyright (c) 2010-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid_config.h"
#include "src/surf/network_interface.hpp"
#include "src/instr/instr_private.h"
#include "surf/surf.h"
#include "src/surf/surf_private.h"

typedef enum {
  INSTR_US_DECLARE,
  INSTR_US_SET,
  INSTR_US_ADD,
  INSTR_US_SUB
} InstrUserVariable;

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_api, instr, "API");

xbt_dict_t created_categories = NULL;
xbt_dict_t declared_marks = NULL;
xbt_dict_t user_host_variables = NULL;
xbt_dict_t user_vm_variables = NULL;
xbt_dict_t user_link_variables = NULL;
extern xbt_dict_t trivaNodeTypes;
extern xbt_dict_t trivaEdgeTypes;

static xbt_dynar_t instr_dict_to_dynar (xbt_dict_t filter)
{
  if (!TRACE_is_enabled()) return NULL;
  if (!TRACE_needs_platform()) return NULL;

  xbt_dynar_t ret = xbt_dynar_new (sizeof(char*), &xbt_free_ref);
  xbt_dict_cursor_t cursor = NULL;
  char *name, *value;
  xbt_dict_foreach(filter, cursor, name, value) {
    xbt_dynar_push_as (ret, char*, xbt_strdup(name));
  }
  return ret;
}

/** \ingroup TRACE_category
 *  \brief Declare a new category with a random color.
 *
 *  This function should be used to define a user category. The category can be used to differentiate the tasks that
 *  are created during the simulation (for example, tasks from server1, server2, or request tasks, computation tasks,
 *  communication tasks). All resource utilization (host power and link bandwidth) will be classified according to the
 *  task category. Tasks that do not belong to a category are not traced. The color for the category that is being
 *  declared is random. This function has no effect if a category with the same name has been already declared.
 *
 * See \ref tracing for details on how to trace the (categorized) resource utilization.
 *
 *  \param category The name of the new tracing category to be created.
 *
 *  \see TRACE_category_with_color, MSG_task_set_category, SD_task_set_category
 */
void TRACE_category(const char *category)
{
  TRACE_category_with_color (category, NULL);
}

/** \ingroup TRACE_category
 *  \brief Declare a new category with a color.
 *
 *  Same as #TRACE_category, but let user specify a color encoded as a RGB-like string with three floats from 0 to 1.
 *  So, to specify a red color, pass "1 0 0" as color parameter. A light-gray color can be specified using "0.7 0.7 0.7"
 *   as color. This function has no effect if a category with the same name has been already declared.
 *
 * See \ref tracing for details on how to trace the (categorized) resource utilization.
 *
 *  \param category The name of the new tracing category to be created.
 *  \param color The color of the category (see \ref tracing to
 *  know how to correctly specify the color)
 *
 *  \see MSG_task_set_category, SD_task_set_category
 */
void TRACE_category_with_color (const char *category, const char *color)
{
  /* safe switch */
  if (!TRACE_is_enabled()) return;

  if (!(TRACE_categorized() && category != NULL)) return;

  /* if platform is not traced, we can't deal with categories */
  if (!TRACE_needs_platform()) return;

  //check if category is already created
  char *created = (char*)xbt_dict_get_or_null(created_categories, category);
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

  XBT_DEBUG("CAT,declare %s, \"%s\" \"%s\"", category, color, final_color);

  //define the type of this category on top of hosts and links
  instr_new_variable_type (category, final_color);
}

/** \ingroup TRACE_category
 *  \brief Get declared categories
 *
 * This function should be used to get categories that were already declared with #TRACE_category or with
 * #TRACE_category_with_color.
 *
 * See \ref tracing for details on how to trace the (categorized) resource utilization.
 *
 * \return A dynar with the declared categories, must be freed with xbt_dynar_free.
 *
 *  \see MSG_task_set_category, SD_task_set_category
 */
xbt_dynar_t TRACE_get_categories (void)
{
  if (!TRACE_is_enabled()) return NULL;
  if (!TRACE_categorized()) return NULL;

  return instr_dict_to_dynar (created_categories);
}

/** \ingroup TRACE_mark
 * \brief Declare a new type for tracing mark.
 *
 * This function declares a new Paje event type in the trace file that can be used by simulators to declare
 * application-level marks. This function is independent of which API is used in SimGrid.
 *
 * \param mark_type The name of the new type.
 *
 * \see TRACE_mark
 */
void TRACE_declare_mark(const char *mark_type)
{
  /* safe switch */
  if (!TRACE_is_enabled()) return;

  /* if platform is not traced, we don't allow marks */
  if (!TRACE_needs_platform()) return;

  if (!mark_type) THROWF (tracing_error, 1, "mark_type is NULL");

  //check if mark_type is already declared
  char *created = (char*)xbt_dict_get_or_null(declared_marks, mark_type);
  if (created) {
    THROWF (tracing_error, 1, "mark_type with name (%s) is already declared", mark_type);
  }

  XBT_DEBUG("MARK,declare %s", mark_type);
  PJ_type_event_new(mark_type, PJ_type_get_root());
  xbt_dict_set (declared_marks, mark_type, xbt_strdup("1"), NULL);
}

/** \ingroup TRACE_mark
 * \brief Declare a new colored value for a previously declared mark type.
 *
 * This function declares a new colored value for a Paje event type in the trace file that can be used by simulators to
 * declare application-level marks. This function is independent of which API is used in SimGrid. The color needs to be
 * a string with three numbers separated by spaces in the range [0,1].
 * A light-gray color can be specified using "0.7 0.7 0.7" as color. If a NULL color is provided, the color used will
 * be white ("1 1 1").
 *
 * \param mark_type The name of the new type.
 * \param mark_value The name of the new value for this type.
 * \param mark_color The color of the new value for this type.
 *
 * \see TRACE_mark
 */
void TRACE_declare_mark_value_with_color (const char *mark_type, const char *mark_value, const char *mark_color)
{
  /* safe switch */
  if (!TRACE_is_enabled()) return;

  /* if platform is not traced, we don't allow marks */
  if (!TRACE_needs_platform()) return;

  if (!mark_type) THROWF (tracing_error, 1, "mark_type is NULL");
  if (!mark_value) THROWF (tracing_error, 1, "mark_value is NULL");

  type_t type = PJ_type_get (mark_type, PJ_type_get_root());
  if (!type){
    THROWF (tracing_error, 1, "mark_type with name (%s) is not declared", mark_type);
  }

  char white[INSTR_DEFAULT_STR_SIZE] = "1.0 1.0 1.0";
  if (!mark_color) mark_color = white;

  XBT_DEBUG("MARK,declare_value %s %s %s", mark_type, mark_value, mark_color);
  PJ_value_new (mark_value, mark_color, type);
}

/** \ingroup TRACE_mark
 * \brief Declare a new value for a previously declared mark type.
 *
 * This function declares a new value for a Paje event type in the trace file that can be used by simulators to declare
 * application-level marks. This function is independent of which API is used in SimGrid. Calling this function is the
 * same as calling \ref TRACE_declare_mark_value_with_color with a NULL color.
 *
 * \param mark_type The name of the new type.
 * \param mark_value The name of the new value for this type.
 *
 * \see TRACE_mark
 */
void TRACE_declare_mark_value (const char *mark_type, const char *mark_value)
{
  TRACE_declare_mark_value_with_color (mark_type, mark_value, NULL);
}

/**
 * \ingroup TRACE_mark
 * \brief Create a new instance of a tracing mark type.
 *
 * This function creates a mark in the trace file. The first parameter had to be previously declared using
 * #TRACE_declare_mark, the second is the identifier for this mark instance. We recommend that the mark_value is a
 * unique value for the whole simulation. Nevertheless, this is not a strong requirement: the trace will be valid even
 * if there are multiple mark identifiers for the same trace.
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

  /* if platform is not traced, we don't allow marks */
  if (!TRACE_needs_platform()) return;

  if (!mark_type) THROWF (tracing_error, 1, "mark_type is NULL");
  if (!mark_value) THROWF (tracing_error, 1, "mark_value is NULL");

  //check if mark_type is already declared
  type_t type = PJ_type_get (mark_type, PJ_type_get_root());
  if (!type){
    THROWF (tracing_error, 1, "mark_type with name (%s) is not declared", mark_type);
  }

  val_t value = PJ_value_get (mark_value, type);
  XBT_DEBUG("MARK %s %s", mark_type, mark_value);
  new_pajeNewEvent (MSG_get_clock(), PJ_container_get_root(), type, value);
}

/** \ingroup TRACE_mark
 *  \brief Get declared marks
 *
 * This function should be used to get marks that were already declared with #TRACE_declare_mark.
 *
 * \return A dynar with the declared marks, must be freed with xbt_dynar_free.
 */
xbt_dynar_t TRACE_get_marks (void)
{
  if (!TRACE_is_enabled()) return NULL;

  return instr_dict_to_dynar (declared_marks);
}

static void instr_user_variable(double time, const char *resource, const char *variable, const char *father_type,
                         double value, InstrUserVariable what, const char *color, xbt_dict_t filter)
{
  /* safe switch */
  if (!TRACE_is_enabled()) return;

  /* if platform is not traced, we don't allow user variables */
  if (!TRACE_needs_platform()) return;

  //check if variable is already declared
  char *created = (char*)xbt_dict_get_or_null(filter, variable);
  if (what == INSTR_US_DECLARE){
    if (created){//already declared
      return;
    }else{
      xbt_dict_set (filter, variable, xbt_strdup("1"), NULL);
    }
  }else{
    if (!created){//not declared, ignore
      return;
    }
  }

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
    }
    break;
  case INSTR_US_ADD:
    {
      container_t container = PJ_container_get(resource);
      type_t type = PJ_type_get (variable, container->type);
      new_pajeAddVariable(time, container, type, value);
    }
    break;
  case INSTR_US_SUB:
    {
      container_t container = PJ_container_get(resource);
      type_t type = PJ_type_get (variable, container->type);
      new_pajeSubVariable(time, container, type, value);
    }
     break;
  default:
    //TODO: launch exception
    break;
  }
}

static void instr_user_srcdst_variable(double time, const char *src, const char *dst, const char *variable,
                              const char *father_type, double value, InstrUserVariable what)
{
  sg_netcard_t src_elm = sg_netcard_by_name_or_null(src);
  if(!src_elm) xbt_die("Element '%s' not found!",src);

  sg_netcard_t dst_elm = sg_netcard_by_name_or_null(dst);
  if(!dst_elm) xbt_die("Element '%s' not found!",dst);

  std::vector<Link*> *route = new std::vector<Link*>();
  routing_platf->getRouteAndLatency (src_elm, dst_elm, route,NULL);
  for (auto link : *route)
    instr_user_variable (time, link->getName(), variable, father_type, value, what, NULL, user_link_variables);
  delete route;
}

/** \ingroup TRACE_API
 *  \brief Creates a file with the topology of the platform file used for the simulator.
 *
 *  The graph topology will have the following properties: all hosts, links and routers of the platform file are mapped
 *  to graph nodes; routes are mapped to edges.
 *  The platform's AS are not represented in the output.
 *
 *  \param filename The name of the file that will hold the graph.
 *
 *  \return 1 of successful, 0 otherwise.
 */
int TRACE_platform_graph_export_graphviz (const char *filename)
{
  /* returns 1 if successful, 0 otherwise */
  if (!TRACE_is_enabled()) return 0;
  xbt_graph_t g = instr_routing_platform_graph();
  if (g == NULL) return 0;
  instr_routing_platform_graph_export_graphviz (g, filename);
  xbt_graph_free_graph(g, xbt_free_f, xbt_free_f, NULL);
  return 1;
}

/*
 * Derived functions that use instr_user_variable and TRACE_user_srcdst_variable. They were previously defined as
 * pre-processors directives, but were transformed into functions so the user can track them using gdb.
 */

/* for VM variables */
/** \ingroup TRACE_user_variables
 *  \brief Declare a new user variable associated to VMs.
 *
 *  Declare a user variable that will be associated to VMs. A user vm variable can be used to trace user variables
 *  such as the number of tasks in a VM, the number of clients in an application (for VMs), and so on. The color
 *  associated to this new variable will be random.
 *
 *  \param variable The name of the new variable to be declared.
 *
 *  \see TRACE_vm_variable_declare_with_color
 */
void TRACE_vm_variable_declare (const char *variable)
{
  instr_user_variable(0, NULL, variable, "MSG_VM", 0, INSTR_US_DECLARE, NULL, user_vm_variables);
}

/** \ingroup TRACE_user_variables
 *  \brief Declare a new user variable associated to VMs with a color.
 *
 *  Same as #TRACE_vm_variable_declare, but associated a color to the newly created user host variable. The color needs
 *  to be a string with three numbers separated by spaces in the range [0,1].
 *  A light-gray color can be specified using "0.7 0.7 0.7" as color.
 *
 *  \param variable The name of the new variable to be declared.
 *  \param color The color for the new variable.
 */
void TRACE_vm_variable_declare_with_color (const char *variable, const char *color)
{
   instr_user_variable(0, NULL, variable, "MSG_VM", 0, INSTR_US_DECLARE, color, user_vm_variables);
}

/** \ingroup TRACE_user_variables
 *  \brief Set the value of a variable of a host.
 *
 *  \param vm The name of the VM to be considered.
 *  \param variable The name of the variable to be considered.
 *  \param value The new value of the variable.
 *
 *  \see TRACE_vm_variable_declare, TRACE_vm_variable_add, TRACE_vm_variable_sub
 */
void TRACE_vm_variable_set (const char *vm, const char *variable, double value)
{
  TRACE_vm_variable_set_with_time (MSG_get_clock(), vm, variable, value);
}

/** \ingroup TRACE_user_variables
 *  \brief Add a value to a variable of a VM.
 *
 *  \param vm The name of the VM to be considered.
 *  \param variable The name of the variable to be considered.
 *  \param value The value to be added to the variable.
 *
 *  \see TRACE_vm_variable_declare, TRACE_vm_variable_set, TRACE_vm_variable_sub
 */
void TRACE_vm_variable_add (const char *vm, const char *variable, double value)
{
  TRACE_vm_variable_add_with_time (MSG_get_clock(), vm, variable, value);
}

/** \ingroup TRACE_user_variables
 *  \brief Subtract a value from a variable of a VM.
 *
 *  \param vm The name of the vm to be considered.
 *  \param variable The name of the variable to be considered.
 *  \param value The value to be subtracted from the variable.
 *
 *  \see TRACE_vm_variable_declare, TRACE_vm_variable_set, TRACE_vm_variable_add
 */
void TRACE_vm_variable_sub (const char *vm, const char *variable, double value)
{
  TRACE_vm_variable_sub_with_time (MSG_get_clock(), vm, variable, value);
}

/** \ingroup TRACE_user_variables
 *  \brief Set the value of a variable of a VM at a given timestamp.
 *
 *  Same as #TRACE_vm_variable_set, but let user specify  the time used to trace it. Users can specify a time that
 *  is not the simulated clock time as defined by the core  simulator. This allows a fine-grain control of time
 *  definition, but should be used with caution since the trace can be inconsistent if resource utilization traces are
 *  also traced.
 *
 *  \param time The timestamp to be used to tag this change of value.
 *  \param vm The name of the VM to be considered.
 *  \param variable The name of the variable to be considered.
 *  \param value The new value of the variable.
 *
 *  \see TRACE_vm_variable_declare, TRACE_vm_variable_add_with_time, TRACE_vm_variable_sub_with_time
 */
void TRACE_vm_variable_set_with_time (double time, const char *vm, const char *variable, double value)
{
  instr_user_variable(time, vm, variable, "MSG_VM", value, INSTR_US_SET, NULL, user_vm_variables);
}

/** \ingroup TRACE_user_variables
 *  \brief Add a value to a variable of a VM at a given timestamp.
 *
 *  Same as #TRACE_vm_variable_add, but let user specify the time used to trace it. Users can specify a time that
 *  is not the simulated clock time as defined by the core simulator. This allows a fine-grain control of time
 *  definition, but should be used with caution since the trace can be inconsistent if resource utilization traces are
 *  also traced.
 *
 *  \param time The timestamp to be used to tag this change of value.
 *  \param vm The name of the VM to be considered.
 *  \param variable The name of the variable to be considered.
 *  \param value The value to be added to the variable.
 *
 *  \see TRACE_vm_variable_declare, TRACE_vm_variable_set_with_time, TRACE_vm_variable_sub_with_time
 */
void TRACE_vm_variable_add_with_time (double time, const char *vm, const char *variable, double value)
{
  instr_user_variable(time, vm, variable, "MSG_VM", value, INSTR_US_ADD, NULL, user_vm_variables);
}

/** \ingroup TRACE_user_variables
 *  \brief Subtract a value from a variable of a VM at a given timestamp.
 *
 *  Same as #TRACE_vm_variable_sub, but let user specify the time used to trace it. Users can specify a time that
 *  is not the simulated clock time as defined by the core  simulator. This allows a fine-grain control of time
 *  definition, but should be used with caution since the trace can be inconsistent if resource utilization traces are
 *  also traced.
 *
 *  \param time The timestamp to be used to tag this change of value.
 *  \param vm The name of the VM to be considered.
 *  \param variable The name of the variable to be considered.
 *  \param value The value to be subtracted from the variable.
 *
 *  \see TRACE_vm_variable_declare, TRACE_vm_variable_set_with_time, TRACE_vm_variable_add_with_time
 */
void TRACE_vm_variable_sub_with_time (double time, const char *vm, const char *variable, double value)
{
  instr_user_variable(time, vm, variable, "MSG_VM", value, INSTR_US_SUB, NULL, user_vm_variables);
}

/** \ingroup TRACE_user_variables
 *  \brief Get declared user vm variables
 *
 * This function should be used to get VM variables that were already declared with #TRACE_vm_variable_declare or with
 * #TRACE_vm_variable_declare_with_color.
 *
 * \return A dynar with the declared host variables, must be freed with xbt_dynar_free.
 */
xbt_dynar_t TRACE_get_vm_variables (void)
{
  return instr_dict_to_dynar (user_vm_variables);
}

/* for host variables */
/** \ingroup TRACE_user_variables
 *  \brief Declare a new user variable associated to hosts.
 *
 *  Declare a user variable that will be associated to hosts.
 *  A user host variable can be used to trace user variables such as the number of tasks in a server, the number of
 *  clients in an application (for hosts), and so on. The color associated to this new variable will be random.
 *
 *  \param variable The name of the new variable to be declared.
 *
 *  \see TRACE_host_variable_declare_with_color
 */
void TRACE_host_variable_declare (const char *variable)
{
  instr_user_variable(0, NULL, variable, "HOST", 0, INSTR_US_DECLARE, NULL, user_host_variables);
}

/** \ingroup TRACE_user_variables
 *  \brief Declare a new user variable associated to hosts with a color.
 *
 *  Same as #TRACE_host_variable_declare, but associated a color to the newly created user host variable. The color
 *  needs to be a string with three numbers separated by spaces in the range [0,1].
 *  A light-gray color can be specified using "0.7 0.7 0.7" as color.
 *
 *  \param variable The name of the new variable to be declared.
 *  \param color The color for the new variable.
 */
void TRACE_host_variable_declare_with_color (const char *variable, const char *color)
{
  instr_user_variable(0, NULL, variable, "HOST", 0, INSTR_US_DECLARE, color, user_host_variables);
}

/** \ingroup TRACE_user_variables
 *  \brief Set the value of a variable of a host.
 *
 *  \param host The name of the host to be considered.
 *  \param variable The name of the variable to be considered.
 *  \param value The new value of the variable.
 *
 *  \see TRACE_host_variable_declare, TRACE_host_variable_add, TRACE_host_variable_sub
 */
void TRACE_host_variable_set (const char *host, const char *variable, double value)
{
  TRACE_host_variable_set_with_time (MSG_get_clock(), host, variable, value);
}

/** \ingroup TRACE_user_variables
 *  \brief Add a value to a variable of a host.
 *
 *  \param host The name of the host to be considered.
 *  \param variable The name of the variable to be considered.
 *  \param value The value to be added to the variable.
 *
 *  \see TRACE_host_variable_declare, TRACE_host_variable_set, TRACE_host_variable_sub
 */
void TRACE_host_variable_add (const char *host, const char *variable, double value)
{
  TRACE_host_variable_add_with_time (MSG_get_clock(), host, variable, value);
}

/** \ingroup TRACE_user_variables
 *  \brief Subtract a value from a variable of a host.
 *
 *  \param host The name of the host to be considered.
 *  \param variable The name of the variable to be considered.
 *  \param value The value to be subtracted from the variable.
 *
 *  \see TRACE_host_variable_declare, TRACE_host_variable_set, TRACE_host_variable_add
 */
void TRACE_host_variable_sub (const char *host, const char *variable, double value)
{
  TRACE_host_variable_sub_with_time (MSG_get_clock(), host, variable, value);
}

/** \ingroup TRACE_user_variables
 *  \brief Set the value of a variable of a host at a given timestamp.
 *
 *  Same as #TRACE_host_variable_set, but let user specify  the time used to trace it. Users can specify a time that
 *  is not the simulated clock time as defined by the core  simulator. This allows a fine-grain control of time
 *  definition, but should be used with caution since the trace  can be inconsistent if resource utilization traces are
 *  also traced.
 *
 *  \param time The timestamp to be used to tag this change of value.
 *  \param host The name of the host to be considered.
 *  \param variable The name of the variable to be considered.
 *  \param value The new value of the variable.
 *
 *  \see TRACE_host_variable_declare, TRACE_host_variable_add_with_time, TRACE_host_variable_sub_with_time
 */
void TRACE_host_variable_set_with_time (double time, const char *host, const char *variable, double value)
{
  instr_user_variable(time, host, variable, "HOST", value, INSTR_US_SET, NULL, user_host_variables);
}

/** \ingroup TRACE_user_variables
 *  \brief Add a value to a variable of a host at a given timestamp.
 *
 *  Same as #TRACE_host_variable_add, but let user specify the time used to trace it. Users can specify a time that
 *  is not the simulated clock time as defined by the core  simulator. This allows a fine-grain control of time
 *  definition, but should be used with caution since the trace can be inconsistent if resource utilization traces are
 *  also traced.
 *
 *  \param time The timestamp to be used to tag this change of value.
 *  \param host The name of the host to be considered.
 *  \param variable The name of the variable to be considered.
 *  \param value The value to be added to the variable.
 *
 *  \see TRACE_host_variable_declare, TRACE_host_variable_set_with_time, TRACE_host_variable_sub_with_time
 */
void TRACE_host_variable_add_with_time (double time, const char *host, const char *variable, double value)
{
  instr_user_variable(time, host, variable, "HOST", value, INSTR_US_ADD, NULL, user_host_variables);
}

/** \ingroup TRACE_user_variables
 *  \brief Subtract a value from a variable of a host at a given timestamp.
 *
 *  Same as #TRACE_host_variable_sub, but let user specify the time used to trace it. Users can specify a time that
 *  is not the simulated clock time as defined by the core  simulator. This allows a fine-grain control of time
 *  definition, but should be used with caution since the trace can be inconsistent if resource utilization traces are
 *  also traced.
 *
 *  \param time The timestamp to be used to tag this change of value.
 *  \param host The name of the host to be considered.
 *  \param variable The name of the variable to be considered.
 *  \param value The value to be subtracted from the variable.
 *
 *  \see TRACE_host_variable_declare, TRACE_host_variable_set_with_time, TRACE_host_variable_add_with_time
 */
void TRACE_host_variable_sub_with_time (double time, const char *host, const char *variable, double value)
{
  instr_user_variable(time, host, variable, "HOST", value, INSTR_US_SUB, NULL, user_host_variables);
}

/** \ingroup TRACE_user_variables
 *  \brief Get declared user host variables
 *
 * This function should be used to get host variables that were already declared with #TRACE_host_variable_declare or
 * with #TRACE_host_variable_declare_with_color.
 *
 * \return A dynar with the declared host variables, must be freed with xbt_dynar_free.
 */
xbt_dynar_t TRACE_get_host_variables (void)
{
  return instr_dict_to_dynar (user_host_variables);
}

/* for link variables */
/** \ingroup TRACE_user_variables
 *  \brief Declare a new user variable associated to links.
 *
 *  Declare a user variable that will be associated to links.
 *  A user link variable can be used, for example, to trace user variables such as the number of messages being
 *  transferred through network links. The color associated to this new variable will be random.
 *
 *  \param variable The name of the new variable to be declared.
 *
 *  \see TRACE_link_variable_declare_with_color
 */
void TRACE_link_variable_declare (const char *variable)
{
  instr_user_variable (0, NULL, variable, "LINK", 0, INSTR_US_DECLARE, NULL, user_link_variables);
}

/** \ingroup TRACE_user_variables
 *  \brief Declare a new user variable associated to links with a color.
 *
 *  Same as #TRACE_link_variable_declare, but associated a color to the newly created user link variable. The color
 *  needs to be a string with three numbers separated by spaces in the range [0,1].
 *  A light-gray color can be specified using "0.7 0.7 0.7" as color.
 *
 *  \param variable The name of the new variable to be declared.
 *  \param color The color for the new variable.
 */
void TRACE_link_variable_declare_with_color (const char *variable, const char *color)
{
  instr_user_variable (0, NULL, variable, "LINK", 0, INSTR_US_DECLARE, color, user_link_variables);
}

/** \ingroup TRACE_user_variables
 *  \brief Set the value of a variable of a link.
 *
 *  \param link The name of the link to be considered.
 *  \param variable The name of the variable to be considered.
 *  \param value The new value of the variable.
 *
 *  \see TRACE_link_variable_declare, TRACE_link_variable_add, TRACE_link_variable_sub
 */
void TRACE_link_variable_set (const char *link, const char *variable, double value)
{
  TRACE_link_variable_set_with_time (MSG_get_clock(), link, variable, value);
}

/** \ingroup TRACE_user_variables
 *  \brief Add a value to a variable of a link.
 *
 *  \param link The name of the link to be considered.
 *  \param variable The name of the variable to be considered.
 *  \param value The value to be added to the variable.
 *
 *  \see TRACE_link_variable_declare, TRACE_link_variable_set, TRACE_link_variable_sub
 */
void TRACE_link_variable_add (const char *link, const char *variable, double value)
{
  TRACE_link_variable_add_with_time (MSG_get_clock(), link, variable, value);
}

/** \ingroup TRACE_user_variables
 *  \brief Subtract a value from a variable of a link.
 *
 *  \param link The name of the link to be considered.
 *  \param variable The name of the variable to be considered.
 *  \param value The value to be subtracted from the variable.
 *
 *  \see TRACE_link_variable_declare, TRACE_link_variable_set, TRACE_link_variable_add
 */
void TRACE_link_variable_sub (const char *link, const char *variable, double value)
{
  TRACE_link_variable_sub_with_time (MSG_get_clock(), link, variable, value);
}

/** \ingroup TRACE_user_variables
 *  \brief Set the value of a variable of a link at a given timestamp.
 *
 *  Same as #TRACE_link_variable_set, but let user specify the time used to trace it. Users can specify a time that
 *  is not the simulated clock time as defined by the core simulator. This allows a fine-grain control of time
 *  definition, but should be used with caution since the trace can be inconsistent if resource utilization traces are
 *  also traced.
 *
 *  \param time The timestamp to be used to tag this change of value.
 *  \param link The name of the link to be considered.
 *  \param variable The name of the variable to be considered.
 *  \param value The new value of the variable.
 *
 *  \see TRACE_link_variable_declare, TRACE_link_variable_add_with_time, TRACE_link_variable_sub_with_time
 */
void TRACE_link_variable_set_with_time (double time, const char *link, const char *variable, double value)
{
  instr_user_variable (time, link, variable, "LINK", value, INSTR_US_SET, NULL, user_link_variables);
}

/** \ingroup TRACE_user_variables
 *  \brief Add a value to a variable of a link at a given timestamp.
 *
 *  Same as #TRACE_link_variable_add, but let user specify the time used to trace it. Users can specify a time that
 *  is not the simulated clock time as defined by the core simulator. This allows a fine-grain control of time
 *  definition, but should be used with caution since the trace can be inconsistent if resource utilization traces are
 *  also traced.
 *
 *  \param time The timestamp to be used to tag this change of value.
 *  \param link The name of the link to be considered.
 *  \param variable The name of the variable to be considered.
 *  \param value The value to be added to the variable.
 *
 *  \see TRACE_link_variable_declare, TRACE_link_variable_set_with_time, TRACE_link_variable_sub_with_time
 */
void TRACE_link_variable_add_with_time (double time, const char *link, const char *variable, double value)
{
  instr_user_variable (time, link, variable, "LINK", value, INSTR_US_ADD, NULL, user_link_variables);
}

/** \ingroup TRACE_user_variables
 *  \brief Subtract a value from a variable of a link at a given timestamp.
 *
 *  Same as #TRACE_link_variable_sub, but let user specify the time used to trace it. Users can specify a time that
 *  is not the simulated clock time as defined by the core simulator. This allows a fine-grain control of time
 *  definition, but should be used with caution since the trace can be inconsistent if resource utilization traces are
 *  also traced.
 *
 *  \param time The timestamp to be used to tag this change of value.
 *  \param link The name of the link to be considered.
 *  \param variable The name of the variable to be considered.
 *  \param value The value to be subtracted from the variable.
 *
 *  \see TRACE_link_variable_declare, TRACE_link_variable_set_with_time, TRACE_link_variable_add_with_time
 */
void TRACE_link_variable_sub_with_time (double time, const char *link, const char *variable, double value)
{
  instr_user_variable (time, link, variable, "LINK", value, INSTR_US_SUB, NULL, user_link_variables);
}

/* for link variables, but with src and dst used for get_route */
/** \ingroup TRACE_user_variables
 *  \brief Set the value of the variable present in the links connecting source and destination.
 *
 *  Same as #TRACE_link_variable_set, but instead of providing the name of link to be considered, provide the source
 *  and destination hosts. All links that are part of the route between source and destination will have the variable
 *  set to the provided value.
 *
 *  \param src The name of the source host for get route.
 *  \param dst The name of the destination host for get route.
 *  \param variable The name of the variable to be considered.
 *  \param value The new value of the variable.
 *
 *  \see TRACE_link_variable_declare, TRACE_link_srcdst_variable_add, TRACE_link_srcdst_variable_sub
 */
void TRACE_link_srcdst_variable_set (const char *src, const char *dst, const char *variable, double value)
{
  TRACE_link_srcdst_variable_set_with_time (MSG_get_clock(), src, dst, variable, value);
}

/** \ingroup TRACE_user_variables
 *  \brief Add a value to the variable present in the links connecting source and destination.
 *
 *  Same as #TRACE_link_variable_add, but instead of providing the name of link to be considered, provide the source
 *  and destination hosts. All links that are part of the route between source and destination will have the value
 *  passed as parameter added to the current value of the variable name to be considered.
 *
 *  \param src The name of the source host for get route.
 *  \param dst The name of the destination host for get route.
 *  \param variable The name of the variable to be considered.
 *  \param value The value to be added to the variable.
 *
 *  \see TRACE_link_variable_declare, TRACE_link_srcdst_variable_set, TRACE_link_srcdst_variable_sub
 */
void TRACE_link_srcdst_variable_add (const char *src, const char *dst, const char *variable, double value)
{
  TRACE_link_srcdst_variable_add_with_time (MSG_get_clock(), src, dst, variable, value);
}

/** \ingroup TRACE_user_variables
 *  \brief Subtract a value from the variable present in the links connecting source and destination.
 *
 *  Same as #TRACE_link_variable_sub, but instead of providing the name of link to be considered, provide the source
 *  and destination hosts. All links that are part of the route between source and destination will have the value
 *  passed as parameter subtracted from the current value of the variable name to be considered.
 *
 *  \param src The name of the source host for get route.
 *  \param dst The name of the destination host for get route.
 *  \param variable The name of the variable to be considered.
 *  \param value The value to be subtracted from the variable.
 *
 *  \see TRACE_link_variable_declare, TRACE_link_srcdst_variable_set, TRACE_link_srcdst_variable_add
 */
void TRACE_link_srcdst_variable_sub (const char *src, const char *dst, const char *variable, double value)
{
  TRACE_link_srcdst_variable_sub_with_time (MSG_get_clock(), src, dst, variable, value);
}

/** \ingroup TRACE_user_variables
 *  \brief Set the value of the variable present in the links connecting source and destination at a given timestamp.
 *
 *  Same as #TRACE_link_srcdst_variable_set, but let user specify the time used to trace it. Users can specify a time
 *  that is not the simulated clock time as defined by the core simulator. This allows a fine-grain control of time
 *  definition, but should be used with caution since the trace can be inconsistent if resource utilization traces are
 *  also traced.
 *
 *  \param time The timestamp to be used to tag this change of value.
 *  \param src The name of the source host for get route.
 *  \param dst The name of the destination host for get route.
 *  \param variable The name of the variable to be considered.
 *  \param value The new value of the variable.
 *
 *  \see TRACE_link_variable_declare, TRACE_link_srcdst_variable_add_with_time, TRACE_link_srcdst_variable_sub_with_time
 */
void TRACE_link_srcdst_variable_set_with_time (double time, const char *src, const char *dst, const char *variable,
                                               double value)
{
  instr_user_srcdst_variable (time, src, dst, variable, "LINK", value, INSTR_US_SET);
}

/** \ingroup TRACE_user_variables
 *  \brief Add a value to the variable present in the links connecting source and destination at a given timestamp.
 *
 *  Same as #TRACE_link_srcdst_variable_add, but let user specify the time used to trace it. Users can specify a time
 *  that is not the simulated clock time as defined by the core simulator. This allows a fine-grain control of time
 *  definition, but should be used with caution since the trace can be inconsistent if resource utilization traces are
 *  also traced.
 *
 *  \param time The timestamp to be used to tag this change of value.
 *  \param src The name of the source host for get route.
 *  \param dst The name of the destination host for get route.
 *  \param variable The name of the variable to be considered.
 *  \param value The value to be added to the variable.
 *
 *  \see TRACE_link_variable_declare, TRACE_link_srcdst_variable_set_with_time, TRACE_link_srcdst_variable_sub_with_time
 */
void TRACE_link_srcdst_variable_add_with_time (double time, const char *src, const char *dst, const char *variable,
                                               double value)
{
  instr_user_srcdst_variable (time, src, dst, variable, "LINK", value, INSTR_US_ADD);
}

/** \ingroup TRACE_user_variables
 *  \brief Subtract a value from the variable present in the links connecting source and dest. at a given timestamp.
 *
 *  Same as #TRACE_link_srcdst_variable_sub, but let user specify the time used to trace it. Users can specify a time
 *  that is not the simulated clock time as defined by the core simulator. This allows a fine-grain control of time
 *  definition, but should be used with caution since the trace can be inconsistent if resource utilization traces are
 *  also traced.
 *
 *  \param time The timestamp to be used to tag this change of value.
 *  \param src The name of the source host for get route.
 *  \param dst The name of the destination host for get route.
 *  \param variable The name of the variable to be considered.
 *  \param value The value to be subtracted from the variable.
 *
 *  \see TRACE_link_variable_declare, TRACE_link_srcdst_variable_set_with_time, TRACE_link_srcdst_variable_add_with_time
 */
void TRACE_link_srcdst_variable_sub_with_time (double time, const char *src, const char *dst, const char *variable,
                                               double value)
{
  instr_user_srcdst_variable (time, src, dst, variable, "LINK", value, INSTR_US_SUB);
}

/** \ingroup TRACE_user_variables
 *  \brief Get declared user link variables
 *
 * This function should be used to get link variables that were already declared with #TRACE_link_variable_declare or
 * with #TRACE_link_variable_declare_with_color.
 *
 * \return A dynar with the declared link variables, must be freed with xbt_dynar_free.
 */
xbt_dynar_t TRACE_get_link_variables (void)
{
  return instr_dict_to_dynar (user_link_variables);
}

/** \ingroup TRACE_user_variables
 *  \brief Declare a new user state associated to hosts.
 *
 *  Declare a user state that will be associated to hosts.
 *  A user host state can be used to trace application states.
 *
 *  \param state The name of the new state to be declared.
 *
 *  \see TRACE_host_state_declare_value
 */
void TRACE_host_state_declare (const char *state)
{
  instr_new_user_state_type("HOST", state);
}

/** \ingroup TRACE_user_variables
 *  \brief Declare a new value for a user state associated to hosts.
 *
 *  Declare a value for a state. The color needs to be a string with 3 numbers separated by spaces in the range [0,1].
 *  A light-gray color can be specified using "0.7 0.7 0.7" as color.
 *
 *  \param state The name of the new state to be declared.
 *  \param value The name of the value
 *  \param color The color of the value
 *
 *  \see TRACE_host_state_declare
 */
void TRACE_host_state_declare_value (const char *state, const char *value, const char *color)
{
  instr_new_value_for_user_state_type (state, value, color);
}

/** \ingroup TRACE_user_variables
 *  \brief Set the user state to the given value.
 *
 *  Change a user state previously declared to the given value.
 *
 *  \param host The name of the host to be considered.
 *  \param state The name of the state previously declared.
 *  \param value The new value of the state.
 *
 *  \see TRACE_host_state_declare, TRACE_host_push_state, TRACE_host_pop_state, TRACE_host_reset_state
 */
void TRACE_host_set_state (const char *host, const char *state, const char *value)
{
  container_t container = PJ_container_get(host);
  type_t type = PJ_type_get (state, container->type);
  val_t val = PJ_value_get_or_new (value, NULL, type); /* if user didn't declare a value with a color, use NULL color */
  new_pajeSetState(MSG_get_clock(), container, type, val);
}

/** \ingroup TRACE_user_variables
 *  \brief Push a new value for a state of a given host.
 *
 *  Change a user state previously declared by pushing the new value to the state.
 *
 *  \param host The name of the host to be considered.
 *  \param state The name of the state previously declared.
 *  \param value The value to be pushed.
 *
 *  \see TRACE_host_state_declare, TRACE_host_set_state, TRACE_host_pop_state, TRACE_host_reset_state
 */
void TRACE_host_push_state (const char *host, const char *state, const char *value)
{
  container_t container = PJ_container_get(host);
  type_t type = PJ_type_get (state, container->type);
  val_t val = PJ_value_get_or_new (value, NULL, type); /* if user didn't declare a value with a color, use NULL color */
  new_pajePushState(MSG_get_clock(), container, type, val);
}

/** \ingroup TRACE_user_variables
 *  \brief Pop the last value of a state of a given host.
 *
 *  Change a user state previously declared by removing the last value of the state.
 *
 *  \param host The name of the host to be considered.
 *  \param state The name of the state to be popped.
 *
 *  \see TRACE_host_state_declare, TRACE_host_set_state, TRACE_host_push_state, TRACE_host_reset_state
 */
void TRACE_host_pop_state (const char *host, const char *state)
{
  container_t container = PJ_container_get(host);
  type_t type = PJ_type_get (state, container->type);
  new_pajePopState(MSG_get_clock(), container, type);
}

/** \ingroup TRACE_user_variables
 *  \brief Reset the state of a given host.
 *
 *  Clear all previous values of a user state.
 *
 *  \param host The name of the host to be considered.
 *  \param state The name of the state to be cleared.
 *
 *  \see TRACE_host_state_declare, TRACE_host_set_state, TRACE_host_push_state, TRACE_host_pop_state
 */
void TRACE_host_reset_state (const char *host, const char *state)
{
  container_t container = PJ_container_get(host);
  type_t type = PJ_type_get (state, container->type);
  new_pajeResetState(MSG_get_clock(), container, type);
}

/** \ingroup TRACE_API
 *  \brief Get Paje container types that can be mapped to the nodes of a graph.
 *
 *  This function can be used to create a user made  graph configuration file for Triva. Normally, it is used with the
 *  functions defined in \ref TRACE_user_variables.
 *
 *  \return A dynar with the types, must be freed with xbt_dynar_free.
 */
xbt_dynar_t TRACE_get_node_types (void)
{
  return instr_dict_to_dynar (trivaNodeTypes);
}

/** \ingroup TRACE_API
 *  \brief Get Paje container types that can be mapped to the edges of a graph.
 *
 *  This function can be used to create a user made graph configuration file for Triva. Normally, it is used with the
 *  functions defined in \ref TRACE_user_variables.
 *
 *  \return A dynar with the types, must be freed with xbt_dynar_free.
 */
xbt_dynar_t TRACE_get_edge_types (void)
{
  return instr_dict_to_dynar (trivaEdgeTypes);
}

/** \ingroup TRACE_API
 *  \brief Pauses all tracing activities.
 *  \see TRACE_resume
 */
void TRACE_pause (void)
{
  instr_pause_tracing();
}

/** \ingroup TRACE_API
 *  \brief Resumes all tracing activities.
 *  \see TRACE_pause
 */
void TRACE_resume (void)
{
  instr_resume_tracing();
}
