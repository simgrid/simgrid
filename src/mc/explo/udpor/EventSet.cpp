/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/udpor/EventSet.hpp"
#include "src/mc/explo/udpor/Configuration.hpp"
#include "src/mc/explo/udpor/History.hpp"

#include <iterator>

namespace simgrid::mc::udpor {

EventSet::EventSet(Configuration&& config) : EventSet(config.get_events()) {}

void EventSet::remove(UnfoldingEvent* e)
{
  this->events_.erase(e);
}

void EventSet::subtract(const EventSet& other)
{
  this->events_ = std::move(subtracting(other).events_);
}

void EventSet::subtract(const Configuration& config)
{
  subtract(config.get_events());
}

EventSet EventSet::subtracting(const EventSet& other) const
{
  std::unordered_set<UnfoldingEvent*> result = this->events_;

  for (UnfoldingEvent* e : other.events_)
    result.erase(e);

  return EventSet(std::move(result));
}

EventSet EventSet::subtracting(const Configuration& config) const
{
  return subtracting(config.get_events());
}

EventSet EventSet::subtracting(UnfoldingEvent* e) const
{
  auto result = this->events_;
  result.erase(e);
  return EventSet(std::move(result));
}

void EventSet::insert(UnfoldingEvent* e)
{
  this->events_.insert(e);
}

void EventSet::form_union(const EventSet& other)
{
  this->events_ = std::move(make_union(other).events_);
}

void EventSet::form_union(const Configuration& config)
{
  form_union(config.get_events());
}

EventSet EventSet::make_union(UnfoldingEvent* e) const
{
  auto result = this->events_;
  result.insert(e);
  return EventSet(std::move(result));
}

EventSet EventSet::make_union(const EventSet& other) const
{
  std::unordered_set<UnfoldingEvent*> result = this->events_;

  for (UnfoldingEvent* e : other.events_)
    result.insert(e);

  return EventSet(std::move(result));
}

EventSet EventSet::make_union(const Configuration& config) const
{
  return make_union(config.get_events());
}

size_t EventSet::size() const
{
  return this->events_.size();
}

bool EventSet::empty() const
{
  return this->events_.empty();
}

bool EventSet::contains(UnfoldingEvent* e) const
{
  return this->events_.find(e) != this->events_.end();
}

bool EventSet::is_subset_of(const EventSet& other) const
{
  // If there is some element not contained in `other`, then
  // the set difference will contain that element and the
  // result won't be empty
  return subtracting(other).empty();
}

bool EventSet::is_valid_configuration() const
{
  /// @invariant: A collection of events `E` is a configuration
  /// if and only if following while following the history of
  /// each event `e` of `E` you remain in `E`. In other words, you
  /// only see events from set `E`
  ///
  /// The simple proof is based on the definition of a configuration
  /// which requires that all events have their history contained
  /// in the set
  const History history(*this);
  return this->contains(history);
}

bool EventSet::contains(const History& history) const
{
  return std::all_of(history.begin(), history.end(), [=](UnfoldingEvent* e) { return this->contains(e); });
}

bool EventSet::is_maximal_event_set() const
{
  const History history(*this);
  return *this == history.get_all_maximal_events();
}

} // namespace simgrid::mc::udpor