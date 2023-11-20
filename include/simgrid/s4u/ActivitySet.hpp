/* Copyright (c) 2006-2023. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_ACTIVITYSET_HPP
#define SIMGRID_S4U_ACTIVITYSET_HPP

#include <simgrid/forward.h>
#include <simgrid/s4u/Activity.hpp>

#include <vector>

namespace simgrid {

extern template class XBT_PUBLIC xbt::Extendable<s4u::ActivitySet>;

namespace s4u {
/** @brief ActivitiesSet
 *
 * This class is a container of activities, allowing to wait for the completion of any or all activities in the set.
 * This is somehow similar to the select(2) system call under UNIX, allowing you to wait for the next event about these
 * activities.
 */
class XBT_PUBLIC ActivitySet : public xbt::Extendable<ActivitySet> {
  std::atomic_int_fast32_t refcount_{1};
  std::vector<ActivityPtr> activities_; // Use vectors, not sets for better reproductibility accross architectures
  std::vector<ActivityPtr> failed_activities_;

  void handle_failed_activities();

public:
  ActivitySet()  = default;
  ActivitySet(const std::vector<ActivityPtr> init) : activities_(init) {}
  ~ActivitySet() = default;

  /** Add an activity to the set */
  void push(ActivityPtr a) { activities_.push_back(a); }
  /** Remove that activity from the set (no-op if the activity is not in the set) */
  void erase(ActivityPtr a);

  /** Get the amount of activities in the set. Failed activities (if any) are not counted */
  int size() { return activities_.size(); }
  /** Return whether the set is empty. Failed activities (if any) are not counted */
  int empty() { return activities_.empty(); }

  /** Wait for the completion of all activities in the set, but not longer than the provided timeout
   *
   * On timeout, an exception is raised.
   *
   * In any case, the completed activities remain in the set. Use test_any() to retrieve them.
   */
  void wait_all_for(double timeout);
  /** Wait for the completion of all activities in the set. The set is NOT emptied afterward. */
  void wait_all() { wait_all_for(-1); }
  /** Returns the first terminated activity if any, or ActivityPtr(nullptr) if no activity is terminated */
  ActivityPtr test_any();

  /** Wait for the completion of one activity from the set, but not longer than the provided timeout.
   *
   *  See wait_any() for details.
   *
   * @return the first terminated activity, which is automatically removed from the set.
   */

  ActivityPtr wait_any_for(double timeout);
  /** Wait for the completion of one activity from the set.
   *
   * If an activity fails during that time, an exception is raised, and the failed exception is marked as failed in the
   * set. Use get_failed_activity() to retrieve it.
   *
   * If more than one activity failed, the other ones are also removed from the set. Use get_failed_activity() several
   * time to retrieve them all.
   *
   * @return the first terminated activity, which is automatically removed from the set. If more than one activity
   * terminated at the same timestamp, then the other ones are still in the set. Use either test_any() or wait_any() to
   * retrieve the other ones.
   */
  ActivityPtr wait_any() { return wait_any_for(-1); }

  /** Return one of the failed activity of the set that was revealed during the previous wait operation, or
   * ActivityPtr() if no failed activity exist in the set. */
  ActivityPtr get_failed_activity();
  /** Return whether the set contains any failed activity. */
  bool has_failed_activities() { return not failed_activities_.empty(); }

  // boost::intrusive_ptr<ActivitySet> support:
  friend void intrusive_ptr_add_ref(ActivitySet* as)
  {
    XBT_ATTRIB_UNUSED auto previous = as->refcount_.fetch_add(1);
    xbt_assert(previous != 0);
  }

  friend void intrusive_ptr_release(ActivitySet* as)
  {
    if (as->refcount_.fetch_sub(1) == 1)
      delete as;
  }
};

} // namespace s4u
} // namespace simgrid

#endif
