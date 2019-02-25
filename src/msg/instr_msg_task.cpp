/* Copyright (c) 2010-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc/mc.h"
#include "src/instr/instr_private.hpp"
#include "src/msg/msg_private.hpp"

#include <atomic>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(instr_msg, instr, "MSG instrumentation");

/* MSG_task_put related functions */
void TRACE_msg_task_put_start(msg_task_t task)
{
  XBT_DEBUG("PUT,in %p, %lld, %s", task, task->counter, task->simdata->get_tracing_category().c_str());

  if (TRACE_actor_is_enabled()) {
    container_t process_container = simgrid::instr::Container::by_name(instr_pid(MSG_process_self()));
    std::string key = std::string("p") + std::to_string(task->counter);
    simgrid::instr::Container::get_root()->get_link("ACTOR_TASK_LINK")->start_event(process_container, "SR", key);
  }
}
