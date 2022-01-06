/* Copyright (c) 2008-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "udpor_global.hpp"
#include "xbt/log.h"
#include <algorithm>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_udpor_global, mc, "udpor_global");

namespace simgrid {
namespace mc {

EventSet EvtSetTools::makeUnion(const EventSet& s1, const EventSet& s2)
{
  EventSet res = s1;
  for (auto evt : s2)
    EvtSetTools::pushBack(res, evt);
  return res;
}

void EvtSetTools::pushBack(EventSet& events, UnfoldingEvent* e)
{
  if (not EvtSetTools::contains(events, e))
    events.push_back(e);
}

bool EvtSetTools::contains(const EventSet& events, const UnfoldingEvent* e)
{
  return std::any_of(events.begin(), events.end(), [e](const UnfoldingEvent* evt) { return *evt == *e; });
}

} // namespace mc
} // namespace simgrid
