/* Copyright (c) 2006-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_ACTIVITY_HPP
#define SIMGRID_S4U_ACTIVITY_HPP

#include <simgrid/s4u/forward.hpp>
#include <simgrid/forward.h>

enum e_s4u_activity_state_t { inited = 0, started, canceled, errored, finished };

namespace simgrid {
namespace s4u {

/** @brief Activities
 *
 * This class is the ancestor of every activities that an actor can undertake, that is, of the actions that do take time in the simulated world.
 */
XBT_PUBLIC_CLASS Activity {
  friend Comm;
  friend XBT_PUBLIC(void) intrusive_ptr_release(Comm * c);
  friend XBT_PUBLIC(void) intrusive_ptr_add_ref(Comm * c);
  friend Exec;
  friend XBT_PUBLIC(void) intrusive_ptr_release(Exec * e);
  friend XBT_PUBLIC(void) intrusive_ptr_add_ref(Exec * e);

protected:
  Activity()  = default;
  virtual ~Activity() = default;

public:
  Activity(Activity const&) = delete;
  Activity& operator=(Activity const&) = delete;

  /** Starts a previously created activity.
   *
   * This function is optional: you can call wait() even if you didn't call start()
   */
  virtual Activity* start() = 0;
  /** Tests whether the given activity is terminated yet. This is a pure function. */
  //virtual bool test()=0;
  /** Blocks until the activity is terminated */
  virtual Activity* wait() = 0;
  /** Blocks until the activity is terminated, or until the timeout is elapsed
   *  Raises: timeout exception.*/
  virtual Activity* wait(double timeout) = 0;
  /** Cancel that activity */
  //virtual void cancel();
  /** Retrieve the current state of the activity */
  e_s4u_activity_state_t getState() {return state_;}

  /** Get the remaining amount of work that this Activity entails. When it's 0, it's done. */
  virtual double getRemains();
  /** Set the [remaining] amount of work that this Activity will entail
   *
   * It is forbidden to change the amount of work once the Activity is started */
  Activity* setRemains(double remains);

  /** Put some user data onto the Activity */
  Activity* setUserData(void* data)
  {
    userData_ = data;
    return this;
  }
  /** Retrieve the user data of the Activity */
  void *getUserData() { return userData_; }

private:
  simgrid::kernel::activity::ActivityImplPtr pimpl_ = nullptr;
  e_s4u_activity_state_t state_ = inited;
  double remains_ = 0;
  void *userData_ = nullptr;
}; // class

}}; // Namespace simgrid::s4u

#endif /* SIMGRID_S4U_ACTIVITY_HPP */
