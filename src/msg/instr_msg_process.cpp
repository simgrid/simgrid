/* Copyright (c) 2010, 2012-2017. The SimGrid Team. All rights reserved.    */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/Host.hpp"
#include "src/instr/instr_private.hpp"
#include "src/msg/msg_private.hpp"
#include "src/simix/ActorImpl.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_msg_process, instr, "MSG process");

std::string instr_process_id(msg_process_t proc)
{
  return std::string(proc->getCname()) + "-" + std::to_string(proc->getPid());
}

void TRACE_msg_process_change_host(msg_process_t process, msg_host_t new_host)
{
  if (TRACE_msg_process_is_enabled()){
    static long long int counter = 0;

    std::string key = std::to_string(counter++);

    //start link
    container_t msg            = simgrid::instr::Container::byName(instr_process_id(process));
    simgrid::instr::Type* type = simgrid::instr::Type::getRootType()->byName("MSG_PROCESS_LINK");
    new simgrid::instr::StartLinkEvent(MSG_get_clock(), PJ_container_get_root(), type, msg, "M", key);

    //destroy existing container of this process
    TRACE_msg_process_destroy (MSG_process_get_name (process), MSG_process_get_PID (process));

    //create new container on the new_host location
    TRACE_msg_process_create (MSG_process_get_name (process), MSG_process_get_PID (process), new_host);

    //end link
    msg  = simgrid::instr::Container::byName(instr_process_id(process));
    type = simgrid::instr::Type::getRootType()->byName("MSG_PROCESS_LINK");
    new simgrid::instr::EndLinkEvent(MSG_get_clock(), PJ_container_get_root(), type, msg, "M", key);
  }
}

void TRACE_msg_process_create (const char *process_name, int process_pid, msg_host_t host)
{
  if (TRACE_msg_process_is_enabled()){
    container_t host_container = simgrid::instr::Container::byName(host->getName());
    new simgrid::instr::Container(std::string(process_name) + "-" + std::to_string(process_pid),
                                  simgrid::instr::INSTR_MSG_PROCESS, host_container);
  }
}

void TRACE_msg_process_destroy (const char *process_name, int process_pid)
{
  if (TRACE_msg_process_is_enabled()) {
    container_t process =
        simgrid::instr::Container::byNameOrNull(std::string(process_name) + "-" + std::to_string(process_pid));
    if (process) {
      process->removeFromParent();
      delete process;
    }
  }
}

void TRACE_msg_process_kill(smx_process_exit_status_t status, msg_process_t process)
{
  if (TRACE_msg_process_is_enabled() && status == SMX_EXIT_FAILURE) {
    //kill means that this process no longer exists, let's destroy it
    TRACE_msg_process_destroy(process->getCname(), process->getPid());
  }
}

void TRACE_msg_process_suspend(msg_process_t process)
{
  if (TRACE_msg_process_is_enabled()){
    container_t process_container = simgrid::instr::Container::byName(instr_process_id(process));
    simgrid::instr::Type* state   = process_container->type_->byName("MSG_PROCESS_STATE");
    simgrid::instr::Value* val    = state->getEntityValue("suspend");
    new simgrid::instr::PushStateEvent(MSG_get_clock(), process_container, state, val);
  }
}

void TRACE_msg_process_resume(msg_process_t process)
{
  if (TRACE_msg_process_is_enabled()){
    container_t process_container = simgrid::instr::Container::byName(instr_process_id(process));
    simgrid::instr::Type* type    = process_container->type_->byName("MSG_PROCESS_STATE");
    new simgrid::instr::PopStateEvent(MSG_get_clock(), process_container, type);
  }
}

void TRACE_msg_process_sleep_in(msg_process_t process)
{
  if (TRACE_msg_process_is_enabled()){
    container_t process_container = simgrid::instr::Container::byName(instr_process_id(process));
    simgrid::instr::Type* state   = process_container->type_->byName("MSG_PROCESS_STATE");
    simgrid::instr::Value* val    = state->getEntityValue("sleep");
    new simgrid::instr::PushStateEvent(MSG_get_clock(), process_container, state, val);
  }
}

void TRACE_msg_process_sleep_out(msg_process_t process)
{
  if (TRACE_msg_process_is_enabled()){
    container_t process_container = simgrid::instr::Container::byName(instr_process_id(process));
    simgrid::instr::Type* type    = process_container->type_->byName("MSG_PROCESS_STATE");
    new simgrid::instr::PopStateEvent(MSG_get_clock(), process_container, type);
  }
}
