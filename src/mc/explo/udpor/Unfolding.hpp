/* Copyright (c) 2007-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_MC_UDPOR_UNFOLDING_HPP
#define SIMGRID_MC_UDPOR_UNFOLDING_HPP

#include "src/mc/explo/udpor/EventSet.hpp"
#include "src/mc/explo/udpor/UnfoldingEvent.hpp"
#include "src/mc/explo/udpor/udpor_forward.hpp"

#include <memory>
#include <unordered_map>

namespace simgrid::mc::udpor {

class Unfolding {
public:
  Unfolding()                       = default;
  Unfolding& operator=(Unfolding&&) = default;
  Unfolding(Unfolding&&)            = default;

  auto begin() const { return this->event_handles.begin(); }
  auto end() const { return this->event_handles.end(); }
  auto cbegin() const { return this->event_handles.cbegin(); }
  auto cend() const { return this->event_handles.cend(); }
  size_t size() const { return this->event_handles.size(); }
  bool empty() const { return this->event_handles.empty(); }

  /**
   * @brief Moves an event from UDPOR's global set `U` to
   * the global set `G`
   */
  void mark_finished(const UnfoldingEvent* e);

  /**
   * @brief Moves all events in a set from UDPOR's global
   * set `U` to the global set `G`
   */
  void mark_finished(const EventSet& events);

  /// @brief Adds a new event `e` to the Unfolding if that
  /// event is not equivalent to any of those already contained
  /// in the unfolding
  const UnfoldingEvent* insert(std::unique_ptr<UnfoldingEvent> e);

  /**
   * @brief Informs the unfolding of a (potentially) new event
   *
   * The unfolding of a concurrent program is a well-defined
   * structure. Given the labeled transition system (LTS) of
   * a program, the unfolding of that program can be determined
   * algorithmically. However, UDPOR does not a priori know the structure of the
   * unfolding as it performs its exploration. Thus, events in the
   * unfolding are "discovered" as they are encountered, specifically
   * when computing the extension sets of the configurations that
   * UDPOR decides to search.
   *
   * This lends itself to the following problem: the extension sets
   * of two different configurations may overlap one another. That
   * is, for two configurations C and C' explored by UDPOR where C != C',
   *
   * ex(C) - ex(C') != empty
   *
   * Hence, when extending both `C` and `C'`, any events contained in
   * the intersection of ex(C) and ex(C') will be attempted to be added
   * twice. The unfolding will notice that these events have already
   * been added and simply return the event already added to the unfolding
   *
   * @tparam ...Args arguments passed to the `UnfoldingEvent` constructor
   * @return the handle to either the newly created event OR
   * to an equivalent event that was already noted by the unfolding
   * at some point in the past
   */
  template <typename... Args> const UnfoldingEvent* discover_event(Args&&... args)
  {
    auto candidate_event = std::make_unique<UnfoldingEvent>(std::forward<Args>(args)...);
    return insert(std::move(candidate_event));
  }

  /// @brief Computes "#ⁱ_U(e)" for the given event, where `U` is the set
  /// of the events in this unfolding
  EventSet get_immediate_conflicts_of(const UnfoldingEvent*) const;

private:
  /**
   * @brief All of the events that are currently are a part of the unfolding
   *
   * @invariant Each unfolding event maps itself to the owner of that event,
   * i.e. the unique pointer that manages the data at the address. The Unfolding owns all
   * of the addresses that are referenced by EventSet instances and Configuration
   * instances. UDPOR guarantees that events are persisted for as long as necessary
   */
  std::unordered_map<const UnfoldingEvent*, std::unique_ptr<UnfoldingEvent>> global_events_;

  /**
   * @brief: The collection of events in the unfolding
   *
   * @invariant: All of the events in this set are elements of `global_events_`
   * and is kept updated at the same time as `global_events_`
   *
   * @note: This is for the convenience of iteration over the unfolding
   */
  EventSet event_handles;

  /**
   * @brief: The collection of events in the unfolding that are "important"
   */
  EventSet U;

  /**
   * @brief The "irrelevant" portions of the unfolding that do not need to be kept
   * around to ensure that UDPOR functions correctly
   *
   * The set `G` is another global variable maintained by the UDPOR algorithm which
   * is used to keep track of all events which used to be important to UDPOR.
   */
  EventSet G;
};

} // namespace simgrid::mc::udpor
#endif
