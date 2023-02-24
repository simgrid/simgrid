/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/udpor/Configuration.hpp"
#include "src/mc/explo/udpor/History.hpp"

#include <algorithm>
#include <stdexcept>

namespace simgrid::mc::udpor {

Configuration::Configuration(std::initializer_list<UnfoldingEvent*> events) : Configuration(EventSet(std::move(events)))
{
}

Configuration::Configuration(EventSet events) : events_(events)
{
  if (!events_.is_valid_configuration()) {
    throw std::invalid_argument("The events do not form a valid configuration");
  }
}

void Configuration::add_event(UnfoldingEvent* e)
{
  if (e == nullptr) {
    throw std::invalid_argument("Expected a nonnull `UnfoldingEvent*` but received NULL instead");
  }

  if (this->events_.contains(e)) {
    return;
  }

  this->events_.insert(e);
  this->newest_event = e;

  // Preserves the property that the configuration is valid
  History history(e);
  if (!this->events_.contains(history)) {
    throw std::invalid_argument("The newly added event has dependencies "
                                "which are missing from this configuration");
  }
}

} // namespace simgrid::mc::udpor
