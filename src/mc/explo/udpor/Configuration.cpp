/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/udpor/Configuration.hpp"
#include "src/mc/explo/udpor/Comb.hpp"
#include "src/mc/explo/udpor/History.hpp"
#include "src/mc/explo/udpor/Unfolding.hpp"
#include "src/mc/explo/udpor/UnfoldingEvent.hpp"
#include "src/mc/explo/udpor/maximal_subsets_iterator.hpp"
#include "src/xbt/utils/iter/variable_for_loop.hpp"
#include "xbt/asserts.h"

#include <algorithm>
#include <stdexcept>

namespace simgrid::mc::udpor {

Configuration::Configuration(std::initializer_list<const UnfoldingEvent*> events)
    : Configuration(EventSet(std::move(events)))
{
}

Configuration::Configuration(const UnfoldingEvent* e) : Configuration(e->get_local_config())
{
  // The local configuration should always be a valid configuration. We
  // check the invariant regardless as a sanity check
}

Configuration::Configuration(const History& history) : Configuration(history.get_all_events()) {}

Configuration::Configuration(const EventSet& events) : events_(events)
{
  if (not events_.is_valid_configuration()) {
    throw std::invalid_argument("The events do not form a valid configuration");
  }

  // Since we add in topological order under `<`, we know that the "most-recent"
  // transition executed by each actor will appear last
  for (const UnfoldingEvent* e : get_topologically_sorted_events()) {
    this->latest_event_mapping[e->get_actor()] = e;
  }
}

void Configuration::add_event(const UnfoldingEvent* e)
{
  if (e == nullptr) {
    throw std::invalid_argument("Expected a nonnull `UnfoldingEvent*` but received NULL instead");
  }

  // The event is already a member of the configuration: there's
  // nothing to do in this case
  if (this->events_.contains(e)) {
    return;
  }

  // Preserves the property that the configuration is conflict-free
  if (e->conflicts_with_any(this->events_)) {
    throw std::invalid_argument("The newly added event conflicts with the events already "
                                "contained in the configuration. Adding this event violates "
                                "the property that a configuration is conflict-free");
  }

  this->events_.insert(e);
  this->newest_event                         = e;
  this->latest_event_mapping[e->get_actor()] = e;

  // Preserves the property that the configuration is causally closed
  if (auto history = History(e); not this->events_.contains(history)) {
    throw std::invalid_argument("The newly added event has dependencies "
                                "which are missing from this configuration");
  }
}

bool Configuration::is_compatible_with(const UnfoldingEvent* e) const
{
  // 1. `e`'s history must be contained in the configuration;
  // otherwise adding the event would violate the invariant
  // that a configuration is causally-closed
  //
  // 2. `e` itself must not conflict with any events of
  // the configuration; otherwise adding the event would
  // violate the invariant that a configuration is conflict-free
  return contains(e->get_history()) && (not e->conflicts_with_any(this->events_));
}

bool Configuration::is_compatible_with(const History& history) const
{
  // Note: We don't need to check if the `C` will be causally-closed
  // after adding `history` to it since a) `C` itself is already
  // causally-closed and b) the history is already causally closed
  const auto event_diff = history.get_event_diff_with(*this);

  // The events that are contained outside of the configuration
  // must themselves be free of conflicts.
  if (not event_diff.is_conflict_free()) {
    return false;
  }

  // Now we need only ensure that there are no conflicts
  // between events of the configuration and the events
  // that lie outside of the configuration. There is no
  // need to check if there are conflicts in `C`: we already
  // know that it's conflict free
  const auto begin = simgrid::xbt::variable_for_loop<const EventSet>{{event_diff}, {this->events_}};
  const auto end   = simgrid::xbt::variable_for_loop<const EventSet>();
  return std::none_of(begin, end, [=](const auto event_pair) {
    const UnfoldingEvent* e1 = *event_pair[0];
    const UnfoldingEvent* e2 = *event_pair[1];
    return e1->conflicts_with(e2);
  });
}

std::vector<const UnfoldingEvent*> Configuration::get_topologically_sorted_events() const
{
  return this->events_.get_topological_ordering();
}

std::vector<const UnfoldingEvent*> Configuration::get_topologically_sorted_events_of_reverse_graph() const
{
  return this->events_.get_topological_ordering_of_reverse_graph();
}

EventSet Configuration::get_minimally_reproducible_events() const
{
  // The implementation exploits the following observations:
  //
  // To select the smallest reproducible set of events, we want
  // to pick events that "knock out" a lot of others. Furthermore,
  // we need to ensure that the events furthest down in the
  // causality graph are also selected. If you combine these ideas,
  // you're basically left with traversing the set of maximal
  // subsets of C! And we have an iterator for that already!
  //
  // The next observation is that the moment we don't increase in size
  // the current maximal set (or decrease the number of events),
  // we know that the prior set `S` covered the entire history of C and
  // was maximal. Subsequent sets will miss events earlier in the
  // topological ordering that appear in `S`
  EventSet minimally_reproducible_events;

  for (const auto& maximal_set : maximal_subsets_iterator_wrapper<Configuration>(*this)) {
    if (maximal_set.size() > minimally_reproducible_events.size()) {
      minimally_reproducible_events = maximal_set;
    } else {
      // The moment we see the iterator generate a set of size
      // that is not monotonically increasing, we can stop:
      // the set prior was the minimally-reproducible one
      return minimally_reproducible_events;
    }
  }
  return minimally_reproducible_events;
}

std::optional<Configuration> Configuration::compute_alternative_to(const EventSet& D, const Unfolding& U) const
{
  // A full alternative can be computed by checking against everything in D
  return compute_k_partial_alternative_to(D, U, D.size());
}

std::optional<Configuration> Configuration::compute_k_partial_alternative_to(const EventSet& D, const Unfolding& U,
                                                                             size_t k) const
{
  // 1. Select k (of |D|, whichever is smaller) arbitrary events e_1, ..., e_k from D
  const size_t k_alt_size = std::min(k, D.size());
  const auto D_hat        = [&k_alt_size, &D]() {
    std::vector<const UnfoldingEvent*> D_hat(k_alt_size);
    // TODO: Since any subset suffices for computing `k`-partial alternatives,
    // potentially select intelligently here (e.g. perhaps pick events
    // with transitions that we know are totally independent). This may be
    // especially important if the enumeration is the slowest part of
    // UDPOR
    //
    // For now, simply pick the first `k` events
    std::copy_n(D.begin(), k_alt_size, D_hat.begin());
    return D_hat;
  }();

  // 2. Build a U-comb <s_1, ..., s_k> of size k, where spike `s_i` contains
  // all events in conflict with `e_i`
  //
  // 3. EXCEPT those events e' for which [e'] + C is not a configuration or
  // [e'] intersects D
  //
  // NOTE: This is an expensive operation as we must traverse the entire unfolding
  // and compute `C.is_compatible_with(History)` for every event in the structure :/.
  // A later performance improvement would be to incorporate the work of Nguyen et al.
  // into SimGrid which associated additonal data structures with each unfolding event.
  // Since that is a rather complicated addition, we defer it to a later time...
  Comb comb(k);

  for (const auto* e : U) {
    for (size_t i = 0; i < k_alt_size; i++) {
      const UnfoldingEvent* e_i = D_hat[i];
      if (const auto e_local_config = History(e);
          e_i->conflicts_with(e) and (not D.intersects(e_local_config)) and is_compatible_with(e_local_config)) {
        comb[i].push_back(e);
      }
    }
  }

  // 4. Find any such combination <e_1', ..., e_k'> in comb satisfying
  // ~(e_i' # e_j') for i != j
  //
  // NOTE: This is a VERY expensive operation: it enumerates all possible
  // ways to select an element from each spike. Unfortunately there's no
  // way around the enumeration, as computing a full alternative in general is
  // NP-complete (although computing the k-partial alternative is polynomial in
  // the number of events)
  const auto map_events = [](const std::vector<Spike::const_iterator>& spikes) {
    std::vector<const UnfoldingEvent*> events;
    for (const auto& event_in_spike : spikes) {
      events.push_back(*event_in_spike);
    }
    return EventSet(events);
  };
  const auto alternative =
      std::find_if(comb.combinations_begin(), comb.combinations_end(),
                   [&map_events](const auto& vector) { return map_events(vector).is_conflict_free(); });

  // No such alternative exists
  if (alternative == comb.combinations_end()) {
    return std::nullopt;
  }

  // 5. J := [e_1] + [e_2] + ... + [e_k] is a k-partial alternative
  return Configuration(History(map_events(*alternative)));
}

std::optional<const UnfoldingEvent*> Configuration::get_latest_event_of(aid_t aid) const
{
  if (const auto latest_event = latest_event_mapping.find(aid); latest_event != latest_event_mapping.end()) {
    return std::optional<const UnfoldingEvent*>{latest_event->second};
  }
  return std::nullopt;
}

std::optional<const Transition*> Configuration::get_latest_action_of(aid_t aid) const
{
  if (const auto latest_event = get_latest_event_of(aid); latest_event.has_value()) {
    return std::optional<const Transition*>{latest_event.value()->get_transition()};
  }
  return std::nullopt;
}

} // namespace simgrid::mc::udpor
