/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/udpor/UnfoldingEvent.hpp"
#include "src/mc/explo/udpor/History.hpp"

#include <xbt/string.hpp>

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
  // Intrinsic identity check
  if (this == &other) {
    return true;
  }
  // Two events are equivalent iff:
  // 1. they have the same action
  // 2. they have the same history
  //
  // NOTE: All unfolding event objects are created in reference to
  // an `Unfolding` object which owns them. Hence, the references
  // they contain to other events in the unfolding can
  // be used as intrinsic identities (i.e. we don't need to
  // recursively check if each of our causes has a `==` in
  // the other event's causes)
  return associated_transition->aid_ == other.associated_transition->aid_ &&
         associated_transition->type_ == other.associated_transition->type_ &&
         associated_transition->times_considered_ == other.associated_transition->times_considered_ &&
         this->immediate_causes == other.immediate_causes;
}

std::string UnfoldingEvent::to_string() const
{
  return xbt::string_printf("e(%s) (%zu dependencies)", associated_transition->to_string().c_str(),
                            immediate_causes.size());
}

EventSet UnfoldingEvent::get_history() const
{
  return History(this).get_all_events();
}

bool UnfoldingEvent::related_to(const UnfoldingEvent* other) const
{
  return this->in_history_of(other) or other->in_history_of(this);
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

  const bool conflicts_with_me    = std::any_of(unique_to_me.begin(), unique_to_me.end(),
                                                [&](const UnfoldingEvent* e) { return e->is_dependent_with(other); });
  const bool conflicts_with_other = std::any_of(unique_to_other.begin(), unique_to_other.end(),
                                                [&](const UnfoldingEvent* e) { return e->is_dependent_with(this); });
  return conflicts_with_me or conflicts_with_other;
}

bool UnfoldingEvent::conflicts_with(const Configuration& config) const
{
  // A configuration is itself already conflict-free. Thus, it is
  // simply a matter of testing whether or not the transition associated
  // with the event is dependent with any already in `config` that are
  // OUTSIDE this event's history (in an unfolding, events only conflict
  // if they are not related)
  const EventSet potential_conflicts = config.get_events().subtracting(get_history());
  return std::any_of(potential_conflicts.cbegin(), potential_conflicts.cend(),
                     [&](const UnfoldingEvent* e) { return this->is_dependent_with(e); });
}

bool UnfoldingEvent::immediately_conflicts_with(const UnfoldingEvent* other) const
{
  // They have to be in conflict at a minimum
  if (not conflicts_with(other)) {
    return false;
  }

  auto combined_events = History(EventSet{this, other}).get_all_events();

  // See the definition of immediate conflicts in the original paper on UDPOR
  {
    combined_events.remove(this);
    if (not combined_events.is_valid_configuration()) {
      return false;
    }
    combined_events.insert(this);
  }

  {
    combined_events.remove(other);
    if (not combined_events.is_valid_configuration()) {
      return false;
    }
    combined_events.insert(other);
  }

  return true;
}

bool UnfoldingEvent::is_dependent_with(const Transition* t) const
{
  return associated_transition->depends(t);
}

bool UnfoldingEvent::is_dependent_with(const UnfoldingEvent* other) const
{
  return is_dependent_with(other->associated_transition.get());
}

} // namespace simgrid::mc::udpor
