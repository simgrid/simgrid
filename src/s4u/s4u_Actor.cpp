/* Copyright (c) 2006-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/Actor.hpp"
#include "simgrid/s4u/Exec.hpp"
#include "simgrid/s4u/Host.hpp"
#include "src/instr/instr_private.hpp"
#include "src/simix/smx_private.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_actor, "S4U actors");

namespace simgrid {
namespace s4u {

simgrid::xbt::signal<void(simgrid::s4u::ActorPtr)> s4u::Actor::on_creation;
simgrid::xbt::signal<void(simgrid::s4u::ActorPtr)> s4u::Actor::on_destruction;

// ***** Actor creation *****
ActorPtr Actor::self()
{
  smx_context_t self_context = SIMIX_context_self();
  if (self_context == nullptr)
    return simgrid::s4u::ActorPtr();

  return self_context->process()->iface();
}

ActorPtr Actor::create(const char* name, s4u::Host* host, std::function<void()> code)
{
  simgrid::kernel::actor::ActorImpl* actor = simcall_process_create(name, std::move(code), nullptr, host, nullptr);
  return actor->iface();
}

ActorPtr Actor::create(const char* name, s4u::Host* host, const char* function, std::vector<std::string> args)
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
  simgrid::simix::kernelImmediate([this, autorestart]() { pimpl_->auto_restart = autorestart; });
}

void Actor::on_exit(int_f_pvoid_pvoid_t fun, void* data)
{
  simgrid::simix::kernelImmediate([&] { SIMIX_process_on_exit(pimpl_, fun, data); });
}

/** @brief Moves the actor to another host
 *
 * If the actor is currently blocked on an execution activity, the activity is also
 * migrated to the new host. If it's blocked on another kind of activity, an error is
 * raised as the mandated code is not written yet. Please report that bug if you need it.
 *
 * Asynchronous activities started by the actor are not migrated automatically, so you have
 * to take care of this yourself (only you knows which ones should be migrated).
 */
void Actor::migrate(Host* new_host)
{
  std::string key;
  simgrid::instr::LinkType* link = nullptr;
  bool tracing                   = TRACE_actor_is_enabled();
  if (tracing) {
    static long long int counter = 0;

    key = std::to_string(counter);
    counter++;

    // start link
    container_t actor_container = simgrid::instr::Container::byName(instr_pid(this));
    link                        = simgrid::instr::Container::getRoot()->getLink("ACTOR_LINK");
    link->startEvent(actor_container, "M", key);

    // destroy existing container of this process
    actor_container->removeFromParent();
  }

  simgrid::simix::kernelImmediate([this, new_host]() {
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

  if (tracing) {
    // create new container on the new_host location
    simgrid::instr::Container::byName(new_host->get_name())->createChild(instr_pid(this), "ACTOR");
    // end link
    link->endEvent(simgrid::instr::Container::byName(instr_pid(this)), "M", key);
  }
}

s4u::Host* Actor::get_host()
{
  return this->pimpl_->host;
}

void Actor::daemonize()
{
  simgrid::simix::kernelImmediate([this]() { pimpl_->daemonize(); });
}

bool Actor::is_daemon() const
{
  return this->pimpl_->isDaemon();
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
  return this->pimpl_->pid;
}

aid_t Actor::get_ppid() const
{
  return this->pimpl_->ppid;
}

void Actor::suspend()
{
  if (TRACE_actor_is_enabled())
    simgrid::instr::Container::byName(instr_pid(this))->getState("ACTOR_STATE")->pushEvent("suspend");

  simcall_process_suspend(pimpl_);
}

void Actor::resume()
{
  simgrid::simix::kernelImmediate([this] { pimpl_->resume(); });
  if (TRACE_actor_is_enabled())
    simgrid::instr::Container::byName(instr_pid(this))->getState("ACTOR_STATE")->popEvent();
}

int Actor::is_suspended()
{
  return simgrid::simix::kernelImmediate([this] { return pimpl_->suspended; });
}

void Actor::set_kill_time(double time)
{
  simcall_process_set_kill_time(pimpl_, time);
}

/** \brief Get the kill time of an actor(or 0 if unset). */
double Actor::get_kill_time()
{
  return SIMIX_timer_get_date(pimpl_->kill_timer);
}

void Actor::kill(aid_t pid)
{
  smx_actor_t killer  = SIMIX_process_self();
  smx_actor_t process = SIMIX_process_from_PID(pid);
  if (process != nullptr) {
    simgrid::simix::kernelImmediate([killer, process] { SIMIX_process_kill(process, killer); });
  } else {
    std::ostringstream oss;
    oss << "kill: (" << pid << ") - No such actor" << std::endl;
    throw std::runtime_error(oss.str());
  }
}

void Actor::kill()
{
  smx_actor_t process = SIMIX_process_self();
  simgrid::simix::kernelImmediate(
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
  simgrid::simix::kernelImmediate([&self] { SIMIX_process_killall(self); });
}

std::map<std::string, std::string>* Actor::get_properties()
{
  return simgrid::simix::kernelImmediate([this] { return this->pimpl_->getProperties(); });
}

/** Retrieve the property value (or nullptr if not set) */
const char* Actor::get_property(const char* key)
{
  return simgrid::simix::kernelImmediate([this, key] { return pimpl_->getProperty(key); });
}

void Actor::set_property(const char* key, const char* value)
{
  simgrid::simix::kernelImmediate([this, key, value] { pimpl_->setProperty(key, value); });
}

Actor* Actor::restart()
{
  return simgrid::simix::kernelImmediate([this]() { return pimpl_->restart(); });
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
  if (duration > 0)
    simcall_process_sleep(duration);
}

void yield()
{
  simgrid::simix::kernelImmediate([] { /* do nothing*/ });
}

XBT_PUBLIC void sleep_until(double timeout)
{
  double now = SIMIX_get_clock();
  if (timeout > now)
    simcall_process_sleep(timeout - now);
}

void execute(double flops)
{
  smx_activity_t s = simcall_execution_start(nullptr, flops, 1.0 /*priority*/, 0. /*bound*/, get_host());
  simcall_execution_wait(s);
}

void execute(double flops, double priority)
{
  smx_activity_t s = simcall_execution_start(nullptr, flops, 1 / priority /*priority*/, 0. /*bound*/, get_host());
  simcall_execution_wait(s);
}

void parallel_execute(int host_nb, sg_host_t* host_list, double* flops_amount, double* bytes_amount, double timeout)
{
  smx_activity_t s =
      simcall_execution_parallel_start(nullptr, host_nb, host_list, flops_amount, bytes_amount, -1, timeout);
  simcall_execution_wait(s);
}

void parallel_execute(int host_nb, sg_host_t* host_list, double* flops_amount, double* bytes_amount)
{
  smx_activity_t s = simcall_execution_parallel_start(nullptr, host_nb, host_list, flops_amount, bytes_amount, -1, -1);
  simcall_execution_wait(s);
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
  return SIMIX_process_self()->pid;
}

aid_t get_ppid()
{
  return SIMIX_process_self()->ppid;
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
  return SIMIX_process_self()->host;
}

void suspend()
{
  if (TRACE_actor_is_enabled())
    instr::Container::byName(get_name() + "-" + std::to_string(get_pid()))
        ->getState("ACTOR_STATE")
        ->pushEvent("suspend");
  simcall_process_suspend(SIMIX_process_self());
}

void resume()
{
  smx_actor_t process = SIMIX_process_self();
  simgrid::simix::kernelImmediate([process] { process->resume(); });

  if (TRACE_actor_is_enabled())
    instr::Container::byName(get_name() + "-" + std::to_string(get_pid()))->getState("ACTOR_STATE")->popEvent();
}

bool is_suspended()
{
  smx_actor_t process = SIMIX_process_self();
  return simgrid::simix::kernelImmediate([process] { return process->suspended; });
}

void kill()
{
  smx_actor_t process = SIMIX_process_self();
  simgrid::simix::kernelImmediate([process] { SIMIX_process_kill(process, process); });
}

void on_exit(int_f_pvoid_pvoid_t fun, void* data)
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
void onExit /* deprecated */ (int_f_pvoid_pvoid_t fun, void* data)
{
  on_exit(fun, data);
}

} // namespace this_actor
} // namespace s4u
} // namespace simgrid

/* **************************** Public C interface *************************** */

/** \ingroup m_actor_management
 * \brief Returns the process ID of \a actor.
 *
 * This function checks whether \a actor is a valid pointer and return its PID (or 0 in case of problem).
 */
int sg_actor_get_PID(sg_actor_t actor)
{
  /* Do not raise an exception here: this function is called by the logs
   * and the exceptions, so it would be called back again and again */
  if (actor == nullptr || actor->get_impl() == nullptr)
    return 0;
  return actor->get_pid();
}

/** \ingroup m_actor_management
 * \brief Returns the process ID of the parent of \a actor.
 *
 * This function checks whether \a actor is a valid pointer and return its parent's PID.
 * Returns -1 if the actor has not been created by any other actor.
 */
int sg_actor_get_PPID(sg_actor_t actor)
{
  return actor->get_ppid();
}

/** \ingroup m_actor_management
 * \brief Return the name of an actor.
 */
const char* sg_actor_get_name(sg_actor_t actor)
{
  return actor->get_cname();
}

sg_host_t sg_actor_get_host(sg_actor_t actor)
{
  return actor->get_host();
}

/** \ingroup m_actor_management
 * \brief Returns the value of a given actor property
 *
 * \param actor an actor
 * \param name a property name
 * \return value of a property (or nullptr if the property is not set)
 */
const char* sg_actor_get_property_value(sg_actor_t actor, const char* name)
{
  return actor->get_property(name);
}

/** \ingroup m_actor_management
 * \brief Return the list of properties
 *
 * This function returns all the parameters associated with an actor
 */
xbt_dict_t sg_actor_get_properties(sg_actor_t actor)
{
  xbt_assert(actor != nullptr, "Invalid parameter: First argument must not be nullptr");
  xbt_dict_t as_dict                        = xbt_dict_new_homogeneous(xbt_free_f);
  std::map<std::string, std::string>* props = actor->get_properties();
  if (props == nullptr)
    return nullptr;
  for (auto const& elm : *props) {
    xbt_dict_set(as_dict, elm.first.c_str(), xbt_strdup(elm.second.c_str()), nullptr);
  }
  return as_dict;
}

/** \ingroup m_actor_management
 * \brief Suspend the actor.
 *
 * This function suspends the actor by suspending the task on which it was waiting for the completion.
 */
void sg_actor_suspend(sg_actor_t actor)
{
  xbt_assert(actor != nullptr, "Invalid parameter: First argument must not be nullptr");
  actor->suspend();
}

/** \ingroup m_actor_management
 * \brief Resume a suspended actor.
 *
 * This function resumes a suspended actor by resuming the task on which it was waiting for the completion.
 */
void sg_actor_resume(sg_actor_t actor)
{
  xbt_assert(actor != nullptr, "Invalid parameter: First argument must not be nullptr");
  actor->resume();
}

/** \ingroup m_actor_management
 * \brief Returns true if the actor is suspended .
 *
 * This checks whether an actor is suspended or not by inspecting the task on which it was waiting for the completion.
 */
int sg_actor_is_suspended(sg_actor_t actor)
{
  return actor->is_suspended();
}

/**
 * \ingroup m_actor_management
 * \brief Restarts an actor from the beginning.
 */
sg_actor_t sg_actor_restart(sg_actor_t actor)
{
  return actor->restart();
}

/** @ingroup m_actor_management
 * @brief This actor will be terminated automatically when the last non-daemon actor finishes
 */
void sg_actor_daemonize(sg_actor_t actor)
{
  actor->daemonize();
}

/** \ingroup m_actor_management
 * \brief Migrates an actor to another location.
 *
 * This function changes the value of the #sg_host_t on  which \a actor is running.
 */
void sg_actor_migrate(sg_actor_t process, sg_host_t host)
{
  process->migrate(host);
}

/** \ingroup m_actor_management
 * \brief Wait for the completion of a #sg_actor_t.
 *
 * \param actor the actor to wait for
 * \param timeout wait until the actor is over, or the timeout expires
 */
void sg_actor_join(sg_actor_t actor, double timeout)
{
  actor->join(timeout);
}

void sg_actor_kill(sg_actor_t actor)
{
  actor->kill();
}

/** \ingroup m_actor_management
 * \brief Set the kill time of an actor.
 *
 * \param actor an actor
 * \param kill_time the time when the actor is killed.
 */
void sg_actor_set_kill_time(sg_actor_t actor, double kill_time)
{
  actor->set_kill_time(kill_time);
}
