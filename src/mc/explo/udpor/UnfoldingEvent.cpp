/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/udpor/UnfoldingEvent.hpp"

namespace simgrid::mc::udpor {

UnfoldingEvent::UnfoldingEvent(std::shared_ptr<Transition> transition, EventSet immediate_causes,
                               unsigned long event_id)
    : associated_transition(std::move(transition)), immediate_causes(std::move(immediate_causes)), event_id(event_id)
{
}

bool UnfoldingEvent::operator==(const UnfoldingEvent&) const
{
  // TODO: Implement semantic equality
  return false;
}

} // namespace simgrid::mc::udpor
