/* Copyright (c) 2010-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/Exception.hpp"
#include "simgrid/kernel/routing/NetPoint.hpp"
#include "src/instr/instr_private.hpp"
#include "src/surf/network_interface.hpp"
#include "src/surf/surf_private.hpp"
#include "surf/surf.hpp"
#include "xbt/random.hpp"
#include <algorithm>
#include <cmath>

enum class InstrUserVariable { DECLARE, SET, ADD, SUB };

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_api, instr, "API");

std::set<std::string, std::less<>> created_categories;
std::set<std::string, std::less<>> declared_marks;
std::set<std::string, std::less<>> user_host_variables;
std::set<std::string, std::less<>> user_vm_variables;
std::set<std::string, std::less<>> user_link_variables;

static xbt_dynar_t instr_set_to_dynar(const std::set<std::string, std::less<>>& filter)
{
  if (not TRACE_is_enabled() || not TRACE_needs_platform())
    return nullptr;

  xbt_dynar_t ret = xbt_dynar_new (sizeof(char*), &xbt_free_ref);
  for (auto const& name : filter)
    xbt_dynar_push_as(ret, char*, xbt_strdup(name.c_str()));

  return ret;
}

/** @ingroup TRACE_category
 *  @brief Declare a new category with a random color.
 *
 *  This function should be used to define a user category. The category can be used to differentiate the tasks that
 *  are created during the simulation (for example, tasks from server1, server2, or request tasks, computation tasks,
 *  communication tasks). All resource utilization (host power and link bandwidth) will be classified according to the
 *  task category. Tasks that do not belong to a category are not traced. The color for the category that is being
 *  declared is random. This function has no effect if a category with the same name has been already declared.
 *
 * See @ref outcomes_vizu for details on how to trace the (categorized) resource utilization.
 *
 *  @param category The name of the new tracing category to be created.
 *
 *  @see TRACE_category_with_color, MSG_task_set_category, SD_task_set_category
 */
void TRACE_category(const char *category)
{
  TRACE_category_with_color (category, nullptr);
}

/** @ingroup TRACE_category
 *  @brief Declare a new category with a color.
 *
 *  Same as #TRACE_category, but let user specify a color encoded as an RGB-like string with three floats from 0 to 1.
 *  So, to specify a red color, pass "1 0 0" as color parameter. A light-gray color can be specified using "0.7 0.7 0.7"
 *   as color. This function has no effect if a category with the same name has been already declared.
 *
 * See @ref outcomes_vizu for details on how to trace the (categorized) resource utilization.
 *
 *  @param category The name of the new tracing category to be created.
 *  @param color The color of the category (see @ref outcomes_vizu to
 *  know how to correctly specify the color)
 *
 *  @see MSG_task_set_category, SD_task_set_category
 */
void TRACE_category_with_color (const char *category, const char *color)
{
  /* safe switches. tracing has to be activated and if platform is not traced, we can't deal with categories */
  if (not TRACE_is_enabled() || not TRACE_needs_platform())
    return;

  if (not(TRACE_categorized() && category != nullptr))
    return;

  //check if category is already created
  if (created_categories.find(category) != created_categories.end())
    return;

  created_categories.emplace(category);

  //define final_color
  std::string final_color;
  if (not color) {
    //generate a random color
    double red   = simgrid::xbt::random::uniform_real(0.0, std::nextafter(1.0, 2.0));
    double green = simgrid::xbt::random::uniform_real(0.0, std::nextafter(1.0, 2.0));
    double blue  = simgrid::xbt::random::uniform_real(0.0, std::nextafter(1.0, 2.0));
    final_color  = std::to_string(red) + " " + std::to_string(green) + " " + std::to_string(blue);
  }else{
    final_color = std::string(color);
  }

  XBT_DEBUG("CAT,declare %s, \"%s\" \"%s\"", category, color, final_color.c_str());

  //define the type of this category on top of hosts and links
  instr_new_variable_type (category, final_color);
}

/** @ingroup TRACE_category
 *  @brief Get declared categories
 *
 * This function should be used to get categories that were already declared with #TRACE_category or with
 * #TRACE_category_with_color.
 *
 * See @ref outcomes_vizu for details on how to trace the (categorized) resource utilization.
 *
 * @return A dynar with the declared categories, must be freed with xbt_dynar_free.
 *
 *  @see MSG_task_set_category, SD_task_set_category
 */
xbt_dynar_t TRACE_get_categories ()
{
  if (not TRACE_is_enabled() || not TRACE_categorized())
    return nullptr;
  return instr_set_to_dynar(created_categories);
}

/** @ingroup TRACE_mark
 * @brief Declare a new type for tracing mark.
 *
 * This function declares a new Paje event type in the trace file that can be used by simulators to declare
 * application-level marks. This function is independent of which API is used in SimGrid.
 *
 * @param mark_type The name of the new type.
 *
 * @see TRACE_mark
 */
void TRACE_declare_mark(const char *mark_type)
{
  /* safe switches. tracing has to be activated and if platform is not traced, we can't deal with marks */
  if (not TRACE_is_enabled() || not TRACE_needs_platform())
    return;

  xbt_assert(mark_type, "mark_type is nullptr");

  //check if mark_type is already declared
  if (declared_marks.find(mark_type) != declared_marks.end()) {
    throw simgrid::TracingError(XBT_THROW_POINT,
                                simgrid::xbt::string_printf("mark_type with name (%s) is already declared", mark_type));
  }

  XBT_DEBUG("MARK,declare %s", mark_type);
  simgrid::instr::Container::get_root()->type_->by_name_or_create<simgrid::instr::EventType>(mark_type);
  declared_marks.emplace(mark_type);
}

/** @ingroup TRACE_mark
 * @brief Declare a new colored value for a previously declared mark type.
 *
 * This function declares a new colored value for a Paje event type in the trace file that can be used by simulators to
 * declare application-level marks. This function is independent of which API is used in SimGrid. The color needs to be
 * a string with three numbers separated by spaces in the range [0,1].
 * A light-gray color can be specified using "0.7 0.7 0.7" as color. If a nullptr color is provided, the color used will
 * be white ("1 1 1").
 *
 * @param mark_type The name of the new type.
 * @param mark_value The name of the new value for this type.
 * @param mark_color The color of the new value for this type.
 *
 * @see TRACE_mark
 */
void TRACE_declare_mark_value_with_color (const char *mark_type, const char *mark_value, const char *mark_color)
{
  /* safe switches. tracing has to be activated and if platform is not traced, we can't deal with marks */
  if (not TRACE_is_enabled() || not TRACE_needs_platform())
    return;

  xbt_assert(mark_type, "mark_type is nullptr");
  xbt_assert(mark_value, "mark_value is nullptr");

  auto* type =
      static_cast<simgrid::instr::EventType*>(simgrid::instr::Container::get_root()->type_->by_name(mark_type));
  if (not type) {
    throw simgrid::TracingError(XBT_THROW_POINT,
                                simgrid::xbt::string_printf("mark_type with name (%s) is not declared", mark_type));
  } else {
    if (not mark_color)
      mark_color = "1.0 1.0 1.0" /*white*/;

    XBT_DEBUG("MARK,declare_value %s %s %s", mark_type, mark_value, mark_color);
    type->add_entity_value(mark_value, mark_color);
  }
}

/** @ingroup TRACE_mark
 * @brief Declare a new value for a previously declared mark type.
 *
 * This function declares a new value for a Paje event type in the trace file that can be used by simulators to declare
 * application-level marks. This function is independent of which API is used in SimGrid. Calling this function is the
 * same as calling @ref TRACE_declare_mark_value_with_color with a nullptr color.
 *
 * @param mark_type The name of the new type.
 * @param mark_value The name of the new value for this type.
 *
 * @see TRACE_mark
 */
void TRACE_declare_mark_value (const char *mark_type, const char *mark_value)
{
  TRACE_declare_mark_value_with_color (mark_type, mark_value, nullptr);
}

/**
 * @ingroup TRACE_mark
 * @brief Create a new instance of a tracing mark type.
 *
 * This function creates a mark in the trace file. The first parameter had to be previously declared using
 * #TRACE_declare_mark, the second is the identifier for this mark instance. We recommend that the mark_value is a
 * unique value for the whole simulation. Nevertheless, this is not a strong requirement: the trace will be valid even
 * if there are multiple mark identifiers for the same trace.
 *
 * @param mark_type The name of the type for which the new instance will belong.
 * @param mark_value The name of the new instance mark.
 *
 * @see TRACE_declare_mark
 */
void TRACE_mark(const char *mark_type, const char *mark_value)
{
  /* safe switches. tracing has to be activated and if platform is not traced, we can't deal with marks */
  if (not TRACE_is_enabled() || not TRACE_needs_platform())
    return;

  xbt_assert(mark_type, "mark_type is nullptr");
  xbt_assert(mark_value, "mark_value is nullptr");

  //check if mark_type is already declared
  auto* type =
      static_cast<simgrid::instr::EventType*>(simgrid::instr::Container::get_root()->type_->by_name(mark_type));
  if (not type) {
    throw simgrid::TracingError(XBT_THROW_POINT,
                                simgrid::xbt::string_printf("mark_type with name (%s) is not declared", mark_type));
  } else {
    XBT_DEBUG("MARK %s %s", mark_type, mark_value);
    new simgrid::instr::NewEvent(simgrid_get_clock(), simgrid::instr::Container::get_root(), type,
                                 type->get_entity_value(mark_value));
  }
}

/** @ingroup TRACE_mark
 *  @brief Get declared marks
 *
 * This function should be used to get marks that were already declared with #TRACE_declare_mark.
 *
 * @return A dynar with the declared marks, must be freed with xbt_dynar_free.
 */
xbt_dynar_t TRACE_get_marks ()
{
  if (not TRACE_is_enabled())
    return nullptr;

  return instr_set_to_dynar(declared_marks);
}

static void instr_user_variable(double time, const char* resource, const char* variable_name, const char* parent_type,
                                double value, InstrUserVariable what, const char* color,
                                std::set<std::string, std::less<>>* filter)
{
  /* safe switches. tracing has to be activated and if platform is not traced, we don't allow user variables */
  if (not TRACE_is_enabled() || not TRACE_needs_platform())
    return;

  //check if variable is already declared
  auto created = filter->find(variable_name);
  if (what == InstrUserVariable::DECLARE) {
    if (created == filter->end()) { // not declared yet
      filter->insert(variable_name);
      instr_new_user_variable_type(parent_type, variable_name, color == nullptr ? "" : color);
    }
  }else{
    if (created != filter->end()) { // declared, let's work
      simgrid::instr::VariableType* variable =
          simgrid::instr::Container::by_name(resource)->get_variable(variable_name);
      switch (what){
        case InstrUserVariable::SET:
          variable->set_event(time, value);
          break;
        case InstrUserVariable::ADD:
          variable->add_event(time, value);
          break;
        case InstrUserVariable::SUB:
          variable->sub_event(time, value);
          break;
        default:
          THROW_IMPOSSIBLE;
      }
    }
  }
}

static void instr_user_srcdst_variable(double time, const char* src, const char* dst, const char* variable,
                                       const char* parent_type, double value, InstrUserVariable what)
{
  const simgrid::kernel::routing::NetPoint* src_elm = sg_netpoint_by_name_or_null(src);
  xbt_assert(src_elm, "Element '%s' not found!", src);

  const simgrid::kernel::routing::NetPoint* dst_elm = sg_netpoint_by_name_or_null(dst);
  xbt_assert(dst_elm, "Element '%s' not found!", dst);

  std::vector<simgrid::kernel::resource::LinkImpl*> route;
  simgrid::kernel::routing::NetZoneImpl::get_global_route(src_elm, dst_elm, route, nullptr);
  for (auto const& link : route)
    instr_user_variable(time, link->get_cname(), variable, parent_type, value, what, nullptr, &user_link_variables);
}

/** @ingroup TRACE_API
 *  @brief Creates a file with the topology of the platform file used for the simulator.
 *
 *  The graph topology will have the following properties: all hosts, links and routers of the platform file are mapped
 *  to graph nodes; routes are mapped to edges.
 *  The platform's zones are not represented in the output.
 *
 *  @param filename The name of the file that will hold the graph.
 *
 *  @return 1 of successful, 0 otherwise.
 */
int TRACE_platform_graph_export_graphviz (const char *filename)
{
  simgrid::instr::platform_graph_export_graphviz(filename);
  return 1;
}

/*
 * Derived functions that use instr_user_variable and TRACE_user_srcdst_variable. They were previously defined as
 * pre-processors directives, but were transformed into functions so the user can track them using gdb.
 */

/* for VM variables */
/** @ingroup TRACE_user_variables
 *  @brief Declare a new user variable associated to VMs.
 *
 *  Declare a user variable that will be associated to VMs. A user vm variable can be used to trace user variables
 *  such as the number of tasks in a VM, the number of clients in an application (for VMs), and so on. The color
 *  associated to this new variable will be random.
 *
 *  @param variable The name of the new variable to be declared.
 *
 *  @see TRACE_vm_variable_declare_with_color
 */
void TRACE_vm_variable_declare (const char *variable)
{
  instr_user_variable(0, nullptr, variable, "VM", 0, InstrUserVariable::DECLARE, nullptr, &user_vm_variables);
}

/** @ingroup TRACE_user_variables
 *  @brief Declare a new user variable associated to VMs with a color.
 *
 *  Same as #TRACE_vm_variable_declare, but associated a color to the newly created user host variable. The color needs
 *  to be a string with three numbers separated by spaces in the range [0,1].
 *  A light-gray color can be specified using "0.7 0.7 0.7" as color.
 *
 *  @param variable The name of the new variable to be declared.
 *  @param color The color for the new variable.
 */
void TRACE_vm_variable_declare_with_color (const char *variable, const char *color)
{
  instr_user_variable(0, nullptr, variable, "VM", 0, InstrUserVariable::DECLARE, color, &user_vm_variables);
}

/** @ingroup TRACE_user_variables
 *  @brief Set the value of a variable of a host.
 *
 *  @param vm The name of the VM to be considered.
 *  @param variable The name of the variable to be considered.
 *  @param value The new value of the variable.
 *
 *  @see TRACE_vm_variable_declare, TRACE_vm_variable_add, TRACE_vm_variable_sub
 */
void TRACE_vm_variable_set (const char *vm, const char *variable, double value)
{
  TRACE_vm_variable_set_with_time(simgrid_get_clock(), vm, variable, value);
}

/** @ingroup TRACE_user_variables
 *  @brief Add a value to a variable of a VM.
 *
 *  @param vm The name of the VM to be considered.
 *  @param variable The name of the variable to be considered.
 *  @param value The value to be added to the variable.
 *
 *  @see TRACE_vm_variable_declare, TRACE_vm_variable_set, TRACE_vm_variable_sub
 */
void TRACE_vm_variable_add (const char *vm, const char *variable, double value)
{
  TRACE_vm_variable_add_with_time(simgrid_get_clock(), vm, variable, value);
}

/** @ingroup TRACE_user_variables
 *  @brief Subtract a value from a variable of a VM.
 *
 *  @param vm The name of the vm to be considered.
 *  @param variable The name of the variable to be considered.
 *  @param value The value to be subtracted from the variable.
 *
 *  @see TRACE_vm_variable_declare, TRACE_vm_variable_set, TRACE_vm_variable_add
 */
void TRACE_vm_variable_sub (const char *vm, const char *variable, double value)
{
  TRACE_vm_variable_sub_with_time(simgrid_get_clock(), vm, variable, value);
}

/** @ingroup TRACE_user_variables
 *  @brief Set the value of a variable of a VM at a given timestamp.
 *
 *  Same as #TRACE_vm_variable_set, but let user specify  the time used to trace it. Users can specify a time that
 *  is not the simulated clock time as defined by the core  simulator. This allows a fine-grain control of time
 *  definition, but should be used with caution since the trace can be inconsistent if resource utilization traces are
 *  also traced.
 *
 *  @param time The timestamp to be used to tag this change of value.
 *  @param vm The name of the VM to be considered.
 *  @param variable The name of the variable to be considered.
 *  @param value The new value of the variable.
 *
 *  @see TRACE_vm_variable_declare, TRACE_vm_variable_add_with_time, TRACE_vm_variable_sub_with_time
 */
void TRACE_vm_variable_set_with_time (double time, const char *vm, const char *variable, double value)
{
  instr_user_variable(time, vm, variable, "VM", value, InstrUserVariable::SET, nullptr, &user_vm_variables);
}

/** @ingroup TRACE_user_variables
 *  @brief Add a value to a variable of a VM at a given timestamp.
 *
 *  Same as #TRACE_vm_variable_add, but let user specify the time used to trace it. Users can specify a time that
 *  is not the simulated clock time as defined by the core simulator. This allows a fine-grain control of time
 *  definition, but should be used with caution since the trace can be inconsistent if resource utilization traces are
 *  also traced.
 *
 *  @param time The timestamp to be used to tag this change of value.
 *  @param vm The name of the VM to be considered.
 *  @param variable The name of the variable to be considered.
 *  @param value The value to be added to the variable.
 *
 *  @see TRACE_vm_variable_declare, TRACE_vm_variable_set_with_time, TRACE_vm_variable_sub_with_time
 */
void TRACE_vm_variable_add_with_time (double time, const char *vm, const char *variable, double value)
{
  instr_user_variable(time, vm, variable, "VM", value, InstrUserVariable::ADD, nullptr, &user_vm_variables);
}

/** @ingroup TRACE_user_variables
 *  @brief Subtract a value from a variable of a VM at a given timestamp.
 *
 *  Same as #TRACE_vm_variable_sub, but let user specify the time used to trace it. Users can specify a time that
 *  is not the simulated clock time as defined by the core  simulator. This allows a fine-grain control of time
 *  definition, but should be used with caution since the trace can be inconsistent if resource utilization traces are
 *  also traced.
 *
 *  @param time The timestamp to be used to tag this change of value.
 *  @param vm The name of the VM to be considered.
 *  @param variable The name of the variable to be considered.
 *  @param value The value to be subtracted from the variable.
 *
 *  @see TRACE_vm_variable_declare, TRACE_vm_variable_set_with_time, TRACE_vm_variable_add_with_time
 */
void TRACE_vm_variable_sub_with_time (double time, const char *vm, const char *variable, double value)
{
  instr_user_variable(time, vm, variable, "VM", value, InstrUserVariable::SUB, nullptr, &user_vm_variables);
}

/* for host variables */
/** @ingroup TRACE_user_variables
 *  @brief Declare a new user variable associated to hosts.
 *
 *  Declare a user variable that will be associated to hosts.
 *  A user host variable can be used to trace user variables such as the number of tasks in a server, the number of
 *  clients in an application (for hosts), and so on. The color associated to this new variable will be random.
 *
 *  @param variable The name of the new variable to be declared.
 *
 *  @see TRACE_host_variable_declare_with_color
 */
void TRACE_host_variable_declare (const char *variable)
{
  instr_user_variable(0, nullptr, variable, "HOST", 0, InstrUserVariable::DECLARE, nullptr, &user_host_variables);
}

/** @ingroup TRACE_user_variables
 *  @brief Declare a new user variable associated to hosts with a color.
 *
 *  Same as #TRACE_host_variable_declare, but associated a color to the newly created user host variable. The color
 *  needs to be a string with three numbers separated by spaces in the range [0,1].
 *  A light-gray color can be specified using "0.7 0.7 0.7" as color.
 *
 *  @param variable The name of the new variable to be declared.
 *  @param color The color for the new variable.
 */
void TRACE_host_variable_declare_with_color (const char *variable, const char *color)
{
  instr_user_variable(0, nullptr, variable, "HOST", 0, InstrUserVariable::DECLARE, color, &user_host_variables);
}

/** @ingroup TRACE_user_variables
 *  @brief Set the value of a variable of a host.
 *
 *  @param host The name of the host to be considered.
 *  @param variable The name of the variable to be considered.
 *  @param value The new value of the variable.
 *
 *  @see TRACE_host_variable_declare, TRACE_host_variable_add, TRACE_host_variable_sub
 */
void TRACE_host_variable_set (const char *host, const char *variable, double value)
{
  TRACE_host_variable_set_with_time(simgrid_get_clock(), host, variable, value);
}

/** @ingroup TRACE_user_variables
 *  @brief Add a value to a variable of a host.
 *
 *  @param host The name of the host to be considered.
 *  @param variable The name of the variable to be considered.
 *  @param value The value to be added to the variable.
 *
 *  @see TRACE_host_variable_declare, TRACE_host_variable_set, TRACE_host_variable_sub
 */
void TRACE_host_variable_add (const char *host, const char *variable, double value)
{
  TRACE_host_variable_add_with_time(simgrid_get_clock(), host, variable, value);
}

/** @ingroup TRACE_user_variables
 *  @brief Subtract a value from a variable of a host.
 *
 *  @param host The name of the host to be considered.
 *  @param variable The name of the variable to be considered.
 *  @param value The value to be subtracted from the variable.
 *
 *  @see TRACE_host_variable_declare, TRACE_host_variable_set, TRACE_host_variable_add
 */
void TRACE_host_variable_sub (const char *host, const char *variable, double value)
{
  TRACE_host_variable_sub_with_time(simgrid_get_clock(), host, variable, value);
}

/** @ingroup TRACE_user_variables
 *  @brief Set the value of a variable of a host at a given timestamp.
 *
 *  Same as #TRACE_host_variable_set, but let user specify  the time used to trace it. Users can specify a time that
 *  is not the simulated clock time as defined by the core  simulator. This allows a fine-grain control of time
 *  definition, but should be used with caution since the trace  can be inconsistent if resource utilization traces are
 *  also traced.
 *
 *  @param time The timestamp to be used to tag this change of value.
 *  @param host The name of the host to be considered.
 *  @param variable The name of the variable to be considered.
 *  @param value The new value of the variable.
 *
 *  @see TRACE_host_variable_declare, TRACE_host_variable_add_with_time, TRACE_host_variable_sub_with_time
 */
void TRACE_host_variable_set_with_time (double time, const char *host, const char *variable, double value)
{
  instr_user_variable(time, host, variable, "HOST", value, InstrUserVariable::SET, nullptr, &user_host_variables);
}

/** @ingroup TRACE_user_variables
 *  @brief Add a value to a variable of a host at a given timestamp.
 *
 *  Same as #TRACE_host_variable_add, but let user specify the time used to trace it. Users can specify a time that
 *  is not the simulated clock time as defined by the core  simulator. This allows a fine-grain control of time
 *  definition, but should be used with caution since the trace can be inconsistent if resource utilization traces are
 *  also traced.
 *
 *  @param time The timestamp to be used to tag this change of value.
 *  @param host The name of the host to be considered.
 *  @param variable The name of the variable to be considered.
 *  @param value The value to be added to the variable.
 *
 *  @see TRACE_host_variable_declare, TRACE_host_variable_set_with_time, TRACE_host_variable_sub_with_time
 */
void TRACE_host_variable_add_with_time (double time, const char *host, const char *variable, double value)
{
  instr_user_variable(time, host, variable, "HOST", value, InstrUserVariable::ADD, nullptr, &user_host_variables);
}

/** @ingroup TRACE_user_variables
 *  @brief Subtract a value from a variable of a host at a given timestamp.
 *
 *  Same as #TRACE_host_variable_sub, but let user specify the time used to trace it. Users can specify a time that
 *  is not the simulated clock time as defined by the core  simulator. This allows a fine-grain control of time
 *  definition, but should be used with caution since the trace can be inconsistent if resource utilization traces are
 *  also traced.
 *
 *  @param time The timestamp to be used to tag this change of value.
 *  @param host The name of the host to be considered.
 *  @param variable The name of the variable to be considered.
 *  @param value The value to be subtracted from the variable.
 *
 *  @see TRACE_host_variable_declare, TRACE_host_variable_set_with_time, TRACE_host_variable_add_with_time
 */
void TRACE_host_variable_sub_with_time (double time, const char *host, const char *variable, double value)
{
  instr_user_variable(time, host, variable, "HOST", value, InstrUserVariable::SUB, nullptr, &user_host_variables);
}

/** @ingroup TRACE_user_variables
 *  @brief Get declared user host variables
 *
 * This function should be used to get host variables that were already declared with #TRACE_host_variable_declare or
 * with #TRACE_host_variable_declare_with_color.
 *
 * @return A dynar with the declared host variables, must be freed with xbt_dynar_free.
 */
xbt_dynar_t TRACE_get_host_variables ()
{
  return instr_set_to_dynar(user_host_variables);
}

/* for link variables */
/** @ingroup TRACE_user_variables
 *  @brief Declare a new user variable associated to links.
 *
 *  Declare a user variable that will be associated to links.
 *  A user link variable can be used, for example, to trace user variables such as the number of messages being
 *  transferred through network links. The color associated to this new variable will be random.
 *
 *  @param variable The name of the new variable to be declared.
 *
 *  @see TRACE_link_variable_declare_with_color
 */
void TRACE_link_variable_declare (const char *variable)
{
  instr_user_variable(0, nullptr, variable, "LINK", 0, InstrUserVariable::DECLARE, nullptr, &user_link_variables);
}

/** @ingroup TRACE_user_variables
 *  @brief Declare a new user variable associated to links with a color.
 *
 *  Same as #TRACE_link_variable_declare, but associated a color to the newly created user link variable. The color
 *  needs to be a string with three numbers separated by spaces in the range [0,1].
 *  A light-gray color can be specified using "0.7 0.7 0.7" as color.
 *
 *  @param variable The name of the new variable to be declared.
 *  @param color The color for the new variable.
 */
void TRACE_link_variable_declare_with_color (const char *variable, const char *color)
{
  instr_user_variable(0, nullptr, variable, "LINK", 0, InstrUserVariable::DECLARE, color, &user_link_variables);
}

/** @ingroup TRACE_user_variables
 *  @brief Set the value of a variable of a link.
 *
 *  @param link The name of the link to be considered.
 *  @param variable The name of the variable to be considered.
 *  @param value The new value of the variable.
 *
 *  @see TRACE_link_variable_declare, TRACE_link_variable_add, TRACE_link_variable_sub
 */
void TRACE_link_variable_set (const char *link, const char *variable, double value)
{
  TRACE_link_variable_set_with_time(simgrid_get_clock(), link, variable, value);
}

/** @ingroup TRACE_user_variables
 *  @brief Add a value to a variable of a link.
 *
 *  @param link The name of the link to be considered.
 *  @param variable The name of the variable to be considered.
 *  @param value The value to be added to the variable.
 *
 *  @see TRACE_link_variable_declare, TRACE_link_variable_set, TRACE_link_variable_sub
 */
void TRACE_link_variable_add (const char *link, const char *variable, double value)
{
  TRACE_link_variable_add_with_time(simgrid_get_clock(), link, variable, value);
}

/** @ingroup TRACE_user_variables
 *  @brief Subtract a value from a variable of a link.
 *
 *  @param link The name of the link to be considered.
 *  @param variable The name of the variable to be considered.
 *  @param value The value to be subtracted from the variable.
 *
 *  @see TRACE_link_variable_declare, TRACE_link_variable_set, TRACE_link_variable_add
 */
void TRACE_link_variable_sub (const char *link, const char *variable, double value)
{
  TRACE_link_variable_sub_with_time(simgrid_get_clock(), link, variable, value);
}

/** @ingroup TRACE_user_variables
 *  @brief Set the value of a variable of a link at a given timestamp.
 *
 *  Same as #TRACE_link_variable_set, but let user specify the time used to trace it. Users can specify a time that
 *  is not the simulated clock time as defined by the core simulator. This allows a fine-grain control of time
 *  definition, but should be used with caution since the trace can be inconsistent if resource utilization traces are
 *  also traced.
 *
 *  @param time The timestamp to be used to tag this change of value.
 *  @param link The name of the link to be considered.
 *  @param variable The name of the variable to be considered.
 *  @param value The new value of the variable.
 *
 *  @see TRACE_link_variable_declare, TRACE_link_variable_add_with_time, TRACE_link_variable_sub_with_time
 */
void TRACE_link_variable_set_with_time (double time, const char *link, const char *variable, double value)
{
  instr_user_variable(time, link, variable, "LINK", value, InstrUserVariable::SET, nullptr, &user_link_variables);
}

/** @ingroup TRACE_user_variables
 *  @brief Add a value to a variable of a link at a given timestamp.
 *
 *  Same as #TRACE_link_variable_add, but let user specify the time used to trace it. Users can specify a time that
 *  is not the simulated clock time as defined by the core simulator. This allows a fine-grain control of time
 *  definition, but should be used with caution since the trace can be inconsistent if resource utilization traces are
 *  also traced.
 *
 *  @param time The timestamp to be used to tag this change of value.
 *  @param link The name of the link to be considered.
 *  @param variable The name of the variable to be considered.
 *  @param value The value to be added to the variable.
 *
 *  @see TRACE_link_variable_declare, TRACE_link_variable_set_with_time, TRACE_link_variable_sub_with_time
 */
void TRACE_link_variable_add_with_time (double time, const char *link, const char *variable, double value)
{
  instr_user_variable(time, link, variable, "LINK", value, InstrUserVariable::ADD, nullptr, &user_link_variables);
}

/** @ingroup TRACE_user_variables
 *  @brief Subtract a value from a variable of a link at a given timestamp.
 *
 *  Same as #TRACE_link_variable_sub, but let user specify the time used to trace it. Users can specify a time that
 *  is not the simulated clock time as defined by the core simulator. This allows a fine-grain control of time
 *  definition, but should be used with caution since the trace can be inconsistent if resource utilization traces are
 *  also traced.
 *
 *  @param time The timestamp to be used to tag this change of value.
 *  @param link The name of the link to be considered.
 *  @param variable The name of the variable to be considered.
 *  @param value The value to be subtracted from the variable.
 *
 *  @see TRACE_link_variable_declare, TRACE_link_variable_set_with_time, TRACE_link_variable_add_with_time
 */
void TRACE_link_variable_sub_with_time (double time, const char *link, const char *variable, double value)
{
  instr_user_variable(time, link, variable, "LINK", value, InstrUserVariable::SUB, nullptr, &user_link_variables);
}

/* for link variables, but with src and dst used for get_route */
/** @ingroup TRACE_user_variables
 *  @brief Set the value of the variable present in the links connecting source and destination.
 *
 *  Same as #TRACE_link_variable_set, but instead of providing the name of link to be considered, provide the source
 *  and destination hosts. All links that are part of the route between source and destination will have the variable
 *  set to the provided value.
 *
 *  @param src The name of the source host for get route.
 *  @param dst The name of the destination host for get route.
 *  @param variable The name of the variable to be considered.
 *  @param value The new value of the variable.
 *
 *  @see TRACE_link_variable_declare, TRACE_link_srcdst_variable_add, TRACE_link_srcdst_variable_sub
 */
void TRACE_link_srcdst_variable_set (const char *src, const char *dst, const char *variable, double value)
{
  TRACE_link_srcdst_variable_set_with_time(simgrid_get_clock(), src, dst, variable, value);
}

/** @ingroup TRACE_user_variables
 *  @brief Add a value to the variable present in the links connecting source and destination.
 *
 *  Same as #TRACE_link_variable_add, but instead of providing the name of link to be considered, provide the source
 *  and destination hosts. All links that are part of the route between source and destination will have the value
 *  passed as parameter added to the current value of the variable name to be considered.
 *
 *  @param src The name of the source host for get route.
 *  @param dst The name of the destination host for get route.
 *  @param variable The name of the variable to be considered.
 *  @param value The value to be added to the variable.
 *
 *  @see TRACE_link_variable_declare, TRACE_link_srcdst_variable_set, TRACE_link_srcdst_variable_sub
 */
void TRACE_link_srcdst_variable_add (const char *src, const char *dst, const char *variable, double value)
{
  TRACE_link_srcdst_variable_add_with_time(simgrid_get_clock(), src, dst, variable, value);
}

/** @ingroup TRACE_user_variables
 *  @brief Subtract a value from the variable present in the links connecting source and destination.
 *
 *  Same as #TRACE_link_variable_sub, but instead of providing the name of link to be considered, provide the source
 *  and destination hosts. All links that are part of the route between source and destination will have the value
 *  passed as parameter subtracted from the current value of the variable name to be considered.
 *
 *  @param src The name of the source host for get route.
 *  @param dst The name of the destination host for get route.
 *  @param variable The name of the variable to be considered.
 *  @param value The value to be subtracted from the variable.
 *
 *  @see TRACE_link_variable_declare, TRACE_link_srcdst_variable_set, TRACE_link_srcdst_variable_add
 */
void TRACE_link_srcdst_variable_sub (const char *src, const char *dst, const char *variable, double value)
{
  TRACE_link_srcdst_variable_sub_with_time(simgrid_get_clock(), src, dst, variable, value);
}

/** @ingroup TRACE_user_variables
 *  @brief Set the value of the variable present in the links connecting source and destination at a given timestamp.
 *
 *  Same as #TRACE_link_srcdst_variable_set, but let user specify the time used to trace it. Users can specify a time
 *  that is not the simulated clock time as defined by the core simulator. This allows a fine-grain control of time
 *  definition, but should be used with caution since the trace can be inconsistent if resource utilization traces are
 *  also traced.
 *
 *  @param time The timestamp to be used to tag this change of value.
 *  @param src The name of the source host for get route.
 *  @param dst The name of the destination host for get route.
 *  @param variable The name of the variable to be considered.
 *  @param value The new value of the variable.
 *
 *  @see TRACE_link_variable_declare, TRACE_link_srcdst_variable_add_with_time, TRACE_link_srcdst_variable_sub_with_time
 */
void TRACE_link_srcdst_variable_set_with_time (double time, const char *src, const char *dst, const char *variable,
                                               double value)
{
  instr_user_srcdst_variable(time, src, dst, variable, "LINK", value, InstrUserVariable::SET);
}

/** @ingroup TRACE_user_variables
 *  @brief Add a value to the variable present in the links connecting source and destination at a given timestamp.
 *
 *  Same as #TRACE_link_srcdst_variable_add, but let user specify the time used to trace it. Users can specify a time
 *  that is not the simulated clock time as defined by the core simulator. This allows a fine-grain control of time
 *  definition, but should be used with caution since the trace can be inconsistent if resource utilization traces are
 *  also traced.
 *
 *  @param time The timestamp to be used to tag this change of value.
 *  @param src The name of the source host for get route.
 *  @param dst The name of the destination host for get route.
 *  @param variable The name of the variable to be considered.
 *  @param value The value to be added to the variable.
 *
 *  @see TRACE_link_variable_declare, TRACE_link_srcdst_variable_set_with_time, TRACE_link_srcdst_variable_sub_with_time
 */
void TRACE_link_srcdst_variable_add_with_time (double time, const char *src, const char *dst, const char *variable,
                                               double value)
{
  instr_user_srcdst_variable(time, src, dst, variable, "LINK", value, InstrUserVariable::ADD);
}

/** @ingroup TRACE_user_variables
 *  @brief Subtract a value from the variable present in the links connecting source and dest. at a given timestamp.
 *
 *  Same as #TRACE_link_srcdst_variable_sub, but let user specify the time used to trace it. Users can specify a time
 *  that is not the simulated clock time as defined by the core simulator. This allows a fine-grain control of time
 *  definition, but should be used with caution since the trace can be inconsistent if resource utilization traces are
 *  also traced.
 *
 *  @param time The timestamp to be used to tag this change of value.
 *  @param src The name of the source host for get route.
 *  @param dst The name of the destination host for get route.
 *  @param variable The name of the variable to be considered.
 *  @param value The value to be subtracted from the variable.
 *
 *  @see TRACE_link_variable_declare, TRACE_link_srcdst_variable_set_with_time, TRACE_link_srcdst_variable_add_with_time
 */
void TRACE_link_srcdst_variable_sub_with_time (double time, const char *src, const char *dst, const char *variable,
                                               double value)
{
  instr_user_srcdst_variable(time, src, dst, variable, "LINK", value, InstrUserVariable::SUB);
}

/** @ingroup TRACE_user_variables
 *  @brief Get declared user link variables
 *
 * This function should be used to get link variables that were already declared with #TRACE_link_variable_declare or
 * with #TRACE_link_variable_declare_with_color.
 *
 * @return A dynar with the declared link variables, must be freed with xbt_dynar_free.
 */
xbt_dynar_t TRACE_get_link_variables ()
{
  return instr_set_to_dynar(user_link_variables);
}

/** @ingroup TRACE_user_variables
 *  @brief Declare a new user state associated to hosts.
 *
 *  Declare a user state that will be associated to hosts.
 *  A user host state can be used to trace application states.
 *
 *  @param state The name of the new state to be declared.
 *
 *  @see TRACE_host_state_declare_value
 */
void TRACE_host_state_declare (const char *state)
{
  instr_new_user_state_type("HOST", state);
}

/** @ingroup TRACE_user_variables
 *  @brief Declare a new value for a user state associated to hosts.
 *
 *  Declare a value for a state. The color needs to be a string with 3 numbers separated by spaces in the range [0,1].
 *  A light-gray color can be specified using "0.7 0.7 0.7" as color.
 *
 *  @param state The name of the new state to be declared.
 *  @param value The name of the value
 *  @param color The color of the value
 *
 *  @see TRACE_host_state_declare
 */
void TRACE_host_state_declare_value (const char *state, const char *value, const char *color)
{
  instr_new_value_for_user_state_type (state, value, color);
}

/** @ingroup TRACE_user_variables
 *  @brief Set the user state to the given value.
 *
 *  Change a user state previously declared to the given value.
 *
 *  @param host The name of the host to be considered.
 *  @param state_name The name of the state previously declared.
 *  @param value_name The new value of the state.
 *
 *  @see TRACE_host_state_declare, TRACE_host_push_state, TRACE_host_pop_state, TRACE_host_reset_state
 */
void TRACE_host_set_state(const char* host, const char* state_name, const char* value_name)
{
  simgrid::instr::StateType* state = simgrid::instr::Container::by_name(host)->get_state(state_name);
  state->add_entity_value(value_name);
  state->set_event(value_name);
}

/** @ingroup TRACE_user_variables
 *  @brief Push a new value for a state of a given host.
 *
 *  Change a user state previously declared by pushing the new value to the state.
 *
 *  @param host The name of the host to be considered.
 *  @param state_name The name of the state previously declared.
 *  @param value_name The value to be pushed.
 *
 *  @see TRACE_host_state_declare, TRACE_host_set_state, TRACE_host_pop_state, TRACE_host_reset_state
 */
void TRACE_host_push_state(const char* host, const char* state_name, const char* value_name)
{
  simgrid::instr::Container::by_name(host)->get_state(state_name)->push_event(value_name);
}

/** @ingroup TRACE_user_variables
 *  @brief Pop the last value of a state of a given host.
 *
 *  Change a user state previously declared by removing the last value of the state.
 *
 *  @param host The name of the host to be considered.
 *  @param state_name The name of the state to be popped.
 *
 *  @see TRACE_host_state_declare, TRACE_host_set_state, TRACE_host_push_state, TRACE_host_reset_state
 */
void TRACE_host_pop_state(const char* host, const char* state_name)
{
  simgrid::instr::Container::by_name(host)->get_state(state_name)->pop_event();
}
