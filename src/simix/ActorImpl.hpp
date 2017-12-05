/* Copyright (c) 2007-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMIX_ACTORIMPL_H
#define SIMIX_ACTORIMPL_H

#include "simgrid/s4u/Actor.hpp"
#include "src/simix/popping_private.hpp"
#include "src/surf/PropertyHolder.hpp"
#include <boost/intrusive/list.hpp>
#include <list>
#include <map>

struct s_smx_process_exit_fun_t {
  int_f_pvoid_pvoid_t fun;
  void *arg;
};

namespace simgrid {
namespace simix {

class ProcessArg {
public:
  std::string name;
  std::function<void()> code;
  void *data            = nullptr;
  sg_host_t host        = nullptr;
  double kill_time      = 0.0;
  std::shared_ptr<std::map<std::string, std::string>> properties;
  bool auto_restart     = false;
};

class ActorImpl : public simgrid::surf::PropertyHolder {
public:
  ActorImpl() : piface_(this) {}
  ~ActorImpl();

  boost::intrusive::list_member_hook<> host_process_list_hook; /* simgrid::simix::Host::process_list */
  boost::intrusive::list_member_hook<> smx_destroy_list_hook;  /* simix_global->process_to_destroy */
  boost::intrusive::list_member_hook<> smx_synchro_hook;       /* {mutex,cond,sem}->sleeping */

  aid_t pid  = 0;
  aid_t ppid = -1;
  simgrid::xbt::string name;
  const simgrid::xbt::string& getName() const { return name; }
  const char* getCname() const { return name.c_str(); }
  s4u::Host* host       = nullptr; /* the host on which the process is running */
  smx_context_t context = nullptr; /* the context (uctx/raw/thread) that executes the user function */

  // TODO, pack them
  std::exception_ptr exception;
  bool finished     = false;
  bool blocked      = false;
  bool suspended    = false;
  bool auto_restart = false;

  sg_host_t new_host             = nullptr; /* if not null, the host on which the process must migrate to */
  smx_activity_t waiting_synchro = nullptr; /* the current blocking synchro if any */
  std::list<smx_activity_t> comms;          /* the current non-blocking communication synchros */
  s_smx_simcall_t simcall;
  void* userdata = nullptr;                      /* kept for compatibility, it should be replaced with moddata */
  std::vector<s_smx_process_exit_fun_t> on_exit; /* list of functions executed when the process dies */

  std::function<void()> code;
  smx_timer_t kill_timer = nullptr;
  int segment_index = -1; /* Reference to an SMPI process' data segment. Default value is -1 if not in SMPI context*/

  /* Refcounting */
private:
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
  bool daemon = false;
public:
  void daemonize();
  bool isDaemon() { return daemon; } /** Whether this actor has been daemonized */
  bool isSuspended() { return suspended; }
  simgrid::s4u::Actor* restart();
  smx_activity_t suspend(ActorImpl* issuer);
  void resume();
  smx_activity_t sleep(double duration);
  void setUserData(void* data) { userdata = data; }
  void* getUserData() { return userdata; }
};

}
}

typedef simgrid::simix::ProcessArg *smx_process_arg_t;

typedef simgrid::simix::ActorImpl* smx_actor_t;

extern "C" {

XBT_PRIVATE smx_actor_t SIMIX_process_create(const char* name, std::function<void()> code, void* data, sg_host_t host,
                                             std::map<std::string, std::string>* properties,
                                             smx_actor_t parent_process);

XBT_PRIVATE void SIMIX_process_runall();
XBT_PRIVATE void SIMIX_process_kill(smx_actor_t process, smx_actor_t issuer);
XBT_PRIVATE void SIMIX_process_killall(smx_actor_t issuer, int reset_pid);
XBT_PRIVATE void SIMIX_process_cleanup(smx_actor_t arg);
XBT_PRIVATE void SIMIX_process_empty_trash();
XBT_PRIVATE void SIMIX_process_yield(smx_actor_t self);
XBT_PRIVATE void SIMIX_process_exception_terminate(xbt_ex_t * e);
XBT_PRIVATE void SIMIX_process_change_host(smx_actor_t process, sg_host_t dest);
XBT_PRIVATE void SIMIX_process_set_data(smx_actor_t process, void *data);
XBT_PRIVATE smx_actor_t SIMIX_process_get_by_name(const char* name);

XBT_PRIVATE void SIMIX_process_auto_restart_set(smx_actor_t process, int auto_restart);

extern void (*SMPI_switch_data_segment)(int dest);
}

XBT_PRIVATE void SIMIX_process_sleep_destroy(smx_activity_t synchro);
XBT_PRIVATE smx_activity_t SIMIX_process_join(smx_actor_t issuer, smx_actor_t process, double timeout);

#endif
