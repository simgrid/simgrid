/* Copyright (c) 2010-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/instr/instr_private.hpp"
#include <string>
#include <unordered_map>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_resource, instr, "tracing (un)-categorized resource utilization");

//to check if variables were previously set to 0, otherwise paje won't simulate them
static std::unordered_map<std::string, std::string> platform_variables;

static void instr_event(double now, double delta, simgrid::instr::Type* variable, container_t resource, double value)
{
  /* To trace resource utilization, we use AddVariableEvent and SubVariableEvent only. This implies to add a
   * SetVariableEvent first to set the initial value of all variables for subsequent adds/subs. If we don't do so,
   * the first AddVariableEvent would be added to a non-determined value, hence causing analysis problems.
   */

  // create a key considering the resource and variable
  std::string key = resource->getName() + variable->getName();

  // check if key exists: if it doesn't, set the variable to zero and mark this in the global map.
  if (platform_variables.find(key) == platform_variables.end()) {
    new simgrid::instr::SetVariableEvent(now, resource, variable, 0);
    platform_variables[key] = std::string("");
  }

  new simgrid::instr::AddVariableEvent(now, resource, variable, value);
  new simgrid::instr::SubVariableEvent(now + delta, resource, variable, value);
}

/* TRACE_surf_link_set_utilization: entry point from SimGrid */
void TRACE_surf_link_set_utilization(const char *resource, const char *category, double value, double now, double delta)
{
  //only trace link utilization if link is known by tracing mechanism
  container_t container = simgrid::instr::Container::byNameOrNull(resource);
  if (not container || not value)
    return;

  //trace uncategorized link utilization
  if (TRACE_uncategorized()){
    XBT_DEBUG("UNCAT LINK [%f - %f] %s bandwidth_used %f", now, now + delta, resource, value);
    simgrid::instr::Type* type = container->type_->byName("bandwidth_used");
    instr_event (now, delta, type, container, value);
  }

  //trace categorized utilization
  if (TRACE_categorized()){
    if (not category)
      return;
    //variable of this category starts by 'b', because we have a link here
    std::string category_type = std::string("b") + category;
    XBT_DEBUG("CAT LINK [%f - %f] %s %s %f", now, now + delta, resource, category_type.c_str(), value);
    simgrid::instr::Type* type = container->type_->byName(category_type);
    instr_event (now, delta, type, container, value);
  }
}

/* TRACE_surf_host_set_utilization: entry point from SimGrid */
void TRACE_surf_host_set_utilization(const char *resource, const char *category, double value, double now, double delta)
{
  //only trace host utilization if host is known by tracing mechanism
  container_t container = simgrid::instr::Container::byNameOrNull(resource);
  if (not container || not value)
    return;

  //trace uncategorized host utilization
  if (TRACE_uncategorized()){
    XBT_DEBUG("UNCAT HOST [%f - %f] %s power_used %f", now, now+delta, resource, value);
    simgrid::instr::Type* type = container->type_->byName("power_used");
    instr_event (now, delta, type, container, value);
  }

  //trace categorized utilization
  if (TRACE_categorized()){
    if (not category)
      return;
    //variable of this category starts by 'p', because we have a host here
    std::string category_type = std::string("p") + category;
    XBT_DEBUG("CAT HOST [%f - %f] %s %s %f", now, now + delta, resource, category_type.c_str(), value);
    simgrid::instr::Type* type = container->type_->byName(category_type);
    instr_event (now, delta, type, container, value);
  }
}
