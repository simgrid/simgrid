/* Copyright (c) 2006-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/actor.h"
#include "simgrid/s4u/Actor.hpp"
#include "simgrid/s4u/Exec.hpp"
#include "simgrid/s4u/Host.hpp"
#include "src/kernel/activity/ExecImpl.hpp"
#include "src/simix/smx_host_private.hpp"
#include "src/simix/smx_private.hpp"
#include "src/surf/HostImpl.hpp"

#include <sstream>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_actor, "S4U actors");

namespace simgrid {
namespace s4u {

simgrid::xbt::signal<void(simgrid::s4u::ActorPtr)> s4u::Actor::on_creation;
simgrid::xbt::signal<void(simgrid::s4u::ActorPtr)> s4u::Actor::on_suspend;
simgrid::xbt::signal<void(simgrid::s4u::ActorPtr)> s4u::Actor::on_resume;
simgrid::xbt::signal<void(simgrid::s4u::ActorPtr)> s4u::Actor::on_sleep;
simgrid::xbt::signal<void(simgrid::s4u::ActorPtr)> s4u::Actor::on_wake_up;
simgrid::xbt::signal<void(simgrid::s4u::ActorPtr)> s4u::Actor::on_migration_start;
simgrid::xbt::signal<void(simgrid::s4u::ActorPtr)> s4u::Actor::on_migration_end;
simgrid::xbt::signal<void(simgrid::s4u::ActorPtr)> s4u::Actor::on_destruction;

// ***** Actor creation *****
ActorPtr Actor::self()
{
  smx_context_t self_context = SIMIX_context_self();
  if (self_context == nullptr)
    return simgrid::s4u::ActorPtr();

  return self_context->process()->iface();
}

ActorPtr Actor::create(std::string name, s4u::Host* host, std::function<void()> code)
{
  simgrid::kernel::actor::ActorImpl* actor = simcall_process_create(name, std::move(code), nullptr, host, nullptr);
  return actor->iface();
}

ActorPtr Actor::create(std::string name, s4u::Host* host, std::string function, std::vector<std::string> args)
{
  simgrid::simix::ActorCodeFactory& factory = SIMIX_get_actor_code_factory(function);
  simgrid::simix::ActorCode code            = factory(std::move(args));
  simgrid::kernel::actor::ActorImpl* actor  = simcall_process_create(name, std::move(code), nullptr, host, nullptr);
  return actor->iface();
}

void intrusive_ptr_add_ref(Actor* actor)
{
  intrusive_ptr_add_ref(actor->pimpl_);
}
void intrusive_ptr_release(Actor* actor)
{
  intrusive_ptr_release(actor->pimpl_);
}

// ***** Actor methods *****

void Actor::join()
{
  simcall_process_join(this->pimpl_, -1);
}

void Actor::join(double timeout)
{
  simcall_process_join(this->pimpl_, timeout);
}

void Actor::set_auto_restart(bool autorestart)
{
  simgrid::simix::simcall([this, autorestart]() {
    xbt_assert(autorestart && not pimpl_->auto_restart_); // FIXME: handle all cases
    pimpl_->set_auto_restart(autorestart);

    simgrid::kernel::actor::ProcessArg* arg = new simgrid::kernel::actor::ProcessArg(pimpl_->host_, pimpl_);
    XBT_DEBUG("Adding Process %s to the actors_at_boot_ list of Host %s", arg->name.c_str(), arg->host->get_cname());
    pimpl_->host_->pimpl_->actors_at_boot_.emplace_back(arg);
  });
}

void Actor::on_exit(int_f_pvoid_pvoid_t fun, void* data) /* deprecated */
{
  simgrid::simix::simcall([this, fun, data] { SIMIX_process_on_exit(pimpl_, fun, data); });
}

void Actor::on_exit(std::function<void(int, void*)> fun, void* data)
{
  simgrid::simix::simcall([this, fun, data] { SIMIX_process_on_exit(pimpl_, fun, data); });
}

void Actor::migrate(Host* new_host)
{
  s4u::Actor::on_migration_start(this);

  simgrid::simix::simcall([this, new_host]() {
    if (pimpl_->waiting_synchro != nullptr) {
      // The actor is blocked on an activity. If it's an exec, migrate it too.
      // FIXME: implement the migration of other kind of activities
      simgrid::kernel::activity::ExecImplPtr exec =
          boost::dynamic_pointer_cast<simgrid::kernel::activity::ExecImpl>(pimpl_->waiting_synchro);
      xbt_assert(exec.get() != nullptr, "We can only migrate blocked actors when they are blocked on executions.");
      exec->migrate(new_host);
    }
    SIMIX_process_change_host(this->pimpl_, new_host);
  });

  s4u::Actor::on_migration_end(this);
}

s4u::Host* Actor::get_host()
{
  return this->pimpl_->host_;
}

void Actor::daemonize()
{
  simgrid::simix::simcall([this]() { pimpl_->daemonize(); });
}

bool Actor::is_daemon() const
{
  return this->pimpl_->is_daemon();
}

const simgrid::xbt::string& Actor::get_name() const
{
  return this->pimpl_->get_name();
}

const char* Actor::get_cname() const
{
  return this->pimpl_->get_cname();
}

aid_t Actor::get_pid() const
{
  return this->pimpl_->pid_;
}

aid_t Actor::get_ppid() const
{
  return this->pimpl_->ppid_;
}

void Actor::suspend()
{
  s4u::Actor::on_suspend(this);
  simcall_process_suspend(pimpl_);
}

void Actor::resume()
{
  simgrid::simix::simcall([this] { pimpl_->resume(); });
  s4u::Actor::on_resume(this);
}

bool Actor::is_suspended()
{
  return simgrid::simix::simcall([this] { return pimpl_->suspended_; });
}

void Actor::set_kill_time(double time)
{
  simcall_process_set_kill_time(pimpl_, time);
}

/** @brief Get the kill time of an actor(or 0 if unset). */
double Actor::get_kill_time()
{
  return SIMIX_timer_get_date(pimpl_->kill_timer);
}

void Actor::kill(aid_t pid)
{
  smx_actor_t killer  = SIMIX_process_self();
  smx_actor_t process = SIMIX_process_from_PID(pid);
  if (process != nullptr) {
    simgrid::simix::simcall([killer, process] { SIMIX_process_kill(process, killer); });
  } else {
    std::ostringstream oss;
    oss << "kill: (" << pid << ") - No such actor" << std::endl;
    throw std::runtime_error(oss.str());
  }
}

void Actor::kill()
{
  smx_actor_t process = SIMIX_process_self();
  simgrid::simix::simcall(
      [this, process] { SIMIX_process_kill(pimpl_, (pimpl_ == simix_global->maestro_process) ? pimpl_ : process); });
}

smx_actor_t Actor::get_impl()
{
  return pimpl_;
}

// ***** Static functions *****

ActorPtr Actor::by_pid(aid_t pid)
{
  smx_actor_t process = SIMIX_process_from_PID(pid);
  if (process != nullptr)
    return process->iface();
  else
    return ActorPtr();
}

void Actor::kill_all()
{
  smx_actor_t self = SIMIX_process_self();
  simgrid::simix::simcall([&self] { SIMIX_process_killall(self); });
}

std::unordered_map<std::string, std::string>* Actor::get_properties()
{
  return simgrid::simix::simcall([this] { return this->pimpl_->get_properties(); });
}

/** Retrieve the property value (or nullptr if not set) */
const char* Actor::get_property(std::string key)
{
  return simgrid::simix::simcall([this, key] { return pimpl_->get_property(key); });
}

void Actor::set_property(std::string key, std::string value)
{
  simgrid::simix::simcall([this, key, value] { pimpl_->set_property(key, value); });
}

Actor* Actor::restart()
{
  return simgrid::simix::simcall([this]() { return pimpl_->restart(); });
}

// ***** this_actor *****

namespace this_actor {

/** Returns true if run from the kernel mode, and false if run from a real actor
 *
 * Everything that is run out of any actor (simulation setup before the engine is run,
 * computing the model evolutions as a result to the actors' action, etc) is run in
 * kernel mode, just as in any operating systems.
 *
 * In SimGrid, the actor in charge of doing the stuff in kernel mode is called Maestro,
 * because it is the one scheduling when the others should move or wait.
 */
bool is_maestro()
{
  smx_actor_t process = SIMIX_process_self();
  return process == nullptr || process == simix_global->maestro_process;
}

void sleep_for(double duration)
{
  if (duration > 0) {
    smx_actor_t actor = SIMIX_process_self();
    simgrid::s4u::Actor::on_sleep(actor->iface());

    simcall_process_sleep(duration);

    simgrid::s4u::Actor::on_wake_up(actor->iface());
  }
}

void yield()
{
  simgrid::simix::simcall([] { /* do nothing*/ });
}

XBT_PUBLIC void sleep_until(double timeout)
{
  double now = SIMIX_get_clock();
  if (timeout > now)
    sleep_for(timeout - now);
}

void execute(double flops)
{
  execute(flops, 1.0 /* priority */);
}

void execute(double flops, double priority)
{
  exec_init(flops)->set_priority(priority)->start()->wait();
}

void parallel_execute(int host_nb, s4u::Host** host_list, double* flops_amount, double* bytes_amount, double timeout)
{
  smx_activity_t s =
      simcall_execution_parallel_start("", host_nb, host_list, flops_amount, bytes_amount, /* rate */ -1, timeout);
  simcall_execution_wait(s);
}

void parallel_execute(int host_nb, sg_host_t* host_list, double* flops_amount, double* bytes_amount)
{
  parallel_execute(host_nb, host_list, flops_amount, bytes_amount, /* timeout */ -1);
}

ExecPtr exec_init(double flops_amount)
{
  ExecPtr res        = ExecPtr(new Exec());
  res->host_         = get_host();
  res->flops_amount_ = flops_amount;
  res->set_remaining(flops_amount);
  return res;
}

ExecPtr exec_async(double flops)
{
  ExecPtr res = exec_init(flops);
  res->start();
  return res;
}

aid_t get_pid()
{
  return SIMIX_process_self()->pid_;
}

aid_t get_ppid()
{
  return SIMIX_process_self()->ppid_;
}

std::string get_name()
{
  return SIMIX_process_self()->get_name();
}

const char* get_cname()
{
  return SIMIX_process_self()->get_cname();
}

Host* get_host()
{
  return SIMIX_process_self()->host_;
}

void suspend()
{
  smx_actor_t actor = SIMIX_process_self();
  simgrid::s4u::Actor::on_suspend(actor->iface());

  simcall_process_suspend(actor);
}

void resume()
{
  smx_actor_t process = SIMIX_process_self();
  simgrid::simix::simcall([process] { process->resume(); });
  simgrid::s4u::Actor::on_resume(process->iface());
}

bool is_suspended()
{
  smx_actor_t process = SIMIX_process_self();
  return simgrid::simix::simcall([process] { return process->suspended_; });
}

void exit()
{
  smx_actor_t process = SIMIX_process_self();
  simgrid::simix::simcall([process] { SIMIX_process_kill(process, process); });
}

void on_exit(std::function<void(int, void*)> fun, void* data)
{
  SIMIX_process_self()->iface()->on_exit(fun, data);
}

/** @brief Moves the current actor to another host
 *
 * @see simgrid::s4u::Actor::migrate() for more information
 */
void migrate(Host* new_host)
{
  SIMIX_process_self()->iface()->migrate(new_host);
}

std::string getName() /* deprecated */
{
  return get_name();
}
const char* getCname() /* deprecated */
{
  return get_cname();
}
bool isMaestro() /* deprecated */
{
  return is_maestro();
}
aid_t getPid() /* deprecated */
{
  return get_pid();
}
aid_t getPpid() /* deprecated */
{
  return get_ppid();
}
Host* getHost() /* deprecated */
{
  return get_host();
}
bool isSuspended() /* deprecated */
{
  return is_suspended();
}
void on_exit(int_f_pvoid_pvoid_t fun, void* data) /* deprecated */
{
  SIMIX_process_self()->iface()->on_exit([fun](int a, void* b) { fun((void*)(intptr_t)a, b); }, data);
}
void onExit(int_f_pvoid_pvoid_t fun, void* data) /* deprecated */
{
  on_exit([fun](int a, void* b) { fun((void*)(intptr_t)a, b); }, data);
}
void kill() /* deprecated */
{
  exit();
}

} // namespace this_actor
} // namespace s4u
} // namespace simgrid

/* **************************** Public C interface *************************** */

/** @ingroup m_actor_management
 * @brief Returns the process ID of @a actor.
 *
 * This function checks whether @a actor is a valid pointer and return its PID (or 0 in case of problem).
 */
int sg_actor_get_PID(sg_actor_t actor)
{
  /* Do not raise an exception here: this function is called by the logs
   * and the exceptions, so it would be called back again and again */
  if (actor == nullptr || actor->get_impl() == nullptr)
    return 0;
  return actor->get_pid();
}

/** @ingroup m_actor_management
 * @brief Returns the process ID of the parent of @a actor.
 *
 * This function checks whether @a actor is a valid pointer and return its parent's PID.
 * Returns -1 if the actor has not been created by any other actor.
 */
int sg_actor_get_PPID(sg_actor_t actor)
{
  return actor->get_ppid();
}

/** @ingroup m_actor_management
 *
 * @brief Return a #sg_actor_t given its PID.
 *
 * This function search in the list of all the created sg_actor_t for a sg_actor_t  whose PID is equal to @a PID.
 * If none is found, @c nullptr is returned.
   Note that the PID are unique in the whole simulation, not only on a given host.
 */
sg_actor_t sg_actor_by_PID(aid_t pid)
{
  return simgrid::s4u::Actor::by_pid(pid).get();
}

/** @ingroup m_actor_management
 * @brief Return the name of an actor.
 */
const char* sg_actor_get_name(sg_actor_t actor)
{
  return actor->get_cname();
}

sg_host_t sg_actor_get_host(sg_actor_t actor)
{
  return actor->get_host();
}

/** @ingroup m_actor_management
 * @brief Returns the value of a given actor property
 *
 * @param actor an actor
 * @param name a property name
 * @return value of a property (or nullptr if the property is not set)
 */
const char* sg_actor_get_property_value(sg_actor_t actor, const char* name)
{
  return actor->get_property(name);
}

/** @ingroup m_actor_management
 * @brief Return the list of properties
 *
 * This function returns all the parameters associated with an actor
 */
xbt_dict_t sg_actor_get_properties(sg_actor_t actor)
{
  xbt_assert(actor != nullptr, "Invalid parameter: First argument must not be nullptr");
  xbt_dict_t as_dict                        = xbt_dict_new_homogeneous(xbt_free_f);
  std::unordered_map<std::string, std::string>* props = actor->get_properties();
  if (props == nullptr)
    return nullptr;
  for (auto const& kv : *props) {
    xbt_dict_set(as_dict, kv.first.c_str(), xbt_strdup(kv.second.c_str()), nullptr);
  }
  return as_dict;
}

/** @ingroup m_actor_management
 * @brief Suspend the actor.
 *
 * This function suspends the actor by suspending the task on which it was waiting for the completion.
 */
void sg_actor_suspend(sg_actor_t actor)
{
  xbt_assert(actor != nullptr, "Invalid parameter: First argument must not be nullptr");
  actor->suspend();
}

/** @ingroup m_actor_management
 * @brief Resume a suspended actor.
 *
 * This function resumes a suspended actor by resuming the task on which it was waiting for the completion.
 */
void sg_actor_resume(sg_actor_t actor)
{
  xbt_assert(actor != nullptr, "Invalid parameter: First argument must not be nullptr");
  actor->resume();
}

/** @ingroup m_actor_management
 * @brief Returns true if the actor is suspended .
 *
 * This checks whether an actor is suspended or not by inspecting the task on which it was waiting for the completion.
 */
int sg_actor_is_suspended(sg_actor_t actor)
{
  return actor->is_suspended();
}

/**
 * @ingroup m_actor_management
 * @brief Restarts an actor from the beginning.
 */
sg_actor_t sg_actor_restart(sg_actor_t actor)
{
  return actor->restart();
}

/**
 * @ingroup m_actor_management
 * @brief Sets the "auto-restart" flag of the actor.
 * If the flag is set to 1, the actor will be automatically restarted when its host comes back up.
 */
void sg_actor_set_auto_restart(sg_actor_t actor, int auto_restart)
{
  actor->set_auto_restart(auto_restart);
}

/** @ingroup m_actor_management
 * @brief This actor will be terminated automatically when the last non-daemon actor finishes
 */
void sg_actor_daemonize(sg_actor_t actor)
{
  actor->daemonize();
}

/** @ingroup m_actor_management
 * @brief Migrates an actor to another location.
 *
 * This function changes the value of the #sg_host_t on  which @a actor is running.
 */
void sg_actor_migrate(sg_actor_t process, sg_host_t host)
{
  process->migrate(host);
}

/** @ingroup m_actor_management
 * @brief Wait for the completion of a #sg_actor_t.
 *
 * @param actor the actor to wait for
 * @param timeout wait until the actor is over, or the timeout expires
 */
void sg_actor_join(sg_actor_t actor, double timeout)
{
  actor->join(timeout);
}

void sg_actor_kill(sg_actor_t actor)
{
  actor->kill();
}

void sg_actor_kill_all()
{
  simgrid::s4u::Actor::kill_all();
}

/** @ingroup m_actor_management
 * @brief Set the kill time of an actor.
 *
 * @param actor an actor
 * @param kill_time the time when the actor is killed.
 */
void sg_actor_set_kill_time(sg_actor_t actor, double kill_time)
{
  actor->set_kill_time(kill_time);
}

/** Yield the current actor; let the other actors execute first */
void sg_actor_yield()
{
  simgrid::s4u::this_actor::yield();
}
