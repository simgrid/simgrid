/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/udpor/Configuration.hpp"
#include "src/mc/explo/udpor/History.hpp"

#include <algorithm>
#include <stdexcept>

namespace simgrid::mc::udpor {

void Configuration::add_event(UnfoldingEvent* e)
{
  this->events_.insert(e);
  this->newest_event = e;

  History history(e);
  if (!this->events_.contains(history)) {
    throw std::invalid_argument("The newly added event has dependencies "
                                "which are missing from this configuration");
  }
}

} // namespace simgrid::mc::udpor
