/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/udpor/EventSet.hpp"
#include "src/mc/explo/udpor/Configuration.hpp"
#include "src/mc/explo/udpor/History.hpp"
#include "src/mc/explo/udpor/UnfoldingEvent.hpp"
#include "src/xbt/utils/iter/variable_for_loop.hpp"

#include <stack>
#include <vector>

namespace simgrid::mc::udpor {

EventSet::EventSet(const Configuration& config) : EventSet(config.get_events()) {}

void EventSet::remove(const UnfoldingEvent* e)
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
  std::unordered_set<const UnfoldingEvent*> result = this->events_;

  for (const UnfoldingEvent* e : other.events_)
    result.erase(e);

  return EventSet(std::move(result));
}

EventSet EventSet::subtracting(const Configuration& config) const
{
  return subtracting(config.get_events());
}

EventSet EventSet::subtracting(const UnfoldingEvent* e) const
{
  auto result = this->events_;
  result.erase(e);
  return EventSet(std::move(result));
}

void EventSet::insert(const UnfoldingEvent* e)
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

EventSet EventSet::make_union(const UnfoldingEvent* e) const
{
  auto result = this->events_;
  result.insert(e);
  return EventSet(std::move(result));
}

EventSet EventSet::make_union(const EventSet& other) const
{
  std::unordered_set<const UnfoldingEvent*> result = this->events_;

  for (const UnfoldingEvent* e : other.events_)
    result.insert(e);

  return EventSet(std::move(result));
}

EventSet EventSet::make_union(const Configuration& config) const
{
  return make_union(config.get_events());
}

EventSet EventSet::make_intersection(const EventSet& other) const
{
  std::unordered_set<const UnfoldingEvent*> result;

  for (const UnfoldingEvent* e : other.events_) {
    if (contains(e)) {
      result.insert(e);
    }
  }

  return EventSet(std::move(result));
}

EventSet EventSet::get_local_config() const
{
  return History(*this).get_all_events();
}

size_t EventSet::size() const
{
  return this->events_.size();
}

bool EventSet::empty() const
{
  return this->events_.empty();
}

bool EventSet::contains(const UnfoldingEvent* e) const
{
  return this->events_.find(e) != this->events_.end();
}

bool EventSet::contains_equivalent_to(const UnfoldingEvent* e) const
{
  return std::find_if(begin(), end(), [=](const UnfoldingEvent* e_in_set) { return *e == *e_in_set; }) != end();
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
  return contains(history) && is_conflict_free();
}

bool EventSet::contains(const History& history) const
{
  return std::all_of(history.begin(), history.end(), [=](const UnfoldingEvent* e) { return this->contains(e); });
}

bool EventSet::intersects(const History& history) const
{
  return std::any_of(history.begin(), history.end(), [=](const UnfoldingEvent* e) { return this->contains(e); });
}

bool EventSet::intersects(const EventSet& other) const
{
  return std::any_of(other.begin(), other.end(), [=](const UnfoldingEvent* e) { return this->contains(e); });
}

EventSet EventSet::get_largest_maximal_subset() const
{
  const History history(*this);
  return history.get_all_maximal_events();
}

bool EventSet::is_maximal() const
{
  // A set of events is maximal if no event from
  // the original set is ruled out when traversing
  // the history of the events
  return *this == this->get_largest_maximal_subset();
}

bool EventSet::is_conflict_free() const
{
  const auto begin = simgrid::xbt::variable_for_loop<const EventSet>{{*this}, {*this}};
  const auto end   = simgrid::xbt::variable_for_loop<const EventSet>();
  return std::none_of(begin, end, [=](const auto event_pair) {
    const UnfoldingEvent* e1 = *event_pair[0];
    const UnfoldingEvent* e2 = *event_pair[1];
    return e1->conflicts_with(e2);
  });
}

std::vector<const UnfoldingEvent*> EventSet::get_topological_ordering() const
{
  // This is essentially an implementation of detecting cycles
  // in a graph with coloring, except it makes a topological
  // ordering out of it
  if (empty()) {
    return std::vector<const UnfoldingEvent*>();
  }

  std::stack<const UnfoldingEvent*> event_stack;
  std::vector<const UnfoldingEvent*> topological_ordering;
  EventSet unknown_events = *this;
  EventSet temporarily_marked_events;
  EventSet permanently_marked_events;

  while (not unknown_events.empty()) {
    EventSet discovered_events;
    event_stack.push(*unknown_events.begin());

    while (not event_stack.empty()) {
      const UnfoldingEvent* evt = event_stack.top();
      discovered_events.insert(evt);

      if (not temporarily_marked_events.contains(evt)) {
        // If this event hasn't yet been marked, do
        // so now so that if we both see it
        // again in a child we can detect a cycle
        temporarily_marked_events.insert(evt);

        EventSet immediate_causes = evt->get_immediate_causes();
        if (not immediate_causes.empty() && immediate_causes.is_subset_of(temporarily_marked_events)) {
          throw std::invalid_argument("Attempted to perform a topological sort on a configuration "
                                      "whose contents contain a cycle. The configuration (and the graph "
                                      "connecting all of the events) is an invalid event structure");
        }
        immediate_causes.subtract(discovered_events);
        immediate_causes.subtract(permanently_marked_events);
        std::for_each(immediate_causes.begin(), immediate_causes.end(),
                      [&event_stack](const UnfoldingEvent* cause) { event_stack.push(cause); });
      } else {
        unknown_events.remove(evt);
        temporarily_marked_events.remove(evt);
        permanently_marked_events.insert(evt);

        // In moving this event to the end of the list,
        // we are saying this events "happens before" other
        // events that are added later.
        if (this->contains(evt)) {
          topological_ordering.push_back(evt);
        }

        // Only now do we remove the event, i.e. once
        // we've processed the same event twice
        event_stack.pop();
      }
    }
  }
  return topological_ordering;
}

std::vector<const UnfoldingEvent*> EventSet::get_topological_ordering_of_reverse_graph() const
{
  // The implementation exploits the property that
  // a topological sorting S^R of the reverse graph G^R
  // of some graph G is simply the reverse of any
  // topological sorting S of G.
  auto topological_events = get_topological_ordering();
  std::reverse(topological_events.begin(), topological_events.end());
  return topological_events;
}

std::string EventSet::to_string() const
{
  std::string contents;

  for (const auto* event : *this) {
    contents += event->to_string();
    contents += " + ";
  }

  return contents;
}

std::vector<const UnfoldingEvent*> EventSet::move_into_vector() const&&
{
  std::vector<const UnfoldingEvent*> contents;
  contents.reserve(size());

  for (auto&& event : *this) {
    contents.push_back(event);
  }

  return contents;
}

} // namespace simgrid::mc::udpor
