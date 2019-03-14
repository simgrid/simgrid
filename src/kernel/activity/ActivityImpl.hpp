/* Copyright (c) 2007-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_ACTIVITY_ACTIVITYIMPL_HPP
#define SIMGRID_KERNEL_ACTIVITY_ACTIVITYIMPL_HPP

#include <string>
#include <list>

#include <xbt/base.h>
#include "simgrid/forward.h"

#include <atomic>
#include <simgrid/kernel/resource/Action.hpp>
#include <simgrid/simix.hpp>

namespace simgrid {
namespace kernel {
namespace activity {

class XBT_PUBLIC ActivityImpl {
public:
  ActivityImpl() = default;
  explicit ActivityImpl(const std::string& name) : name_(name) {}
  virtual ~ActivityImpl() = default;
  e_smx_state_t state_ = SIMIX_WAITING; /* State of the activity */
  std::list<smx_simcall_t> simcalls_;   /* List of simcalls waiting for this activity */
  resource::Action* surf_action_ = nullptr;

  const std::string& get_name() const { return name_; }
  const char* get_cname() const { return name_.c_str(); }
  void set_name(const std::string& name) { name_ = name; }

  virtual void suspend();
  virtual void resume();
  virtual void post()   = 0; // What to do when a simcall terminates
  virtual void finish() = 0;
  void set_category(const std::string& category);

  // boost::intrusive_ptr<ActivityImpl> support:
  friend XBT_PUBLIC void intrusive_ptr_add_ref(ActivityImpl* activity);
  friend XBT_PUBLIC void intrusive_ptr_release(ActivityImpl* activity);

private:
  std::atomic_int_fast32_t refcount_{0};
  std::string name_;                    /* Activity name if any */

public:
  static xbt::signal<void(ActivityImplPtr)> on_suspended;
  static xbt::signal<void(ActivityImplPtr)> on_resumed;
};
} // namespace activity
} // namespace kernel
} // namespace simgrid

#endif /* SIMGRID_KERNEL_ACTIVITY_ACTIVITYIMPL_HPP */
