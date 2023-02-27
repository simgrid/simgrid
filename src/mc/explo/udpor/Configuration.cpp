/* Copyright (c) 2008-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/mc/explo/udpor/Configuration.hpp"
#include "src/mc/explo/udpor/History.hpp"
#include "src/mc/explo/udpor/UnfoldingEvent.hpp"
#include "xbt/asserts.h"

#include <algorithm>
#include <stack>
#include <stdexcept>

namespace simgrid::mc::udpor {

Configuration::Configuration(std::initializer_list<UnfoldingEvent*> events) : Configuration(EventSet(std::move(events)))
{
}

Configuration::Configuration(const EventSet& events) : events_(events)
{
  if (!events_.is_valid_configuration()) {
    throw std::invalid_argument("The events do not form a valid configuration");
  }
}

void Configuration::add_event(UnfoldingEvent* e)
{
  if (e == nullptr) {
    throw std::invalid_argument("Expected a nonnull `UnfoldingEvent*` but received NULL instead");
  }

  if (this->events_.contains(e)) {
    return;
  }

  this->events_.insert(e);
  this->newest_event = e;

  // Preserves the property that the configuration is valid
  History history(e);
  if (!this->events_.contains(history)) {
    throw std::invalid_argument("The newly added event has dependencies "
                                "which are missing from this configuration");
  }
}

std::vector<UnfoldingEvent*> Configuration::get_topologically_sorted_events() const
{
  if (events_.empty()) {
    return std::vector<UnfoldingEvent*>();
  }

  std::stack<UnfoldingEvent*> event_stack;
  std::vector<UnfoldingEvent*> topological_ordering;
  EventSet unknown_events = events_;
  EventSet temporarily_marked_events;
  EventSet permanently_marked_events;

  while (not unknown_events.empty()) {
    EventSet discovered_events;
    event_stack.push(*unknown_events.begin());

    while (not event_stack.empty()) {
      UnfoldingEvent* evt = event_stack.top();
      discovered_events.insert(evt);

      if (not temporarily_marked_events.contains(evt)) {
        // If this event hasn't yet been marked, do
        // so now so that if we see it again in a child we can
        // detect a cycle and if we see it again here
        // we can detect that the node is re-processed
        temporarily_marked_events.insert(evt);

        EventSet immediate_causes = evt->get_immediate_causes();
        if (!immediate_causes.empty() && immediate_causes.is_subset_of(temporarily_marked_events)) {
          throw std::invalid_argument("Attempted to perform a topological sort on a configuration "
                                      "whose contents contain a cycle. The configuration (and the graph "
                                      "connecting all of the events) is an invalid event structure");
        }
        immediate_causes.subtract(discovered_events);
        immediate_causes.subtract(permanently_marked_events);
        const EventSet undiscovered_causes = std::move(immediate_causes);

        for (const auto cause : undiscovered_causes) {
          event_stack.push(cause);
        }
      } else {
        // Mark this event as:
        // 1. discovered across all DFSs performed
        // 2. permanently marked
        // 3. part of the topological search
        unknown_events.remove(evt);
        temporarily_marked_events.remove(evt);
        permanently_marked_events.insert(evt);

        // In moving this event to the end of the list,
        // we are saying this events "happens before" other
        // events that are added later.
        topological_ordering.push_back(evt);

        // Only now do we remove the event, i.e. once
        // we've processed the same event again
        event_stack.pop();
      }
    }
  }
  return topological_ordering;
}

std::vector<UnfoldingEvent*> Configuration::get_topologically_sorted_events_of_reverse_graph() const
{
  // The method exploits the property that
  // a topological sorting S^R of the reverse graph G^R
  // of some graph G is simply the reverse of any
  // topological sorting S of G.
  auto topological_events = get_topologically_sorted_events();
  std::reverse(topological_events.begin(), topological_events.end());
  return topological_events;
}

std::unique_ptr<CompatibilityGraph>
Configuration::make_compatibility_graph_filtered_on(std::function<bool(const UnfoldingEvent*)> pred) const
{
  auto G = std::make_unique<CompatibilityGraph>();

  struct UnfoldingEventSearchData {
    int immediate_children_count                          = 0;
    CompatibilityGraphNode* potential_placement           = nullptr;
    std::unordered_set<CompatibilityGraphNode*> conflicts = std::unordered_set<CompatibilityGraphNode*>();
  };
  std::unordered_map<UnfoldingEvent*, UnfoldingEventSearchData> search_data;

  for (auto* e : get_topologically_sorted_events_of_reverse_graph()) {

    // 1. Figure out where to place `e` in `G`

    // Determine which nodes in the graph are in conflict
    // with this event. These nodes would have been added by child
    // events while iterating over the topological ordering of the reverse graph

    const auto e_search_data_loc    = search_data.find(e);
    const bool e_has_no_search_data = e_search_data_loc == search_data.end();
    const auto e_search_data = e_has_no_search_data ? UnfoldingEventSearchData() : std::move(e_search_data_loc->second);

    const auto& e_conflicts           = e_search_data.conflicts;
    const auto& e_potential_placement = e_search_data.potential_placement;
    const auto e_child_count          = e_search_data.immediate_children_count;

    const bool e_should_appear          = pred(e);
    CompatibilityGraphNode* e_placement = nullptr;

    if (e_should_appear) {
      // The justification is as follows:
      //
      // e_has_no_search_data:
      //  The event `e` is a leaf node, so there are no prior
      //  nodes in `G` to join
      //
      // child_count >= 2:
      //  If there are two or more events that this event causes,
      //  then we certainly must be part of a compatibility
      //  graph node that conflicts with each of our children
      //
      // e_potential_placement == nullptr:
      //  If nobody told us about a placement and yet still have search
      //  data, this means means that our child `C` had more than one child itself,
      //  so it we could not have moved into `C`'s _potential_ placement.
      const bool new_placement_required =
          e_has_no_search_data || e_child_count >= 2 || e_potential_placement == nullptr;

      if (new_placement_required) {
        auto new_graph_node = std::make_unique<CompatibilityGraphNode>(e_conflicts, EventSet({e}));
        e_placement         = new_graph_node.get();
        G->insert(std::move(new_graph_node));
      } else {
        xbt_assert(e_child_count == 1, "An event was informed by an immediate child of placement in "
                                       "the same compatibility graph node, yet the child did not inform "
                                       "the parent about its presence");
        // A child event told us this node can be in the
        // same compatibility node in the graph G. Add ourselves now
        e_placement = e_potential_placement;
        e_placement->add_event(e);
      }
    }

    // 2. Update the children of `e`

    const EventSet& e_immediate_causes = e->get_immediate_causes();

    // If there is only a single ancestor, then it MAY BE in
    // the same "chain" of events as us. Note that the ancestor must
    // also have only a single child (see the note on `new_placement_required`).
    //
    // If there is more than one child, then each child is in conflict with `e`
    // so we don't potentially place it
    if (e_immediate_causes.size() == 1) {
      UnfoldingEvent* only_ancestor = *e_immediate_causes.begin();

      // If `e` is included in the graph, forward its placement on to
      // the sole child. Otherwise attempt to forward `e`'s _potential_
      // (potential is stressed) placement. We can only forward `e`'s
      // potential placement iff `e` has only a single child; for if
      // `e` had more children, then our sole ancestor would conflict with
      // each one of `e`'s children and thus couldn't be in the same group
      // as any of them
      if (e_should_appear) {
        search_data[only_ancestor].potential_placement = e_placement;
      } else {
        search_data[only_ancestor].potential_placement = e_child_count == 1 ? e_potential_placement : nullptr;
      }
    }

    // Our ancestors conflict with everyone `e` does else PLUS `e` itself
    // ONLY IF e actually was placed
    auto parent_conflicts = std::move(e_conflicts);
    if (e_should_appear) {
      parent_conflicts.insert(e_placement);
    }
    for (auto* cause : e_immediate_causes) {
      search_data[cause].immediate_children_count += 1;

      for (auto parent_conflict : parent_conflicts) {
        search_data[cause].conflicts.insert(parent_conflict);
      }
    }

    // This event will only ever be seen once in the
    // topological ordering. Hence, its resources do not
    // need to be kept around
    search_data.erase(e);
  }

  return G;
}

std::unique_ptr<CompatibilityGraph> Configuration::make_compatibility_graph() const
{
  return make_compatibility_graph_filtered_on([=](const UnfoldingEvent*) { return true; });
}

} // namespace simgrid::mc::udpor
