/* Copyright (c) 2006-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_ACTIVITY_HPP
#define SIMGRID_S4U_ACTIVITY_HPP

#include <simgrid/forward.h>
#include <xbt/signal.hpp>

namespace simgrid {
namespace s4u {

/** @brief Activities
 *
 * This class is the ancestor of every activities that an actor can undertake.
 * That is, activities are all the things that do take time to the actor in the simulated world.
 *
 * They are somewhat linked but not identical to simgrid::kernel::resource::Action,
 * that are stuff occurring on a resource:
 *
 * - A sequential execution activity encompasses 2 actions: one for the exec itself,
 *   and a time-limited sleep used as timeout detector.
 * - A point-to-point communication activity encompasses 3 actions: one for the comm itself
 *   (which spans on all links of the path), and one infinite sleep used as failure detector
 *   on both sender and receiver hosts.
 * - Synchronization activities may possibly be connected to no action.
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

  /** Put some user data onto the Activity */
  Activity* set_user_data(void* data)
  {
    user_data_ = data;
    return this;
  }
  /** Retrieve the user data of the Activity */
  void* get_user_data() { return user_data_; }

  kernel::activity::ActivityImpl* get_impl() const { return pimpl_.get(); }

private:
  kernel::activity::ActivityImplPtr pimpl_ = nullptr;
  Activity::State state_                   = Activity::State::INITED;
  double remains_                          = 0;
  void* user_data_                         = nullptr;
};

} // namespace s4u
} // namespace simgrid

#endif /* SIMGRID_S4U_ACTIVITY_HPP */
