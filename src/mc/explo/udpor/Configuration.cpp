/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/udpor/Configuration.hpp"
#include "src/mc/explo/udpor/Comb.hpp"
#include "src/mc/explo/udpor/History.hpp"
#include "src/mc/explo/udpor/Unfolding.hpp"
#include "src/mc/explo/udpor/UnfoldingEvent.hpp"
#include "src/mc/explo/udpor/maximal_subsets_iterator.hpp"
#include "xbt/asserts.h"

#include <algorithm>
#include <stdexcept>

namespace simgrid::mc::udpor {

Configuration::Configuration(std::initializer_list<const UnfoldingEvent*> events)
    : Configuration(EventSet(std::move(events)))
{
}

Configuration::Configuration(const UnfoldingEvent* e) : Configuration(e->get_history())
{
  // The local configuration should always be a valid configuration. We
  // check the invariant regardless as a sanity check
}

Configuration::Configuration(const EventSet& events) : events_(events)
{
  if (!events_.is_valid_configuration()) {
    throw std::invalid_argument("The events do not form a valid configuration");
  }
}

Configuration::Configuration(const History& history) : Configuration(history.get_all_events()) {}

void Configuration::add_event(const UnfoldingEvent* e)
{
  if (e == nullptr) {
    throw std::invalid_argument("Expected a nonnull `UnfoldingEvent*` but received NULL instead");
  }

  if (this->events_.contains(e)) {
    return;
  }

  // Preserves the property that the configuration is conflict-free
  if (e->conflicts_with(*this)) {
    throw std::invalid_argument("The newly added event conflicts with the events already "
                                "contained in the configuration. Adding this event violates "
                                "the property that a configuration is conflict-free");
  }

  this->events_.insert(e);
  this->newest_event = e;

  // Preserves the property that the configuration is causally closed
  if (auto history = History(e); !this->events_.contains(history)) {
    throw std::invalid_argument("The newly added event has dependencies "
                                "which are missing from this configuration");
  }
}

bool Configuration::is_compatible_with(const UnfoldingEvent* e) const
{
  return not e->conflicts_with(*this);
}

bool Configuration::is_compatible_with(const History& history) const
{
  return std::none_of(history.begin(), history.end(),
                      [&](const UnfoldingEvent* e) { return e->conflicts_with(*this); });
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
  EventSet minimally_reproducible_events = EventSet();

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
  const auto D_hat = [&]() {
    const size_t size = std::min(k, D.size());
    std::vector<const UnfoldingEvent*> D_hat(size);
    // Potentially select intelligently here (e.g. perhaps pick events
    // with transitions that we know are totally independent)...
    //
    // For now, simply pick the first `k` events (any subset suffices)
    std::copy_n(D.begin(), size, D_hat.begin());
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
    for (unsigned i = 0; i < k; i++) {
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
    return EventSet(std::move(events));
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

} // namespace simgrid::mc::udpor
