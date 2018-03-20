/* Copyright (c) 2010, 2012-2017. The SimGrid Team. All rights reserved.    */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "simgrid/s4u/Host.hpp"
#include "src/instr/instr_private.hpp"
#include "src/msg/msg_private.hpp"
#include "src/simix/ActorImpl.hpp"

XBT_LOG_NEW_DEFAULT_SUBCATEGORY (instr_msg_process, instr, "MSG process");

std::string instr_pid(msg_process_t proc)
{
  return std::string(proc->getCname()) + "-" + std::to_string(proc->getPid());
}

void TRACE_msg_process_change_host(msg_process_t process, msg_host_t new_host)
{
  if (TRACE_actor_is_enabled()) {
    static long long int counter = 0;

    std::string key = std::to_string(counter);
    counter++;

    //start link
    container_t msg                = simgrid::instr::Container::byName(instr_pid(process));
    simgrid::instr::LinkType* link = simgrid::instr::Container::getRoot()->getLink("MSG_PROCESS_LINK");
    link->startEvent(msg, "M", key);

    //destroy existing container of this process
    TRACE_msg_process_destroy(process);

    //create new container on the new_host location
    TRACE_msg_process_create(process, new_host);

    //end link
    msg = simgrid::instr::Container::byName(instr_pid(process));
    link->endEvent(msg, "M", key);
  }
}

void TRACE_msg_process_create(msg_process_t process, msg_host_t host)
{
  if (TRACE_actor_is_enabled()) {
    container_t host_container = simgrid::instr::Container::byName(host->getName());
    new simgrid::instr::Container(instr_pid(process), "MSG_PROCESS", host_container);
  }
}

void TRACE_msg_process_destroy(msg_process_t process)
{
  if (TRACE_actor_is_enabled()) {
    container_t container = simgrid::instr::Container::byNameOrNull(instr_pid(process));
    if (container) {
      container->removeFromParent();
      delete container;
    }
  }
}

void TRACE_msg_process_kill(smx_process_exit_status_t status, msg_process_t process)
{
  if (TRACE_actor_is_enabled() && status == SMX_EXIT_FAILURE) {
    //kill means that this process no longer exists, let's destroy it
    TRACE_msg_process_destroy(process);
  }
}
