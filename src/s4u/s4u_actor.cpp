/* Copyright (c) 2006-2017. The SimGrid Team. All rights reserved.          */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/log.h"

#include "simgrid/s4u/Actor.hpp"
#include "simgrid/s4u/Comm.hpp"
#include "simgrid/s4u/Host.hpp"
#include "simgrid/s4u/Mailbox.hpp"

#include "src/kernel/context/Context.hpp"
#include "src/simix/smx_private.h"

#include <sstream>

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_actor, "S4U actors");

namespace simgrid {
namespace s4u {

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
  xbt_assert(actor != nullptr);
  intrusive_ptr_add_ref(actor->pimpl_);
}
void intrusive_ptr_release(Actor* actor)
{
  xbt_assert(actor != nullptr);
  intrusive_ptr_release(actor->pimpl_);
}

// ***** Actor methods *****

void Actor::join() {
  simcall_process_join(this->pimpl_, -1);
}

void Actor::setAutoRestart(bool autorestart) {
  simcall_process_auto_restart_set(pimpl_,autorestart);
}

void Actor::onExit(int_f_pvoid_pvoid_t fun, void* data)
{
  simcall_process_on_exit(pimpl_, fun, data);
}

void Actor::migrate(Host* new_host)
{
  simgrid::simix::kernelImmediate([this, new_host]() { pimpl_->new_host = new_host; });
}

s4u::Host* Actor::host()
{
  return this->pimpl_->host;
}

void Actor::daemonize()
{
  simgrid::simix::kernelImmediate([this]() { pimpl_->daemonize(); });
}

const char* Actor::cname()
{
  return this->pimpl_->name.c_str();
}

simgrid::xbt::string Actor::name()
{
  return this->pimpl_->name;
}

aid_t Actor::pid()
{
  return this->pimpl_->pid;
}

aid_t Actor::ppid()
{
  return this->pimpl_->ppid;
}

void Actor::suspend()
{
  simcall_process_suspend(pimpl_);
}

void Actor::resume()
{
  simgrid::simix::kernelImmediate([this] { pimpl_->resume(); });
}

int Actor::isSuspended()
{
  return simgrid::simix::kernelImmediate([this] { return pimpl_->suspended; });
}

void Actor::setKillTime(double time) {
  simcall_process_set_kill_time(pimpl_,time);
}

/** \brief Get the kill time of an actor(or 0 if unset). */
double Actor::killTime()
{
  return SIMIX_timer_get_date(pimpl_->kill_timer);
}

void Actor::kill(aid_t pid)
{
  smx_actor_t process = SIMIX_process_from_PID(pid);
  if(process != nullptr) {
    simcall_process_kill(process);
  } else {
    std::ostringstream oss;
    oss << "kill: ("<< pid <<") - No such process" << std::endl;
    throw std::runtime_error(oss.str());
  }
}

smx_actor_t Actor::getImpl() {
  return pimpl_;
}

void Actor::kill() {
  simcall_process_kill(pimpl_);
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
  simcall_process_killall(1);
}

void Actor::killAll(int resetPid)
{
  simcall_process_killall(resetPid);
}

/** Retrieve the property value (or nullptr if not set) */
const char* Actor::property(const char* key)
{
  return (char*)xbt_dict_get_or_null(simcall_process_get_properties(pimpl_), key);
}

void Actor::setProperty(const char* key, const char* value)
{
  simgrid::simix::kernelImmediate([this, key, value] {
    xbt_dict_set(simcall_process_get_properties(pimpl_), key, (char*)value, (void_f_pvoid_t) nullptr);
  });
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

XBT_PUBLIC(void) sleep_until(double timeout)
{
  double now = SIMIX_get_clock();
  if (timeout > now)
    simcall_process_sleep(timeout - now);
}

e_smx_state_t execute(double flops) {
  smx_activity_t s = simcall_execution_start(nullptr,flops,1.0/*priority*/,0./*bound*/);
  return simcall_execution_wait(s);
}

void* recv(MailboxPtr chan) {
  return chan->get();
}

void* recv(MailboxPtr chan, double timeout)
{
  return chan->get(timeout);
}

void send(MailboxPtr chan, void* payload, double simulatedSize)
{
  chan->put(payload, simulatedSize);
}

void send(MailboxPtr chan, void* payload, double simulatedSize, double timeout)
{
  chan->put(payload, simulatedSize, timeout);
}

CommPtr isend(MailboxPtr chan, void* payload, double simulatedSize)
{
  return chan->put_async(payload, simulatedSize);
}

CommPtr irecv(MailboxPtr chan, void** data)
{
  return chan->get_async(data);
}

aid_t pid()
{
  return SIMIX_process_self()->pid;
}

aid_t ppid()
{
  return SIMIX_process_self()->ppid;
}

std::string name()
{
  return SIMIX_process_self()->name;
}

Host* host()
{
  return SIMIX_process_self()->host;
}

void suspend()
{
  simcall_process_suspend(SIMIX_process_self());
}

void resume()
{
  smx_actor_t process = SIMIX_process_self();
  simgrid::simix::kernelImmediate([process] { process->resume(); });
}

bool isSuspended()
{
  smx_actor_t process = SIMIX_process_self();
  return simgrid::simix::kernelImmediate([process] { return process->suspended; });
}

void kill()
{
  simcall_process_kill(SIMIX_process_self());
}

void onExit(int_f_pvoid_pvoid_t fun, void* data)
{
  simcall_process_on_exit(SIMIX_process_self(), fun, data);
}

void migrate(Host* new_host)
{
  smx_actor_t process = SIMIX_process_self();
  simgrid::simix::kernelImmediate([process, new_host] { process->new_host = new_host; });
}
}
}
}
