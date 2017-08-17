/* Copyright (c) 2010, 2012-2015. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/instr/instr_private.h"
#include "src/surf/surf_interface.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(instr_surf, instr, "Tracing Surf");

void TRACE_surf_host_set_speed(double date, const char *resource, double speed)
{
  if (TRACE_categorized() || TRACE_uncategorized() || TRACE_platform()) {
    container_t container = s_container::s_container_get(resource);
    type_t type = PJ_type_get ("power", container->type);
    new SetVariableEvent(date, container, type, speed);
  }
}

void TRACE_surf_link_set_bandwidth(double date, const char *resource, double bandwidth)
{
  if (TRACE_categorized() || TRACE_uncategorized() || TRACE_platform()) {
    container_t container = s_container::s_container_get(resource);
    type_t type = PJ_type_get ("bandwidth", container->type);
    new SetVariableEvent(date, container, type, bandwidth);
  }
}

void TRACE_surf_action(surf_action_t surf_action, const char *category)
{
  if (not TRACE_is_enabled())
    return;
  if (not TRACE_categorized())
    return;
  if (not category)
    return;

  surf_action->setCategory(category);
}
