/* Copyright (c) 2006-2014. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "xbt/log.h"
#include "src/msg/msg_private.h"

#include "simgrid/s4u/actor.hpp"
#include "simgrid/s4u/comm.hpp"
#include "simgrid/s4u/host.hpp"
#include "simgrid/s4u/mailbox.hpp"

XBT_LOG_NEW_DEFAULT_CATEGORY(s4u_actor,"S4U actors");

static int s4u_actor_runner(int argc, char **argv)
{
  // Move the callback from the heap to the stack:
  std::unique_ptr<std::function<int()>> code2 =
    std::unique_ptr<std::function<int()>>(
      static_cast<std::function<int()>*>(
        SIMIX_process_self_get_data()));
  std::function<int()> code = std::move(*code2);
  code2 = nullptr;
  // Call it:
  // TODO, handle exceptions
  return code();
}

using namespace simgrid;

s4u::Actor::Actor(smx_process_t smx_proc) : pimpl_(smx_proc) {}

s4u::Actor::Actor(const char* name, s4u::Host *host, double killTime, std::function<int()> code)
{
  std::function<int()>* code2 = new std::function<int()>(std::move(code));
  this->pimpl_ = simcall_process_create(
    name, s4u_actor_runner, code2, host->name().c_str(),
    killTime, 0, NULL, NULL, 0);
}

s4u::Actor::~Actor() {}

void s4u::Actor::setAutoRestart(bool autorestart) {
  simcall_process_auto_restart_set(pimpl_,autorestart);
}

s4u::Host *s4u::Actor::getHost() {
  return s4u::Host::by_name(sg_host_get_name(simcall_process_get_host(pimpl_)));
}

const char* s4u::Actor::getName() {
  return simcall_process_get_name(pimpl_);
}

int s4u::Actor::getPid(){
  return simcall_process_get_PID(pimpl_);
}

void s4u::Actor::setKillTime(double time) {
  simcall_process_set_kill_time(pimpl_,time);
}

double s4u::Actor::getKillTime() {
  return simcall_process_get_kill_time(pimpl_);
}

void s4u::Actor::kill() {
  simcall_process_kill(pimpl_);
}

// static stuff:

void s4u::Actor::killAll() {
  simcall_process_killall(1);
}

void s4u::Actor::sleep(double duration) {
  simcall_process_sleep(duration);
}

e_smx_state_t s4u::Actor::execute(double flops) {
  smx_synchro_t s = simcall_execution_start(NULL,flops,1.0/*priority*/,0./*bound*/, 0L/*affinity*/);
  return simcall_execution_wait(s);
}

void *s4u::Actor::recv(Mailbox &chan) {
  void *res = NULL;
  Comm c = Comm::recv_init(chan);
  c.setDstData(&res,sizeof(res));
  c.wait();
  return res;
}

void s4u::Actor::send(Mailbox &chan, void *payload, size_t simulatedSize) {
  Comm c = Comm::send_init(chan);
  c.setRemains(simulatedSize);
  c.setSrcData(payload);
  // c.start() is optional.
  c.wait();
}
