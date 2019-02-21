/* Copyright (c) 2010-2019. The SimGrid Team.
 * All rights reserved.                                                     */

/* This program is free software; you can redistribute it and/or modify it
 * under the terms of the license (GNU LGPL) which comes with this package. */

#include "mc/mc.h"
#include "src/instr/instr_private.hpp"
#include "src/msg/msg_private.hpp"

#include <atomic>

XBT_LOG_NEW_DEFAULT_SUBCATEGORY(instr_msg, instr, "MSG instrumentation");

void TRACE_msg_set_task_category(msg_task_t task, const char *category)
{
  xbt_assert(task->category == nullptr, "Task %p(%s) already has a category (%s).",
      task, task->name, task->category);

  //if user provides a nullptr category, task is no longer traced
  if (category == nullptr) {
    xbt_free (task->category);
    task->category = nullptr;
    XBT_DEBUG("MSG task %p(%s), category removed", task, task->name);
  } else {
    // set task category
    task->category = xbt_strdup(category);
    XBT_DEBUG("MSG task %p(%s), category %s", task, task->name, task->category);
  }
}

void TRACE_msg_task_get_end(msg_task_t task)
{
  XBT_DEBUG("GET,out %p, %lld, %s", task, task->counter, task->category);

  if (TRACE_actor_is_enabled()) {
    container_t process_container = simgrid::instr::Container::by_name(instr_pid(MSG_process_self()));

    std::string key = std::string("p") + std::to_string(task->counter);
    simgrid::instr::Container::get_root()->get_link("ACTOR_TASK_LINK")->end_event(process_container, "SR", key);
  }
}

/* MSG_task_put related functions */
void TRACE_msg_task_put_start(msg_task_t task)
{
  XBT_DEBUG("PUT,in %p, %lld, %s", task, task->counter, task->category);

  if (TRACE_actor_is_enabled()) {
    container_t process_container = simgrid::instr::Container::by_name(instr_pid(MSG_process_self()));
    std::string key = std::string("p") + std::to_string(task->counter);
    simgrid::instr::Container::get_root()->get_link("ACTOR_TASK_LINK")->start_event(process_container, "SR", key);
  }
}
