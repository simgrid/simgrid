/* Copyright (c) 2007-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMIX_ACTORIMPL_H
#define SIMIX_ACTORIMPL_H

#include "simgrid/s4u/Actor.hpp"
#include "src/simix/popping_private.hpp"
#include "src/surf/PropertyHolder.hpp"
#include <boost/intrusive/list.hpp>
#include <functional>
#include <list>
#include <map>

struct s_smx_process_exit_fun_t {
  std::function<void(int, void*)> fun;
  void *arg;
};

namespace simgrid {
namespace kernel {
namespace actor {

class ActorImpl : public simgrid::surf::PropertyHolder {
public:
  ActorImpl() : piface_(this) {}
  ~ActorImpl();

  void set_auto_restart(bool autorestart) { auto_restart_ = autorestart; }

  boost::intrusive::list_member_hook<> host_process_list_hook; /* simgrid::simix::Host::process_list */
  boost::intrusive::list_member_hook<> smx_destroy_list_hook;  /* simix_global->process_to_destroy */
  boost::intrusive::list_member_hook<> smx_synchro_hook;       /* {mutex,cond,sem}->sleeping */

  aid_t pid_  = 0;
  aid_t ppid_ = -1;
  simgrid::xbt::string name_;
  const simgrid::xbt::string& get_name() const { return name_; }
  const char* get_cname() const { return name_.c_str(); }
  s4u::Host* host_       = nullptr; /* the host on which the process is running */
  smx_context_t context_ = nullptr; /* the context (uctx/raw/thread) that executes the user function */

  std::exception_ptr exception;
  bool finished_    = false;
  bool blocked_     = false;
  bool suspended_   = false;
  bool auto_restart_ = false;

  smx_activity_t waiting_synchro = nullptr; /* the current blocking synchro if any */
  std::list<smx_activity_t> comms;          /* the current non-blocking communication synchros */
  s_smx_simcall simcall;
  std::vector<s_smx_process_exit_fun_t> on_exit; /* list of functions executed when the process dies */

  std::function<void()> code;
  smx_timer_t kill_timer = nullptr;

private:
  void* userdata_ = nullptr; /* kept for compatibility, it should be replaced with moddata */
  /* Refcounting */
  std::atomic_int_fast32_t refcount_{0};

public:
  friend void intrusive_ptr_add_ref(ActorImpl* process)
  {
    // std::memory_order_relaxed ought to be enough here instead of std::memory_order_seq_cst
    // But then, we have a threading issue when an actor commits a suicide:
    //  it seems that in this case, the worker thread kills the last occurrence of the actor
    //  while usually, the maestro does so. FIXME: we should change how actors suicide
    process->refcount_.fetch_add(1, std::memory_order_seq_cst);
  }
  friend void intrusive_ptr_release(ActorImpl* process)
  {
    // inspired from http://www.boost.org/doc/libs/1_55_0/doc/html/atomic/usage_examples.html
    if (process->refcount_.fetch_sub(1, std::memory_order_release) == 1) {
      // Make sure that any changes done on other threads before their acquire are committed before our delete
      // http://stackoverflow.com/questions/27751025/why-is-an-acquire-barrier-needed-before-deleting-the-data-in-an-atomically-refer
      std::atomic_thread_fence(std::memory_order_acquire);
      delete process;
    }
  }

  /* S4U/implem interfaces */
private:
  simgrid::s4u::Actor piface_; // Our interface is part of ourselves
public:
  simgrid::s4u::ActorPtr iface() { return s4u::ActorPtr(&piface_); }
  simgrid::s4u::Actor* ciface() { return &piface_; }

  /* Daemon actors are automatically killed when the last non-daemon leaves */
private:
  bool daemon_ = false;
public:
  void daemonize();
  bool is_daemon() { return daemon_; } /** Whether this actor has been daemonized */
  bool is_suspended() { return suspended_; }
  simgrid::s4u::Actor* restart();
  smx_activity_t suspend(ActorImpl* issuer);
  void resume();
  smx_activity_t sleep(double duration);
  void set_user_data(void* data) { userdata_ = data; }
  void* get_user_data() { return userdata_; }
  /** Ask the actor to throw an exception right away */
  void throw_exception(std::exception_ptr e);
};

class ProcessArg {
public:
  std::string name;
  std::function<void()> code;
  void* data                                                               = nullptr;
  s4u::Host* host                                                          = nullptr;
  double kill_time                                                         = 0.0;
  std::shared_ptr<std::unordered_map<std::string, std::string>> properties = nullptr;
  bool auto_restart                                                        = false;
  bool daemon_                                                             = false;
  ProcessArg()                                                             = default;

  explicit ProcessArg(std::string name, std::function<void()> code, void* data, s4u::Host* host, double kill_time,
                      std::shared_ptr<std::unordered_map<std::string, std::string>> properties, bool auto_restart)
      : name(name)
      , code(std::move(code))
      , data(data)
      , host(host)
      , kill_time(kill_time)
      , properties(properties)
      , auto_restart(auto_restart)
  {
  }

  explicit ProcessArg(s4u::Host* host, ActorImpl* actor)
      : name(actor->get_name())
      , code(std::move(actor->code))
      , data(actor->get_user_data())
      , host(host)
      , kill_time(SIMIX_timer_get_date(actor->kill_timer))
      , auto_restart(actor->auto_restart_)
      , daemon_(actor->is_daemon())
  {
    properties.reset(actor->get_properties(), [](decltype(actor->get_properties())) {});
  }
};

/* Used to keep the list of actors blocked on a synchro  */
typedef boost::intrusive::list<ActorImpl, boost::intrusive::member_hook<ActorImpl, boost::intrusive::list_member_hook<>,
                                                                        &ActorImpl::smx_synchro_hook>>
    SynchroList;

XBT_PUBLIC void create_maestro(std::function<void()> code);
}
} // namespace kernel
} // namespace simgrid

typedef simgrid::kernel::actor::ActorImpl* smx_actor_t;

XBT_PRIVATE smx_actor_t SIMIX_process_create(std::string name, std::function<void()> code, void* data, sg_host_t host,
                                             std::unordered_map<std::string, std::string>* properties,
                                             smx_actor_t parent_process);

XBT_PRIVATE void SIMIX_process_runall();
XBT_PRIVATE void SIMIX_process_kill(smx_actor_t process, smx_actor_t issuer);
XBT_PRIVATE void SIMIX_process_killall(smx_actor_t issuer);
XBT_PRIVATE void SIMIX_process_cleanup(smx_actor_t arg);
XBT_PRIVATE void SIMIX_process_empty_trash();
XBT_PRIVATE void SIMIX_process_yield(smx_actor_t self);
XBT_PRIVATE void SIMIX_process_change_host(smx_actor_t process, sg_host_t dest);

extern void (*SMPI_switch_data_segment)(simgrid::s4u::ActorPtr actor);

XBT_PRIVATE void SIMIX_process_sleep_destroy(smx_activity_t synchro);
XBT_PRIVATE smx_activity_t SIMIX_process_join(smx_actor_t issuer, smx_actor_t process, double timeout);

#endif
