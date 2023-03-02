#include "src/mc/explo/udpor/maximal_subsets_iterator.hpp"
#include "src/mc/explo/udpor/UnfoldingEvent.hpp"
#include "xbt/asserts.h"

#include <algorithm>

namespace simgrid::mc::udpor {

void maximal_subsets_iterator::increment()
{
  // Until we discover otherwise, we default to being done
  auto next_event_ref = topological_ordering.end();

  while (not backtrack_points.empty()) {
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
    next_event_ref = bookkeeper.find_next_event(latest_event_ref, topological_ordering.end());

    // If we can't find another event to add after `e`,
    // then we retry after first removing the latest event.
    // This effectively tests "check now with all combinations that
    // exclude the latest event".
    //
    // Note: it is important to remove the element FIRST before performing
    // the second search, as removal may enable dependencies of `e` to be selected
    if (next_event_ref == topological_ordering.end()) {
      remove_element_from_current_maximal_set(*latest_event_ref);
      backtrack_points.pop();

      // We begin the search AFTER the event we popped: we only want
      // to consider those events that could be added AFTER `e` and
      // not `e` itself again
      next_event_ref = bookkeeper.find_next_event(latest_event_ref + 1, topological_ordering.end());

      // If we finally found some event, we can stop
      if (next_event_ref != topological_ordering.end()) {
        break;
      }
    }
  }

  // If after all of the backtracking we still have no luck, we've finished
  if (next_event_ref == topological_ordering.end()) {
    return;
  }

  // Otherwise we found some other event `e'` which is not in conflict with anything
  // that currently exists in `current_maximal_set`. Add it in and perform
  // some more bookkeeping
  UnfoldingEvent* next_event = *next_event_ref;
  add_element_to_current_maximal_set(next_event);
  backtrack_points.push(next_event_ref);
}

void maximal_subsets_iterator::add_element_to_current_maximal_set(UnfoldingEvent* e)
{
  current_maximal_set.insert(e);
  bookkeeper.mark_included_in_maximal_set(e);
}

void maximal_subsets_iterator::remove_element_from_current_maximal_set(UnfoldingEvent* e)
{
  current_maximal_set.remove(e);
  bookkeeper.mark_removed_from_maximal_set(e);
}

maximal_subsets_iterator::topological_order_position
maximal_subsets_iterator::bookkeeper::find_next_event(topological_order_position first,
                                                      topological_order_position last) const
{
  return std::find_if(first, last, [&](UnfoldingEvent* e) { return is_candidate_event(e); });
}

bool maximal_subsets_iterator::bookkeeper::is_candidate_event(UnfoldingEvent* e) const
{
  if (const auto e_count = event_counts.find(e); e_count != event_counts.end()) {
    return e_count->second == 0;
  }
  return true;
}

void maximal_subsets_iterator::bookkeeper::mark_included_in_maximal_set(UnfoldingEvent* e)
{
  const auto e_history = e->get_history();
  for (const auto e_hist : e_history) {
    event_counts[e_hist]++;
  }
}

void maximal_subsets_iterator::bookkeeper::mark_removed_from_maximal_set(UnfoldingEvent* e)
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