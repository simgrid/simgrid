/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/udpor/UnfoldingEvent.hpp"

namespace simgrid::mc::udpor {

UnfoldingEvent::UnfoldingEvent(std::initializer_list<UnfoldingEvent*> init_list)
    : UnfoldingEvent(EventSet(std::move(init_list)))
{
}

UnfoldingEvent::UnfoldingEvent(EventSet immediate_causes, std::shared_ptr<Transition> transition)
    : associated_transition(std::move(transition)), immediate_causes(std::move(immediate_causes))
{
}

bool UnfoldingEvent::operator==(const UnfoldingEvent& other) const
{
  const bool same_actor = associated_transition->aid_ == other.associated_transition->aid_;
  if (!same_actor)
    return false;

  // TODO: Add in information to determine which step in the sequence this actor was executed

  // All unfolding event objects are created in reference to
  // an Unfolding object which owns them. Hence, the references
  // they contain to other events in the unvolding can
  // be used as intrinsic identities (i.e. we don't need to
  // recursively check if each of our causes has a `==` in
  // the other event's causes)
  return this->immediate_causes == other.immediate_causes;
}

} // namespace simgrid::mc::udpor
