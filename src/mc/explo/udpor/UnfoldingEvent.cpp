/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/udpor/UnfoldingEvent.hpp"
#include "src/mc/explo/udpor/History.hpp"

namespace simgrid::mc::udpor {

UnfoldingEvent::UnfoldingEvent(std::initializer_list<const UnfoldingEvent*> init_list)
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
  // they contain to other events in the unfolding can
  // be used as intrinsic identities (i.e. we don't need to
  // recursively check if each of our causes has a `==` in
  // the other event's causes)
  return this->immediate_causes == other.immediate_causes;
}

EventSet UnfoldingEvent::get_history() const
{
  return History(this).get_all_events();
}

bool UnfoldingEvent::related_to(const UnfoldingEvent* other) const
{
  return EventSet({this, other}).is_maximal();
}

bool UnfoldingEvent::in_history_of(const UnfoldingEvent* other) const
{
  return History(other).contains(this);
}

bool UnfoldingEvent::conflicts_with(const UnfoldingEvent* other) const
{
  // Events that have a causal relation never are in conflict
  // in an unfolding structure. Two events in conflict must
  // not be contained in each other's histories
  if (related_to(other)) {
    return false;
  }

  const EventSet my_history      = get_history();
  const EventSet other_history   = other->get_history();
  const EventSet unique_to_me    = my_history.subtracting(other_history);
  const EventSet unique_to_other = other_history.subtracting(my_history);

  for (const auto e_me : unique_to_me) {
    for (const auto e_other : unique_to_other) {
      if (e_me->has_conflicting_transition_with(e_other)) {
        return true;
      }
    }
  }
  return false;
}

bool UnfoldingEvent::has_conflicting_transition_with(const UnfoldingEvent* other) const
{
  return associated_transition->depends(other->associated_transition.get());
}

} // namespace simgrid::mc::udpor
