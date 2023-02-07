/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "udpor_global.hpp"
#include "xbt/log.h"

#include <algorithm>
#include <iterator>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(mc_udpor_global, mc, "udpor_global");

namespace simgrid::mc::udpor {

void EventSet::remove(UnfoldingEvent* e)
{
  this->events_.erase(e);
}

void EventSet::subtract(const EventSet& other)
{
  this->events_ = std::move(subtracting(other).events_);
}

// void EventSet::subtract(const Configuration& other);

EventSet EventSet::subtracting(const EventSet& other) const
{
  std::set<UnfoldingEvent*> result;
  std::set_difference(this->events_.begin(), this->events_.end(), other.events_.begin(), other.events_.end(),
                      std::inserter(result, result.end()));
  return EventSet(std::move(result));
}

EventSet EventSet::subtracting(UnfoldingEvent* e) const
{
  auto result = this->events_;
  result.erase(e);
  return EventSet(std::move(result));
}
// EventSet EventSet::subtracting(const Configuration* e) const;

void EventSet::insert(UnfoldingEvent* e)
{
  // TODO: Potentially react if the event is already inserted
  this->events_.insert(e);
}

void EventSet::form_union(const EventSet& other)
{
  this->events_ = std::move(make_union(other).events_);
}

// void EventSet::form_union(const Configuration&);
EventSet EventSet::make_union(UnfoldingEvent* e) const
{
  auto result = this->events_;
  result.insert(e);
  return EventSet(std::move(result));
}

EventSet EventSet::make_union(const EventSet& other) const
{
  std::set<UnfoldingEvent*> result;
  std::set_union(this->events_.begin(), this->events_.end(), other.events_.begin(), other.events_.end(),
                 std::inserter(result, result.end()));
  return EventSet(std::move(result));
}

// EventSet EventSet::make_union(const Configuration& e) const;

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

void Configuration::add_event(UnfoldingEvent* e)
{
  this->events_.insert(e);

  // Re-compute the maxmimal events
}

UnfoldingEvent::UnfoldingEvent(unsigned int nb_events, std::string const& trans_tag, EventSet const& causes,
                               StateHandle sid)
{
  // TODO: Implement this
}

StateManager::Handle StateManager::record_state(const std::unique_ptr<State>&& state)
{
  // // TODO: Throw an error perhaps if the state is nullptr?

  // // std::map<int, std::unique_ptr<int>> a;
  // // auto test = std::make_unique<int>(10);
  // // std::move(state);
  // // // const auto&& ab = std::move(test);
  // // a.insert({1, test});

  // const auto&& state2 = std::move(state);

  // const auto integer_handle = this->current_handle_;
  // this->state_map_.insert(std::make_pair(std::move(integer_handle), state2));

  // // Increment the current handle
  // // TODO: Check for state handle overflow!
  // this->current_handle_++;
  return 0;
}

std::optional<std::reference_wrapper<State>> StateManager::get_state(StateManager::Handle handle)
{
  // TODO: Return the actual state based on the handle provided
  return std::nullopt;
}

} // namespace simgrid::mc::udpor
