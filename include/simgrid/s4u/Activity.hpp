/* Copyright (c) 2006-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_ACTIVITY_HPP
#define SIMGRID_S4U_ACTIVITY_HPP

#include "xbt/asserts.h"
#include <atomic>
#include <simgrid/forward.h>
#include <string>
#include <xbt/signal.hpp>

namespace simgrid {
namespace s4u {

/** @brief Activities
 *
 * This class is the ancestor of every activities that an actor can undertake.
 * That is, activities are all the things that do take time to the actor in the simulated world.
 */
class XBT_PUBLIC Activity {
  friend Comm;
  friend XBT_PUBLIC void intrusive_ptr_release(Comm * c);
  friend XBT_PUBLIC void intrusive_ptr_add_ref(Comm * c);

  friend Exec;
  friend ExecSeq;
  friend ExecPar;
  friend XBT_PUBLIC void intrusive_ptr_release(Exec * e);
  friend XBT_PUBLIC void intrusive_ptr_add_ref(Exec * e);

  friend Io;
  friend XBT_PUBLIC void intrusive_ptr_release(Io* i);
  friend XBT_PUBLIC void intrusive_ptr_add_ref(Io* i);

protected:
  Activity()  = default;
  virtual ~Activity() = default;

public:
#ifndef DOXYGEN
  Activity(Activity const&) = delete;
  Activity& operator=(Activity const&) = delete;
#endif

  enum class State { INITED = 0, STARTED, CANCELED, ERRORED, FINISHED };

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
  /** Tests whether the given activity is terminated yet. This is a pure function. */
  virtual bool test() = 0;

  /** Get the remaining amount of work that this Activity entails. When it's 0, it's done. */
  virtual double get_remaining();

  /** Set the [remaining] amount of work that this Activity will entail
   *
   * It is forbidden to change the amount of work once the Activity is started */
  Activity* set_remaining(double remains);

  /** Returns the internal implementation of this Activity */
  kernel::activity::ActivityImpl* get_impl() const { return pimpl_.get(); }

private:
  kernel::activity::ActivityImplPtr pimpl_ = nullptr;
  Activity::State state_                   = Activity::State::INITED;
  double remains_                          = 0;
};

template <class AnyActivity> class Activity_T : public Activity {
private:
  std::string name_             = "";
  std::string tracing_category_ = "";
  void* user_data_              = nullptr;
  std::atomic_int_fast32_t refcount_{0};

public:
#ifndef DOXYGEN
  friend void intrusive_ptr_release(AnyActivity* a)
  {
    if (a->refcount_.fetch_sub(1, std::memory_order_release) == 1) {
      std::atomic_thread_fence(std::memory_order_acquire);
      delete a;
    }
  }
  friend void intrusive_ptr_add_ref(AnyActivity* a) { a->refcount_.fetch_add(1, std::memory_order_relaxed); }
#endif
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
};

} // namespace s4u
} // namespace simgrid

#endif /* SIMGRID_S4U_ACTIVITY_HPP */
