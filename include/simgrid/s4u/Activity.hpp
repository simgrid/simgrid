/* Copyright (c) 2006-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_S4U_ACTIVITY_HPP
#define SIMGRID_S4U_ACTIVITY_HPP

#include <simgrid/s4u/forward.hpp>
#include <simgrid/forward.h>

namespace simgrid {
namespace s4u {

/** @brief Activities
 *
 * This class is the ancestor of every activities that an actor can undertake.
 * That is, of the actions that do take time in the simulated world.
 */
class XBT_PUBLIC Activity {
  friend Comm;
  friend XBT_PUBLIC void intrusive_ptr_release(Comm * c);
  friend XBT_PUBLIC void intrusive_ptr_add_ref(Comm * c);
  friend Exec;
  friend XBT_PUBLIC void intrusive_ptr_release(Exec * e);
  friend XBT_PUBLIC void intrusive_ptr_add_ref(Exec * e);

protected:
  Activity()  = default;
  virtual ~Activity() = default;

public:
  Activity(Activity const&) = delete;
  Activity& operator=(Activity const&) = delete;

  enum class State { inited = 0, started, canceled, errored, finished };

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
  Activity::State get_state() { return state_; }

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

  XBT_ATTRIB_DEPRECATED_v323("Please use Activity::get_state()") Activity::State getState() { return state_; }
  XBT_ATTRIB_DEPRECATED_v323("Please use Activity::get_remaining()") double getRemains() { return get_remaining(); }
  XBT_ATTRIB_DEPRECATED_v323("Please use Activity::set_remaining()") Activity* setRemains(double remains)
  {
    return set_remaining(remains);
  }
  XBT_ATTRIB_DEPRECATED_v323("Please use Activity::set_user_data()") Activity* setUserData(void* data)
  {
    return set_user_data(data);
  }
  XBT_ATTRIB_DEPRECATED_v323("Please use Activity::get_user_data()") void* getUserData() { return user_data_; }

private:
  simgrid::kernel::activity::ActivityImplPtr pimpl_ = nullptr;
  Activity::State state_                            = Activity::State::inited;
  double remains_ = 0;
  void* user_data_                                  = nullptr;
}; // class

}}; // Namespace simgrid::s4u

#endif /* SIMGRID_S4U_ACTIVITY_HPP */
