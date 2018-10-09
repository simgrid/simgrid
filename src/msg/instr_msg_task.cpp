/* Copyright (c) 2010-2018. The SimGrid Team.
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
    return;
  }

  //set task category
  task->category = xbt_strdup (category);
  XBT_DEBUG("MSG task %p(%s), category %s", task, task->name, task->category);
}

/* MSG_task_create related function*/
void TRACE_msg_task_create(msg_task_t task)
{
  static std::atomic_ullong counter{0};
  task->counter = counter++;
  task->category = nullptr;

  if(MC_is_active())
    MC_ignore_heap(&(task->counter), sizeof(task->counter));

  XBT_DEBUG("CREATE %p, %lld", task, task->counter);
}

/* MSG_task_execute related functions */
void TRACE_msg_task_execute_start(msg_task_t task)
{
  XBT_DEBUG("EXEC,in %p, %lld, %s", task, task->counter, task->category);

  if (TRACE_actor_is_enabled())
    simgrid::instr::Container::by_name(instr_pid(MSG_process_self()))->get_state("ACTOR_STATE")->push_event("execute");
}

void TRACE_msg_task_execute_end(msg_task_t task)
{
  XBT_DEBUG("EXEC,out %p, %lld, %s", task, task->counter, task->category);

  if (TRACE_actor_is_enabled())
    simgrid::instr::Container::by_name(instr_pid(MSG_process_self()))->get_state("ACTOR_STATE")->pop_event();
}

/* MSG_task_destroy related functions */
void TRACE_msg_task_destroy(msg_task_t task)
{
  XBT_DEBUG("DESTROY %p, %lld, %s", task, task->counter, task->category);

  //free category
  xbt_free(task->category);
  task->category = nullptr;
}

/* MSG_task_get related functions */
void TRACE_msg_task_get_start()
{
  XBT_DEBUG("GET,in");

  if (TRACE_actor_is_enabled())
    simgrid::instr::Container::by_name(instr_pid(MSG_process_self()))->get_state("ACTOR_STATE")->push_event("receive");
}

void TRACE_msg_task_get_end(msg_task_t task)
{
  XBT_DEBUG("GET,out %p, %lld, %s", task, task->counter, task->category);

  if (TRACE_actor_is_enabled()) {
    container_t process_container = simgrid::instr::Container::by_name(instr_pid(MSG_process_self()));
    process_container->get_state("ACTOR_STATE")->pop_event();

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
    process_container->get_state("ACTOR_STATE")->push_event("send");

    std::string key = std::string("p") + std::to_string(task->counter);
    simgrid::instr::Container::get_root()->get_link("ACTOR_TASK_LINK")->start_event(process_container, "SR", key);
  }
}

void TRACE_msg_task_put_end()
{
  XBT_DEBUG("PUT,out");

  if (TRACE_actor_is_enabled())
    simgrid::instr::Container::by_name(instr_pid(MSG_process_self()))->get_state("ACTOR_STATE")->pop_event();
}
