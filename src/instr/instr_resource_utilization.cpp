/* Copyright (c) 2010-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/instr/instr_private.hpp"
#include <set>
#include <string>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_resource, instr, "tracing (un)-categorized resource utilization");

//to check if variables were previously set to 0, otherwise paje won't simulate them
static std::set<std::string> platform_variables;

static void instr_event(double now, double delta, simgrid::instr::VariableType* variable, container_t resource,
                        double value)
{
  /* To trace resource utilization, we use AddEvent and SubEvent only. This implies to add a SetEvent first to set the
   * initial value of all variables for subsequent adds/subs. If we don't do so, the first AddEvent would be added to a
   * non-determined value, hence causing analysis problems.
   */

  // create a key considering the resource and variable
  std::string key = resource->getName() + variable->getName();

  // check if key exists: if it doesn't, set the variable to zero and mark this in the global map.
  if (platform_variables.find(key) == platform_variables.end()) {
    variable->setEvent(now, 0);
    platform_variables.insert(key);
  }

  variable->addEvent(now, value);
  variable->subEvent(now + delta, value);
}

static void TRACE_surf_resource_set_utilization(const char* type, const char* name, const char* resource,
                                                const char* category, double value, double now, double delta)
{
  // only trace resource utilization if resource is known by tracing mechanism
  container_t container = simgrid::instr::Container::byNameOrNull(resource);
  if (not container || not value)
    return;

  // trace uncategorized resource utilization
  if (TRACE_uncategorized()){
    XBT_DEBUG("UNCAT %s [%f - %f] %s %s %f", type, now, now + delta, resource, name, value);
    simgrid::instr::VariableType* variable = container->getVariable(name);
    instr_event(now, delta, variable, container, value);
  }

  // trace categorized resource utilization
  if (TRACE_categorized()){
    if (not category)
      return;
    std::string category_type = name[0] + std::string(category);
    XBT_DEBUG("CAT %s [%f - %f] %s %s %f", type, now, now + delta, resource, category_type.c_str(), value);
    simgrid::instr::VariableType* variable = container->getVariable(category_type);
    instr_event(now, delta, variable, container, value);
  }
}

/* TRACE_surf_link_set_utilization: entry point from SimGrid */
void TRACE_surf_link_set_utilization(const char* resource, const char* category, double value, double now, double delta)
{
  TRACE_surf_resource_set_utilization("LINK", "bandwidth_used", resource, category, value, now, delta);
}

/* TRACE_surf_host_set_utilization: entry point from SimGrid */
void TRACE_surf_host_set_utilization(const char* resource, const char* category, double value, double now, double delta)
{
  TRACE_surf_resource_set_utilization("HOST", "power_used", resource, category, value, now, delta);
}
