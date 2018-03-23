/* Copyright (c) 2010, 2012-2017. The SimGrid Team. All rights reserved.    */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "src/instr/instr_private.hpp"
#include "src/msg/msg_private.hpp"
#include "src/simix/ActorImpl.hpp"
#include <simgrid/actor.h>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_msg_process, instr, "MSG process");

std::string instr_pid(msg_process_t proc)
{
  return std::string(proc->getCname()) + "-" + std::to_string(proc->getPid());
}

void TRACE_msg_process_kill(smx_process_exit_status_t status, msg_process_t process)
{
  if (TRACE_actor_is_enabled() && status == SMX_EXIT_FAILURE) {
    //kill means that this process no longer exists, let's destroy it
    simgrid::instr::Container::byName(instr_pid(process))->removeFromParent();
  }
}
