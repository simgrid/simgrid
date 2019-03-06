/* Copyright (c) 2006-2019. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/Exception.hpp"
#include "simgrid/actor.h"
#include "simgrid/s4u/Actor.hpp"
#include "simgrid/s4u/Exec.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/s4u/VirtualMachine.hpp"
#include "src/kernel/activity/ExecImpl.hpp"
#include "src/simix/smx_private.hpp"
#include "src/surf/HostImpl.hpp"

#include <algorithm>
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
  smx_context_t self_context = simgrid::kernel::context::Context::self();
  if (self_context == nullptr)
    return simgrid::s4u::ActorPtr();

  return self_context->get_actor()->iface();
}

ActorPtr Actor::create(std::string name, s4u::Host* host, std::function<void()> code)
{
  simgrid::kernel::actor::ActorImpl* actor =
      simcall_process_create(std::move(name), std::move(code), nullptr, host, nullptr);
  return actor->iface();
}

ActorPtr Actor::create(std::string name, s4u::Host* host, const std::string& function, std::vector<std::string> args)
{
  simgrid::simix::ActorCodeFactory& factory = SIMIX_get_actor_code_factory(function);
  return create(std::move(name), host, factory(std::move(args)));
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
    xbt_assert(autorestart && not pimpl_->has_to_auto_restart()); // FIXME: handle all cases
    pimpl_->set_auto_restart(autorestart);

    simgrid::kernel::actor::ProcessArg* arg = new simgrid::kernel::actor::ProcessArg(pimpl_->get_host(), pimpl_);
    XBT_DEBUG("Adding Process %s to the actors_at_boot_ list of Host %s", arg->name.c_str(), arg->host->get_cname());
    pimpl_->get_host()->pimpl_->actors_at_boot_.emplace_back(arg);
  });
}

void Actor::on_exit(int_f_pvoid_pvoid_t fun,
                    void* data) /* deprecated: cleanup SIMIX_process_on_exit: change prototype of second parameter and
                                   remove the last one */
{
  simgrid::simix::simcall([this, fun, data] { SIMIX_process_on_exit(pimpl_, fun, data); });
}

void Actor::on_exit(std::function<void(bool /*failed*/)> const fun)
{
  simgrid::simix::simcall(
      [this, fun] { SIMIX_process_on_exit(pimpl_, [fun](int a, void* /*data*/) { fun(a != 0); }, nullptr); });
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
    this->pimpl_->set_host(new_host);
  });

  s4u::Actor::on_migration_end(this);
}

s4u::Host* Actor::get_host()
{
  return this->pimpl_->get_host();
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
  return this->pimpl_->get_pid();
}

aid_t Actor::get_ppid() const
{
  return this->pimpl_->get_ppid();
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
  return simgrid::simix::simcall([this] { return pimpl_->is_suspended(); });
}

void Actor::set_kill_time(double kill_time)
{
  simgrid::simix::simcall([this, kill_time] { pimpl_->set_kill_time(kill_time); });
}

/** @brief Get the kill time of an actor(or 0 if unset). */
double Actor::get_kill_time()
{
  return pimpl_->get_kill_time();
}

void Actor::kill(aid_t pid) // deprecated
{
  smx_actor_t killer  = SIMIX_process_self();
  smx_actor_t victim  = SIMIX_process_from_PID(pid);
  if (victim != nullptr) {
    simgrid::simix::simcall([killer, victim] { killer->kill(victim); });
  } else {
    std::ostringstream oss;
    oss << "kill: (" << pid << ") - No such actor" << std::endl;
    throw std::runtime_error(oss.str());
  }
}

void Actor::kill()
{
  smx_actor_t process = SIMIX_process_self();
  simgrid::simix::simcall([this, process] {
    if (pimpl_ == simix_global->maestro_process)
      pimpl_->exit();
    else
      process->kill(pimpl_);
  });
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
  simgrid::simix::simcall([self] { self->kill_all(); });
}

std::unordered_map<std::string, std::string>* Actor::get_properties()
{
  return simgrid::simix::simcall([this] { return this->pimpl_->get_properties(); });
}

/** Retrieve the property value (or nullptr if not set) */
const char* Actor::get_property(const std::string& key)
{
  return simgrid::simix::simcall([this, key] { return pimpl_->get_property(key); });
}

void Actor::set_property(const std::string& key, std::string value)
{
  simgrid::simix::simcall([this, key, value] { pimpl_->set_property(key, std::move(value)); });
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

void parallel_execute(const std::vector<s4u::Host*>& hosts, const std::vector<double>& flops_amounts,
                      const std::vector<double>& bytes_amounts)
{
  parallel_execute(hosts, flops_amounts, bytes_amounts, -1);
}

void parallel_execute(const std::vector<s4u::Host*>& hosts, const std::vector<double>& flops_amounts,
                      const std::vector<double>& bytes_amounts, double timeout)
{
  xbt_assert(hosts.size() > 0, "Your parallel executions must span over at least one host.");
  xbt_assert(hosts.size() == flops_amounts.size() || flops_amounts.empty(),
             "Host count (%zu) does not match flops_amount count (%zu).", hosts.size(), flops_amounts.size());
  xbt_assert(hosts.size() * hosts.size() == bytes_amounts.size() || bytes_amounts.empty(),
             "bytes_amounts must be a matrix of size host_count * host_count (%zu*%zu), but it's of size %zu.",
             hosts.size(), hosts.size(), flops_amounts.size());
  /* Check that we are not mixing VMs and PMs in the parallel task */
  bool is_a_vm = (nullptr != dynamic_cast<simgrid::s4u::VirtualMachine*>(hosts.front()));
  xbt_assert(std::all_of(hosts.begin(), hosts.end(),
                         [is_a_vm](s4u::Host* elm) {
                           bool tmp_is_a_vm = (nullptr != dynamic_cast<simgrid::s4u::VirtualMachine*>(elm));
                           return is_a_vm == tmp_is_a_vm;
                         }),
             "parallel_execute: mixing VMs and PMs is not supported (yet).");
  /* checking for infinite values */
  xbt_assert(std::all_of(flops_amounts.begin(), flops_amounts.end(), [](double elm) { return std::isfinite(elm); }),
             "flops_amounts comprises infinite values!");
  xbt_assert(std::all_of(bytes_amounts.begin(), bytes_amounts.end(), [](double elm) { return std::isfinite(elm); }),
             "flops_amounts comprises infinite values!");

  exec_init(hosts, flops_amounts, bytes_amounts)->set_timeout(timeout)->wait();
}

// deprecated
void parallel_execute(int host_nb, s4u::Host* const* host_list, const double* flops_amount, const double* bytes_amount,
                      double timeout)
{
  smx_activity_t s =
      simcall_execution_parallel_start("", host_nb, host_list, flops_amount, bytes_amount, /* rate */ -1, timeout);
  simcall_execution_wait(s);
  delete[] flops_amount;
  delete[] bytes_amount;
}

// deprecated
void parallel_execute(int host_nb, s4u::Host* const* host_list, const double* flops_amount, const double* bytes_amount)
{
  smx_activity_t s = simcall_execution_parallel_start("", host_nb, host_list, flops_amount, bytes_amount,
                                                      /* rate */ -1, /*timeout*/ -1);
  simcall_execution_wait(s);
  delete[] flops_amount;
  delete[] bytes_amount;
}

ExecPtr exec_init(double flops_amount)
{
  return ExecPtr(new ExecSeq(get_host(), flops_amount));
}

ExecPtr exec_init(const std::vector<s4u::Host*>& hosts, const std::vector<double>& flops_amounts,
                  const std::vector<double>& bytes_amounts)
{
  return ExecPtr(new ExecPar(hosts, flops_amounts, bytes_amounts));
}

ExecPtr exec_async(double flops)
{
  ExecPtr res = exec_init(flops);
  res->start();
  return res;
}

aid_t get_pid()
{
  return SIMIX_process_self()->get_pid();
}

aid_t get_ppid()
{
  return SIMIX_process_self()->get_ppid();
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
  return SIMIX_process_self()->get_host();
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

void exit()
{
  smx_actor_t actor = SIMIX_process_self();
  simgrid::simix::simcall([actor] { actor->exit(); });
}

void on_exit(std::function<void(bool)> const fun)
{
  SIMIX_process_self()->iface()->on_exit(fun);
}

void on_exit(std::function<void(int, void*)> const fun, void* data) /* deprecated */
{
  SIMIX_process_self()->iface()->on_exit([fun, data](bool exit) { fun(exit, data); });
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
void on_exit(int_f_pvoid_pvoid_t fun, void* data) /* deprecated */
{
  SIMIX_process_self()->iface()->on_exit([fun, data](int a) { fun((void*)(intptr_t)a, data); });
}
void onExit(int_f_pvoid_pvoid_t fun, void* data) /* deprecated */
{
  on_exit([fun, data](int a) { fun((void*)(intptr_t)a, data); });
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
aid_t sg_actor_get_PID(sg_actor_t actor)
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
aid_t sg_actor_get_PPID(sg_actor_t actor)
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

void sg_actor_sleep_for(double duration)
{
  simgrid::s4u::this_actor::sleep_for(duration);
}

sg_actor_t sg_actor_attach(const char* name, void* data, sg_host_t host, xbt_dict_t properties)
{
  xbt_assert(host != nullptr, "Invalid parameters: host and code params must not be nullptr");
  std::unordered_map<std::string, std::string> props;
  xbt_dict_cursor_t cursor = nullptr;
  char* key;
  char* value;
  xbt_dict_foreach (properties, cursor, key, value)
    props[key] = value;
  xbt_dict_free(&properties);

  /* Let's create the process: SIMIX may decide to start it right now, even before returning the flow control to us */
  smx_actor_t actor = nullptr;
  try {
    actor = simgrid::kernel::actor::ActorImpl::attach(name, data, host, &props).get();
  } catch (simgrid::HostFailureException const&) {
    xbt_die("Could not attach");
  }

  simgrid::s4u::this_actor::yield();
  return actor->ciface();
}

void sg_actor_detach()
{
  simgrid::kernel::actor::ActorImpl::detach();
}
