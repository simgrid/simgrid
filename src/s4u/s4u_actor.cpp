/* Copyright (c) 2006-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/log.h"
#include "simgrid/s4u/Actor.hpp"
#include "simgrid/s4u/comm.hpp"
#include "simgrid/s4u/host.hpp"

#include "../msg/msg_private.hpp"
#include "simgrid/s4u/Mailbox.hpp"

#include "src/kernel/context/Context.hpp"
#include "src/simix/smx_private.h"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_actor,"S4U actors");

namespace simgrid {
namespace s4u {

// ***** Actor creation *****
ActorPtr Actor::self()
{
  smx_context_t self_context = SIMIX_context_self();
  if (self_context == nullptr)
    return simgrid::s4u::ActorPtr();

  return simgrid::s4u::ActorPtr(&self_context->process()->getIface());
}


ActorPtr Actor::createActor(const char* name, s4u::Host *host, double killTime, std::function<void()> code)
{
  // TODO, when autorestart is used, the std::function is copied so the new
  // instance will get a fresh (reinitialized) state. Is this what we want?
  smx_actor_t process = simcall_process_create(
    name, std::move(code), nullptr, host->name().c_str(),
    killTime, nullptr, 0);
  return ActorPtr(&process->getIface());
}

ActorPtr Actor::createActor(const char* name, s4u::Host *host, double killTime,
  const char* function, std::vector<std::string> args)
{
  simgrid::simix::ActorCodeFactory& factory = SIMIX_get_actor_code_factory(function);
  simgrid::simix::ActorCode code = factory(std::move(args));
  smx_actor_t process = simcall_process_create(
    name, std::move(code), nullptr, host->name().c_str(),
    killTime, nullptr, 0);
  return ActorPtr(&process->getIface());
}

// ***** Actor methods *****

void Actor::join() {
  simcall_process_join(pimpl_, -1);
}

void Actor::setAutoRestart(bool autorestart) {
  simcall_process_auto_restart_set(pimpl_,autorestart);
}

s4u::Host *Actor::getHost() {
  return pimpl_->host;
}

simgrid::xbt::string Actor::getName() {
  return pimpl_->name;
}

int Actor::getPid(){
  return pimpl_->pid;
}

int Actor::getPpid() {
  return pimpl_->ppid;
}

void Actor::setKillTime(double time) {
  simcall_process_set_kill_time(pimpl_,time);
}

double Actor::getKillTime() {
  return simcall_process_get_kill_time(pimpl_);
}

void Actor::kill(int pid) {
  msg_process_t process = SIMIX_process_from_PID(pid);
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

ActorPtr Actor::forPid(int pid)
{
  smx_actor_t process = SIMIX_process_from_PID(pid);
  if (process != nullptr)
    return ActorPtr(&process->getIface());
  else
    return nullptr;
}

void Actor::killAll() {
  simcall_process_killall(1);
}

// ***** this_actor *****

namespace this_actor {

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
  void *res = nullptr;
  Comm& c = Comm::recv_init(chan);
  c.setDstData(&res,sizeof(res));
  c.wait();
  return res;
}

void send(MailboxPtr chan, void *payload, size_t simulatedSize) {
  Comm& c = Comm::send_init(chan);
  c.setRemains(simulatedSize);
  c.setSrcData(payload);
  // c.start() is optional.
  c.wait();
}

int getPid() {
  return SIMIX_process_self()->pid;
}

int getPpid() {
  return SIMIX_process_self()->ppid;
}

}
}
}
