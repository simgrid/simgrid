/* Copyright (c) 2006-2021. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_ACTIVITY_HPP
#define SIMGRID_S4U_ACTIVITY_HPP

#include <algorithm>
#include <atomic>
#include <set>
#include <simgrid/forward.h>
#include <stdexcept>
#include <string>
#include <vector>
#include <xbt/Extendable.hpp>
#include <xbt/asserts.h>
#include <xbt/signal.hpp>
#include <xbt/utility.hpp>

XBT_LOG_EXTERNAL_CATEGORY(s4u_activity);

namespace simgrid {

extern template class XBT_PUBLIC xbt::Extendable<s4u::Activity>;

namespace s4u {

/** @brief Activities
 *
 * This class is the ancestor of every activities that an actor can undertake.
 * That is, activities are all the things that do take time to the actor in the simulated world.
 */
class XBT_PUBLIC Activity : public xbt::Extendable<Activity> {
  friend Comm;
  friend Exec;
  friend Io;
#ifndef DOXYGEN
  friend std::vector<ActivityPtr> create_DAG_from_dot(const std::string& filename);
  friend std::vector<ActivityPtr> create_DAG_from_DAX(const std::string& filename);
#endif

public:
  // enum class State { ... }
  XBT_DECLARE_ENUM_CLASS(State, INITED, STARTING, STARTED, FAILED, CANCELED, FINISHED);

  virtual bool is_assigned() const = 0;
  virtual bool dependencies_solved() const { return dependencies_.empty(); }
  virtual unsigned long is_waited_by() const { return successors_.size(); }
  const std::set<ActivityPtr>& get_dependencies() const { return dependencies_; }
  const std::vector<ActivityPtr>& get_successors() const { return successors_; }

protected:
  Activity()  = default;
  virtual ~Activity() = default;
  void destroy();

  void release_dependencies()
  {
    while (not successors_.empty()) {
      ActivityPtr b = successors_.back();
      XBT_CVERB(s4u_activity, "Remove a dependency from '%s' on '%s'", get_cname(), b->get_cname());
      b->dependencies_.erase(this);
      if (b->dependencies_solved()) {
        b->vetoable_start();
      }
      successors_.pop_back();
    }
  }

  void add_successor(ActivityPtr a)
  {
    if(this == a)
      throw std::invalid_argument("Cannot be its own successor");
    auto p = std::find_if(successors_.begin(), successors_.end(), [a](ActivityPtr const& i){ return i.get() == a.get(); });
    if (p != successors_.end())
      throw std::invalid_argument("Dependency already exists");

    successors_.push_back(a);
    a->dependencies_.insert({this});
  }

  void remove_successor(ActivityPtr a)
  {
    if(this == a)
      throw std::invalid_argument("Cannot ask to remove itself from successors list");

    auto p = std::find_if(successors_.begin(), successors_.end(), [a](ActivityPtr const& i){ return i.get() == a.get(); });
    if (p != successors_.end()){
      successors_.erase(p);
      a->dependencies_.erase({this});
    } else
      throw std::invalid_argument("Dependency does not exist. Can not be removed.");
  }

  static std::set<Activity*>* vetoed_activities_;

public:
  /*! Signal fired each time that the activity fails to start because of a veto (e.g., unsolved dependency or no
   * resource assigned) */
  static xbt::signal<void(Activity&)> on_veto;
  /*! Signal fired when theactivity completes  (either normally, cancelled or failed) */
  static xbt::signal<void(Activity&)> on_completion;

  void vetoable_start()
  {
    state_ = State::STARTING;
    if (dependencies_solved() && is_assigned()) {
      XBT_CVERB(s4u_activity, "'%s' is assigned to a resource and all dependencies are solved. Let's start", get_cname());
      start();
    } else {
      if (vetoed_activities_ != nullptr)
        vetoed_activities_->insert(this);
      on_veto(*this);
    }
  }

  void complete(Activity::State state)
  {
    state_ = state;
    on_completion(*this);
    if (state == State::FINISHED)
      release_dependencies();
  }

  static std::set<Activity*>* get_vetoed_activities() { return vetoed_activities_; }
  static void set_vetoed_activities(std::set<Activity*>* whereto) { vetoed_activities_ = whereto; }

#ifndef DOXYGEN
  Activity(Activity const&) = delete;
  Activity& operator=(Activity const&) = delete;
#endif

  /** Starts a previously created activity.
   *
   * This function is optional: you can call wait() even if you didn't call start()
   */
  virtual Activity* start() = 0;
  /** Blocks the current actor until the activity is terminated */
  Activity* wait() { return wait_for(-1.0); }
  /** Blocks the current actor until the activity is terminated, or until the timeout is elapsed\n
   *  Raises: timeout exception.*/
  Activity* wait_for(double timeout);
  /** Blocks the current actor until the activity is terminated, or until the time limit is reached\n
   * Raises: timeout exception. */
  void wait_until(double time_limit);

  /** Cancel that activity */
  Activity* cancel();
  /** Retrieve the current state of the activity */
  Activity::State get_state() const { return state_; }
  /** Return a string representation of the activity's state (one of INITED, STARTING, STARTED, CANCELED, FINISHED) */
  const char* get_state_str() const;
  void set_state(Activity::State state) { state_ = state; }
  /** Tests whether the given activity is terminated yet. */
  virtual bool test();

  /** Blocks the progression of this activity until it gets resumed */
  virtual Activity* suspend();
  /** Unblock the progression of this activity if it was suspended previously */
  virtual Activity* resume();
  /** Whether or not the progression of this activity is blocked */
  bool is_suspended() const { return suspended_; }

  virtual const char* get_cname() const       = 0;
  virtual const std::string& get_name() const = 0;

  /** Get the remaining amount of work that this Activity entails. When it's 0, it's done. */
  virtual double get_remaining() const;
  /** Set the [remaining] amount of work that this Activity will entail
   *
   * It is forbidden to change the amount of work once the Activity is started */
  Activity* set_remaining(double remains);

  double get_start_time() const;
  double get_finish_time() const;
  void mark() { marked_ = true; }
  bool is_marked() const { return marked_; }

  /** Returns the internal implementation of this Activity */
  kernel::activity::ActivityImpl* get_impl() const { return pimpl_.get(); }

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
  Activity* add_ref()
  {
    intrusive_ptr_add_ref(this);
    return this;
  }
  void unref() { intrusive_ptr_release(this); }

private:
  kernel::activity::ActivityImplPtr pimpl_ = nullptr;
  Activity::State state_                   = Activity::State::INITED;
  double remains_                          = 0;
  bool suspended_                          = false;
  bool marked_                             = false;
  std::vector<ActivityPtr> successors_;
  std::set<ActivityPtr> dependencies_;
  std::atomic_int_fast32_t refcount_{0};
};

template <class AnyActivity> class Activity_T : public Activity {
  std::string name_             = "unnamed";
  std::string tracing_category_ = "";
  void* user_data_              = nullptr;

public:
  AnyActivity* add_successor(ActivityPtr a)
  {
    Activity::add_successor(a);
    return static_cast<AnyActivity*>(this);
  }
  AnyActivity* remove_successor(ActivityPtr a)
  {
    Activity::remove_successor(a);
    return static_cast<AnyActivity*>(this);
  }
  AnyActivity* set_name(const std::string& name)
  {
    xbt_assert(get_state() == State::INITED, "Cannot change the name of an activity after its start");
    name_ = name;
    return static_cast<AnyActivity*>(this);
  }
  const std::string& get_name() const override { return name_; }
  const char* get_cname() const override { return name_.c_str(); }

  AnyActivity* set_tracing_category(const std::string& category)
  {
    xbt_assert(get_state() == State::INITED, "Cannot change the tracing category of an activity after its start");
    tracing_category_ = category;
    return static_cast<AnyActivity*>(this);
  }
  const std::string& get_tracing_category() const { return tracing_category_; }

  AnyActivity* set_user_data(void* data)
  {
    user_data_ = data;
    return static_cast<AnyActivity*>(this);
  }

  void* get_user_data() const { return user_data_; }

  AnyActivity* vetoable_start()
  {
    Activity::vetoable_start();
    return static_cast<AnyActivity*>(this);
  }

  AnyActivity* cancel() { return static_cast<AnyActivity*>(Activity::cancel()); }
  AnyActivity* wait() { return wait_for(-1.0); }
  virtual AnyActivity* wait_for(double timeout) { return static_cast<AnyActivity*>(Activity::wait_for(timeout)); }

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
