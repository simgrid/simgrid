/* Copyright (c) 2010, 2012-2017. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/instr/instr_private.hpp"
#include "src/surf/surf_interface.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(instr_surf, instr, "Tracing Surf");

void TRACE_surf_host_set_speed(double date, const char *resource, double speed)
{
  if (TRACE_categorized() || TRACE_uncategorized() || TRACE_platform()) {
    simgrid::instr::Container::byName(resource)->getVariable("power")->setEvent(date, speed);
  }
}

void TRACE_surf_link_set_bandwidth(double date, const char *resource, double bandwidth)
{
  if (TRACE_categorized() || TRACE_uncategorized() || TRACE_platform()) {
    simgrid::instr::Container::byName(resource)->getVariable("bandwidth")->setEvent(date, bandwidth);
  }
}

void TRACE_surf_action(surf_action_t surf_action, const char *category)
{
  if (not TRACE_is_enabled() || not TRACE_categorized() || not category)
    return;

  surf_action->setCategory(category);
}
