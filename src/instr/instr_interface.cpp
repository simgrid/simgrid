/* Copyright (c) 2010-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include <simgrid/Exception.hpp>
#include <simgrid/kernel/routing/NetPoint.hpp>
#include <simgrid/s4u/Engine.hpp>
#include <xbt/random.hpp>

#include "src/instr/instr_private.hpp"
#include "src/kernel/resource/StandardLinkImpl.hpp"
#include <algorithm>
#include <cmath>

enum class InstrUserVariable { DECLARE, SET, ADD, SUB };

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_api, instr, "API");

std::set<std::string, std::less<>> created_categories;
std::set<std::string, std::less<>> declared_marks;
std::set<std::string, std::less<>> user_host_variables;
std::set<std::string, std::less<>> user_vm_variables;
std::set<std::string, std::less<>> user_link_variables;

static void instr_user_variable(double time, const std::string& resource, const std::string& variable_name,
                                const std::string& parent_type, double value, InstrUserVariable what,
                                const std::string& color, std::set<std::string, std::less<>>* filter)
{
  /* safe switches. tracing has to be activated and if platform is not traced, we don't allow user variables */
  if (not TRACE_is_enabled() || not TRACE_needs_platform())
    return;

  // check if variable is already declared
  auto created = filter->find(variable_name);
  if (what == InstrUserVariable::DECLARE) {
    if (created == filter->end()) { // not declared yet
      filter->insert(variable_name);
      instr_new_user_variable_type(parent_type, variable_name, color);
    }
  } else {
    if (created != filter->end()) { // declared, let's work
      simgrid::instr::VariableType* variable =
          simgrid::instr::Container::by_name(resource)->get_variable(variable_name);
      switch (what) {
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

static void instr_user_srcdst_variable(double time, const std::string& src, const std::string& dst,
                                       const std::string& variable, double value, InstrUserVariable what)
{
  const auto* engine  = simgrid::s4u::Engine::get_instance();
  const auto* src_elm = engine->netpoint_by_name_or_null(src);
  xbt_assert(src_elm, "Element '%s' not found!", src.c_str());

  const auto* dst_elm = engine->netpoint_by_name_or_null(dst);
  xbt_assert(dst_elm, "Element '%s' not found!", dst.c_str());

  std::vector<simgrid::kernel::resource::StandardLinkImpl*> route;
  simgrid::kernel::routing::NetZoneImpl::get_global_route(src_elm, dst_elm, route, nullptr);
  for (auto const& link : route)
    instr_user_variable(time, link->get_cname(), variable, "LINK", value, what, "", &user_link_variables);
}

namespace simgrid {
namespace instr {
/* for host variables */
/** @brief Declare a new user variable associated to hosts.
 *
 *  Declare a user variable that will be associated to hosts.
 *  A user host variable can be used to trace user variables such as the number of tasks in a server, the number of
 *  clients in an application (for hosts), and so on. The color associated to this new variable will be random if
 *  not given as parameter.
 */
void declare_host_variable(const std::string& variable, const std::string& color)
{
  instr_user_variable(0, "", variable, "HOST", 0, InstrUserVariable::DECLARE, color, &user_host_variables);
}

void set_host_variable(const std::string& host, const std::string& variable, double value, double time)
{
  instr_user_variable(time, host, variable, "HOST", value, InstrUserVariable::SET, "", &user_host_variables);
}

/** @brief Add a value to a variable of a host */
void add_host_variable(const std::string& host, const std::string& variable, double value, double time)
{
  instr_user_variable(time, host, variable, "HOST", value, InstrUserVariable::ADD, "", &user_host_variables);
}

/** @brief Subtract a value to a variable of a host */
void sub_host_variable(const std::string& host, const std::string& variable, double value, double time)
{
  instr_user_variable(time, host, variable, "HOST", value, InstrUserVariable::SUB, "", &user_host_variables);
}

/** @brief Get host variables that were already declared with #declare_host_variable. */
const std::set<std::string, std::less<>>& get_host_variables()
{
  return user_host_variables;
}

/* for link variables */
/** @brief Declare a new user variable associated to links.
 *
 *  Declare a user variable that will be associated to links.
 *  A user link variable can be used, for example, to trace user variables such as the number of messages being
 *  transferred through network links. The color associated to this new variable will be random if not given as
 *  parameter.
 */
void declare_link_variable(const std::string& variable, const std::string& color)
{
  instr_user_variable(0, "", variable, "LINK", 0, InstrUserVariable::DECLARE, color, &user_link_variables);
}

/** @brief Set the value of a variable of a link */
void set_link_variable(const std::string& link, const std::string& variable, double value, double time)
{
  instr_user_variable(time, link, variable, "LINK", value, InstrUserVariable::SET, "", &user_link_variables);
}

void set_link_variable(const std::string& src, const std::string& dst, const std::string& variable, double value,
                       double time)
{
  instr_user_srcdst_variable(time, src, dst, variable, value, InstrUserVariable::SET);
}

/** @brief Add a value to a variable of a link */
void add_link_variable(const std::string& link, const std::string& variable, double value, double time)
{
  instr_user_variable(time, link, variable, "LINK", value, InstrUserVariable::ADD, "", &user_link_variables);
}

/** @brief Add a value to a variable of a link */
void add_link_variable(const std::string& src, const std::string& dst, const std::string& variable, double value,
                       double time)
{
  instr_user_srcdst_variable(time, src, dst, variable, value, InstrUserVariable::ADD);
}

/** @brief Subtract a value to a variable of a link */
void sub_link_variable(const std::string& link, const std::string& variable, double value, double time)
{
  instr_user_variable(time, link, variable, "LINK", value, InstrUserVariable::SUB, "", &user_link_variables);
}

/** @brief Subtract a value to a variable of a link */
void sub_link_variable(const std::string& src, const std::string& dst, const std::string& variable, double value,
                       double time)
{
  instr_user_srcdst_variable(time, src, dst, variable, value, InstrUserVariable::SUB);
}

/** @brief Get link variables that were already declared with #declare_link_variable. */
const std::set<std::string, std::less<>>& get_link_variables()
{
  return user_link_variables;
}

/* for VM variables */
/** @brief Declare a new user variable associated to VMs.
 *
 *  Declare a user variable that will be associated to VMs. A user host variable can be used to trace user variables
 *  such as the number of tasks in a VM, the number of clients in an application (for hosts), and so on. The color
 *  associated to this new variable will be random if not given as parameter.
 */
void declare_vm_variable(const std::string& variable, const std::string& color)
{
  instr_user_variable(0, "", variable, "VM", 0, InstrUserVariable::DECLARE, color, &user_vm_variables);
}

/** @brief Set the value of a variable of a vm */
void set_vm_variable(const std::string& vm, const std::string& variable, double value, double time)
{
  instr_user_variable(time, vm, variable, "VM", value, InstrUserVariable::SET, "", &user_vm_variables);
}

/** @brief Add a value to a variable of a VM */
void add_vm_variable(const std::string& vm, const std::string& variable, double value, double time)
{
  instr_user_variable(time, vm, variable, "VM", value, InstrUserVariable::ADD, "", &user_vm_variables);
}

/** @brief Subtract a value from a variable of a VM */
void sub_vm_variable(const std::string& vm, const std::string& variable, double value, double time)
{
  instr_user_variable(time, vm, variable, "VM", value, InstrUserVariable::SUB, "", &user_vm_variables);
}

/** @brief Get VM variables that were already declared with #declare_vm_variable. */
const std::set<std::string, std::less<>>& get_vm_variables()
{
  return user_vm_variables;
}

/**@brief Declare a new type for tracing mark.
 *
 * This function declares a new Paje event type in the trace file that can be used by simulators to declare
 * application-level marks. This function is independent of which API is used in SimGrid.
 */
void declare_mark(const std::string& mark_type)
{
  /* safe switches. tracing has to be activated and if platform is not traced, we can't deal with marks */
  if (not TRACE_is_enabled() || not TRACE_needs_platform())
    return;

  // check if mark_type is already declared
  if (declared_marks.find(mark_type) != declared_marks.end()) {
    throw TracingError(XBT_THROW_POINT,
                       xbt::string_printf("mark_type with name (%s) is already declared", mark_type.c_str()));
  }

  XBT_DEBUG("MARK,declare %s", mark_type.c_str());
  Container::get_root()->get_type()->by_name_or_create<EventType>(mark_type);
  declared_marks.emplace(mark_type);
}

/** @brief Declare a new colored value for a previously declared mark type.
 *
 * This function declares a new colored value for a Paje event type in the trace file that can be used by simulators to
 * declare application-level marks. This function is independent of which API is used in SimGrid. The color needs to be
 * a string with three numbers separated by spaces in the range [0,1].
 * A light-gray color can be specified using "0.7 0.7 0.7" as color. If no color is provided, the default color used
 * will be white ("1 1 1").
 */
void declare_mark_value(const std::string& mark_type, const std::string& mark_value, const std::string& mark_color)
{
  /* safe switches. tracing has to be activated and if platform is not traced, we can't deal with marks */
  if (not TRACE_is_enabled() || not TRACE_needs_platform())
    return;

  auto* type = static_cast<EventType*>(Container::get_root()->get_type()->by_name(mark_type));
  if (not type) {
    throw TracingError(XBT_THROW_POINT,
                       xbt::string_printf("mark_type with name (%s) is not declared", mark_type.c_str()));
  } else {
    XBT_DEBUG("MARK, declare_value %s %s %s", mark_type.c_str(), mark_value.c_str(), mark_color.c_str());
    type->add_entity_value(mark_value, mark_color);
  }
}

/** @brief Create a new instance of a tracing mark type.
 *
 * This function creates a mark in the trace file. The first parameter had to be previously declared using
 * #declare_mark, the second is the identifier for this mark instance. We recommend that the mark_value is a
 * unique value for the whole simulation. Nevertheless, this is not a strong requirement: the trace will be valid even
 * if there are multiple mark identifiers for the same trace.
 */
void mark(const std::string& mark_type, const std::string& mark_value)
{
  /* safe switches. tracing has to be activated and if platform is not traced, we can't deal with marks */
  if (not TRACE_is_enabled() || not TRACE_needs_platform())
    return;

  // check if mark_type is already declared
  auto* type = static_cast<EventType*>(Container::get_root()->get_type()->by_name(mark_type));
  if (not type) {
    throw TracingError(XBT_THROW_POINT,
                       xbt::string_printf("mark_type with name (%s) is not declared", mark_type.c_str()));
  } else {
    XBT_DEBUG("MARK %s %s", mark_type.c_str(), mark_value.c_str());
    new NewEvent(simgrid_get_clock(), Container::get_root(), type, type->get_entity_value(mark_value));
  }
}

/** @brief Get marks that were already declared with #declare_mark. */
const std::set<std::string, std::less<>>& get_marks()
{
  return declared_marks;
}

/** @brief Declare a new category.
 *
 *  This function should be used to define a user category. The category can be used to differentiate the tasks that
 *  are created during the simulation (for example, tasks from server1, server2, or request tasks, computation tasks,
 *  communication tasks). All resource utilization (host power and link bandwidth) will be classified according to the
 *  task category. Tasks that do not belong to a category are not traced. The color for the category that is being
 *  declared is random. This function has no effect if a category with the same name has been already declared.
 *
 * See @ref outcomes_vizu for details on how to trace the (categorized) resource utilization.
 */
void declare_tracing_category(const std::string& name, const std::string& color)
{
  /* safe switches. tracing has to be activated and if platform is not traced, we can't deal with categories */
  if (not TRACE_is_enabled() || not TRACE_needs_platform() || not TRACE_categorized())
    return;

  // check if category is already created
  if (created_categories.find(name) != created_categories.end())
    return;

  created_categories.emplace(name);

  // define final_color
  std::string final_color;
  if (color.empty()) {
    // generate a random color
    double red   = simgrid::xbt::random::uniform_real(0.0, std::nextafter(1.0, 2.0));
    double green = simgrid::xbt::random::uniform_real(0.0, std::nextafter(1.0, 2.0));
    double blue  = simgrid::xbt::random::uniform_real(0.0, std::nextafter(1.0, 2.0));
    final_color  = std::to_string(red) + " " + std::to_string(green) + " " + std::to_string(blue);
  } else {
    final_color = std::string(color);
  }

  XBT_DEBUG("CAT,declare %s, \"%s\" \"%s\"", name.c_str(), color.c_str(), final_color.c_str());

  // define the type of this category on top of hosts and links
  instr_new_variable_type(name, final_color);
}

/** @brief Get categories that were already declared with #declare_tracing_category.
 *
 * See @ref outcomes_vizu for details on how to trace the (categorized) resource utilization.
 */
const std::set<std::string, std::less<>>& get_tracing_categories()
{
  return created_categories;
}

} // namespace instr
} // namespace simgrid

static xbt_dynar_t instr_set_to_dynar(const std::set<std::string, std::less<>>& filter) // XBT_ATTRIB_DEPRECATED_v333
{
  if (not TRACE_is_enabled() || not TRACE_needs_platform())
    return nullptr;

  xbt_dynar_t ret = xbt_dynar_new (sizeof(char*), &xbt_free_ref);
  for (auto const& name : filter)
    xbt_dynar_push_as(ret, char*, xbt_strdup(name.c_str()));

  return ret;
}

void TRACE_category(const char* category) // XBT_ATTRIB_DEPRECATED_v333
{
  simgrid::instr::declare_tracing_category(category);
}

void TRACE_category_with_color(const char* category, const char* color) // XBT_ATTRIB_DEPRECATED_v333
{
  simgrid::instr::declare_tracing_category(category, color);
}

xbt_dynar_t TRACE_get_categories() // XBT_ATTRIB_DEPRECATED_v333
{
  if (not TRACE_is_enabled() || not TRACE_categorized())
    return nullptr;
  return instr_set_to_dynar(created_categories);
}

void TRACE_declare_mark(const char* mark_type) // XBT_ATTRIB_DEPRECATED_v333
{
  simgrid::instr::declare_mark(mark_type);
}

void TRACE_declare_mark_value_with_color(const char* mark_type, const char* mark_value,
                                         const char* mark_color) // XBT_ATTRIB_DEPRECATED_v333
{
  simgrid::instr::declare_mark_value(mark_type, mark_value, mark_color);
}

void TRACE_declare_mark_value(const char* mark_type, const char* mark_value) // XBT_ATTRIB_DEPRECATED_v333
{
  simgrid::instr::declare_mark_value(mark_type, mark_value);
}

void TRACE_mark(const char* mark_type, const char* mark_value) // XBT_ATTRIB_DEPRECATED_v333
{
  simgrid::instr::mark(mark_type, mark_value);
}

xbt_dynar_t TRACE_get_marks() // XBT_ATTRIB_DEPRECATED_v333
{
  if (not TRACE_is_enabled())
    return nullptr;

  return instr_set_to_dynar(declared_marks);
}

int TRACE_platform_graph_export_graphviz(const char* filename) // XBT_ATTRIB_DEPRECATED_v333
{
  simgrid::instr::platform_graph_export_graphviz(filename);
  return 1;
}

void TRACE_vm_variable_declare(const char* variable) // XBT_ATTRIB_DEPRECATED_v333
{
  instr_user_variable(0, "", variable, "VM", 0, InstrUserVariable::DECLARE, "", &user_vm_variables);
}
void TRACE_vm_variable_declare_with_color(const char* variable, const char* color) // XBT_ATTRIB_DEPRECATED_v333
{
  instr_user_variable(0, "", variable, "VM", 0, InstrUserVariable::DECLARE, color, &user_vm_variables);
}

void TRACE_vm_variable_set(const char* vm, const char* variable, double value) // XBT_ATTRIB_DEPRECATED_v333
{
  instr_user_variable(simgrid_get_clock(), vm, variable, "VM", value, InstrUserVariable::SET, "", &user_vm_variables);
}

void TRACE_vm_variable_add(const char* vm, const char* variable, double value) // XBT_ATTRIB_DEPRECATED_v333
{
  instr_user_variable(simgrid_get_clock(), vm, variable, "VM", value, InstrUserVariable::ADD, "", &user_vm_variables);
}
void TRACE_vm_variable_sub(const char* vm, const char* variable, double value) // XBT_ATTRIB_DEPRECATED_v333
{
  instr_user_variable(simgrid_get_clock(), vm, variable, "VM", value, InstrUserVariable::SUB, "", &user_vm_variables);
}

void TRACE_vm_variable_set_with_time(double time, const char* vm, const char* variable,
                                     double value) // XBT_ATTRIB_DEPRECATED_v333
{
  instr_user_variable(time, vm, variable, "VM", value, InstrUserVariable::SET, "", &user_vm_variables);
}

void TRACE_vm_variable_add_with_time(double time, const char* vm, const char* variable,
                                     double value) // XBT_ATTRIB_DEPRECATED_v333
{
  instr_user_variable(time, vm, variable, "VM", value, InstrUserVariable::ADD, "", &user_vm_variables);
}
void TRACE_vm_variable_sub_with_time(double time, const char* vm, const char* variable,
                                     double value) // XBT_ATTRIB_DEPRECATED_v333
{
  instr_user_variable(time, vm, variable, "VM", value, InstrUserVariable::SUB, "", &user_vm_variables);
}

void TRACE_host_variable_declare(const char* variable) // XBT_ATTRIB_DEPRECATED_v333
{
  simgrid::instr::declare_host_variable(variable);
}

void TRACE_host_variable_declare_with_color(const char* variable, const char* color) // XBT_ATTRIB_DEPRECATED_v333
{
  simgrid::instr::declare_host_variable(variable, color);
}

void TRACE_host_variable_set(const char* host, const char* variable, double value) // XBT_ATTRIB_DEPRECATED_v333
{
  instr_user_variable(simgrid_get_clock(), host, variable, "HOST", value, InstrUserVariable::SET, "",
                      &user_host_variables);
}

void TRACE_host_variable_add(const char* host, const char* variable, double value) // XBT_ATTRIB_DEPRECATED_v333
{
  instr_user_variable(simgrid_get_clock(), host, variable, "HOST", value, InstrUserVariable::ADD, "",
                      &user_host_variables);
}

void TRACE_host_variable_sub(const char* host, const char* variable, double value) // XBT_ATTRIB_DEPRECATED_v333
{
  instr_user_variable(simgrid_get_clock(), host, variable, "HOST", value, InstrUserVariable::SUB, "",
                      &user_host_variables);
}

void TRACE_host_variable_set_with_time(double time, const char* host, const char* variable,
                                       double value) // XBT_ATTRIB_DEPRECATED_v333
{
  instr_user_variable(time, host, variable, "HOST", value, InstrUserVariable::SET, "", &user_host_variables);
}

void TRACE_host_variable_add_with_time(double time, const char* host, const char* variable,
                                       double value) // XBT_ATTRIB_DEPRECATED_v333
{
  instr_user_variable(time, host, variable, "HOST", value, InstrUserVariable::ADD, "", &user_host_variables);
}

void TRACE_host_variable_sub_with_time(double time, const char* host, const char* variable,
                                       double value) // XBT_ATTRIB_DEPRECATED_v333
{
  instr_user_variable(time, host, variable, "HOST", value, InstrUserVariable::SUB, "", &user_host_variables);
}

xbt_dynar_t TRACE_get_host_variables() // XBT_ATTRIB_DEPRECATED_v333
{
  return instr_set_to_dynar(user_host_variables);
}

void TRACE_link_variable_declare(const char* variable) // XBT_ATTRIB_DEPRECATED_v333
{
  simgrid::instr::declare_link_variable(variable);
}

void TRACE_link_variable_declare_with_color(const char* variable, const char* color) // XBT_ATTRIB_DEPRECATED_v333
{
  simgrid::instr::declare_link_variable(variable, color);
}

void TRACE_link_variable_set(const char* link, const char* variable, double value) // XBT_ATTRIB_DEPRECATED_v333
{
  instr_user_variable(simgrid_get_clock(), link, variable, "LINK", value, InstrUserVariable::SET, "",
                      &user_link_variables);
}

void TRACE_link_variable_add(const char* link, const char* variable, double value) // XBT_ATTRIB_DEPRECATED_v333
{
  instr_user_variable(simgrid_get_clock(), link, variable, "LINK", value, InstrUserVariable::ADD, "",
                      &user_link_variables);
}

void TRACE_link_variable_sub(const char* link, const char* variable, double value) // XBT_ATTRIB_DEPRECATED_v333
{
  instr_user_variable(simgrid_get_clock(), link, variable, "LINK", value, InstrUserVariable::SUB, "",
                      &user_link_variables);
}

void TRACE_link_variable_set_with_time(double time, const char* link, const char* variable,
                                       double value) // XBT_ATTRIB_DEPRECATED_v333
{
  instr_user_variable(time, link, variable, "LINK", value, InstrUserVariable::SET, "", &user_link_variables);
}

void TRACE_link_variable_add_with_time(double time, const char* link, const char* variable,
                                       double value) // XBT_ATTRIB_DEPRECATED_v333
{
  instr_user_variable(time, link, variable, "LINK", value, InstrUserVariable::ADD, "", &user_link_variables);
}

void TRACE_link_variable_sub_with_time(double time, const char* link, const char* variable,
                                       double value) // XBT_ATTRIB_DEPRECATED_v333
{
  instr_user_variable(time, link, variable, "LINK", value, InstrUserVariable::SUB, "", &user_link_variables);
}

void TRACE_link_srcdst_variable_set(const char* src, const char* dst, const char* variable,
                                    double value) // XBT_ATTRIB_DEPRECATED_v333
{
  instr_user_srcdst_variable(simgrid_get_clock(), src, dst, variable, value, InstrUserVariable::SET);
}

void TRACE_link_srcdst_variable_add(const char* src, const char* dst, const char* variable,
                                    double value) // XBT_ATTRIB_DEPRECATED_v333
{
  instr_user_srcdst_variable(simgrid_get_clock(), src, dst, variable, value, InstrUserVariable::ADD);
}

void TRACE_link_srcdst_variable_sub(const char* src, const char* dst, const char* variable,
                                    double value) // XBT_ATTRIB_DEPRECATED_v333
{
  instr_user_srcdst_variable(simgrid_get_clock(), src, dst, variable, value, InstrUserVariable::SUB);
}

void TRACE_link_srcdst_variable_set_with_time(double time, const char* src, const char* dst, const char* variable,
                                              double value) // XBT_ATTRIB_DEPRECATED_v333
{
  instr_user_srcdst_variable(time, src, dst, variable, value, InstrUserVariable::SET);
}

void TRACE_link_srcdst_variable_add_with_time(double time, const char* src, const char* dst, const char* variable,
                                              double value) // XBT_ATTRIB_DEPRECATED_v333
{
  instr_user_srcdst_variable(time, src, dst, variable, value, InstrUserVariable::ADD);
}

void TRACE_link_srcdst_variable_sub_with_time(double time, const char* src, const char* dst, const char* variable,
                                              double value) // XBT_ATTRIB_DEPRECATED_v333
{
  instr_user_srcdst_variable(time, src, dst, variable, value, InstrUserVariable::SUB);
}

xbt_dynar_t TRACE_get_link_variables() // XBT_ATTRIB_DEPRECATED_v333
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
