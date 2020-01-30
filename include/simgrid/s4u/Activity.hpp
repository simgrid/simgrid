/* Copyright (c) 2006-2020. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_ACTIVITY_HPP
#define SIMGRID_S4U_ACTIVITY_HPP

#include "xbt/asserts.h"
#include <atomic>
#include <set>
#include <simgrid/forward.h>
#include <string>
#include <vector>
#include <xbt/signal.hpp>

XBT_LOG_EXTERNAL_CATEGORY(s4u_activity);

namespace simgrid {
namespace s4u {

/** @brief Activities
 *
 * This class is the ancestor of every activities that an actor can undertake.
 * That is, activities are all the things that do take time to the actor in the simulated world.
 */
class XBT_PUBLIC Activity {
  friend Comm;
  friend Exec;
  friend ExecSeq;
  friend ExecPar;
  friend Io;

protected:
  Activity()  = default;
  virtual ~Activity() = default;
public:
#ifndef DOXYGEN
  Activity(Activity const&) = delete;
  Activity& operator=(Activity const&) = delete;
#endif

  enum class State { INITED = 0, STARTING, STARTED, CANCELED, ERRORED, FINISHED };

  /** Starts a previously created activity.
   *
   * This function is optional: you can call wait() even if you didn't call start()
   */
  virtual Activity* start() = 0;
  /** Blocks until the activity is terminated */
  virtual Activity* wait() = 0;
  /** Blocks until the activity is terminated, or until the timeout is elapsed
   *  Raises: timeout exception.*/
  virtual Activity* wait_for(double timeout) = 0;
  /** Blocks until the activity is terminated, or until the time limit is reached
   * Raises: timeout exception. */
  void wait_until(double time_limit);

  /** Cancel that activity */
  virtual Activity* cancel() = 0;
  /** Retrieve the current state of the activity */
  Activity::State get_state() { return state_; }
  void set_state(Activity::State state) { state_ = state; }
  /** Tests whether the given activity is terminated yet. This is a pure function. */
  virtual bool test() = 0;
  virtual const char* get_cname()       = 0;
  virtual const std::string& get_name() = 0;

  /** Get the remaining amount of work that this Activity entails. When it's 0, it's done. */
  virtual double get_remaining();
  /** Set the [remaining] amount of work that this Activity will entail
   *
   * It is forbidden to change the amount of work once the Activity is started */
  Activity* set_remaining(double remains);

  /** Returns the internal implementation of this Activity */
  kernel::activity::ActivityImpl* get_impl() const { return pimpl_.get(); }

  void add_successor(ActivityPtr a)
  {
    successors_.push_back(a);
    a->add_dependency_on(this);
  }
  void remove_successor() { successors_.pop_back(); }
  ActivityPtr get_successor() { return successors_.back(); }
  bool has_successors() { return not successors_.empty(); }

  void add_dependency_on(ActivityPtr a) { dependencies_.insert({a}); }
  void remove_dependency_on(ActivityPtr a) { dependencies_.erase(a); }
  bool has_dependencies() { return not dependencies_.empty(); }
  void release_dependencies()
  {
    while (has_successors()) {
      ActivityPtr b = get_successor();
      XBT_CDEBUG(s4u_activity, "Remove a dependency from '%s' on '%s'", get_cname(), b->get_cname());
      b->remove_dependency_on(this);
      if (not b->has_dependencies()) {
        b->vetoable_start();
      }
      remove_successor();
    }
  }

  void vetoable_start()
  {
    state_ = State::STARTING;
    if (not has_dependencies()) {
      state_ = State::STARTED;
      XBT_CDEBUG(s4u_activity, "All dependencies are solved, let's start '%s'", get_cname());
      start();
    }
  }

#ifndef DOXYGEN
  friend void intrusive_ptr_release(Activity* a)
  {
    if (a->refcount_.fetch_sub(1, std::memory_order_release) == 1) {
      std::atomic_thread_fence(std::memory_order_acquire);
      delete a;
    }
  }
  friend void intrusive_ptr_add_ref(Activity* a) { a->refcount_.fetch_add(1, std::memory_order_relaxed); }
#endif
private:
  kernel::activity::ActivityImplPtr pimpl_ = nullptr;
  Activity::State state_                   = Activity::State::INITED;
  double remains_                          = 0;
  std::vector<ActivityPtr> successors_;
  std::set<ActivityPtr> dependencies_;
  std::atomic_int_fast32_t refcount_{0};
};

template <class AnyActivity> class Activity_T : public Activity {
  std::string name_             = "unnamed";
  std::string tracing_category_ = "";
  void* user_data_              = nullptr;

public:
  AnyActivity* set_name(const std::string& name)
  {
    xbt_assert(get_state() == State::INITED, "Cannot change the name of an activity after its start");
    name_ = name;
    return static_cast<AnyActivity*>(this);
  }
  const std::string& get_name() { return name_; }
  const char* get_cname() { return name_.c_str(); }

  AnyActivity* set_tracing_category(const std::string& category)
  {
    xbt_assert(get_state() == State::INITED, "Cannot change the tracing category of an activity after its start");
    tracing_category_ = category;
    return static_cast<AnyActivity*>(this);
  }
  const std::string& get_tracing_category() { return tracing_category_; }

  AnyActivity* set_user_data(void* data)
  {
    user_data_ = data;
    return static_cast<AnyActivity*>(this);
  }

  void* get_user_data() { return user_data_; }
#ifndef DOXYGEN
  /* The refcounting is done in the ancestor class, Activity, but we want each of the classes benefiting of the CRTP
   * (Exec, Comm, etc) to have smart pointers too, so we define these methods here, that forward the ptr_release and
   * add_ref to the Activity class. Hopefully, the "inline" helps to not hinder the perf here.
   */
  friend void inline intrusive_ptr_release(AnyActivity* a) { intrusive_ptr_release(static_cast<Activity*>(a)); }
  friend void inline intrusive_ptr_add_ref(AnyActivity* a) { intrusive_ptr_add_ref(static_cast<Activity*>(a)); }
#endif
};

} // namespace s4u
} // namespace simgrid

#endif /* SIMGRID_S4U_ACTIVITY_HPP */
