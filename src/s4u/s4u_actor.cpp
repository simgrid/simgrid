/* Copyright (c) 2006-2018. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/log.h"

#include "simgrid/s4u/Actor.hpp"
#include "simgrid/s4u/Comm.hpp"
#include "simgrid/s4u/Exec.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/s4u/Mailbox.hpp"

#include "src/kernel/context/Context.hpp"
#include "src/simix/smx_private.hpp"

#include <sstream>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_actor, "S4U actors");

namespace simgrid {
namespace s4u {

simgrid::xbt::signal<void(simgrid::s4u::ActorPtr)> s4u::Actor::onCreation;
simgrid::xbt::signal<void(simgrid::s4u::ActorPtr)> s4u::Actor::onDestruction;

// ***** Actor creation *****
ActorPtr Actor::self()
{
  smx_context_t self_context = SIMIX_context_self();
  if (self_context == nullptr)
    return simgrid::s4u::ActorPtr();

  return self_context->process()->iface();
}

ActorPtr Actor::createActor(const char* name, s4u::Host* host, std::function<void()> code)
{
  simgrid::simix::ActorImpl* actor = simcall_process_create(name, std::move(code), nullptr, host, nullptr);
  return actor->iface();
}

ActorPtr Actor::createActor(const char* name, s4u::Host* host, const char* function, std::vector<std::string> args)
{
  simgrid::simix::ActorCodeFactory& factory = SIMIX_get_actor_code_factory(function);
  simgrid::simix::ActorCode code = factory(std::move(args));
  simgrid::simix::ActorImpl* actor          = simcall_process_create(name, std::move(code), nullptr, host, nullptr);
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

void Actor::join() {
  simcall_process_join(this->pimpl_, -1);
}

void Actor::join(double timeout)
{
  simcall_process_join(this->pimpl_, timeout);
}

void Actor::setAutoRestart(bool autorestart) {
  simgrid::simix::kernelImmediate([this, autorestart]() { pimpl_->auto_restart = autorestart; });
}

void Actor::onExit(int_f_pvoid_pvoid_t fun, void* data)
{
  simcall_process_on_exit(pimpl_, fun, data);
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
}

s4u::Host* Actor::getHost()
{
  return this->pimpl_->host;
}

void Actor::daemonize()
{
  simgrid::simix::kernelImmediate([this]() { pimpl_->daemonize(); });
}

bool Actor::isDaemon()
{
  return this->pimpl_->isDaemon();
}

const simgrid::xbt::string& Actor::getName() const
{
  return this->pimpl_->getName();
}

const char* Actor::getCname() const
{
  return this->pimpl_->getCname();
}

aid_t Actor::getPid()
{
  return this->pimpl_->pid;
}

aid_t Actor::getPpid()
{
  return this->pimpl_->ppid;
}

void Actor::suspend()
{
  if (TRACE_actor_is_enabled())
    simgrid::instr::Container::byName(instr_pid(this))->getState("MSG_PROCESS_STATE")->pushEvent("suspend");

  simcall_process_suspend(pimpl_);
}

void Actor::resume()
{
  simgrid::simix::kernelImmediate([this] { pimpl_->resume(); });
  if (TRACE_actor_is_enabled())
    simgrid::instr::Container::byName(instr_pid(this))->getState("MSG_PROCESS_STATE")->popEvent();
}

int Actor::isSuspended()
{
  return simgrid::simix::kernelImmediate([this] { return pimpl_->suspended; });
}

void Actor::setKillTime(double time) {
  simcall_process_set_kill_time(pimpl_,time);
}

/** \brief Get the kill time of an actor(or 0 if unset). */
double Actor::getKillTime()
{
  return SIMIX_timer_get_date(pimpl_->kill_timer);
}

void Actor::kill(aid_t pid)
{
  smx_actor_t killer  = SIMIX_process_self();
  smx_actor_t process = SIMIX_process_from_PID(pid);
  if(process != nullptr) {
    simgrid::simix::kernelImmediate([killer, process] { SIMIX_process_kill(process, killer); });
  } else {
    std::ostringstream oss;
    oss << "kill: (" << pid << ") - No such actor" << std::endl;
    throw std::runtime_error(oss.str());
  }
}

void Actor::kill() {
  smx_actor_t process = SIMIX_process_self();
  simgrid::simix::kernelImmediate(
      [this, process] { SIMIX_process_kill(pimpl_, (pimpl_ == simix_global->maestro_process) ? pimpl_ : process); });
}

smx_actor_t Actor::getImpl()
{
  return pimpl_;
}

// ***** Static functions *****

ActorPtr Actor::byPid(aid_t pid)
{
  smx_actor_t process = SIMIX_process_from_PID(pid);
  if (process != nullptr)
    return process->iface();
  else
    return ActorPtr();
}

void Actor::killAll()
{
  simcall_process_killall();
}

std::map<std::string, std::string>* Actor::getProperties()
{
  return simgrid::simix::kernelImmediate([this] { return this->pimpl_->getProperties(); });
}

/** Retrieve the property value (or nullptr if not set) */
const char* Actor::getProperty(const char* key)
{
  return simgrid::simix::kernelImmediate([this, key] { return pimpl_->getProperty(key); });
}

void Actor::setProperty(const char* key, const char* value)
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
bool isMaestro()
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
  smx_activity_t s = simcall_execution_start(nullptr, flops, 1.0 /*priority*/, 0. /*bound*/, getHost());
  simcall_execution_wait(s);
}

void execute(double flops, double priority)
{
  smx_activity_t s = simcall_execution_start(nullptr, flops, 1 / priority /*priority*/, 0. /*bound*/, getHost());
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
  res->host_         = getHost();
  res->flops_amount_ = flops_amount;
  res->setRemains(flops_amount);
  return res;
}

ExecPtr exec_async(double flops)
{
  ExecPtr res = exec_init(flops);
  res->start();
  return res;
}

void* recv(MailboxPtr chan) // deprecated
{
  return chan->get();
}

void* recv(MailboxPtr chan, double timeout) // deprecated
{
  return chan->get(timeout);
}

void send(MailboxPtr chan, void* payload, double simulatedSize) // deprecated
{
  chan->put(payload, simulatedSize);
}

void send(MailboxPtr chan, void* payload, double simulatedSize, double timeout) // deprecated
{
  chan->put(payload, simulatedSize, timeout);
}

CommPtr isend(MailboxPtr chan, void* payload, double simulatedSize) // deprecated
{
  return chan->put_async(payload, simulatedSize);
}

CommPtr irecv(MailboxPtr chan, void** data) // deprecated
{
  return chan->get_async(data);
}

aid_t getPid()
{
  return SIMIX_process_self()->pid;
}

aid_t getPpid()
{
  return SIMIX_process_self()->ppid;
}

std::string getName()
{
  return SIMIX_process_self()->getName();
}

const char* getCname()
{
  return SIMIX_process_self()->getCname();
}

Host* getHost()
{
  return SIMIX_process_self()->host;
}

void suspend()
{
  if (TRACE_actor_is_enabled())
    instr::Container::byName(getName() + "-" + std::to_string(getPid()))
        ->getState("MSG_PROCESS_STATE")
        ->pushEvent("suspend");
  simcall_process_suspend(SIMIX_process_self());
}

void resume()
{
  smx_actor_t process = SIMIX_process_self();
  simgrid::simix::kernelImmediate([process] { process->resume(); });

  if (TRACE_actor_is_enabled())
    instr::Container::byName(getName() + "-" + std::to_string(getPid()))->getState("MSG_PROCESS_STATE")->popEvent();
}

bool isSuspended()
{
  smx_actor_t process = SIMIX_process_self();
  return simgrid::simix::kernelImmediate([process] { return process->suspended; });
}

void kill()
{
  smx_actor_t process = SIMIX_process_self();
  simgrid::simix::kernelImmediate([process] { SIMIX_process_kill(process, process); });
}

void onExit(int_f_pvoid_pvoid_t fun, void* data)
{
  simcall_process_on_exit(SIMIX_process_self(), fun, data);
}

/** @brief Moves the current actor to another host
 *
 * @see simgrid::s4u::Actor::migrate() for more information
 */
void migrate(Host* new_host)
{
  SIMIX_process_self()->iface()->migrate(new_host);
}
}
}
}
