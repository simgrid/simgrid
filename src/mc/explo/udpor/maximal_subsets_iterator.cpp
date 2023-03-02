#include "src/mc/explo/udpor/maximal_subsets_iterator.hpp"
#include "src/mc/explo/udpor/UnfoldingEvent.hpp"
#include "xbt/asserts.h"

#include <algorithm>

namespace simgrid::mc::udpor {

void maximal_subsets_iterator::increment()
{
  if (current_maximal_set == std::nullopt) {
    return;
  }

  if (topological_ordering.empty()) {
    // Stop immediately if there's nothing to search
    current_maximal_set = std::nullopt;
    return;
  }

  const auto next_event_ref = [&]() {
    if (!has_started_searching) {
      has_started_searching = true;
      return bookkeeper.find_next_candidate_event(topological_ordering.begin(), topological_ordering.end());
    } else {
      return continue_traversal_of_maximal_events_tree();
    }
  }();

  if (next_event_ref == topological_ordering.end()) {
    current_maximal_set = std::nullopt;
    return;
  }

  // We found some other event `e'` which is not in causally related with anything
  // that currently exists in `current_maximal_set`, so add it in
  add_element_to_current_maximal_set(*next_event_ref);
  backtrack_points.push(next_event_ref);
}

maximal_subsets_iterator::topological_order_position
maximal_subsets_iterator::continue_traversal_of_maximal_events_tree()
{
  // Nothing needs to be done if there isn't anyone to search for...
  if (backtrack_points.empty()) {
    return topological_ordering.end();
  }

  // 1. First, check if we can keep expanding from the
  // maximal set that we currently have
  {
    // This is an iterator which points to the latest event `e` that
    // was added to what is currently the maximal set
    const auto latest_event_ref = backtrack_points.top();

    // Look for the next event to test with what we currently
    // have based on the conflicts we've already kept track of.
    //
    // NOTE: We only need to search FROM `e` and not
    // from the beginning of the topological sort. The fact that the
    // set is topologically ordered ensures that removing `e`
    // will not change whether or not to now allow someone before `e`
    // in the ordering (otherwise, they would have to be in `e`'s history
    // and therefore would come after `e`)
    const auto next_event_ref = bookkeeper.find_next_candidate_event(latest_event_ref, topological_ordering.end());

    // If we can expand from what we currently have, we can stop
    if (next_event_ref != topological_ordering.end()) {
      return next_event_ref;
    }
  }

  // Otherwise, we backtrack: we repeatedly pop off events that we know we
  // are finished with
  while (not backtrack_points.empty()) {
    // Note: it is important to remove the element FIRST before performing
    // the search, as removal may enable dependencies of `e` to be selected
    const auto latest_event_ref = backtrack_points.top();
    remove_element_from_current_maximal_set(*latest_event_ref);
    backtrack_points.pop();

    // We begin the search AFTER the event we popped: we only want
    // to consider those events that could be added AFTER `e` and
    // not `e` itself again
    const auto next_event_ref = bookkeeper.find_next_candidate_event(latest_event_ref + 1, topological_ordering.end());
    if (next_event_ref != topological_ordering.end()) {
      return next_event_ref;
    }
  }
  return topological_ordering.end();
}

bool maximal_subsets_iterator::bookkeeper::is_candidate_event(const UnfoldingEvent* e) const
{
  // The event must pass the filter, if it exists
  if (filter_function.has_value() && not filter_function.value()(e)) {
    return false;
  }

  if (const auto e_count = event_counts.find(e); e_count != event_counts.end()) {
    return e_count->second == 0;
  }
  return true;
}

void maximal_subsets_iterator::add_element_to_current_maximal_set(const UnfoldingEvent* e)
{
  xbt_assert(current_maximal_set.has_value(), "Attempting to add an event to the maximal set "
                                              "when iteration has completed. This indicates that "
                                              "the termination condition for the iterator is broken");
  current_maximal_set.value().insert(e);
  bookkeeper.mark_included_in_maximal_set(e);
}

void maximal_subsets_iterator::remove_element_from_current_maximal_set(const UnfoldingEvent* e)
{
  xbt_assert(current_maximal_set.has_value(), "Attempting to remove an event to the maximal set "
                                              "when iteration has completed. This indicates that "
                                              "the termination condition for the iterator is broken");
  current_maximal_set.value().remove(e);
  bookkeeper.mark_removed_from_maximal_set(e);
}

maximal_subsets_iterator::topological_order_position
maximal_subsets_iterator::bookkeeper::find_next_candidate_event(topological_order_position first,
                                                                topological_order_position last) const
{
  return std::find_if(first, last, [&](const UnfoldingEvent* e) { return is_candidate_event(e); });
}

void maximal_subsets_iterator::bookkeeper::mark_included_in_maximal_set(const UnfoldingEvent* e)
{
  const auto e_history = e->get_history();
  for (const auto e_hist : e_history) {
    event_counts[e_hist]++;
  }
}

void maximal_subsets_iterator::bookkeeper::mark_removed_from_maximal_set(const UnfoldingEvent* e)
{
  const auto e_history = e->get_history();
  for (const auto e_hist : e_history) {
    xbt_assert(event_counts.find(e_hist) != event_counts.end(),
               "Invariant Violation: Attempted to remove an event which was not previously added");
    xbt_assert(event_counts[e_hist] > 0, "Invariant Violation: An event `e` had a count of `0` at this point "
                                         "of the bookkeeping, which means that it is a candidate maximal event. "
                                         "Yet some event that `e'` which contains `e` in its history was removed "
                                         "first. This incidates that the topological sorting of events of the "
                                         "configuration has failed and should be investigated first");
    event_counts[e_hist]--;
  }
}

} // namespace simgrid::mc::udpor