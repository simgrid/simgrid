/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/udpor/Configuration.hpp"

namespace simgrid::mc::udpor {

void Configuration::add_event(UnfoldingEvent* e)
{
  this->events_.insert(e);
  this->newest_event = e;

  // TODO: Re-compute the maxmimal events
}

} // namespace simgrid::mc::udpor
