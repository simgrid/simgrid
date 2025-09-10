/* Copyright (c) 2007-2025. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#ifndef SIMGRID_KERNEL_ACTOR_ACTORIMPL_HPP
#define SIMGRID_KERNEL_ACTOR_ACTORIMPL_HPP

#include "Simcall.hpp"
#include "simgrid/kernel/Timer.hpp"
#include "simgrid/s4u/Actor.hpp"
#include "src/kernel/actor/Simcall.hpp"
#include "xbt/PropertyHolder.hpp"

#include <atomic>
#include <boost/intrusive/list.hpp>
#include <functional>
#include <list>
#include <map>
#include <set>
#include <unordered_set>
#include <memory>

namespace simgrid::kernel::actor {
class ProcessArg;

/*------------------------- [ ActorIDTrait ] -------------------------*/
class XBT_PUBLIC ActorIDTrait {
  std::string name_;
  aid_t pid_  = 0;
  aid_t ppid_ = -1;

  static unsigned long maxpid_;

public:
  explicit ActorIDTrait(const std::string& name, aid_t ppid);
  const std::string& get_name() const { return name_; }
  const char* get_cname() const { return name_.c_str(); }
  aid_t get_pid() const { return pid_; }
  aid_t get_ppid() const { return ppid_; }

  static unsigned long get_maxpid() { return maxpid_; }
};

/*------------------------- [ ActorRestartingTrait ] -------------------------*/
class XBT_PUBLIC ActorRestartingTrait {
  bool auto_restart_ = false;
  int restart_count_ = 0;

  friend ActorImpl;

public:
  bool has_to_auto_restart() const { return auto_restart_; }
  void set_auto_restart(bool autorestart) { auto_restart_ = autorestart; }
  int get_restart_count() const { return restart_count_; }
};

/*------------------------- [ ActorImpl ] -------------------------*/
class XBT_PUBLIC ActorImpl : public xbt::PropertyHolder, public ActorIDTrait, public ActorRestartingTrait {
  s4u::Host* host_   = nullptr; /* the host on which the actor is running */
  bool daemon_       = false; /* Daemon actors are automatically killed when the last non-daemon leaves */
  unsigned stacksize_; // set to default value in constructor
  bool iwannadie_   = false; // True if we need to do some cleanups in actor mode.
  bool to_be_freed_ = false; // True if cleanups in actor mode done, but cleanups in kernel mode pending

  std::vector<activity::MailboxImpl*> mailboxes_;
  friend activity::MailboxImpl;

public:
  ActorImpl(const std::string& name, s4u::Host* host, aid_t ppid);
  ActorImpl(const ActorImpl&) = delete;
  ActorImpl& operator=(const ActorImpl&) = delete;
  ~ActorImpl();

  static ActorImpl* self();
  double get_kill_time() const;
  void set_kill_time(double kill_time);
  boost::intrusive::list_member_hook<> host_actor_list_hook;     /* resource::HostImpl::actor_list_ */
  boost::intrusive::list_member_hook<> kernel_destroy_list_hook; /* EngineImpl actors_to_destroy */
  boost::intrusive::list_member_hook<> smx_synchro_hook;       /* {mutex,cond,sem}->sleeping */


  // Life-cycle
  bool wannadie() const { return iwannadie_; }
  void set_wannadie(bool value = true);
  bool to_be_freed() const { return to_be_freed_; }
  void set_to_be_freed() { to_be_freed_ = true; }

  // Accessors to private fields
  s4u::Host* get_host() const { return host_; }
  void set_host(s4u::Host* dest);
  bool is_maestro() const; /** Whether this actor is actually maestro (cheap call but may segfault before actor creation
                              / after terminaison) */
  void set_stacksize(unsigned stacksize) { stacksize_ = stacksize; }
  unsigned get_stacksize() const { return stacksize_; }

  // Daemonize
  bool is_daemon() const { return daemon_; } /** Whether this actor has been daemonized */
  void daemonize();
  void undaemonize();

  std::unique_ptr<context::Context> context_; /* the context (uctx/raw/thread) that executes the user function */

  std::exception_ptr exception_;
  bool suspended_ = false;

  activity::MemoryAccessImplPtr recorded_memory_accesses_;
  /* the activities on which the actor is currently blocked with a wait(). There is more than one e.g. in a wait_any() */
  std::vector<activity::ActivityImplPtr> waiting_synchros_; 
  /* the activities linked to that actor. They are canceled when it dies, suspend/resumed when it is */
  std::set<activity::ActivityImplPtr> activities_; 
  Simcall simcall_;
  /* list of functions executed when the actor dies */
  std::shared_ptr<std::vector<std::function<void(bool)>>> on_exit =
      std::make_shared<std::vector<std::function<void(bool)>>>();

  std::function<void()> code_; // to restart the actor on host reboot
  timer::Timer* kill_timer_ = nullptr;

private:
  /* Refcounting */
  std::atomic_int_fast32_t refcount_{0};

public:
  int get_refcount() const { return static_cast<int>(refcount_); }
  friend void intrusive_ptr_add_ref(ActorImpl* actor)
  {
    // This whole memory consistency semantic drives me nuts.
    // std::memory_order_relaxed proves to not be enough: There is a threading issue when actors commit suicide.
    //   My guess is that the maestro context wants to propagate changes to the actor's fields after the
    //   actor context frees that memory area or something. But I'm not 100% certain of what's going on.
    // std::memory_order_seq_cst works but that's rather demanding.
    // AFAIK, std::memory_order_acq_rel works on all tested platforms, so let's stick to it.
    // Reducing the requirements to _relaxed would require to fix our suicide procedure, which is a messy piece of code.
    actor->refcount_.fetch_add(1, std::memory_order_acq_rel);
  }
  friend void intrusive_ptr_release(ActorImpl* actor)
  {
    // inspired from http://www.boost.org/doc/libs/1_55_0/doc/html/atomic/usage_examples.html
    if (actor->refcount_.fetch_sub(1, std::memory_order_release) == 1) {
      // Make sure that any changes done on other threads before their acquire are committed before our delete
      // http://stackoverflow.com/questions/27751025/why-is-an-acquire-barrier-needed-before-deleting-the-data-in-an-atomically-refer
      std::atomic_thread_fence(std::memory_order_acquire);
      delete actor;
    }
  }

  simgrid::kernel::activity::MemoryAccessImpl* get_memory_access() { return recorded_memory_accesses_.get(); }

  /* S4U/implem interfaces */
private:
  s4u::Actor piface_; // Our interface is part of ourselves


public:
  s4u::ActorPtr get_iface() { return s4u::ActorPtr(&piface_); }
  s4u::Actor* get_ciface() { return &piface_; }

  ActorImplPtr init(const std::string& name, s4u::Host* host) const;
  ActorImpl* start(const ActorCode& code);

  static ActorImplPtr create(const std::string& name, const ActorCode& code, void* data, s4u::Host* host,
                             const ActorImpl* parent_actor);
  static ActorImplPtr create(ProcessArg* args);
  static ActorImplPtr attach(const std::string& name, void* data, s4u::Host* host);
  static void detach();
  void cleanup_from_self();
  void cleanup_from_kernel();
  void exit();
  void kill(ActorImpl* actor) const;
  void kill_all() const;

  void yield();
  bool is_suspended() const { return suspended_; }
  s4u::Actor* restart();
  void suspend();
  void resume();
  activity::ActivityImplPtr join(const ActorImpl* actor, double timeout);
  activity::ActivityImplPtr sleep(double duration);
  /** Ask the actor to throw an exception right away */
  void throw_exception(std::exception_ptr e);

  /** execute the pending simcall -- must be called from the maestro context */
  void simcall_handle(int value);
  /** Terminates a simcall currently executed in maestro context. The actor will be restarted in the next scheduling
   * round */
  void simcall_answer();
};

class ProcessArg {
public:
  std::string name;
  std::function<void()> code;
  void* data                                                               = nullptr;
  s4u::Host* host                                                          = nullptr;
  double kill_time                                                         = 0.0;
  const std::unordered_map<std::string, std::string> properties{};
  bool auto_restart                                                        = false;
  bool daemon_;
  /* list of functions executed when the actor dies */
  const std::shared_ptr<std::vector<std::function<void(bool)>>> on_exit;
  int restart_count_ = 0;

  ProcessArg()                  = delete;
  ProcessArg(const ProcessArg&) = delete;
  ProcessArg& operator=(const ProcessArg&) = delete;

  explicit ProcessArg(const std::string& name, const std::function<void()>& code, void* data, s4u::Host* host,
                      double kill_time, const std::unordered_map<std::string, std::string>& properties,
                      bool auto_restart, bool daemon, int restart_count)
      : name(name)
      , code(code)
      , data(data)
      , host(host)
      , kill_time(kill_time)
      , properties(properties)
      , auto_restart(auto_restart)
      , daemon_(daemon)
      , restart_count_(restart_count)
  {
  }

  explicit ProcessArg(s4u::Host* host, ActorImpl* actor)
      : name(actor->get_name())
      , code(actor->code_)
      , data(actor->get_ciface()->get_data<void>())
      , host(host)
      , kill_time(actor->get_kill_time())
      , auto_restart(actor->has_to_auto_restart())
      , daemon_(actor->is_daemon())
      , on_exit(actor->on_exit)
      , restart_count_(actor->get_restart_count() + 1)
  {
  }
};

/* Used to keep the list of actors blocked on a synchro  */
using SynchroList =
    boost::intrusive::list<ActorImpl, boost::intrusive::member_hook<ActorImpl, boost::intrusive::list_member_hook<>,
                                                                    &ActorImpl::smx_synchro_hook>>;

XBT_PUBLIC void create_maestro(const std::function<void()>& code);

} // namespace simgrid::kernel::actor

#endif
