/* Copyright (c) 2007-2022. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_ACTIVITY_ACTIVITYIMPL_HPP
#define SIMGRID_KERNEL_ACTIVITY_ACTIVITYIMPL_HPP

#include <string>
#include <list>

#include "simgrid/forward.h"
#include <xbt/utility.hpp>

#include <atomic>
#include <simgrid/kernel/resource/Action.hpp>
#include <simgrid/simix.hpp>

namespace simgrid {
namespace kernel {
namespace activity {

XBT_DECLARE_ENUM_CLASS(State, WAITING, READY, RUNNING, DONE, CANCELED, FAILED, SRC_HOST_FAILURE, DST_HOST_FAILURE,
                       TIMEOUT, SRC_TIMEOUT, DST_TIMEOUT, LINK_FAILURE);

class XBT_PUBLIC ActivityImpl {
  std::atomic_int_fast32_t refcount_{0};
  std::string name_ = "";

public:
  virtual ~ActivityImpl();
  ActivityImpl() = default;
  State state_   = State::WAITING;      /* State of the activity */
  std::list<smx_simcall_t> simcalls_;   /* List of simcalls waiting for this activity */
  resource::Action* surf_action_ = nullptr;
  actor::ActorImpl* actor_       = nullptr;
  s4u::Activity* piface_         = nullptr;
  double start_time_             = -1.0;
  double finish_time_            = -1.0;

protected:
  void inline set_name(const std::string& name)
  {
    // This is to keep name_ private while allowing ActivityImpl_T<??> to set it and then return a Ptr to qualified
    // child type
    name_ = name;
  }

public:
  const std::string& get_name() const { return name_; }
  const char* get_cname() const { return name_.c_str(); }

  void set_actor(actor::ActorImpl* actor) { actor_ = actor; }
  actor::ActorImpl* get_actor() const { return actor_; }

  void set_iface(s4u::Activity* iface) { piface_ = iface; }
  s4u::Activity* get_iface() { return piface_; }

  double get_start_time() const { return start_time_; }
  void set_finish_time(double finish_time) { finish_time_ = finish_time; }
  double get_finish_time() const { return finish_time_; }

  virtual bool test();
  virtual void wait_for(actor::ActorImpl* issuer, double timeout);
  virtual ActivityImpl& set_timeout(double) { THROW_UNIMPLEMENTED; }

  virtual void suspend();
  virtual void resume();
  virtual void cancel();

  virtual void post() = 0; // Called by the main loop when the activity is marked as terminated or failed by its model.
                           // Setups the status, clean things up, and call finish()
  virtual void set_exception(actor::ActorImpl* issuer) = 0; // Raising exceptions and stuff
  virtual void finish() = 0; // Unlock all simcalls blocked on that activity, either because it was marked as done by
                             // the model or because it terminated without waiting for the model

  void register_simcall(smx_simcall_t simcall);
  void unregister_simcall(smx_simcall_t simcall);
  void clean_action();
  virtual double get_remaining() const;
  const char* get_state_str() const;
  // Support for the boost::intrusive_ptr<ActivityImpl> datatype
  friend XBT_PUBLIC void intrusive_ptr_add_ref(ActivityImpl* activity);
  friend XBT_PUBLIC void intrusive_ptr_release(ActivityImpl* activity);

  static xbt::signal<void(ActivityImpl const&)> on_suspended;
  static xbt::signal<void(ActivityImpl const&)> on_resumed;
};

/* This class exists to allow chained setters as in exec->set_name()->set_priority()->set_blah()
 * The difficulty is that set_name() must return a qualified child class, not the generic ancestor
 * But the getter is still in the ancestor to be usable on generic activities with no downcast */
template <class AnyActivityImpl> class ActivityImpl_T : public ActivityImpl {
private:
  std::string tracing_category_ = "";

public:
  AnyActivityImpl& set_name(const std::string& name) /* Hides the function in the ancestor class */
  {
    ActivityImpl::set_name(name);
    return static_cast<AnyActivityImpl&>(*this);
  }

  AnyActivityImpl& set_tracing_category(const std::string& category)
  {
    tracing_category_ = category;
    return static_cast<AnyActivityImpl&>(*this);
  }
  const std::string& get_tracing_category() const { return tracing_category_; }
};

} // namespace activity
} // namespace kernel
} // namespace simgrid

#endif /* SIMGRID_KERNEL_ACTIVITY_ACTIVITYIMPL_HPP */
